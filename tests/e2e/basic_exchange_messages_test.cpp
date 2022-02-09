#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>
#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <cassert>

#include <icon/bzmq/co_stream_watcher.hpp>
#include <icon/client/basic_client.hpp>
#include <icon/core/protocol.hpp>
#include <icon/endpoint/endpoint_config.hpp>
#include <icon/endpoint/message_context.hpp>
#include <icon/protobuf/protobuf_serialization.hpp>

// TODO: Refactor, it's just a very ugly draft.

namespace posix = boost::asio::posix;

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;

constexpr auto ZmqServerEndpointS1 = "tcp://127.0.0.1:9998";
constexpr auto ZmqServerEndpointS2 = "tcp://127.0.0.1:9999";
constexpr size_t NumberOfMessages = 500;
size_t ServerReceivedMessages = 0;
size_t ClientSentMessages = 0;

auto zctx = zmq::context_t{};

awaitable<void> message_watcher(boost::asio::io_context& ctx, const size_t expected_msgs, std::function<size_t()> get_actual)
{
  auto executor = co_await boost::asio::this_coro::executor;
  auto timer = boost::asio::steady_timer{ executor };

  while (expected_msgs != get_actual()) {
    spdlog::info("Expected: {}, Actual: {}", expected_msgs, get_actual());

    timer.expires_after(std::chrono::seconds(1));
    co_await timer.async_wait(use_awaitable);

    spdlog::info("Test");
  }

  spdlog::info("Reseting context");

  ctx.stop();
}

awaitable<void> s1(boost::asio::io_context& bctx, zmq::context_t& zctx)
{
  using namespace icon;
  using namespace icon::details;

  static auto endpoint = icon::setup_default_endpoint(
    icon::use_services(bctx, zctx),
    icon::address(ZmqServerEndpointS1),
    icon::consumer<TestSeqReq>(
      [](MessageContext<TestSeqReq> context) -> awaitable<void> {
        spdlog::info("S1: TestSeqReq");
        auto& req = context.message();
        auto seq_req = req.seq();
        auto seq_rsp = seq_req * 2;

        spdlog::info("S1: recevided seq: {}, sending: {}", seq_req, seq_rsp);
        auto rsp = TestSeqCfm{};
        rsp.set_seq(seq_rsp);

        ++ServerReceivedMessages;

        co_await context.async_respond(std::move(rsp));
      }))
                           .build();

  co_await endpoint->run();
}

awaitable<void> s2(boost::asio::io_context& bctx, zmq::context_t& zctx)
{
  using namespace icon;
  using namespace icon::details;

  static auto endpoint = icon::setup_default_endpoint(
    icon::use_services(bctx, zctx),
    icon::address(ZmqServerEndpointS2),
    icon::consumer<TestSeqReq>(
      [](MessageContext<TestSeqReq> context) -> awaitable<void> {
        spdlog::info("S2: TestSeqReq");
        auto& req = context.message();
        auto seq_req = req.seq();
        auto seq_rsp = seq_req + 1;

        spdlog::info("S2: recevided seq: {}, sending: {}", seq_req, seq_rsp);
        auto rsp = TestSeqCfm{};
        rsp.set_seq(seq_rsp);

        ++ServerReceivedMessages;

        co_await context.async_respond(std::move(rsp));
      }))
                           .build();

  co_await endpoint->run();
}

void server()
{
  using namespace icon;
  using namespace icon::details;

  static auto bctx = boost::asio::io_context{};

  co_spawn(bctx, s1(bctx, zctx), detached);
  co_spawn(bctx, s2(bctx, zctx), detached);
  co_spawn(bctx, message_watcher(bctx, NumberOfMessages * 2, []() { return ServerReceivedMessages; }), detached);

  using work_guard_type =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  work_guard_type work_guard(bctx.get_executor());
  bctx.run();

  spdlog::info("Server thread stop");
}

awaitable<void> run_client_for_s1(icon::BasicClient& client, const char* endpoint)
{
  co_await client.async_connect(endpoint);

  for (size_t i = 0; i < NumberOfMessages; i++) {
    auto seq_req = icon::TestSeqReq{};
    seq_req.set_seq(i);

    spdlog::info("C1: sending seq req: {}", i);

    auto rsp = co_await client.async_send(std::move(seq_req));
    assert(rsp.is<icon::TestSeqCfm>());
    const auto msg = rsp.get<icon::TestSeqCfm>();

    assert(msg.seq() == i * 2);
    ClientSentMessages++;
    spdlog::info("C1: received seq cfm: {}", msg.seq());
  }

  spdlog::info("Client1 completed");
}

awaitable<void> run_client_for_s2(icon::BasicClient& client, const char* endpoint)
{
  co_await client.async_connect(endpoint);

  for (size_t i = 0; i < NumberOfMessages; i++) {
    auto seq_req = icon::TestSeqReq{};
    seq_req.set_seq(i);

    spdlog::info("C2: sending seq req: {}", i);

    auto rsp = co_await client.async_send(std::move(seq_req));
    assert(rsp.is<icon::TestSeqCfm>());
    const auto msg = rsp.get<icon::TestSeqCfm>();

    assert(msg.seq() == i + 1);
    ClientSentMessages++;
    spdlog::info("C2: received seq cfm: {}", msg.seq());
  }

  spdlog::info("Client2 completed");
}

void client()
{
  auto bctx_cl = boost::asio::io_context{};
  auto zctx_cl = zmq::context_t{};
  auto client1 = icon::BasicClient{ zctx_cl, bctx_cl };
  auto client2 = icon::BasicClient{ zctx_cl, bctx_cl };

  co_spawn(bctx_cl, run_client_for_s1(client1, ZmqServerEndpointS1), detached);
  co_spawn(bctx_cl, run_client_for_s2(client2, ZmqServerEndpointS2), detached);
  co_spawn(bctx_cl, message_watcher(bctx_cl, NumberOfMessages * 2, []() { return ClientSentMessages; }), detached);

  using work_guard_type =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  work_guard_type work_guard(bctx_cl.get_executor());
  bctx_cl.run();

  spdlog::info("Client thread stop");
}

bool task_completed()
{
  return (ServerReceivedMessages == NumberOfMessages * 2)
         && ClientSentMessages == NumberOfMessages * 2;
}

void guard_func()
{
  size_t counter{};
  while (1) {
    if (!task_completed()) {
      ++counter;
      std::this_thread::sleep_for(std::chrono::seconds(1));
    } else {
      break;
    }

    if (counter == 30) {
      std::terminate();
    }
  }
}

TEST_CASE("Multiple endpoints exchange messages with multiple clients")
{
  spdlog::set_level(spdlog::level::info);


  auto guard = std::thread(guard_func);
  auto server_th = std::thread(server);
  auto client1_th = std::thread(client);

  guard.join();
  server_th.join();
  spdlog::info("Server thread joined");

  client1_th.join();
  spdlog::info("Client thread joined");

  REQUIRE(ServerReceivedMessages == NumberOfMessages * 2);
  REQUIRE(ClientSentMessages == NumberOfMessages * 2);
}
