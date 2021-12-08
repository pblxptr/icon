#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include "stream_watcher.hpp"
#include "flags.hpp"
#include <cassert>
#include <sstream>
#include <random>
#include "zmq_recv_op.hpp"
#include "zmq_send_op.hpp"
#include "icon.hpp"

auto io_context = boost::asio::io_context{};
auto zmq_context = zmq::context_t{};

constexpr auto ZmqClientEndpoint = "tcp://127.0.0.1:6667";
constexpr auto ZmqServerEndpoint = "tcp://127.0.0.1:6667";

static constexpr size_t Messages = 100000;
static constexpr size_t NumberOfClientThreads = 3;

static auto rd = std::random_device{};

auto random_delay()
{
  static auto gen = std::mt19937{rd()};
  static auto dist = std::uniform_int_distribution<>(0, 1000);

  return dist(gen);
}

struct CountMessagesHandler
{
  icon::details::ZmqRecvOp& recv_op;
  icon::details::ZmqSendOp& send_op;
  size_t msg {0};

  void start_counter()
  {
    spdlog::debug("Start counter");
    recv_op.async_receive([this](std::vector<zmq::message_t>&& m)
    {
      handle(std::move(m));
    });
  }

  void handle(std::vector<zmq::message_t>&& parts)
  {
    assert(parts.size() == 2);

    msg++;

    spdlog::debug("Number of messages: {}", msg);

    if (msg == Messages * NumberOfClientThreads)
    {
      auto ss = std::stringstream{};
      ss << "Messages received: " << Messages ;

      throw std::runtime_error(ss.str());
    }

    auto messages = std::vector<zmq::message_t>{};
    for (auto&& m : parts) {
      auto new_msg = zmq::message_t{};
      new_msg.copy(m);
      messages.push_back(std::move(new_msg));
    }

    auto send_ch = [this]() { start_counter(); };
    auto send_func = [this, p = std::move(messages), ch = std::move(send_ch), parts = std::move(parts)]() mutable
    {
      spdlog::debug("Sending message");
      send_op.async_send(std::move(p), ch);
    };

    send_func();

  }
};


void server()
{
  namespace posix = boost::asio::posix;

  auto start = std::chrono::system_clock::now();

  try {
    auto local_ctx = zmq::context_t{};

    auto socket = zmq::socket_t{local_ctx, zmq::socket_type::router};
    socket.bind(ZmqServerEndpoint);

    auto watcher = icon::details::StreamWatcher{socket, io_context};
    auto zmq_recv_op = icon::details::ZmqRecvOp{socket, watcher};
    auto zmq_send_op = icon::details::ZmqSendOp{socket, watcher};
    auto counter = CountMessagesHandler{zmq_recv_op, zmq_send_op};
    counter.start_counter();
  
    using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
    work_guard_type work_guard(io_context.get_executor());
    io_context.run();

  } catch(const std::exception& e) {
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = end - start;

    spdlog::info("Time to pass {} messages over the system: {}s", Messages * NumberOfClientThreads, diff.count());
  }

}

void client()
{
  spdlog::info("client: started-wait");
  std::this_thread::sleep_for(std::chrono::seconds(5));
  spdlog::info("client: ready");

  auto counter = static_cast<uint32_t>(0);
  auto local_ctx = zmq::context_t{};
  auto socket = zmq::socket_t{local_ctx, zmq::socket_type::dealer};

  auto send_msg = [&socket](const std::string& value_to_send)
  {
    auto events = socket.get(zmq::sockopt::events);
    do {
      events = socket.get(zmq::sockopt::events);
    } while (!(events & ZMQ_POLLOUT));

    auto message = zmq::message_t{value_to_send};
    const auto ret = socket.send(message, zmq::send_flags::dontwait);
  };

  socket.connect(ZmqClientEndpoint);
  spdlog::info("Client connected");

  std::this_thread::sleep_for(std::chrono::seconds(4));

  while(true) {
    auto value_to_send = std::to_string(counter);
    send_msg(value_to_send);

    auto msg = zmq::message_t{};
    const auto ret = socket.recv(msg);

    assert(msg.to_string() == value_to_send);

    counter++;
    if (counter == Messages) {
      break;
    }
  }

  spdlog::debug("Client finished");

  while(1) {}
}

int main()
{
  boost::asio::posix::stream_descriptor s{io_context};
  


  spdlog::set_level(spdlog::level::debug);

  auto th_server = std::thread(server);
  auto threads = std::vector<std::thread>{};

  for (size_t i = 0; i < NumberOfClientThreads; i++) {
    threads.push_back(std::thread(client));
  }
  
  for (auto& th : threads) {
    th.join();
  }

  th_server.join();
}