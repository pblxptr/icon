#pragma once

#include <core/header.hpp>
#include <core/identity.hpp>
#include <serialization/serialization.hpp>
#include <core/protocol.hpp>
#include <core/transport.hpp>
#include <core/unknown_message.hpp>
#include <client/error.hpp>

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
  Response(ErrorCode ec)
    : UnknownMessage()
    , error_code_{std::move(ec)}
  {}

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

  template<class Message>
  Message get_safe() const
  {
    if (!is<Message>()) {
      throw std::runtime_error("Cannot get response message. Requested destination type does not match to message number contained in header");
    }

    return get<Message>();
  }

  auto error_code() const
  {
    return error_code_;
  }

private:
  core::Header header_{};
  std::optional<ErrorCode> error_code_;
};
}// namespace icon::details