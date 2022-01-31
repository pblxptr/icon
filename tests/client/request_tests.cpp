#include <catch2/catch_test_macros.hpp>

#include <icon/client/request.hpp>
#include <icon/protobuf/protobuf_serialization.hpp>
#include <dummy.pb.h>

TEST_CASE("Request contains valid data")
{
  using Serializer_t = icon::details::serialization::protobuf::ProtobufSerializer;
  auto message = icon::dummy::TestSeqReq{};
  auto request = icon::details::Request<decltype(message), Serializer_t>{std::move(message)};

  SECTION( "message buffer is not empty" ) {
    auto buffer = std::move(request).build();
    REQUIRE(!buffer.empty());
  }

  SECTION( "message buffer contains two parts ") {
    auto buffer = std::move(request).build();
    REQUIRE(buffer.size() == 2);
  }
}