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
  const char* op_to_str(ZmqOperation op)
  {
    if (op == ZmqOperation::Read) {
      return "zmq_read";
    } else if (op == ZmqOperation::Write) {
      return "zmq_write";
    } else if (op == ZmqOperation::Error) {
      return "zmq_error";
    } else {
      return "unknown";
    }
  }


  template<class THandler>
  void setup_async_wait(ZmqOperation op, Wait_t wait_type, THandler&& handler)
  {
    if (check_ops(op)) {
      spdlog::debug("StreamWatcher::setup_async_wait for op: {} -> flags was set", op_to_str(op));
      complete(op, std::forward<THandler>(handler));

      return;
    }

    streamd_.async_wait(wait_type, [wait_type, this, op, hnd = std::forward<THandler>(handler)](auto&& ec) mutable
    {
      if (ec) {
        throw ec;
      }
      spdlog::debug("async_wait_triggered");

      if (check_ops(op)) {
        //Ready to read
        complete(op, std::forward<decltype(hnd)>(hnd));
      } else {
        //We've got a false positive here. Restart watcher.
        setup_async_wait(op, wait_type, std::forward<decltype(hnd)>(hnd));
      }
    });
  }

  template<class THandler>
  void complete(ZmqOperation op, THandler&& handler)
  {
    spdlog::debug("StreamWatcher::complete for op: {}", op_to_str(op));

    flags_.clear(op);

    //(1)
    // schedule(context(), std::forward<THandler>(handler));
    // scheduler(context(), check_ops(op));

    //(2)
    // schedule(context(), std::forward<THandler>(handler));
    // check_ops(op, "additional");

    //(3)
    schedule(context(), [op, h = std::forward<THandler>(handler), this]() mutable
    {
      h();
      // check_ops(op, "additional");
    });

    // schedule(context(), [this, op]() { 
    //   spdlog::debug("schedule for check_ops only");
    //   check_ops(op); 
    // });
  }

  bool check_ops(const ZmqOperation op, const std::string& msg = "default") 
  {

    const auto events = socket_.get(zmq::sockopt::events);
    const auto op_event_flag = static_cast<std::underlying_type_t<ZmqOperation>>(op);

    if (events & op_event_flag) {
        spdlog::debug("check_ops: {} setting flag - {}", op_to_str(op), msg);
      flags_.set(op);
      return true;
    } else {
      spdlog::debug("check_ops: {} nothing to set - {}", op_to_str(op), msg);
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