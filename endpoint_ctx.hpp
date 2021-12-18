#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>

#include "co_stream_watcher"
#include "zmq_co_recv_op.hpp"
#include "zmq_co_send_op.hpp"

namespace icon::details
{
template<class Derived>
class BasicEndpointContext
{
  using boost::asio::awaitable;
  using boost::asio::use_awaitable;
public:
  using RawBuffer = std::vector<zmq::message_t>;

  BasicEndpointContext(zmq::socket_t socket)
    : socket_{std::move(socket)}
  {}
  awaitable<void> async_run()
  {
    auto recv_op = ZmqCoRecvOp(socket_, watcher_);

    while(true)
    {
      auto buffer = co_await recv_op.receive_async<RawBuffer>();

      impl().process(std::move(buffer));
    }
  }

private:
  auto& impl()
  {
    return static_cast<Derived&>(*this);
  }
protected:
  Co_StreamWatcher watcher_;
  zmq::socket_t socket_;
};
}

namespace icon::details::config
{
class EndpointBuilder
{
public:
  using Raw_t = int;
  using HandleBase_t = std::unique_ptr<ConsumerHandle<Raw_t>>;

  EndpointBuilder(std::string address)
    : address_{std::move(address)}
  {}

  template<class Message, class Consumer>
  void add_consumer(Consumer&& consumer)
  {
    handles_.push_back(std::make_unique<
      Handle<
        Message,
        Raw_t,
        Consumer
      >
    >(std::forward<Consumer>(consumer)));
  }

  std::unique_ptr<Context> build()
  {
    return std::make_unique<EndpointContext>();
  }

private:
  std::string address_;
  std::vector<HandleBase_t> handles_;
};
}

template<class Message, class Consumer>
struct add_consumer_config
{
  void operator()(EndpointBuilder& builder)
  {
    builder.add_consumer<Message, Consumer>(std::forward<Consumer>(c));
  }

  Consumer c;
};