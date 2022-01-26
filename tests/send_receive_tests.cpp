#include <catch2/catch_test_macros.hpp>
#include <endpoint/endpoint_config.hpp>
#include <protobuf/protobuf_serialization.hpp>
#include <client/basic_client.hpp>
#include <dummy.pb.h>

constexpr auto EndpointS1 = "tcp://127.0.0.1:6909";

using namespace icon::transport;
using namespace icon::dummy;

template<class Message, class Consumer>
auto make_endpoint(boost::asio::io_context& bctx, zmq::context_t& zctx, Consumer&& consumer)
{
  return icon::setup_default_endpoint(
    icon::use_services(bctx, zctx),
    icon::address(EndpointS1),
    icon::consumer<Message>(std::forward<Consumer>(consumer))
  ).build();
}

template<class Message>
awaitable<void> async_connect_send(icon::details::BasicClient& client, Message&& message)
{
  co_await client.async_connect(EndpointS1);
  co_await client.async_send(std::move(message));
}

template<class Message>
auto async_connect_send_wait_rsp(icon::details::BasicClient& client, Message&& message) -> decltype(client.async_send<Message>(std::declval<Message>()))
{
  co_await client.async_connect(EndpointS1);
  co_return co_await client.async_send(std::move(message));
}

template<class Message>
auto create_seq_msg(const uint32_t seq)
{
  auto m = Message{};
  m.set_seq(seq);

  return m;
}

void cleanup(boost::asio::io_context& ctx)
{
  ctx.stop();
}

TEST_CASE("Endpoint receives and sends messages") {
  auto bctx = boost::asio::io_context{};
  auto zctx = zmq::context_t{};
  auto client = icon::details::BasicClient{zctx, bctx};

  SECTION( "received message contains valid data" ) {
    constexpr auto seq_req_value { 9999 };
    auto endpoint = make_endpoint<icon::dummy::TestSeqReq>(bctx, zctx,
      [&bctx](icon::MessageContext<icon::dummy::TestSeqReq>& context) -> awaitable<void> {
      auto& msg = context.message();

      REQUIRE(msg.seq() == seq_req_value);
      cleanup(bctx);
      co_return;
    });

    co_spawn(bctx, endpoint->run(), detached);
    co_spawn(bctx, [&client]() -> awaitable<void> {
      co_await async_connect_send(client, create_seq_msg<icon::dummy::TestSeqReq>(seq_req_value));
    }, detached);

    bctx.run();
  }

  SECTION ( "endpoint responds with a message ") {
    constexpr auto seq_req_value { 9999 };
    constexpr auto seq_rsp_value { 10000 };
    auto endpoint = make_endpoint<icon::dummy::TestSeqReq>(bctx, zctx,
      [](icon::MessageContext<icon::dummy::TestSeqReq>& context) -> awaitable<void> {
      auto& msg = context.message();

      REQUIRE(msg.seq() == seq_req_value);

      co_await context.async_respond(create_seq_msg<icon::dummy::TestSeqCfm>(seq_rsp_value));
    });

    co_spawn(bctx, endpoint->run(), detached);
    co_spawn(bctx, [&client, &bctx]() -> awaitable<void> {
      auto rsp = co_await async_connect_send_wait_rsp(client, create_seq_msg<icon::dummy::TestSeqReq>(seq_req_value));
      auto msg = rsp.get<icon::dummy::TestSeqCfm>();

      REQUIRE(msg.seq() == seq_rsp_value);

      cleanup(bctx);
    }, detached);

    bctx.run();
  }
}
