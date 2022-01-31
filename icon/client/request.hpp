#pragma once

#include <icon/core/protocol.hpp>
#include <icon/core/transport.hpp>
#include <icon/core/header.hpp>

namespace icon::details {
template<class Message, class Serializer>
class Request
{
  using Protocol_t = icon::details::Protocol<
    icon::details::transport::Raw_t,
    icon::details::transport::RawBuffer_t,
    icon::details::DataLayout<
      icon::details::fields::Header,
      icon::details::fields::Body>>;

public:
  explicit Request(Message message)
    : header_{ core::Header{ Serializer::template message_number_for<Message>() } }, message_{ std::move(message) }
  {}

  auto build() &
  {
    auto parser = Parser<Protocol_t>{};
    parser.put<fields::Header>(Serializer::serialize(header_));
    parser.put<fields::Body>(Serializer::serialize(message_));

    return parser.parse();
  }

  auto build() &&
  {
    auto parser = Parser<Protocol_t>{};
    parser.put<fields::Header>(Serializer::serialize(header_));
    parser.put<fields::Body>(Serializer::serialize(message_));

    return std::move(parser).parse();
  }

private:
  core::Header header_;
  Message message_;
};

}// namespace icon::details
