#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>
#include "co_stream_watcher.hpp"
#include "metadata.pb.h"
#include "icon.pb.h"

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

  template<class Number, class Raw>
  static Number extract_message_number(const Raw& raw)
  {
    return static_cast<Number>(std::stoi(raw.to_string()));
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


  template<
    class Raw,
    class Serializer
  >
  class Response
  {
  public:
    Response(uint32_t msg_number, Raw&& raw)
      : msg_number_{msg_number}
      , raw_{std::move(raw)}
    {}

    operator bool() const
    {
      return raw_.empty();
    }

    template<class Message>
    bool is() const
    {
      return Serializer::template extract_message_number<uint32_t, Message>() == msg_number_;
    }

    template<class Message>
    Message get() const
    {
      return Serializer:: template deserialze<Raw, Message>(raw_);
    }

  private:
    uint32_t msg_number_;
    Raw raw_;
  };

  class ZmqClient
  {

  public:
    using Raw_t = zmq::message_t;
    using RawBuffer_t = std::vector<Raw_t>;
    using Response_t = Response<Raw_t, ProtobufSerializer>;

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

      spdlog::debug("ZmqClient: sending syn and waiting for response...");
      const auto response = co_await send_async(icon::ConnectionEstablishReq{});

      process_connection_establish_response(response);

      if (is_connected_) {
        spdlog::debug("Client connected");
      }

      co_return is_connected_;
    }

    template<class Message>
    awaitable<Response_t> send_async(Message&& message)
    {
      //Build message
      auto msg_number = ProtobufSerializer::extract_message_number<uint32_t, Message>();
      auto msg_raw =    ProtobufSerializer::serialize<Raw_t>(std::forward<Message>(message));
      auto raw_buffer = RawBuffer_t{};
      raw_buffer.emplace_back(std::to_string(msg_number));
      raw_buffer.emplace_back(std::move(msg_raw));

      //Send
      auto zmq_send_op = ZmqCoSendOp{socket_, watcher_};
      co_await zmq_send_op.async_send(std::move(raw_buffer));

      //Recv
      auto zmq_recv_op = ZmqCoRecvOp{socket_, watcher_};
      raw_buffer = co_await zmq_recv_op.async_receive<RawBuffer_t>();

      auto& recv_msg_number = raw_buffer[0];
      auto& recv_msg_body = raw_buffer[1];

      co_return Response_t(
          ProtobufSerializer::extract_message_number<uint32_t, Raw_t>(recv_msg_number),
          std::move(recv_msg_body)
        );;
    }

  private:
    void process_connection_establish_response(const Response_t& response)
    {
      spdlog::debug("ZmqClient: process_connection_establish_response");
      if (!response) {
        spdlog::debug("Response invalid");

        return;
      }

      if (not response.is<icon::ConnectionEstablishCfm>()) {
          spdlog::debug("Response invalid");

          return;
      }

      // const auto& resp_msg = response.get<icon::ConnectionEstablishCfm>();
      is_connected_ = true;
    }

  private:
    zmq::socket_t socket_;
    Co_StreamWatcher watcher_;
    bool is_connected_ {false};
  };
}