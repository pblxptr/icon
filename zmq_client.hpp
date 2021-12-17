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
#include "data_types.hpp"

namespace {
  using boost::asio::awaitable;
  using boost::asio::use_awaitable;
}
namespace icon::details
{
  class ZmqClient
  {

  public:
    using Protocol_t = Protocol<
      zmq::message_t,
      std::vector<zmq::message_t>,
      DataLayout<
        fields::Header,
        fields::Body
      >
    >;
    using Header_t = icon::transport::Header;
    using RawProtobufData_ = icon::proto::ProtobufData<Protocol_t::Raw>;
    using BasicResponse_t = BasicResponse<Header_t, RawProtobufData_>;
    using InternalResponse_t = InternalResponse<Header_t, RawProtobufData_>;

    ZmqClient(zmq::context_t& zctx, boost::asio::io_context& bctx)
      : socket_{zctx, zmq::socket_type::dealer}
      , watcher_{socket_, bctx}
    {
      spdlog::debug("ZmqClient: ctor");
    }

    awaitable<bool> connect_async(const char* endpoint)
    {
      if (is_connected_) {
        co_return true;
      }

      spdlog::debug("ZmqClient: connect_async");
      socket_.connect(endpoint);
      co_await init_connection_async();

      if (is_connected_) {
        spdlog::debug("Client connected");
      }

      co_return is_connected_;
    }

    template<MessageToSend Message, class Response =  BasicResponse_t>
    awaitable<Response> send_async(Message&& message)
    {
      spdlog::debug("ZmqClient: async_send");

      auto body = icon::proto::ProtobufData<Message>{std::forward<Message>(message)};
      auto header = get_header_for_message<Message>(body);

      auto parser = Parser<Protocol_t>();
      parser.set<fields::Header>(serialize<Protocol_t::Raw>(std::move(header)));
      parser.set<fields::Body>(serialize<Protocol_t::Raw>(std::move(body)));
      auto raw = std::move(parser).parse();

      //send
      auto zmq_send_op = ZmqCoSendOp{socket_, watcher_};
      co_await zmq_send_op.async_send(std::move(raw));

      //receive
      co_await receive_async<Response>();
    }

  private:
    awaitable<void> init_connection_async()
    {
      const auto req = icon::transport::ConnectionEstablishReq{};
      const auto response = co_await send_async<decltype(req), InternalResponse_t>(std::move(req));

      if (not response.is<icon::transport::ConnectionEstablishCfm>()) {
        spdlog::debug("Response does not contain a valid message");

        co_return;
      }

      is_connected_ = true;
    }

    template<class Response>
    awaitable<Response> receive_async()
    {
      spdlog::debug("ZmqClient: async_receive");

      //recv
      auto zmq_recv_op = ZmqCoRecvOp{socket_, watcher_};
      auto raw_buffer = co_await zmq_recv_op.async_receive<Protocol_t::RawBuffer>();

      //parse
      auto parser = Parser<Protocol_t>{std::move(raw_buffer)};
      auto header = icon::proto::ProtobufData(std::move(parser).get<fields::Header>());
      auto body = icon::proto::ProtobufData(std::move(parser).get<fields::Body>());

      co_return Response{
        deserialize<Header_t>(header),
        std::move(body)
      };
    }

  private:
    zmq::socket_t socket_;
    Co_StreamWatcher watcher_;
    bool is_connected_ {false};
  };
}