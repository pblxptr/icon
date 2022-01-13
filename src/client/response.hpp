#pragma once

#include <core/header.hpp>
#include <core/identity.hpp>
#include <serialization/serialization.hpp>
#include <core/protocol.hpp>
#include <core/transport.hpp>

// TODO: Code duplication

namespace icon::details {
template<class Deserializer>
class Response
{
  using Protocol_t = icon::details::Protocol<
    icon::details::transport::Raw_t,
    icon::details::transport::RawBuffer_t,
    icon::details::DataLayout<
      icon::details::fields::Header,
      icon::details::fields::Body>>;

public:
  explicit Response(icon::details::transport::RawBuffer_t buffer)
  {
    auto parser = Parser<Protocol_t>(std::move(buffer));
    auto [header, body] = std::move(parser).template extract<icon::details::fields::Header, icon::details::fields::Body>();

    header_ = Deserializer::template deserialize<core::Header>(header);
    body_ = std::move(body);
  }

  template<class Message>
  bool is() const
  {
    return Deserializer::template message_number_for<Message>() == header_.message_number();
  }

private:
  core::Header header_ {};
  icon::details::transport::Raw_t body_ {};
};
}// namespace icon::details