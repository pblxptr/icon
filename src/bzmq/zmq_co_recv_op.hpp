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
  ZmqCoRecvOp(zmq::socket_t& socket, Co_StreamWatcher& watcher)
    : socket_{ socket }, watcher_{ watcher } {}



  template<class RawBuffer>
  awaitable<RawBuffer> async_receive()
  {
    spdlog::debug("ZmqCoRecvOp: async_receive() for mulipart.");

    auto [can_receive, ec] = co_await watcher_.async_wait_receive();

    spdlog::debug("ZmqCoRecvOp: can receive - {}, error_code: {}", can_receive, ec.message());

    if (!can_receive || ec) {
      co_return RawBuffer{};
    }

    auto [buffer, nreceived] = receive<RawBuffer>();
    assert(nreceived != 0);

    co_return buffer;
  }

  // template<class RawBuffer>
  // awaitable<RawBuffer> async_receive()
  // {
  //   spdlog::debug("ZmqCoRecvOp: async_receive() for mulipart.");

  //   auto [result, ec] = co_await watcher_.async_wait_receive();
  //   if (!result) {
  //     return RawBuffer{};
  //   }

  //   spdlog::debug("ZmqCoRecvOp: async_receive() for multipart - received.");

  //   auto buffer = RawBuffer{};
  //   const auto numberOfMessages = zmq::recv_multipart(
  //     socket_, std::back_inserter(buffer), zmq::recv_flags::dontwait);
  //   assert(numberOfMessages != 0);

  //   co_return buffer;
  // }

  // template<class RawBuffer>
  // awaitable<RawBuffer>
  //   async_receive() requires std::is_same_v<RawBuffer, zmq::message_t>
  // {
  //   spdlog::debug("ZmqCoSendOp: async_receive() for single message.");

  //   auto [result, ec] = co_await watcher_.async_wait_receive();
  //   if (!result) {
  //     co_return RawBuffer{};
  //   }

  //   spdlog::debug("ZmqCoRecvOp: async_receive() for single message - received.");

  //   auto buffer = zmq::message_t{};
  //   const auto nbytes = socket_.recv(buffer, zmq::recv_flags::dontwait);
  //   assert(nbytes != 0);

  //   co_return buffer;
  // }

  void cancel()
  {
    watcher_.cancel();
  }

private:
  template<class RawBuffer>
  auto receive() requires std::is_same_v<RawBuffer, zmq::message_t>
  {
    spdlog::debug("ZmqCoRecvOp: receive for zmq::message.");

    auto buffer = RawBuffer{};
    auto nbytes = socket_.recv(buffer, zmq::recv_flags::dontwait);

    return std::tuple{std::move(buffer), nbytes};
  }

  template<class RawBuffer>
  auto receive()
  {
    spdlog::debug("ZmqCoRecvOp: receive fo TBuffer<zmq::message>");

    auto buffer = RawBuffer{};
    auto nmessages = zmq::recv_multipart(socket_, std::back_inserter(buffer), zmq::recv_flags::dontwait);

    return std::tuple(std::move(buffer), nmessages);
  }

private:
  zmq::socket_t& socket_;
  Co_StreamWatcher& watcher_;
};
}// namespace icon::details

