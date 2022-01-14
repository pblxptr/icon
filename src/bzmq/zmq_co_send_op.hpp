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
class ZmqCoSendOp
{
public:
  ZmqCoSendOp(zmq::socket_t& socket, Co_StreamWatcher& watcher)
    : socket_{ socket }, watcher_{ watcher } {}

  template<class RawBuffer>
  awaitable<void> async_send(RawBuffer&& buffer)
  {
    spdlog::debug("ZmqCoSendOp: async_send() for mulipart message.");

    const auto ret = co_await watcher_.async_wait_send();
    assert(ret);

    const auto nmessages = zmq::send_multipart(
      socket_, std::forward<RawBuffer>(buffer), zmq::send_flags::dontwait);
    assert(nmessages == buffer.size());
  }

  template<class RawBuffer>
  awaitable<void> async_send(
    RawBuffer&& buffer) requires std::is_same_v<RawBuffer, zmq::message_t>
  {
    spdlog::debug("ZmqCoSendOp: async_send() for single.");

    const auto ret = co_await watcher_.async_wait_send();
    assert(ret);

    const auto nbytes = socket_.send(std::forward<RawBuffer>(buffer),
      zmq::send_flags::dontwait);
    assert(nbytes != 0);
  }

private : zmq::socket_t& socket_;
  Co_StreamWatcher& watcher_;
};
}// namespace icon::details