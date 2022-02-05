#include <catch2/catch_test_macros.hpp>

#include <zmq_addon.hpp>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <cassert>

#include <icon/endpoint/endpoint_config.hpp>
#include <icon/client/basic_client.hpp>
#include <fmt/format.h>
#include <iostream>
#include <sstream>
#include <random>
#include <atomic>
#include <mutex>

struct Info
{
  std::string thread_id;
  std::vector<std::string> endpoints;
  std::vector<std::string> clients;
};

constexpr auto NumberOfThreads{ 4 };
constexpr auto NumberOfEndpoints{ 10 };
constexpr auto NumberOfClientsPerThread{ 8 };
constexpr auto NumberOfMessagesPerClient{ 50 };
constexpr auto MaxExecutionTime{ std::chrono::minutes(5) };
constexpr auto ExpectedProocessedMessages = NumberOfClientsPerThread * NumberOfThreads * NumberOfMessagesPerClient;


std::atomic<size_t> ProcessedMessages{ 0 };

std::vector<std::string> addresses;

std::vector<boost::asio::io_context*> contexts{};
std::mutex contexts_mtx{};

std::vector<Info> infos;
std::mutex info_mtx;

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;


void add_info(Info info)
{
  auto lock = std::scoped_lock{ info_mtx };
  infos.push_back(std::move(info));
}

void dump_info(const Info& info)
{
  spdlog::debug("Dump infor for thread: {}", info.thread_id);

  for (const auto& e : info.endpoints) {
    spdlog::debug("-- Endpoint bind : {}", e);
  }

  for (const auto& c : info.clients) {
    spdlog::debug("-- Client connected: {}", c);
  }

  spdlog::debug("------------------------------------");
}

void add_context(boost::asio::io_context* ctx)
{
  auto lock = std::scoped_lock{ contexts_mtx };
  spdlog::debug("Add context");

  contexts.emplace_back(ctx);
}

size_t rnd(const size_t min, const size_t max)
{
  static auto rd = std::random_device{};
  static auto gen = std::mt19937{ rd() };
  auto dist = std::uniform_int_distribution<size_t>(min, max);

  return dist(gen);
}

auto create_endpoint(boost::asio::io_context& bctx, zmq::context_t& zctx, const std::string& address)
{
  using namespace icon;
  using namespace icon::details;
  using namespace icon::transport;

  return icon::setup_default_endpoint(
    icon::use_services(bctx, zctx),
    icon::address(address),
    icon::consumer<TestSeqReq>(
      [](MessageContext<TestSeqReq> context) -> awaitable<void> {
        auto& req = context.message();
        auto seq_req = req.seq();
        auto seq_rsp = seq_req + 1;

        auto rsp = TestSeqCfm{};
        rsp.set_seq(seq_rsp);

        co_await context.async_respond(std::move(rsp));
      }))
    .build();
}


awaitable<void> run_client(icon::BasicClient& client, const std::string endpoint)
{
  co_await client.async_connect(endpoint.c_str());

  for (size_t i = 0; i < NumberOfMessagesPerClient; i++) {
    auto seq_req = icon::transport::TestSeqReq{};
    seq_req.set_seq(i);

    spdlog::debug("Client send...");

    auto rsp = co_await client.async_send(std::move(seq_req));
    assert(rsp.is<icon::transport::TestSeqCfm>());
    const auto msg = rsp.get<icon::transport::TestSeqCfm>();

    spdlog::debug("Client received...");

    ++ProcessedMessages;

    assert(msg.seq() == i + 1);
  }
}


void worker_thread(std::vector<std::string> endpoint_addresses)
{
  auto bctx = boost::asio::io_context{};
  auto zctx = zmq::context_t{};

  using work_guard_type =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  work_guard_type work_guard(bctx.get_executor());

  add_context(&bctx);

  auto endpoints = std::vector<std::unique_ptr<icon::Endpoint>>{};
  auto clients = std::vector<std::unique_ptr<icon::BasicClient>>{};
  auto info = Info{};
  info.thread_id = "Undefined";

  // Initialize endpoints
  for (const auto& addr : endpoint_addresses) {
    info.endpoints.push_back(addr);
    endpoints.push_back(create_endpoint(bctx, zctx, addr));
  }

  // Initialize clients
  for (size_t i = 0; i < NumberOfClientsPerThread; i++) {
    clients.push_back(std::make_unique<icon::BasicClient>(zctx, bctx));
  }

  // Run endpoints
  for (auto& e : endpoints) {
    co_spawn(bctx, e->run(), detached);
  }

  // Run clients
  for (auto& c : clients) {
    auto addr = addresses[rnd(0, addresses.size() - 1)];
    info.clients.push_back(addr);
    co_spawn(bctx, run_client(*c, addr), detached);
  }

  add_info(std::move(info));

  bctx.run();
}

void generate_addresses(size_t i)
{
  addresses.resize(i);

  std::generate(addresses.begin(), addresses.end(), [n = 5555]() mutable {
    return fmt::format("tcp://127.0.0.1:{}", n++);
  });
}

auto get_add(size_t i, size_t m, std::deque<std::string>& addr)
{
  auto ret = std::vector<std::string>{};

  auto cp = [&ret, &addr](size_t n) {
    for (size_t idx = 0; idx < n; ++idx) {
      ret.push_back(addr.front());
      addr.pop_front();
    }
    return ret;
  };

  if (addr.empty()) {
    return ret;
  }

  if (i < m) {
    cp(rnd(1, addr.size()));
  }

  if (i == m) {
    cp(addr.size());
  }

  return ret;
}

void guard_thread()
{
  auto now = std::chrono::system_clock::now();
  auto deadline = now + MaxExecutionTime;


  while (true) {
    if (std::chrono::system_clock::now() > deadline) {
      spdlog::error("Timeout reached. Terminate.");
      std::terminate();
    }

    if (ProcessedMessages == ExpectedProocessedMessages) {
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      spdlog::debug("ProcessedMessages: {}", ProcessedMessages);
    }
  }

  for (auto* ctx : contexts) {
    ctx->stop();
  }
}

void print_summary()
{
  spdlog::debug("Running on {} threads", NumberOfThreads);
  spdlog::debug("Total endpoints: {}", NumberOfEndpoints);
  spdlog::debug("Total clients: {}", NumberOfClientsPerThread * NumberOfThreads);
  spdlog::debug("Expected messages: {}", ExpectedProocessedMessages);
  spdlog::debug("Processed messages: {}", ProcessedMessages);

  for (const auto& info : infos) {
    dump_info(info);
  }
}


TEST_CASE("Multiple endpooints and clients spawned in multiple threads are exchanging messages.")
{
  spdlog::set_level(spdlog::level::debug);

  generate_addresses(NumberOfEndpoints);

  auto d = std::deque<std::string>{};
  for (auto& a : addresses) {
    d.push_back(a);
  }

  auto addr_per_th = std::vector<std::vector<std::string>>{};

  for (size_t i = 1; i <= NumberOfThreads; i++) {
    auto addr = get_add(i, NumberOfThreads, d);
    for (auto& a : addr) {
      fmt::print("Addr for: {}, {}\n", i, a);
    }
    addr_per_th.push_back(addr);
  }

  auto threads = std::vector<std::thread>{};

  // Push Worker threads
  for (size_t i = 0; i < NumberOfThreads; i++) {
    threads.push_back(std::thread(worker_thread, addr_per_th[i]));
  }

  // Push guard thread
  threads.push_back(std::thread(guard_thread));

  for (auto& th : threads) {
    th.join();
  }

  print_summary();
}
