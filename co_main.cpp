#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>
#include <cassert>

namespace posix = boost::asio::posix;

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::use_awaitable;
using boost::asio::detached;

constexpr auto ZmqServerEndpoint = "tcp://127.0.0.1:6667";
constexpr auto ZmqClientEndpoint = "tcp://127.0.0.1:6667";

auto bctx = boost::asio::io_context{};

void receive_message(zmq::socket_t& socket)
{
  auto vec = std::vector<zmq::message_t>{};
  const auto number_of_parts = zmq::recv_multipart(socket, std::back_inserter(vec), zmq::recv_flags::dontwait);

  assert(number_of_parts == 2);

  auto& id = vec[0];
  auto& msg = vec[1];

  spdlog::debug("Received message: {}", msg.to_string());
}

bool can_receive(zmq::socket_t& socket)
{
  auto events = socket.get(zmq::sockopt::events);

  return events & ZMQ_POLLIN;
}

awaitable<void> read_from_socket(posix::stream_descriptor& streamd, zmq::socket_t& socket)
{
  if (can_receive(socket)) {
    receive_message(socket);
  } else {
    co_await streamd.async_wait(posix::stream_descriptor::wait_type::wait_read, use_awaitable);
    if (can_receive(socket)) {
      receive_message(socket);
    } else {
      co_spawn(bctx, read_from_socket(streamd, socket), detached);
    }
  }
}

void server()
{
  auto zctx = zmq::context_t{};
  auto socket = zmq::socket_t{zctx, zmq::socket_type::router};
  socket.bind(ZmqServerEndpoint);
  auto streamd = posix::stream_descriptor{bctx, socket.get(zmq::sockopt::fd)};

  spdlog::debug("Before spawn");
  co_spawn(bctx, read_from_socket(streamd, socket), detached);
  spdlog::debug("After spawn");

  using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  work_guard_type work_guard(bctx.get_executor());


  bctx.run();
}

void client()
{
  spdlog::debug("Client...will connect in a few seconds...");
  std::this_thread::sleep_for(std::chrono::seconds(5));

  auto zctx = zmq::context_t{};
  auto socket = zmq::socket_t{zctx, zmq::socket_type::dealer};
  socket.connect(ZmqClientEndpoint);

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
  auto client_th = std::thread(client);

  server_th.join();
  client_th.join();
}