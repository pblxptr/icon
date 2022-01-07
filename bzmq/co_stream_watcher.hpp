#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>
#include <utils/flags.hpp>
#include <cassert>

namespace posix = boost::asio::posix;

namespace {
  using boost::asio::awaitable;
  using boost::asio::co_spawn;
  using boost::asio::use_awaitable;
  using boost::asio::detached;
}

namespace icon::details {
class Co_StreamWatcher
{
  enum class ZmqOperation {
    None = 0,
    Read =  ZMQ_POLLIN,
    Write = ZMQ_POLLOUT,
    Error = ZMQ_POLLERR,
    _size
  };

  using Wait_t =  boost::asio::posix::stream_descriptor::wait_type;
  using Flags_t = Flags<ZmqOperation>;
public:
  Co_StreamWatcher(zmq::socket_t& socket, boost::asio::io_context& context)
    : socket_{socket}
    , streamd_{context}
  {}

  Co_StreamWatcher(const Co_StreamWatcher&) = delete;
  Co_StreamWatcher& operator=(const Co_StreamWatcher&) = delete;
  Co_StreamWatcher(Co_StreamWatcher&&) = default;
  Co_StreamWatcher& operator=(Co_StreamWatcher&&) = default;

  ~Co_StreamWatcher()
  {
    stop();
  }

  awaitable<bool> async_wait_receive(bool autonomous_mode = true)
  {
    spdlog::debug("StreamWatcher::async_wait_receive");

    auto result = co_await setup_async_wait(ZmqOperation::Read, Wait_t::wait_read);

    if (not result && autonomous_mode) {
      co_return co_await async_wait_receive(autonomous_mode);
    }

    co_return result;
  }

  awaitable<bool> async_wait_send()
  {
    spdlog::debug("StreamWatcher::async_wait_send");

    auto result = co_await setup_async_wait(ZmqOperation::Write, Wait_t::wait_write);
    co_return result;
  }

private:
  awaitable<bool> setup_async_wait(const ZmqOperation op, const Wait_t wait_type)
  {
    if (!streamd_.is_open()) {
      streamd_.assign(socket_.get(zmq::sockopt::fd));
    }

    if (check_ops(op)) {
      co_return true;
    }

    co_await streamd_.async_wait(posix::stream_descriptor::wait_type::wait_read, use_awaitable);
    co_return check_ops(op);
  }

  bool check_ops(const ZmqOperation op)
  {
    const auto events = socket_.get(zmq::sockopt::events);
    const auto op_event_flag = static_cast<std::underlying_type_t<ZmqOperation>>(op);

    if (events & op_event_flag) {
      flags_.set(op);
      return true;
    }
    return false;
  }

  void stop()
  {
    streamd_.cancel();
    streamd_.release();
  }

  auto context()
  {
    return streamd_.get_executor();
  }
private:
  zmq::socket_t& socket_;
  boost::asio::posix::stream_descriptor streamd_;
  Flags_t flags_;
};
}