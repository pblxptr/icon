#pragma once

#include "icon.hpp"
#include "stream_watcher.hpp"
#include <zmq_addon.hpp>

namespace icon::details {
class ZmqSendOp
{
public:
  ZmqSendOp(zmq::socket_t& socket, StreamWatcher& watcher)
    : socket_{socket} 
    , watcher_{watcher}
  {}

  template<class TMessage, class THandler> requires MessageToSend<TMessage>
  void async_send(TMessage&& message, THandler&& handler)
  { 
    watcher_.async_wait_send(
        [
          this,
          msg = std::forward<TMessage>(message),
          ch = std::forward<THandler>(handler)
        ]() mutable { 
      handle_ready_send(
        std::forward<decltype(msg)>(msg),
        std::forward<decltype(ch)>(ch)
      );
    });
  }
private:
  template<class TMessage, class THandler> requires MessageToSend<TMessage> && RegularMessage<TMessage>
  void handle_ready_send(TMessage&& message, THandler&& handler)
  {
    const auto nbytes = socket_.send(std::forward<TMessage>(message));

    std::invoke(std::forward<THandler>(handler));
  }

  template<class TMessage, class THandler> requires MessageToSend<TMessage> && MultipartDynamicMessage<TMessage>
  void handle_ready_send(TMessage&& messages, THandler&& handler)
  {
    const auto messages_sent = zmq::send_multipart(socket_, std::forward<TMessage>(messages), zmq::send_flags::dontwait);

    spdlog::debug("zmq_send_op::handle_ready_send: {}", messages_sent.value_or(0));

    std::invoke(std::forward<THandler>(handler));
  }
private:
  zmq::socket_t& socket_;
  StreamWatcher& watcher_;
};
}