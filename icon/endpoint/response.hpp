#pragma once

#include <icon/core/identity.hpp>
#include <icon/core/header.hpp>
#include <icon/core/protocol.hpp>


namespace icon::details {
template<class Message, class Serializer>
class EndpointResponse
{
  using Protocol_t = icon::details::Protocol<
    transport::Raw_t,
    transport::RawBuffer_t,
    icon::details::DataLayout<
      icon::details::fields::Identity,
      icon::details::fields::Header,
      icon::details::fields::Body>>;

public:
  EndpointResponse(core::Identity identity, Message message)
    : identity_{ std::move(identity) }, header_{ core::Header{ Serializer::template message_number_for<Message>() } }, message_{ std::move(message) }
  {}

  auto build() &
  {
    auto parser = Parser<Protocol_t>{};
    parser.put<fields::Identity>(Serializer::serialize(identity_));
    parser.put<fields::Header>(Serializer::serialize(header_));
    parser.put<fields::Body>(Serializer::serialize(message_));

    return parser.parse();
  }

  auto build() &&
  {
    auto parser = Parser<Protocol_t>{};
    parser.put<fields::Identity>(Serializer::serialize(identity_));
    parser.put<fields::Header>(Serializer::serialize(header_));
    parser.put<fields::Body>(Serializer::serialize(message_));

    return std::move(parser).parse();
  }

private:
  core::Identity identity_;
  core::Header header_;
  Message message_;
};

}// namespace icon::details