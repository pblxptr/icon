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
#include "protobuf_field.hpp"
#include "protocol.hpp"
#include "data_types.hpp"
#include "endpoint_ctx.hpp"

namespace posix = boost::asio::posix;

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::use_awaitable;
using boost::asio::detached;

constexpr auto ZmqServerEndpoint = "tcp://127.0.0.1:6667";

auto bctx = boost::asio::io_context{};

// auto build_request(Protocol_t::RawBuffer&& messages)
// {
//   auto parser = icon::details::Parser<Protocol_t>{std::move(messages)};
//   auto header = icon::proto::ProtobufData{std::move(parser).get<icon::details::fields::Header>()};
//   auto body = icon::proto::ProtobufData{std::move(parser).get<icon::details::fields::Body>()};

//   return icon::details::InternalRequest{
//     1,
//     icon::proto::deserialize<icon::transport::Header>(std::move(header)),
//     std::move(body)
//   };
// }

void server()
{
  using namespace icon::details;
  using namespace icon::transport;

  auto context = icon::api::setup_stateless_endpoint(
    icon::api::address("tcp://localhost:5343"),
    icon::api::consumer<ConnectionEstablishReq>(
      [](MessageContext<ConnectionEstablishReq> req) { spdlog::debug("ConnectionEstablishReq"); }
    ),
    icon::api::consumer<HeartbeatReq>(
      [](MessageContext<HeartbeatReq> req) { spdlog::debug("HeartbeatReq"); }
    )
  )
  .build();

  context->run();


  // auto zctx = zmq::context_t{};
  // auto socket = zmq::socket_t{zctx, zmq::socket_type::router};
  // socket.bind(ZmqServerEndpoint);

  // while (1)
  // {
  //   spdlog::debug("server loop");
  //   auto recv_messages = std::vector<zmq::message_t>{};
  //   auto nparts = zmq::recv_multipart(socket, std::back_inserter(recv_messages));

  //   spdlog::debug("Server: received");

  //   if (!nparts) {
  //     continue;
  //   }

  //   auto request = build_request(std::move(recv_messages));
  //   if (not request.is<icon::transport::ConnectionEstablishReq>()) {
  //     spdlog::debug("Received invalid con req messaege");

  //     continue;
  //   }

    spdlog::debug("Ok!!");

    // if (header.message_number() == icon::proto::get_message_number<icon::transport::ConnectionEstablishReq>()) {
    //   spdlog::debug("Connection establish");
    //   header.set_message_number(icon::proto::get_message_number<icon::transport::ConnectionEstablishCfm>());
    //   parser.set<icon::details::fields::Header>(icon::proto::serialize<Protocol_t::Raw>(icon::proto::ProtobufData(std::move(header))));
    //   parser.set<icon::details::fields::Body>(icon::proto::serialize<Protocol_t::Raw>(icon::proto::ProtobufData(icon::transport::ConnectionEstablishCfm{})));

    //   const auto nparts = zmq::send_multipart(socket, std::move(parser).parse());
    //   spdlog::debug("Parts sent: {}", nparts.value_or(0));

    // } else {
    //   spdlog::debug("Invalid message");
    // }
  }


  // using work_guard_type = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  // work_guard_type work_guard(bctx.get_executor());


  // bctx.run();
}

awaitable<void> client_run(icon::details::ZmqClient& client, const char* endpoint)
{
  co_await client.connect_async(endpoint);
  co_await client.connect_async(endpoint);
  spdlog::debug("Test");

}

void client(const char* endpoint)
{
  auto local_bctx = boost::asio::io_context{};
  auto zctx = zmq::context_t{};
  auto client = icon::details::ZmqClient{zctx, local_bctx};

  std::this_thread::sleep_for(std::chrono::seconds(5));

  co_spawn(local_bctx, client_run(client, endpoint), detached);
  spdlog::debug("after spawn");


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