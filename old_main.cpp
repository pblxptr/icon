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
#include "icon.hpp"

auto io_context = boost::asio::io_context{};
auto zmq_context = zmq::context_t{};

constexpr auto ZmqClientEndpoint = "tcp://127.0.0.1:6667";
constexpr auto ZmqServerEndpoint = "tcp://127.0.0.1:6667";

static constexpr size_t Messages = 10000;
static constexpr size_t NumberOfClientThreads = 2;

static auto rd = std::random_device{};

auto random_delay()
{
  static auto gen = std::mt19937{rd()};
  static auto dist = std::uniform_int_distribution<>(0, 1000);

  return dist(gen);
}

// class ZmqReadOp
// {
// public:
//   explicit ZmqReadOp(zmq::socket_t& socket, icon::details::StreamWatcher& watcher)
//     : socket_{socket} 
//     , watcher_{watcher}
// {}

//   void run()
//   {
//     watcher_.async_wait_receive([this]() { handle(); });
//   }

//   void handle()
//   {
//     spdlog::debug("ZmqReadOp::handle()");
  
//     auto parts = std::vector<zmq::message_t>();
//     auto parts_count = zmq::recv_multipart(socket_, std::back_inserter(parts));

//     spdlog::debug("Parts received: {}", parts_count.value_or(0));

//     ++count_;

//     assert(parts_count == 2);

//     spdlog::debug("Number of messages: {}", count_);

//     // std::this_thread::sleep_for(std::chrono::milliseconds(random_delay()));

//     if (count_ == Messages * NumberOfClientThreads) {
//       auto ss = std::stringstream{};
//       ss << "Messages received: " << Messages ;

//       throw std::runtime_error(ss.str());
//     }

//     run();
//   }

// size_t count_{0};

// private:
//   zmq::socket_t& socket_;
//   icon::details::StreamWatcher& watcher_;
// };

struct CountMessagesHandler
{
  icon::details::ZmqRecvOp& recv_op;
  size_t msg {0};

  void start_counter()
  {
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

    start_counter();
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
    auto counter = CountMessagesHandler{zmq_recv_op};
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

  size_t counter = 0;

  auto local_ctx = zmq::context_t{};
  auto zmq_socket = zmq::socket_t{local_ctx, zmq::socket_type::dealer};

  auto send_msg = [&zmq_socket]()
  {
    auto events = zmq_socket.get(zmq::sockopt::events);
    do {
      events = zmq_socket.get(zmq::sockopt::events);
    } while (!(events & ZMQ_POLLOUT));

    const auto ret = zmq_socket.send(zmq::str_buffer("Testtesttest"), zmq::send_flags::dontwait);

    // spdlog::info("Send ret: {}, {}", ret.value_or(999999), zmq_errno());
  };

  zmq_socket.connect(ZmqClientEndpoint);
  spdlog::info("Client connected");

  send_msg();
  spdlog::info("Client sent 1st msg\n");

  std::this_thread::sleep_for(std::chrono::seconds(4));
  send_msg();
  spdlog::info("Client sent 2nd message\n");

  while(true) {
    send_msg();
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
  // auto th_client = std::thread(client);

  auto threads = std::vector<std::thread>{};

  for (size_t i = 0; i < NumberOfClientThreads; i++) {
    threads.push_back(std::thread(client));
  }
  
  for (auto& th : threads) {
    th.join();
  }

  th_server.join();
  // th_client.join();
}