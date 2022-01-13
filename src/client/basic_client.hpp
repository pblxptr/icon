#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>
#include <vector>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include <bzmq/co_stream_watcher.hpp>
#include <bzmq/zmq_co_recv_op.hpp>
#include <bzmq/zmq_co_send_op.hpp>
#include <client/response.hpp>
#include <core/header.hpp>
#include <core/protocol.hpp>
#include <icon.hpp>
#include <protobuf/protobuf_serialization.hpp>
#include <client/request.hpp>

/**
 *
 *
 */

/*
basic_client.set_authoriation()
basic_client.set_request_timeout();

auto requset = basic_client.create_request<Message>(
  message
  timeout(),
  authorization()
);
auto response = co_await basic_client.async_send(request);

auto response = co_await requset.send();


*/

namespace icon::details {
class BasicClient
{
  using ConnectionEstablishReq_t = icon::transport::ConnectionEstablishReq;
  using ConnectionEstablishCfm_t = icon::transport::ConnectionEstablishCfm;
  using Serializer_t = icon::details::serialization::protobuf::ProtobufSerializer;
  using Deserializer_t = icon::details::serialization::protobuf::ProtobufDeserializer;
  using Response_t = Response<Deserializer_t>;

public:
  BasicClient(zmq::context_t& zctx, boost::asio::io_context& bctx)
    : socket_{ zctx, zmq::socket_type::dealer }, watcher_{ socket_, bctx }
  {
    spdlog::debug("BasicClient ctor");
  }

  awaitable<bool> async_connect(const char* endpoint)
  {
    if (is_connected_) {
      co_return true;
    }

    // Connection over zmq socket, send init and wait for cfm
    socket_.connect(endpoint);

    co_await init_connection_async();

    co_return is_connected_;
  }

  template<MessageToSend Message>
  awaitable<Response_t> async_send(Message&& message)
  {
    if (not is_connected_) {
      throw std::runtime_error("Client not connected");
    }

    co_return co_await async_send_with_response(
      std::forward<Message>(message));
  }

private:
  awaitable<void> init_connection_async()
  {
    const auto response = co_await async_send_with_response(
      ConnectionEstablishReq_t{});

    if (!response.is<ConnectionEstablishCfm_t>()) {
      throw std::runtime_error("Connection establish failed");
    }
  }

  template<MessageToSend Message>
  awaitable<Response_t> async_send_with_response(Message&& message)
  {
    auto zmq_send_op = ZmqCoSendOp{ socket_, watcher_ };
    auto request = Request<Message, Serializer_t>(std::move(message));
    auto buffer = std::move(request).build();

    co_await zmq_send_op.async_send(std::move(buffer));
    co_return co_await async_receive();
  }

  awaitable<Response_t> async_receive()
  {
    auto zmq_recv_op = ZmqCoRecvOp{ socket_, watcher_ };
    auto raw_buffer = co_await zmq_recv_op.async_receive<icon::details::transport::RawBuffer_t>();

    co_return Response<Deserializer_t>::create(std::move(raw_buffer));
  }

private:
  zmq::socket_t socket_;
  Co_StreamWatcher watcher_;
  bool is_connected_{ false };
};
};// namespace icon::details
