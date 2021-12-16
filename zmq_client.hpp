#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>

#include "icon.hpp"
#include "metadata.pb.h"
#include "icon.pb.h"
#include "co_stream_watcher.hpp"
#include "protocol.hpp"
#include "protobuf_field.hpp"
#include "zmq_co_recv_op.hpp"
#include "zmq_co_send_op.hpp"

namespace {
  using boost::asio::awaitable;
  using boost::asio::use_awaitable;
}
namespace icon::details
{
  template<class Data>
  class Response
  {
  public:
    Response(uint32_t msg_number, Data&& data)
      : msg_number_{msg_number}
      , data_{std::move(data)}
    {}

    operator bool() const
    {
      return data_.has_value();
    }

    template<class Message>
    bool is() const
    {
      return icon::proto::get_message_number<Message>() == msg_number_;
    }

    template<class Message>
    Message get() const
    {
      return deserialize(data_);
    }

  private:
    uint32_t msg_number_;
    Data data_;
  };

  class ZmqClient
  {

  public:
    using Protocol_t = icon::Protocol<
      zmq::message_t,
      std::vector<zmq::message_t>,
      icon::DataLayout<
        icon::fields::Header,
        icon::fields::Body
      >
    >;
    using Response_t = Response<icon::proto::ProtobufField<icon::fields::Body, Protocol_t::Raw>>;

    ZmqClient(zmq::context_t& zctx, boost::asio::io_context& bctx)
      : socket_{zctx, zmq::socket_type::dealer}
      , watcher_{socket_, bctx}
    {
      spdlog::debug("ZmqClient::ctor");
    }

    awaitable<bool> connect_async(const char* endpoint)
    {
      spdlog::debug("ZmqClient: connect_async");
      socket_.connect(endpoint);
      const auto response = co_await send_async(icon::transport::ConnectionEstablishReq{});

      process_connection_establish_response(response);

      if (is_connected_) {
        spdlog::debug("Client connected");
      }

      co_return is_connected_;
    }

    template<MessageToSend Message>
    awaitable<Response_t> send_async(Message&& message)
    {
      spdlog::debug("ZmqClient: async_send");
      // //prepare
      auto header_field = icon::proto::ProtobufField<
          icon::fields::Header,
          icon::transport::Header
        >{icon::proto::get_header_for_message<Message>()
      };

      auto body_field = icon::proto::ProtobufField<
          icon::fields::Body,
          Message
        >{std::forward<Message>(message)
      };

      auto parser = Parser<Protocol_t>();
      parser.set<icon::fields::Header>(std::move(header_field));
      parser.set<icon::fields::Body>(std::move(body_field));
      auto raw = std::move(parser).parse();

      //send
      auto zmq_send_op = ZmqCoSendOp{socket_, watcher_};
      co_await zmq_send_op.async_send(std::move(raw));

      // //recv
      co_return co_await receive_async();
    }

  private:
    awaitable<Response_t> receive_async()
    {
      spdlog::debug("ZmqClient: async_receive");

      using icon::fields::Header;
      using icon::fields::Body;
      using HeaderFieldData_t = icon::proto::ProtobufField<Header, Protocol_t::Raw>;
      using BodyFieldData_t   = icon::proto::ProtobufField<Body,   Protocol_t::Raw>;

      //recv
      auto zmq_recv_op = ZmqCoRecvOp{socket_, watcher_};
      auto raw_buffer = co_await zmq_recv_op.async_receive<Protocol_t::RawBuffer>();

      //parse
      auto parser = Parser<Protocol_t>{std::move(raw_buffer)};
      const auto header = deserialize<icon::transport::Header>(parser.get<Header, HeaderFieldData_t>());

      co_return Response_t{
        header.message_number(),
        parser.get<Body, BodyFieldData_t>()
      };
    }

    void process_connection_establish_response(const Response_t& response)
    {
      spdlog::debug("ZmqClient: process_connection_establish_response");
      if (!response) {
        spdlog::debug("Response invalid");

        return;
      }

      if (not response.is<icon::transport::ConnectionEstablishCfm>()) {
          spdlog::debug("Response invalid");

          return;
      }
      is_connected_ = true;
    }

  private:
    zmq::socket_t socket_;
    Co_StreamWatcher watcher_;
    bool is_connected_ {false};
  };
}