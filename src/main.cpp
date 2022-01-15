#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <bzmq/co_stream_watcher.hpp>
#include <cassert>
#include <client/basic_client.hpp>
#include <core/protocol.hpp>
#include <endpoint/endpoint_config.hpp>
#include <endpoint/message_context.hpp>
#include <protobuf/protobuf_serialization.hpp>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include "icon.pb.h"

namespace posix = boost::asio::posix;

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;

constexpr auto ZmqServerEndpointS1 = "tcp://127.0.0.1:6667";
constexpr auto ZmqServerEndpointS2 = "tcp://127.0.0.1:6668";
void s1()
{
  using namespace icon;
  using namespace icon::details;
  using namespace icon::transport;

  static auto endpoint = icon::setup_default_endpoint(
    icon::address(ZmqServerEndpointS1),
    icon::consumer<ConnectionEstablishReq>(
      [](MessageContext<ConnectionEstablishReq> context) -> awaitable<void> {
        spdlog::info("S1: ConnectionEstablishReq");
        auto& req = context.message();
        auto rsp = ConnectionEstablishCfm{};
        co_await context.async_respond(std::move(rsp));
      }),
    icon::consumer<TestSeqReq>(
      [](MessageContext<TestSeqReq> context) -> awaitable<void> {
        spdlog::info("S1: TestSeqReq");
        auto& req = context.message();
        auto seq_req = req.seq();
        auto seq_rsp = seq_req * 2;

        spdlog::info("S1: recevided seq: {}, sending: {}", seq_req, seq_rsp);
        auto rsp = TestSeqCfm{};
        rsp.set_seq(seq_rsp);

        co_await context.async_respond(std::move(rsp));
      }))
      .build();

  co_spawn(context::boost(), endpoint->run(), detached);
}

void s2()
{
  using namespace icon;
  using namespace icon::details;
  using namespace icon::transport;

  static auto endpoint = icon::setup_default_endpoint(
    icon::address(ZmqServerEndpointS2),
    icon::consumer<ConnectionEstablishReq>(
      [](MessageContext<ConnectionEstablishReq> context) -> awaitable<void> {
        spdlog::info("S2: ConnectionEstablishReq");
        auto& req = context.message();
        auto rsp = ConnectionEstablishCfm{};
        co_await context.async_respond(std::move(rsp));
      }),
    icon::consumer<TestSeqReq>(
      [](MessageContext<TestSeqReq> context) -> awaitable<void> {
        spdlog::info("S2: TestSeqReq");
        auto& req = context.message();
        auto seq_req = req.seq();
        auto seq_rsp = seq_req + 1;

        spdlog::info("S2: recevided seq: {}, sending: {}", seq_req, seq_rsp);
        auto rsp = TestSeqCfm{};
        rsp.set_seq(seq_rsp);

        co_await context.async_respond(std::move(rsp));
      }))
      .build();

  co_spawn(context::boost(), endpoint->run(), detached);
}

void server()
{
  using namespace icon;
  using namespace icon::details;
  using namespace icon::transport;

  s1();
  s2();

  auto& ctx = context::boost();
  using work_guard_type =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  work_guard_type work_guard(ctx.get_executor());
  ctx.run();
}

constexpr size_t NumberOfMessages = 10000;

awaitable<void> run_client_for_s1(icon::details::BasicClient& client, const char* endpoint)
{
  co_await client.async_connect(endpoint);

  for (size_t i = 0; i < NumberOfMessages; i++)
  {
    auto seq_req = icon::transport::TestSeqReq{};
    seq_req.set_seq(i);

    spdlog::info("C1: sending seq req: {}", i);

    auto rsp = co_await client.async_send(std::move(seq_req));
    assert(rsp.is<icon::transport::TestSeqCfm>());
    const auto msg = rsp.get<icon::transport::TestSeqCfm>();

    assert(msg.seq() == i * 2);
    spdlog::info("C1: received seq cfm: {}", msg.seq());
  }
}

awaitable<void> run_client_for_s2(icon::details::BasicClient& client, const char* endpoint)
{
  co_await client.async_connect(endpoint);

  for (size_t i = 0; i < NumberOfMessages; i++)
  {
    auto seq_req = icon::transport::TestSeqReq{};
    seq_req.set_seq(i);

    spdlog::info("C2: sending seq req: {}", i);

    auto rsp = co_await client.async_send(std::move(seq_req));
    assert(rsp.is<icon::transport::TestSeqCfm>());
    const auto msg = rsp.get<icon::transport::TestSeqCfm>();

    assert(msg.seq() == i + 1);
    spdlog::info("C2: received seq cfm: {}", msg.seq());
  }
}

void client()
{
  auto bctx = boost::asio::io_context{};
  auto zctx = zmq::context_t{};
  auto client1 = icon::details::BasicClient{ zctx, bctx };
  auto client2 = icon::details::BasicClient{ zctx, bctx };

  co_spawn(bctx, run_client_for_s1(client1, ZmqServerEndpointS1), detached);
  co_spawn(bctx, run_client_for_s2(client2, ZmqServerEndpointS2), detached);

  using work_guard_type =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  work_guard_type work_guard(bctx.get_executor());
  bctx.run();
}

int main()
{
  spdlog::set_level(spdlog::level::info);

  auto server_th = std::thread(server);
  auto client1_th = std::thread(client);

  server_th.join();
  client1_th.join();
}
