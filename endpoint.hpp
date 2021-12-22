#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>

#include "co_stream_watcher.hpp"
#include "zmq_co_recv_op.hpp"
#include "zmq_co_send_op.hpp"
#include "protocol.hpp"

namespace {
  using boost::asio::awaitable;
  using boost::asio::use_awaitable;



}

namespace icon::details
{
template<class Impl>
class EndpointIO
{
public:
  EndpointIO(zmq::socket_t socket, boost::asio::io_context& io)
    : socket_{std::move(socket)}
    , watcher_{socket_, io}
    , zmq_recv_op_{socket_, watcher_}
    , zmq_send_op_{socket_, watcher_}
  {}
  template<class RawBuffer>
  awaitable<void> async_run()
  {
    while(true)
    {
      auto buffer = co_await zmq_recv_op_.async_receive<RawBuffer>();

      impl().handle_recv(std::move(buffer));
    }
  }

private:
  auto& impl()
  {
    return static_cast<Impl&>(*this);
  }
protected:
  zmq::socket_t socket_;
  Co_StreamWatcher watcher_;
  ZmqCoRecvOp zmq_recv_op_;
  ZmqCoSendOp zmq_send_op_;
};


class BasicEndpoint : public EndpointIO<BasicEndpoint>
{
  using Raw = zmq::message_t;
  template<class Message>
  using DataType = icon::proto::Protobuf<Message>;
  using Request = icon::details::InternalRequest<
    Raw,
    icon::transport::Header,
    DataType<zmq::message_t>
  >;
  using Protocol_t = icon::details::Protocol<
    Raw,
    std::vector<Raw>,
    icon::details::DataLayout<
      icon::details::fields::Identity,
      icon::details::fields::Header,
      icon::details::fields::Body
    >
  >;

private:
  void handle_recv(Protocol_t::RawBuffer&& buffer)
  {
    auto request = extract_request(std::move(buffer));
    auto consumer = find_consumer(request.message_number());
    if (!consumer) 
      return;

    consumer->handle(this, std::move(request));
  }

  auto extract_request(Protocol_t::RawBuffer&& buffer)
  {
    auto parser = icon::details::Parser<Protocol_t>{std::move(buffer)};
    auto identity = std::move(parser).get<icon::fields::Identity>();
    auto header = DataType_t{std::move(parser).get<icon::details::fields::Header>()};
    auto body = DataType_t{std::move(parser).get<icon::details::fields::Body>()};


    auto respond_call = [this](Request&& req, auto&& msg)
    {

    }

    return Request{
      std::move(identity),
      icon::proto::deserialize<icon::transport::Header>(std::move(header)),
      std::move(body)
    };
  }

  template<class Message>
  void send_async(Message&& m)
  {
    auto parser = icon::details::Parser<Protocol_t>{};
  }


private:
  std::vector<std::string> addresses_;
  std::unordered_map<size_t, std::unique_ptr<ConsumerHandler>> handlers_;
};

}




template<
  class Identity,
  class Header,
  class Body
>
class Message
{

}