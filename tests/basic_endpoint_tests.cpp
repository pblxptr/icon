#include <catch2/catch_test_macros.hpp>
#include <endpoint/endpoint_config.hpp>
#include <protobuf/protobuf_serialization.hpp>
#include <client/basic_client.hpp>
#include <dummy.pb.h>

constexpr auto EndpointS1 = "tcp://127.0.0.1:6667";

using namespace icon::transport;

template<class Message, class Consumer>
auto make_endpoint(boost::asio::io_context& bctx, zmq::context_t& zctx, Consumer&& consumer)
{
  return icon::setup_default_endpoint(
    icon::use_services(bctx, zctx),
    icon::address(EndpointS1),
    icon::consumer<ConnectionEstablishReq>(
      [](MessageContext<ConnectionEstablishReq> context) -> awaitable<void> {
        auto& req = context.message();
        auto rsp = ConnectionEstablishCfm{};
        co_await context.async_respond(std::move(rsp));
      }),
    icon::consumer<Message(std::forward<Consumer>(consumer))
  ).build();
}

TEST_CASE("Endpoint can messages", "[vector]") {
  auto bctx = boost::asio::io_context{};
  auto zctx = zmq::context_t{};
  auto client = icon::details::BasicClient{zctx, bctx};

  SECTION( "message contains valid data" ) {
    constexpr auto seq_value { 9999 };
    auto req = icon::dummy::TestSeqReq{};
    req.set_seq(seq_value);

  }
}

// TEST_CASE( "vectors can be sized and resized", "[vector]" ) {

//     std::vector<int> v( 5 );

//     REQUIRE( v.size() == 5 );
//     REQUIRE( v.capacity() >= 5 );

//     SECTION( "resizing bigger changes size and capacity" ) {
//         v.resize( 10 );

//         REQUIRE( v.size() == 10 );
//         REQUIRE( v.capacity() >= 10 );
//     }
//     SECTION( "resizing smaller changes size but not capacity" ) {
//         v.resize( 0 );

//         REQUIRE( v.size() == 0 );
//         REQUIRE( v.capacity() >= 5 );
//     }
//     SECTION( "reserving bigger changes capacity but not size" ) {
//         v.reserve( 10 );

//         REQUIRE( v.size() == 5 );
//         REQUIRE( v.capacity() >= 10 );
//     }
//     SECTION( "reserving smaller does not change size or capacity" ) {
//         v.reserve( 0 );

//         REQUIRE( v.size() == 5 );
//         REQUIRE( v.capacity() >= 5 );
//     }
// }