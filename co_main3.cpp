#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <spdlog/spdlog.h>
#include <cassert>
#include "co_stream_watcher.hpp"
#include "zmq_client.hpp"
#include "icon.pb.h"

namespace posix = boost::asio::posix;

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::use_awaitable;
using boost::asio::detached;

constexpr auto ZmqServerEndpoint = "tcp://127.0.0.1:6667";

auto bctx = boost::asio::io_context{};


void server()
{
  auto zctx = zmq::context_t{};
  auto socket = zmq::socket_t{zctx, zmq::socket_type::router};
  socket.bind(ZmqServerEndpoint);

  while (1)
  {
    spdlog::debug("server loop");
    auto recv_messages = std::vector<zmq::message_t>{};
    auto parts = zmq::recv_multipart(socket, std::back_inserter(recv_messages));

    spdlog::debug("Server: received");

    if (!parts) {
      continue;
    }

    switch(*parts)
    {
      case 2: {
        auto& id = recv_messages[0];
        auto& msg = recv_messages[1];
        if (msg.to_string() == "syn") {
          auto resp_messages = std::vector<zmq::message_t>{};
          resp_messages.push_back(std::move(id));
          resp_messages.emplace_back(std::string("ack"));
          zmq::send_multipart(socket, resp_messages);
        }
      }
      case 3: {
        auto& id = recv_messages[0];
        auto& msg_number = recv_messages[1];
        auto& msg = recv_messages[2];

        if (msg_number.to_string() == std::to_string(0x10000001)) {
          auto resp = icon::ConnectionEstablishCfm{};
          auto resp_buffer = std::vector<zmq::message_t>();
          auto resp_msg_number = zmq::message_t{std::to_string(0x10000002)};
          auto resp_msg_body = zmq::message_t{resp.ByteSizeLong()};
          resp.SerializeToArray(resp_msg_body.data(), resp_msg_body.size());

          resp_buffer.push_back(std::move(id));
          resp_buffer.push_back(std::move(resp_msg_number));
          resp_buffer.push_back(std::move(resp_msg_body));
          zmq::send_multipart(socket, resp_buffer, zmq::send_flags::dontwait);
        }

        spdlog::debug("Msg number: {}", msg_number.to_string());
      }
    }





  }


  // using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  // work_guard_type work_guard(bctx.get_executor());


  // bctx.run();
}

awaitable<void> client_run(icon::details::ZmqClient& client, const char* endpoint)
{
  co_await client.connect_async(endpoint);

}

void client(const char* endpoint)
{
  auto local_bctx = boost::asio::io_context{};
  auto zctx = zmq::context_t{};
  auto client = icon::details::ZmqClient{zctx, local_bctx};

  std::this_thread::sleep_for(std::chrono::seconds(5));

  co_spawn(local_bctx, client_run(client, endpoint), detached);


  using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  work_guard_type work_guard(local_bctx.get_executor());
  local_bctx.run();
}

int main()
{
  spdlog::set_level(spdlog::level::debug);

  auto server_th = std::thread(server);
  auto client1_th = std::thread(client, ZmqServerEndpoint);

  server_th.join();
  client1_th.join();
}