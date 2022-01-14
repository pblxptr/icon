#pragma once

#include <core/header.hpp>
#include <core/identity.hpp>
#include <serialization/serialization.hpp>
#include <core/protocol.hpp>
#include <core/transport.hpp>
#include <core/unknown_message.hpp>

// TODO: Code duplication

namespace icon::details {
template<class Deserializer>
class Response : public core::UnknownMessage
{
  using Protocol_t = icon::details::Protocol<
    icon::details::transport::Raw_t,
    icon::details::transport::RawBuffer_t,
    icon::details::DataLayout<
      icon::details::fields::Header,
      icon::details::fields::Body>>;

public:
  Response(core::Header header, transport::Raw_t body) : UnknownMessage(std::move(body)), header_{ std::move(header) }
  {}

  static Response create(icon::details::transport::RawBuffer_t buffer)
  {
    auto parser = Parser<Protocol_t>(std::move(buffer));
    auto [header, body] = std::move(parser).template extract<icon::details::fields::Header, icon::details::fields::Body>();

    return Response{
      Deserializer::template deserialize<core::Header>(header),
      std::move(body)
    };
  }

  template<class Message>
  bool is() const
  {
    return UnknownMessage::is<Deserializer, Message>(header_.message_number());
  }

  template<class Message>
  Message get() const
  {
    return UnknownMessage::get<Deserializer, Message>();
  }

private:
  core::Header header_{};
};
}// namespace icon::details