#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <spdlog/spdlog.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include <icon/bzmq/co_stream_watcher.hpp>

namespace {
using boost::asio::awaitable;
using boost::asio::use_awaitable;
}// namespace

namespace icon::details {
class ZmqCoSendOp
{
public:
  ZmqCoSendOp(zmq::socket_t& socket, Co_StreamWatcher& watcher)
    : socket_{ socket }, watcher_{ watcher } {}


  template<class RawBuffer>
  awaitable<size_t> async_send(RawBuffer&& buffer)
  {
    auto [can_send, ec] = co_await watcher_.async_wait_send();

    spdlog::debug("ZmqCoSendOp: can_send - {}, error_code: {}", can_send, ec.message());

    if (!can_send || ec) {
      co_return 0;
    }

    const auto nsent = send<RawBuffer>(std::forward<RawBuffer>(buffer));
    assert(nsent.has_value());
    assert(nsent != 0);

    co_return nsent.value_or(0);
  }

  // template<class RawBuffer>
  // awaitable<void> async_send(RawBuffer&& buffer)
  // {
  //   spdlog::debug("ZmqCoSendOp: async_send() for mulipart message.");

  //   const auto [result, ec] = co_await watcher_.async_wait_send();
  //   assert(ret);

  //   const auto nmessages = zmq::send_multipart(
  //     socket_, std::forward<RawBuffer>(buffer), zmq::send_flags::dontwait);
  //   assert(nmessages == buffer.size());

  //   spdlog::debug("ZmqCoSendOp: async_send() for mulipart message - sent.");
  // }

  // template<class RawBuffer>
  // awaitable<void> async_send(
  //   RawBuffer&& buffer) requires std::is_same_v<RawBuffer, zmq::message_t>
  // {
  //   spdlog::debug("ZmqCoSendOp: async_send() for single.");

  //   const auto [result, ec] = co_await watcher_.async_wait_send();
  //   assert(ret);

  //   const auto nbytes = socket_.send(std::forward<RawBuffer>(buffer),
  //     zmq::send_flags::dontwait);
  //   assert(nbytes != 0);
  // }

  void cancel()
  {
    watcher_.cancel();
  }

private:
  template<class RawBuffer>
  auto send(RawBuffer&& buffer) requires std::is_same_v<RawBuffer, zmq::message_t>
  {
    return socket_.send(std::forward<RawBuffer>(buffer), zmq::send_flags::dontwait);
  }

  template<class RawBuffer>
  auto send(RawBuffer&& buffer)
  {
    return zmq::send_multipart(socket_, std::forward<RawBuffer>(buffer), zmq::send_flags::dontwait);
  }

private:
  zmq::socket_t& socket_;
  Co_StreamWatcher& watcher_;
};
}// namespace icon::details