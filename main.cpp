#include <boost/asio.hpp>
#include <zmqpp/zmqpp.hpp>
#include <spdlog/spdlog.h>
#include <thread>
#include <zmqpp/poller.hpp>
#include <vector>
#include "zmq_socket_event_bus.hpp"

auto io_context = boost::asio::io_context{};
auto zmq_context = zmqpp::context_t{};
auto event_bus = ZmqSocketEventBus{};

constexpr auto ZmqEndpoint = "inproc://zmq_boost";

auto get_fd(zmqpp::socket_t& socket)
{
  return socket.get<int>(zmqpp::socket_option::file_descriptor);
}

class ZmqSocketDescriptor
{
public:
  ZmqSocketDescriptor(zmqpp::socket_t zmq_socket, boost::asio::io_context& ctx, ZmqSocketEventBus& bus)
    : zmq_socket_{std::move(zmq_socket)}
    , streamd_{ctx, get_fd(zmq_socket_)}
    , bus_{bus}
{
}
  void async_wait()
  {
    wait_for_error();
    wait_for_read();
  }

  void send(zmqpp::message_t&& msg)
  {
    spdlog::debug("ZmqSocketDescriptor::send()");
    
    zmq_socket_.send(msg);
    check_events();
  }
private:
  void wait_for_error()
  {
    spdlog::debug("ZmqSocketDescriptor::wait_for_error()");
    streamd_.async_wait(boost::asio::posix::stream_descriptor::wait_type::wait_error,
      [this](const auto& ec) { 
        if (ec) {
          spdlog::debug("wait_for_error: {}", ec.message());
          return;
        }
        check_events(); 
      }
    );
  }

  void wait_for_read()
  {
    spdlog::debug("ZmqSocketDescriptor::wait_for_read()");
    streamd_.async_wait(boost::asio::posix::stream_descriptor::wait_type::wait_read,
      [this](const auto& ec) { 
        if (ec) {
          spdlog::debug("wait_for_read: {}", ec.message());
          return;
        }

        check_events(); 
      }
    );
  }

  void check_events()
  {
    const auto events = zmq_socket_.get<int>(zmqpp::socket_option::events);

    if (events & ZMQ_POLLERR) {
      handle_error();
    } else if (events & ZMQ_POLLIN) {
      handle_read();
    } else {
      async_wait();
    }
  }

  void handle_error()
  {
    spdlog::debug("ZmqSocketDescriptor::check_for_error()");
    
    check_events(); 
  }

  void handle_read()
  {
    spdlog::debug("ZmqSocketDescriptor::check_for_read()");

    auto msg = zmqpp::message_t{};

    zmq_socket_.receive(msg);

    bus_.publish(ZmqMessageReceived{
      *this, 
      std::move(msg)
    });

    check_events();
  }

private:
  zmqpp::socket_t zmq_socket_;
  boost::asio::posix::stream_descriptor streamd_;
  ZmqSocketEventBus& bus_;
};

void server()
{
  namespace posix = boost::asio::posix;

  event_bus.subscribe<ZmqMessageReceived>([](const auto& event)
  {
    spdlog::debug("Event: ZmqMessage::Received, Parts: {}", event.message.parts());
  });

  
  auto zmq_socket = zmqpp::socket{zmq_context, zmqpp::socket_type::router};
  zmq_socket.bind(ZmqEndpoint);

  auto zmq_stream_descriptor = ZmqSocketDescriptor{std::move(zmq_socket), io_context, event_bus};
  zmq_stream_descriptor.async_wait();


  io_context.run();
}

void client()
{
  spdlog::info("client: started-wait");
  std::this_thread::sleep_for(std::chrono::seconds(5));
  spdlog::info("client: ready");

  spdlog::info("Client before connect && send");
  auto zmq_socket = zmqpp::socket{zmq_context, zmqpp::socket_type::dealer};
  zmq_socket.connect(ZmqEndpoint);
  zmq_socket.send("test");
  spdlog::info("Client sent 1st msg\n");

  spdlog::info("Client before send");
  std::this_thread::sleep_for(std::chrono::seconds(5));
  zmq_socket.send("test2");
  spdlog::info("Client sent 2nd message\n");

  while (1) {}
}

int main()
{
  spdlog::set_level(spdlog::level::debug);
  auto th_server = std::thread(server);
  auto th_client = std::thread(client);

  th_server.join();
  th_client.join();
}