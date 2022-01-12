#pragma once

#include <bzmq/co_stream_watcher.hpp>
#include <bzmq/zmq_co_recv_op.hpp>
#include <bzmq/zmq_co_send_op.hpp>
#include <endpoint/endpoint.hpp>

namespace icon::details {
class BaseEndpoint : public Endpoint
{
protected:
  using Raw_t = zmq::message_t;
  using RawBuffer_t = std::vector<Raw_t>;

  BaseEndpoint(zmq::socket_t socket, boost::asio::io_context& bcxt)
    : socket_{ std::move(socket) }, watcher_{ socket_, bcxt },
      zmq_recv_op_{ socket_, watcher_ }, zmq_send_op_{ socket_, watcher_ } {}

protected:
  awaitable<RawBuffer_t> async_recv_base()
  {
    co_return co_await zmq_recv_op_.async_receive<RawBuffer_t>();
  }

  awaitable<void> async_send_base(RawBuffer_t&& buffer)
  {
    co_await zmq_send_op_.async_send(std::move(buffer));
  }

protected:
  zmq::socket_t socket_;
  Co_StreamWatcher watcher_;
  ZmqCoRecvOp zmq_recv_op_;
  ZmqCoSendOp zmq_send_op_;
};
}// namespace icon::details