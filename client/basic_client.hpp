#pragma once

#include <vector>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>

#include <core/header.hpp>
#include <core/protocol.hpp>
#include <bzmq/co_stream_watcher.hpp>
#include <bzmq/zmq_co_recv_op.hpp>
#include <bzmq/zmq_co_send_op.hpp>
#include <serialization/protobuf_serialization.hpp>
#include <client/response.hpp>

namespace {
  auto convert_header(const icon::details::core::Header& header)
  {
    auto transport_header = icon::transport::Header{};
    transport_header.set_message_number(header.message_number());

    return transport_header;
  }
}

namespace icon::details
{
class BasicClient
{
  using ConnectionEstablishReq_t = icon::transport::ConnectionEstablishReq;
  using ConnectionEstablishCfm_t = icon::transport::ConnectionEstablishCfm;

  using Raw_t = zmq::message_t;
  using RawBuffer_t = std::vector<Raw_t>;
  template<class Message>
  using MessageData_t = icon::details::serialization::protobuf::ProtobufMessage<Message>;
  using RawMessageData_t = icon::details::serialization::protobuf::ProtobufRawMessage;

  using Protocol_t = icon::details::Protocol<
    Raw_t,
    RawBuffer_t,
    icon::details::DataLayout<
      icon::details::fields::Header,
      icon::details::fields::Body
    >
  >;
  using Response_t         = Response<RawMessageData_t>;
  using InternalResponse_t = InternalResponse<RawMessageData_t>;
public:
  BasicClient(zmq::context_t& zctx, boost::asio::io_context& bctx)
    : socket_{zctx, zmq::socket_type::dealer}
    , watcher_{socket_, bctx}
  {
    spdlog::debug("BasicClient ctor");
  }

  awaitable<bool> async_connect(const char* endpoint)
  {
    if (is_connected_) {
      co_return true;
    }

    //Connection over zmq socket, send init and wait for cfm
    socket_.connect(endpoint);

    co_await init_connection_async();

    co_return is_connected_;
  }

  template<MessageToSend Message>
  awaitable<Response_t> async_send(Message&& message)
  {
    co_return co_await async_send_with_response<Response_t>(std::forward<Message>(message));
  }

private:
  awaitable<void> init_connection_async()
  {
    const auto response = co_await async_send_with_response<InternalResponse_t>(ConnectionEstablishReq_t{});

    if (!response.is<ConnectionEstablishCfm_t>()) {
      throw std::runtime_error("Connection establish failed");
    }
  }

  template<class Response, MessageToSend Message>
  awaitable<Response> async_send_with_response(Message&& message)
  {
    auto header = core::Header{MessageData_t<Message>::message_number()};

    co_await async_do_send(std::move(header), std::forward<Message>(message));
    co_return co_await async_receive<Response>();
  }

  template<class Message>
  awaitable<void> async_do_send(core::Header&& header, Message&& message)
  {
    auto zmq_send_op = ZmqCoSendOp{socket_, watcher_};
    co_await zmq_send_op.async_send(
        build_raw_buffer(
          MessageData_t{convert_header(header)},
          MessageData_t{std::move(message)}
    ));
  }

  template<class Response>
  awaitable<Response> async_receive()
  {
    auto zmq_recv_op = ZmqCoRecvOp{socket_, watcher_};
    auto raw_buffer = co_await zmq_recv_op.async_receive<RawBuffer_t>();
    
    co_return parse_raw_buffer_with_response<Response>(std::move(raw_buffer));
  }

  template<
    Serializable Header,
    Serializable Body
  >
  RawBuffer_t build_raw_buffer(Header&& header, Body&& body)
  {
    auto parser = Parser<Protocol_t>();
    parser.set<fields::Header>(header.serialize());
    parser.set<fields::Body>(body.serialize());

    return std::move(parser).parse();
  }

  template<class Response>
  auto parse_raw_buffer_with_response(RawBuffer_t&& raw_buffer)
  {
    auto parser = Parser<Protocol_t>(std::move(raw_buffer));

    auto header = RawMessageData_t{std::move(parser)
      .template get<icon::details::fields::Header>()}
      .template deserialize<icon::transport::Header>();

    auto body = RawMessageData_t{std::move(parser)
      .template get<icon::details::fields::Body>()};

    return Response{
      icon::details::core::Header{header.message_number()},
      icon::details::core::DeserializableBody{std::move(body)}
    };
  }

private:
  zmq::socket_t socket_;
  Co_StreamWatcher watcher_;
  bool is_connected_ { false };

};
};