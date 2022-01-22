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
#include <boost/asio/steady_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <variant>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <client/error.hpp>
#include <utils/timeout_guard.hpp>

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
  using Serializer_t = icon::details::serialization::protobuf::ProtobufSerializer;
  using Deserializer_t = icon::details::serialization::protobuf::ProtobufDeserializer;
  using Response_t = Response<Deserializer_t>;

public:
  BasicClient(zmq::context_t& zctx, boost::asio::io_context& bctx)
    : socket_{ zctx, zmq::socket_type::dealer }
    , watcher_{ socket_, bctx }
  {
    spdlog::debug("BasicClient ctor");

    socket_.set(zmq::sockopt::linger, 0);
  }

  awaitable<bool> async_connect(const char* endpoint)
  {
    spdlog::debug("BasicClient: connecting to endpoint: {}, is_connected: {}", endpoint, is_socket_connected_);

    if (is_socket_connected_) {
      co_return true;
    }

    // Connection over zmq socket
    socket_.connect(endpoint);

    is_socket_connected_ = true;

    co_return is_socket_connected_;
  }

  template<MessageToSend Message, class Timeout = std::chrono::seconds>
  awaitable<Response_t> async_send(Message&& message, Timeout timeout = std::chrono::seconds(0)) //TODO: Check timers
  {
    using namespace boost::asio::experimental::awaitable_operators;

    spdlog::debug("BasicClient: async_send()");

    if (!is_connected()) {
      throw std::runtime_error("Basic Client not connected");
    }

    if (auto ec = co_await async_send_message(std::forward<Message>(message), timeout); ec) {
      co_return Response_t{*ec};
    }

    co_return co_await async_receive_message(timeout);
  }

  bool is_connected() const
  {
    return is_socket_connected_;
  }

private:
  template<MessageToSend Message, class Timeout>
  awaitable<std::optional<ErrorCode>> async_send_message(Message&& message, Timeout timeout)
  {
    spdlog::debug("BasicClient: async_send_with_response()");

    //TODO: instead of timeout guard, awaitable_operators could be used but, they are still in ::experimental

    auto executor = co_await boost::asio::this_coro::executor;
    auto zmq_send_op = ZmqCoSendOp{ socket_, watcher_ };
    auto guard = TimeoutGuard{executor, [&]() { zmq_send_op.cancel(); }, std::move(timeout)};
    auto request = Request<Message, Serializer_t>(std::forward<Message>(message));

    guard.spawn();
    co_await zmq_send_op.async_send(std::move(request).build());

    if (guard.expired()) {
      co_return ErrorCode::SendTimeout;
    }

    co_return std::nullopt;
  }

  template<class Timeout>
  awaitable<Response_t> async_receive_message(Timeout timeout)
  {
    spdlog::debug("Basic client: async_receive");

    auto executor = co_await boost::asio::this_coro::executor;
    auto zmq_recv_op = ZmqCoRecvOp{ socket_, watcher_ };
    auto guard = TimeoutGuard{executor, [&]() { zmq_recv_op.cancel(); }, std::move(timeout)};

    guard.spawn();
    auto raw_buffer = co_await zmq_recv_op.async_receive<icon::details::transport::RawBuffer_t>();

    if (guard.expired()) {
      co_return Response_t{ ErrorCode::ReceiveTimeout };
    }

    co_return Response_t::create(std::move(raw_buffer));
  }

private:
  zmq::socket_t socket_;
  Co_StreamWatcher watcher_;
  bool is_socket_connected_ { false };
};
};// namespace icon::details
