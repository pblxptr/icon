#pragma once

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>

#include "co_stream_watcher"

namespace icon::details
{
namespace {
  class ZmqCoRecvOp
  {
    using boost::asio::awaitable;
    using boost::asio::use_awaitable;
  public:
    ZmqCoRecvOp(zmq::socket_t& socket, Co_StreamWatcher& watcher)
      : socket_{socket}
      , watcher_{watcher}
    {}

    template<class RawBuffer>
    awaitable<RawBuffer> async_receive()
    {
      auto ret = co_await watcher_.async_wait_receive();
      assert(ret);

      auto buffer = RawBuffer{};
      const auto numberOfMessages = zmq::recv_multipart(socket_, std::back_inserter(buffer), zmq::recv_flags::dontwait);
      assert(nbytes != 0);

      co_return buffer;
    }

    template<class RawBuffer>
    awaitable<RawBuffer> async_receive() requires std::is_same_v<RawBuffer, zmq::message_t>;
    {
      auto ret = co_await watcher_.async_wait_receive();
      if (not ret) {
        co_return RawBuffer{};
      }

      auto buffer = zmq::message_t{};
      const auto nbytes = socket_.recv(buffer, zmq::recv_flags::dontwait);
      assert(nbytes != 0);

      co_return buffer;
    }
  private:
    zmq::socket_t& socket_;
    Co_StreamWatcher& watcher_;
  };

  class ZmqCoSendOp
  {
    using boost::asio::awaitable;
    using boost::asio::use_awaitable;
  public:
    ZmqCoSendOp(zmq::socket_t& socket, Co_StreamWatcher& watcher)
      : socket_{socket}
      , watcher_{watcher}
    {}

    template<class RawBuffer>
    awaitable<void> async_send(RawBuffer&& buffer)
    {
      const auto ret = co_await watcher_.async_wait_send();
      assert(ret);

      const auto nmessages = zmq::send_multipart(socket_, std::forward<RawBuffer>(buffer), zmq::send_flags::dontwait);
      assert(nmessages == buffer.size());
    }

    awaitable<void> async_send(RawBuffer&& buffer) requires std::is_same_v<RawBuffer, zmq::message_t>
    {
      const auto ret = co_await watcher_.async_wait_send();
      assert(ret);

      const auto nbytes = socket_.send(std::forward<RawBuffer>(buffer));
      assert(nbytes != 0);
    }
  private:
    zmq::socket_t& socket_;
    Co_StreamWatcher& watcher_;
  }
}

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