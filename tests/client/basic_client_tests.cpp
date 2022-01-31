#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <spdlog/spdlog.h>

#include <icon/client/basic_client.hpp>
#include <icon/endpoint/endpoint_config.hpp>
#include <icon/protobuf/protobuf_serialization.hpp>
#include <dummy.pb.h>

constexpr auto EndpointS1 = "tcp://127.0.0.1:6667";


TEST_CASE("Client connetecs to endpoint")
{
  spdlog::set_level(spdlog::level::debug);

  auto bctx = boost::asio::io_context{};
  auto zctx = zmq::context_t{};
  using work_guard_type =
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  work_guard_type work_guard(bctx.get_executor());

  auto propagate_exception = [](auto eptr)
  {
    if (eptr)
    {
      std::rethrow_exception(eptr);
    }
  };

  auto endpoint = icon::setup_default_endpoint(
    icon::use_services(bctx, zctx),
    icon::address(EndpointS1),
    icon::consumer<icon::dummy::TestSeqReq>(
      [](icon::MessageContext<icon::dummy::TestSeqReq>& context) -> awaitable<void>
      {
        co_await context.async_respond(icon::dummy::TestSeqCfm{});
      }
    )).build();

  SECTION( "sending a message when client is connected not throws an exception" ) {
    auto client_func = [&zctx, &bctx]() -> awaitable<void> {
      auto client = icon::details::BasicClient{zctx, bctx};
      co_await client.async_connect(EndpointS1);
      REQUIRE_NOTHROW(co_await client.async_send(icon::dummy::TestSeqReq{}));
      bctx.stop();
    };

    co_spawn(bctx, endpoint->run(), propagate_exception);
    co_spawn(bctx, client_func, propagate_exception);

    bctx.run();
  }

  SECTION( "sending a message when client is disconnected throws and exception" ) {
    auto client_func = [&zctx, &bctx]() -> awaitable<void> {
      auto client = icon::details::BasicClient{zctx, bctx};
      REQUIRE_THROWS(co_await client.async_send(icon::dummy::TestSeqReq{}));
      bctx.stop();
    };

    co_spawn(bctx, client_func, propagate_exception);

    bctx.run();
  }

  SECTION( "sending a message with a timeout returns a response with error code assigned" ) {
    auto client_func = [&zctx, &bctx]() -> awaitable<void> {
      auto client = icon::details::BasicClient{zctx, bctx};
      co_await client.async_connect(EndpointS1);
      auto rsp = co_await client.async_send(icon::dummy::TestSeqReq{}, std::chrono::seconds(1));

      REQUIRE(rsp.error_code());

      bctx.stop();
    };

    co_spawn(bctx, client_func(), propagate_exception);
    bctx.run();

  }
}