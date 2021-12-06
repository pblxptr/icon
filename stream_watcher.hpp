#pragma once
#include <zmq.hpp>
#include <boost/asio.hpp>
#include "flags.hpp"
#include "utils.hpp"
#include <spdlog/spdlog.h>

namespace icon::details {
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

  StreamWatcher(const StreamWatcher&) = delete;
  StreamWatcher& operator=(const StreamWatcher&) = delete;
  StreamWatcher(StreamWatcher&&) = default;
  StreamWatcher& operator=(StreamWatcher&&) = default;

  ~StreamWatcher()
  {
    stop();
  }

  template<class THandler>
  void async_wait_receive(THandler&& handler)
  {
    spdlog::debug("StreamWatcher::async_wait_receive");

    setup_async_wait(ZmqOperation::Read, Wait_t::wait_read, std::forward<THandler>(handler));
  }

  template<class THandler>
  void async_wait_send(THandler&& handler)
  {
    spdlog::debug("StreamWatcher::async_wait_send");
  
    setup_async_wait(ZmqOperation::Write, Wait_t::wait_write, std::forward<THandler>(handler));
  }

  template<class THandler>
  void async_wait_error(THandler&& handler)
  {
    spdlog::debug("StreamWatcher::async_wait_error");

    setup_async_wait(ZmqOperation::Error, Wait_t::wait_error, std::forward<THandler>(handler));
  }

  void stop()
  {
    streamd_.cancel();
    streamd_.release();
  }

private:
  template<class THandler>
  void setup_async_wait(ZmqOperation op, Wait_t wait_type, THandler&& handler)
  {
    if (check_ops(op)) {
      spdlog::debug("StreamWatcher::setup_async_wait -> flags was set");
      complete(op, std::forward<THandler>(handler));

      return;
    }
  
    streamd_.async_wait(wait_type, [wait_type, this, op, hnd = std::forward<THandler>(handler)](const auto& ec)
    {
      using Handler_t = decltype(hnd);
      if (ec) {
        throw ec;
      }
      spdlog::debug("async_wait_triggered");
      
      if (check_ops(op)) {
        //Ready to read
        complete(op, std::forward<Handler_t>(hnd));
      } else {
        //We've got a false positive here. Restart watcher.
        setup_async_wait(op, wait_type, std::forward<Handler_t>(hnd));
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
}