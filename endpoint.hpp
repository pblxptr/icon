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
#include "zmq_serialization.hpp"

namespace {
  using boost::asio::awaitable;
  using boost::asio::use_awaitable;
}

namespace icon::details
{

class Endpoint
{
public:
  virtual ~Endpoint() = default;
  virtual awaitable<void> run() = 0;
};

class BaseEndpoint : public Endpoint
{
protected:
  using Raw_t = zmq::message_t;
  using RawBuffer_t = std::vector<Raw_t>;

  BaseEndpoint(zmq::socket_t socket, boost::asio::io_context& bcxt)
    : socket_{std::move(socket)}
    , watcher_{socket_, bcxt}
    , zmq_recv_op_{socket_, watcher_}
    , zmq_send_op_{socket_, watcher_}
  {}

public:
  awaitable<RawBuffer_t> async_recv()
  {
    co_return co_await zmq_recv_op_.async_receive<RawBuffer>();
  }

  template<class Message>
  awaitable<void> async_send(Message&& msg)
  {
    auto buffer = prepare_send_buffer(std::forward<Message>(msg));

    co_await zmq_send_op.async_send(std::move(buffer))
  }

protected:
  zmq::socket_t socket_;
  Co_StreamWatcher watcher_;
  ZmqCoRecvOp zmq_recv_op_;
  ZmqCoSendOp zmq_send_op_;
};

class BasicEndpoint : public BaseEndpoint
{
public:
  using BaseEndpoint::Raw_t;
  using BaseEndpoint::RawBuffer_t;
  template<class Message>
  using ProtocolData_t      = icon::details::serialization::ProtobufData<Message>;
  template<class Message>
  using ZmqData_t           = icon::details::serialization::ZmqData<Message>;
  using Request_t           = icon::details::InternalRequest<ProtocolData_t<Raw_t>>;
  using Response_t          = icon::details::InternalResponse<ProtocolData_t<Raw_t>>;
  using ConsumerHandlerBase_t = ConsumerHandler<BasicEndpoint, Request_t>;
  template<
    class Message,
    class Consumer
  >
  using ConsumerHandler_t   = BasicEndpointConsumerHandler<
    BasicEndpoint, 
    Request_t,
    Message,
    Consumer
  >;
  using Protocol_t = icon::details::Protocol<
    Raw_t,
    RawBuffer_t,
    icon::details::DataLayout<
      icon::details::fields::Identity,
      icon::details::fields::Header,
      icon::details::fields::Body
    >
  >;

  BasicEndpoint
    (
      zmq::context_t& zctx, 
      boost::asio::io_context& bctx,
      std::vector<std::string> addresses,
      std::unordered_map<size_t, std::unique_ptr<ConsumerHandlerBase_t>> handlers
    )
    : BaseEndpoint(zmq::socket_t{zctx, zmq::socket_type::router}, bctx)
    , handlers_{std::move(handlers)}
  {
    for (const auto& addr : addresses)
    {
      socket_.bind(addr);
    }
  }

  awaitable<void> run() override
  {
    while (true)
    {
      auto buffer = co_await async_recv<RawBuffer_t>();
      handle_recv(std::move(buffer));
    }
  }
  
private:
  void handle_recv(Protocol_t::RawBuffer&& buffer)
  {
    auto request = extract_request(buffer);
    const auto& header = request.header();
    auto consumer = find_consumer(header.message_number());
    if (!consumer) 
      return;

    consumer->handle(*this, std::move(request));
    }

  Request_t extract_request(Protocol_t::RawBuffer& buffer)
  {
    auto parser = Parser<Protocol_t>(std::move(buffer));
    auto raw_identity = ZmqData_t(parser.get<icon::details::fields::Identity>());
    auto raw_header   = ProtocolData_t(parser.get<icon::details::fields::Header>());
    auto raw_body     = ProtocolData_t(parser.get<icon::details::fields::Body>());

    return Request_t{
      deserialize<icon::details::fields2::Identity>(raw_identity),
      deserialize<icon::details::fields2::Header>(raw_header),
      std::move(raw_body)
    };
  }

  ConsumerHandlerBase_t* find_consumer(const size_t message_number)
  {
    if (!handlers_.contains(message_number)) {
      return nullptr;
    } else {
      return handlers_[message_number].get();
    }
  }

  template<class Request>
  void async_send(Request&& req)
  {
    // auto parser = Parser<Protocol_t>{};
    // parser.set<icon::fields::Identity>(serialize<Raw>(req.identity()));
    // parser.set<icon::fields::Header>(serialize<Raw>(req.header()));
    // parser.set<icon::fields::Body>(serialize<Raw>(req.body()));
    // auto data = parser.parse();
  }




private:
  std::vector<std::string> addresses_;
  std::unordered_map<size_t, std::unique_ptr<ConsumerHandlerBase_t>> handlers_;
};

}
 