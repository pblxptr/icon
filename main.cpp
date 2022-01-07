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
#include "protobuf_serialization.hpp"
#include "protocol.hpp"
#include "data_types.hpp"
#include "endpoint_config.hpp"

namespace posix = boost::asio::posix;

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::use_awaitable;
using boost::asio::detached;

constexpr auto ZmqServerEndpoint = "tcp://127.0.0.1:6667";
void server()
{
  using namespace icon::details;
  using namespace icon::transport;

   auto endpoint = icon::api::setup_default_endpoint(
    icon::api::address(ZmqServerEndpoint),
    icon::api::consumer<ConnectionEstablishReq>(
      [](MessageContext<ConnectionEstablishReq> req) { spdlog::debug("ConnectionEstablishReq"); }
    ),
    icon::api::consumer<HeartbeatReq>(
      [](MessageContext<HeartbeatReq> req) { spdlog::debug("HeartbeatReq"); }
    )
  )
  .build();

  co_spawn(context::boost(), endpoint->run(), detached);

  auto& ctx = context::boost();
  using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  work_guard_type work_guard(ctx.get_executor());
  ctx.run();
}

// awaitable<void> client_run(icon::details::ZmqClient& client, const char* endpoint)
// {
//   co_await client.connect_async(endpoint);
//   co_await client.connect_async(endpoint);
//   spdlog::debug("Test");
// }

template<class Message>
auto make_message()
{
  auto message_protobuf = icon::details::serialization::protobuf::ProtobufMessage<Message>{{}};
  auto header = icon::transport::Header{};
  header.set_message_number(message_protobuf.message_number());
  auto header_protobuf = icon::details::serialization::protobuf::ProtobufMessage<icon::transport::Header>{header};
 

  auto parts = std::vector<zmq::message_t>{};
  parts.push_back(header_protobuf.serialize());
  parts.push_back(message_protobuf.serialize());

  return parts;
}

void client(const char* endpoint)
{
  auto message_protobuf = icon::details::serialization::protobuf::ProtobufMessage<icon::transport::ConnectionEstablishReq>{{}};

  auto zctx = zmq::context_t{};
  auto socket = zmq::socket_t{zctx, zmq::socket_type::dealer};
  
  socket.connect(ZmqServerEndpoint);
  spdlog::debug("Connected");

  std::this_thread::sleep_for(std::chrono::seconds(3));

  while(1)
  {
    zmq::send_multipart(socket, make_message<icon::transport::ConnectionEstablishReq>());
    spdlog::debug("Sent connection establish req");
    // std::this_thread::sleep_for(std::chrono::seconds(2));
  }
}

int main()
{
  spdlog::set_level(spdlog::level::debug);

  auto server_th = std::thread(server);
  auto client1_th = std::thread(client, ZmqServerEndpoint);

  server_th.join();
  client1_th.join();
}