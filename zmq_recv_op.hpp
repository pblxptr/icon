#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
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

  template<class THandler> requires RegularHandler<THandler>
  void async_receive(THandler&& handler)
  {
    watcher_.async_wait_receive([this, ch = std::forward<THandler>(handler)]() { 
      handle_regular(std::forward<THandler>(ch));
    });
  }

  template<class THandler> requires MultipartDynamicHandler<THandler>
  void async_receive(THandler&& handler)
  {
    watcher_.async_wait_receive([this, ch = std::forward<THandler>(handler)]() { 
      handle_multipart_dynamic(std::forward<THandler>(ch));
    });
  }

  // void run()
  // {
  //   watcher_.async_wait_receive([this]() { handle(); });
  // }


  // void handle()
  // {
  //   spdlog::debug("ZmqReadOp::handle()");
  
  //   auto parts = std::vector<zmq::message_t>();
  //   auto parts_count = zmq::recv_multipart(socket_, std::back_inserter(parts));
  // }

private:
  template<class THandler>
  void handle_regular(THandler&& handler)
  {
    auto msg = zmq::message_t{};
    socket_.recv(msg, zmq::recv_flags::none);

    handler(std::move(msg));
  }

  template<class THandler>
  void handle_multipart_dynamic(THandler&& handler)
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


