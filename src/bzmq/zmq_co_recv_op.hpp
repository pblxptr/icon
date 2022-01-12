#pragma once

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <spdlog/spdlog.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include "co_stream_watcher.hpp"

namespace {
using boost::asio::awaitable;
using boost::asio::use_awaitable;
}// namespace

namespace icon::details {
class ZmqCoRecvOp
{
public:
  ZmqCoRecvOp(zmq::socket_t &socket, Co_StreamWatcher &watcher)
    : socket_{ socket }, watcher_{ watcher } {}

  template<class RawBuffer>
  awaitable<RawBuffer> async_receive()
  {
    spdlog::debug("ZmqCoSendOp: async_receive for mulipart");

    auto ret = co_await watcher_.async_wait_receive();
    assert(ret);

    auto buffer = RawBuffer{};
    const auto numberOfMessages = zmq::recv_multipart(
      socket_, std::back_inserter(buffer), zmq::recv_flags::dontwait);
    assert(numberOfMessages != 0);

    co_return buffer;
  }

  template<class RawBuffer>
  awaitable<RawBuffer>
    async_receive() requires std::is_same_v<RawBuffer, zmq::message_t>
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

private : zmq::socket_t &socket_;
  Co_StreamWatcher &watcher_;
};
}// namespace icon::details