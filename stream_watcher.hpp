#pragma once
#include <zmq.hpp>
#include <boost/asio.hpp>
#include "flags.hpp"
#include "utils.hpp"
#include <spdlog/spdlog.h>

class StreamWatcher
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
  StreamWatcher(zmq::socket_t& socket, boost::asio::io_context& context)
    : socket_{socket}
    , streamd_{context, socket_.get(zmq::sockopt::fd)}
  {}

  ~StreamWatcher()
  {
    stop();
  }

  template<class THandler>
  void async_wait_receive(THandler&& handler)
  {
    spdlog::debug("ZmqStreamDescriptor::async_wait_receive");

    setup_async_wait(ZmqOperation::Read, Wait_t::wait_read, std::forward<THandler>(handler));
  }

  template<class THandler>
  void async_wait_error(THandler&& handler)
  {
    spdlog::debug("ZmqStreamDescriptor::async_wait_error");

    setup_async_wait(ZmqOperation::Error, Wait_t::wait_error, std::forward<THandler>(handler));
  }

  void stop()
  {
    streamd_.cancel();
    streamd_.release();
  }

private:
  template<class THandler>
  void setup_async_wait(ZmqOperation op, Wait_t wt, THandler&& handler)
  {
    if (check_ops(op)) {
      spdlog::debug("StreamWatcher::setup_async_wait -> flags was set");
      complete(op, std::forward<THandler>(handler));

      return;
    }

    streamd_.async_wait(wt, [this, op, h = std::forward<THandler>(handler)](const auto& ec)
    {
      if (ec) {
        std::abort();
      }
      spdlog::debug("async_wait_triggered");
      if (check_ops(op)) {
        complete(op, std::forward<decltype(h)>(h));
      }
    });
  }

  template<class THandler>
  void complete(ZmqOperation op, THandler&& handler)
  {
    spdlog::debug("ZmqStreamDescriptor::complete");

    flags_.clear(op);
    schedule(context(), std::forward<THandler>(handler));
    schedule(context(), [this, op]() { check_ops(op); });
  }

  bool check_ops(const ZmqOperation op) 
  {
    spdlog::debug("check_ops");
    const auto events = socket_.get(zmq::sockopt::events);
    const auto op_event_flag = static_cast<std::underlying_type_t<ZmqOperation>>(op);

    if (events & op_event_flag) {
      spdlog::debug("check_ops::set_flag");
      flags_.set(op);

      return true;
    }

    return false;
  }

private:
  auto context() 
  {
    return streamd_.get_executor();
  }

private:
  zmq::socket_t& socket_;
  boost::asio::posix::stream_descriptor streamd_;
  Flags_t flags_;
};