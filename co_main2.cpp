#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>
#include <cassert>
#include "co_stream_watcher.hpp"

namespace posix = boost::asio::posix;

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::use_awaitable;
using boost::asio::detached;

constexpr auto ZmqServerEndpoint = "tcp://127.0.0.1:6667";
constexpr auto ZmqServerEndpoint2 = "tcp://127.0.0.1:6668";

auto bctx = boost::asio::io_context{};


class Server
{
public:
  Server(boost::asio::io_context& bctx, zmq::context_t& zctx)
    : streamd_{bctx}
    , socket_{zctx, zmq::socket_type::router}
  {}

  void set_endpoint(const char* endpoint)
  {
    socket_.bind(endpoint);
  }

  awaitable<void> run()
  {
    streamd_.assign(socket_.get(zmq::sockopt::fd));

    while(1)
    {
      try_read_and_deliver();

      co_await streamd_.async_wait(posix::stream_descriptor::wait_type::wait_read, use_awaitable);

      try_read_and_deliver();
    }

    co_return;
  }
private:
  void try_read_and_deliver()
  {
    while(ready_to_read()) {
      receive_and_deliver();
    }
  }

  bool ready_to_read()
  {
    return socket_.get(zmq::sockopt::events) & ZMQ_POLLIN;
  }

  void receive_and_deliver()
  {
    auto vec = std::vector<zmq::message_t>{};
    const auto number_of_parts = zmq::recv_multipart(socket_, std::back_inserter(vec), zmq::recv_flags::dontwait);

    assert(number_of_parts == 2);

    auto& id = vec[0];
    auto& msg = vec[1];

    spdlog::debug("Received message: {}", msg.to_string());
  }
private:
  posix::stream_descriptor streamd_;
  zmq::socket_t socket_;
};

struct Client
{
  Client(boost::asio::io_context& bctx, zmq::context_t& zctx)
  : streamd_{bctx}
  , socket_{zctx, zmq::socket_type::dealer}
  {}

  void connect(const char* endpoint)
  {

  }

  awaitable<std::string> 
}


void server()
{
  auto zctx = zmq::context_t{};

  spdlog::debug("Before spawn for s1");
  auto s1 = Server{bctx, zctx};
  s1.set_endpoint(ZmqServerEndpoint);
  co_spawn(bctx, s1.run(), detached);
  spdlog::debug("After spawn for s1");

  spdlog::debug("Before spawn for s2");
  auto s2 = Server(bctx, zctx);
  s2.set_endpoint(ZmqServerEndpoint2);
  co_spawn(bctx, s2.run(), detached);
  spdlog::debug("After spawn for s2");

  using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  work_guard_type work_guard(bctx.get_executor());


  bctx.run();
}

void client(const char* endpoint)
{
  spdlog::debug("Client...will connect in a few seconds...");
  std::this_thread::sleep_for(std::chrono::seconds(5));

  auto zctx = zmq::context_t{};
  auto socket = zmq::socket_t{zctx, zmq::socket_type::dealer};
  socket.connect(endpoint);

  spdlog::debug("Connected...waiting...");
  std::this_thread::sleep_for(std::chrono::seconds(5));
  spdlog::debug("Sending message");

  auto message = zmq::message_t{std::string("Test message")};
  socket.send(message, zmq::send_flags::dontwait);

  while(1);
}

int main()
{
  spdlog::set_level(spdlog::level::debug);

  auto server_th = std::thread(server);
  auto client1_th = std::thread(client, ZmqServerEndpoint);
  auto client2_th = std::thread(client, ZmqServerEndpoint2);

  server_th.join();
  client1_th.join();
  client2_th.join();
}