#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <icon/utils/logger.hpp>
#include <icon/utils/flags.hpp>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <cassert>

// TODO: Remove usings

namespace {
namespace posix = boost::asio::posix;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
}// namespace

namespace icon::details {

class Co_StreamWatcher
{
  enum class ZmqOperation {
    None = 0,
    Read = ZMQ_POLLIN,
    Write = ZMQ_POLLOUT,
    Error = ZMQ_POLLERR,
    _size
  };

  using Wait_t = boost::asio::posix::stream_descriptor::wait_type;
  using Flags_t = Flags<ZmqOperation>;

public:
  using Result_t = std::tuple<bool, boost::system::error_code>;

  Co_StreamWatcher(zmq::socket_ref socket, boost::asio::io_context& context)
    : socket_{ socket }, streamd_{ context }
  {
   icon::utils::get_logger()->debug("SteramWatcher: ctor");
  }

  Co_StreamWatcher(const Co_StreamWatcher&) = delete;
  Co_StreamWatcher& operator=(const Co_StreamWatcher&) = delete;
  Co_StreamWatcher(Co_StreamWatcher&&) = default;
  Co_StreamWatcher& operator=(Co_StreamWatcher&&) = default;

  ~Co_StreamWatcher()
  {
   icon::utils::get_logger()->debug("SteramWatcher: dtor");

    stop();
  }

  awaitable<Result_t> async_wait_receive(bool auto_repeat = true)
  {
   icon::utils::get_logger()->debug("StreamWatcher: async_wait_receive()");

    auto [can_receive, ec] = co_await setup_async_wait(ZmqOperation::Read, Wait_t::wait_read);

    if (ec) {
      co_return std::tuple{ false, ec };
    }

    if (!can_receive && auto_repeat) {
      co_return co_await async_wait_receive(auto_repeat);
    }

    co_return std::tuple{ can_receive, ec };
  }

  awaitable<Result_t> async_wait_send()
  {
    icon::utils::get_logger()->debug("StreamWatcher: async_wait_send()");

    co_return co_await setup_async_wait(ZmqOperation::Write, Wait_t::wait_write);
  }

  void cancel()
  {
    auto ec = boost::system::error_code{};

    icon::utils::get_logger()->debug("StreamWatcher: cancel(), ec: {}", ec.message());

    streamd_.cancel(ec);

    if (ec) {
      throw std::runtime_error("CoStreamWatcher: unexpected error during cancel operation.");
    }
  }

private:
  awaitable<Result_t> setup_async_wait(const ZmqOperation op, const Wait_t wait_type)
  {
    auto ec = boost::system::error_code{};

   icon::utils::get_logger()->debug("StreamWatcher: setup_async_wait()");

    if (!streamd_.is_open()) {
     icon::utils::get_logger()->debug("StreamWatcher: assigning socket descriptor");
      streamd_.assign(socket_.get(zmq::sockopt::fd));
    }

    if (check_ops(op)) {
      co_return std::tuple{ true, ec };
    }

    co_await streamd_.async_wait(wait_type, boost::asio::redirect_error(use_awaitable, ec));

   icon::utils::get_logger()->debug("StreamWatcher: returned from async_wait with: {}", ec.message());

    co_return std::tuple{ check_ops(op), ec };
  }

  bool check_ops(const ZmqOperation op)
  {
    const auto events = socket_.get(zmq::sockopt::events);
    const auto op_event_flag =
      static_cast<std::underlying_type_t<ZmqOperation>>(op);

    if (events & op_event_flag) {
      flags_.set(op);
      return true;
    }
    return false;
  }

  void stop()
  {
   icon::utils::get_logger()->debug("StreamWatcher: stop()");

    if (streamd_.is_open()) {
      cancel();

     icon::utils::get_logger()->debug("StrwamWatcher: stop(), release()");
      streamd_.release();
    }
  }

private:
  zmq::socket_ref socket_;
  boost::asio::posix::stream_descriptor streamd_;
  Flags_t flags_;
};
}// namespace icon::details