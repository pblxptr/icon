#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <vector>
#include "stream_watcher.hpp"
#include "icon.hpp"

namespace icon::details {



class ZmqRecvOp
{
public:
  ZmqRecvOp(zmq::socket_t& socket, StreamWatcher& watcher)
    : socket_{socket} 
    , watcher_{watcher}
  {}

  template<class THandler> requires RegularRecvHandler<THandler> || MultipartDynamicRecvHandler<THandler>
  void async_receive(THandler&& handler)
  {
    watcher_.async_wait_receive([this, ch = std::forward<THandler>(handler)]() { 
      handle_ready_recv(std::forward<decltype(ch)>(ch));
    });
  }

private:
  template<class THandler> requires RegularRecvHandler<THandler>
  void handle_ready_recv(THandler&& handler)
  {
    auto msg = zmq::message_t{};
    const auto ret = socket_.recv(msg, zmq::recv_flags::dontwait);
    assert(ret.has_value());
    handler(std::move(msg));
  }

  template<class THandler> requires MultipartDynamicRecvHandler<THandler>
  void handle_ready_recv(THandler&& handler)
  {
    auto parts = std::vector<zmq::message_t>();
    auto parts_count = zmq::recv_multipart(socket_, std::back_inserter(parts));
    handler(std::move(parts));
  }

private:
  zmq::socket_t& socket_;
  StreamWatcher& watcher_;
};
}


