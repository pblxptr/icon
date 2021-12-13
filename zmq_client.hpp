#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>
#include "co_stream_watcher.hpp"
#include "metadata.pb.h"

namespace {
  using boost::asio::awaitable;
  using boost::asio::use_awaitable;
}

struct ProtobufSerializer
{
  template<class Number, class Message>
  static Number extract_message_number()
  {
    return static_cast<Number>(Message::GetDescriptor()->options().GetExtension(icon::metadata::MESSAGE_NUMBER));
  }

  template<class Raw, class Message>
  static Raw serialize(const Message& message)
  {
    auto raw = Raw{message.ByteSizeLong()};
    message.SerializeToArray(raw.data(), raw.size());

    return raw;
  }

  template<class Raw, class Message>
  static Message deserialze(const Raw& raw)
  {
    auto message = Message{};
    message.ParseFromArray(raw.data(), raw.size());

    return message;
  }
};


namespace icon::details
{
  class ZmqCoRecvOp
  {
  public:
    ZmqCoRecvOp(zmq::socket_t& socket, Co_StreamWatcher& watcher)
      : socket_{socket}
      , watcher_{watcher}
    {}

    template<class RawBuffer>
    awaitable<RawBuffer> async_receive()
    {
      spdlog::debug("ZmqCoSendOp: async_receive for mulipart");

      auto ret = co_await watcher_.async_wait_receive();
      assert(ret);

      auto buffer = RawBuffer{};
      const auto numberOfMessages = zmq::recv_multipart(socket_, std::back_inserter(buffer), zmq::recv_flags::dontwait);
      assert(numberOfMessages != 0);

      co_return buffer;
    }

    template<class RawBuffer>
    awaitable<RawBuffer> async_receive() requires std::is_same_v<RawBuffer, zmq::message_t>
    {
      spdlog::debug("ZmqCoSendOp: async_receive for single");

      auto ret = co_await watcher_.async_wait_receive();
      if (not ret) {
        co_return RawBuffer{};
      }

      auto buffer = zmq::message_t{};
      const auto nbytes = socket_.recv(buffer, zmq::recv_flags::dontwait);
      assert(nbytes != 0);

      co_return buffer;
    }
  private:
    zmq::socket_t& socket_;
    Co_StreamWatcher& watcher_;
  };

  class ZmqCoSendOp
  {
  public:
    ZmqCoSendOp(zmq::socket_t& socket, Co_StreamWatcher& watcher)
      : socket_{socket}
      , watcher_{watcher}
    {}

    template<class RawBuffer>
    awaitable<void> async_send(RawBuffer&& buffer)
    {
      spdlog::debug("ZmqCoSendOp: async_send for mulipart");

      const auto ret = co_await watcher_.async_wait_send();
      assert(ret);

      const auto nmessages = zmq::send_multipart(socket_, std::forward<RawBuffer>(buffer), zmq::send_flags::dontwait);
      assert(nmessages == buffer.size());
    }

    template<class RawBuffer>
    awaitable<void> async_send(RawBuffer&& buffer) requires std::is_same_v<RawBuffer, zmq::message_t>
    {
      spdlog::debug("ZmqCoSendOp: async_send for single");

      const auto ret = co_await watcher_.async_wait_send();
      assert(ret);

      const auto nbytes = socket_.send(std::forward<RawBuffer>(buffer), zmq::send_flags::dontwait);
      assert(nbytes != 0);
    }
  private:
    zmq::socket_t& socket_;
    Co_StreamWatcher& watcher_;
  };

  class ZmqClient
  {

  public:
    ZmqClient(zmq::context_t& zctx, boost::asio::io_context& bctx)
      : socket_{zctx, zmq::socket_type::dealer}
      , watcher_{socket_, bctx}
    {
      spdlog::debug("ZmqClient::ctor");
    }

    awaitable<bool> connect_async(const char* endpoint)
    {
      spdlog::debug("ZmqClient: connect_async");
      spdlog::debug("ZqmClient: socket connecting...");
      socket_.connect(endpoint);
      spdlog::debug("ZmqClient: socket connected...");

      auto zmq_recv_op = ZmqCoRecvOp{socket_, watcher_};
      auto zmq_send_op = ZmqCoSendOp{socket_, watcher_};

      spdlog::debug("ZmqClient: sending syn...");
      co_await send_connection_establish(zmq_send_op);
      spdlog::debug("ZmqClient: syn sent");

      spdlog::debug("ZmqClient: waiting ack");
      is_connected_= co_await recv_connection_establish(zmq_recv_op);
      spdlog::debug("ZmqClient: ack received");
  
      if (is_connected_) {
        spdlog::debug("Received connection ack");
      } else {
        spdlog::debug("Received unknown");
      }

      co_return is_connected_;
    }

    template<class Message>
    awaitable<void> send_async(Message&& message)
    {
      //Build message
      auto msg_number = ProtobufSerializer::extract_message_number<uint32_t, Message>();
      auto msg_raw =    ProtobufSerializer::serialize<zmq::message_t>(std::forward<Message>(message));
      auto raw_buffer = std::vector<zmq::message_t>{};
      raw_buffer.emplace_back(std::to_string(msg_number));
      raw_buffer.emplace_back(std::move(msg_raw));

      //Send
      auto zmq_send_op = ZmqCoSendOp{socket_, watcher_};
      co_await zmq_send_op.async_send(std::move(raw_buffer));

      //Recv
      auto zmq_recv_op = ZmqCoRecvOp{socket_, watcher_};
      auto resp = co_await zmq_recv_op.async_receive<zmq::message_t>();

      spdlog::debug("Resp: {}", resp.to_string());

      //prepare response
      co_return;
    }
  private:
    awaitable<void> send_connection_establish(ZmqCoSendOp& op)
    {
      auto buffer = zmq::message_t(std::string{"syn"});
      co_await op.async_send(std::move(buffer));
    }

    awaitable<bool> recv_connection_establish(ZmqCoRecvOp& op)
    {
      const auto buffer = co_await op.async_receive<zmq::message_t>();

      if (buffer.to_string() == "ack") {
        co_return true;
      } else {
        co_return false;
      }
    }

  private:
    zmq::socket_t socket_;
    Co_StreamWatcher watcher_;
    bool is_connected_ {false};
  };
}