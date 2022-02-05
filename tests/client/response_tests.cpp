#include <catch2/catch_test_macros.hpp>

#include <icon/client/request.hpp>
#include <icon/client/response.hpp>
#include <icon/protobuf/protobuf_serialization.hpp>
#include <dummy.pb.h>

TEST_CASE("Response contains data")
{
  using Serializer_t = icon::details::serialization::protobuf::ProtobufSerializer;
  using Deserializer_t = icon::details::serialization::protobuf::ProtobufDeserializer;
  auto message = icon::dummy::TestSeqReq{};
  auto request = icon::details::Request<decltype(message), Serializer_t>{ std::move(message) };
  auto buffer = std::move(request).build();
  auto response = icon::Response<Deserializer_t>::create(std::move(buffer));

  SECTION("the response contains a valid message")
  {
    REQUIRE(response.is<icon::dummy::TestSeqReq>());
  }

  SECTION("the response message can be deserialized without exception")
  {
    REQUIRE_NOTHROW(response.get_safe<icon::dummy::TestSeqReq>());
  }

  SECTION("the response contains different types of data")
  {
    REQUIRE_FALSE(response.is<icon::dummy::TestSeqCfm>());
  }

  SECTION("response throws an exception when trying to get the message that is not contained in the response")
  {
    REQUIRE_THROWS(response.get_safe<icon::dummy::TestSeqCfm>());
  }
}