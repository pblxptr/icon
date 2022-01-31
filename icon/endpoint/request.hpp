#pragma once

#include <icon/core/header.hpp>
#include <icon/core/identity.hpp>
#include <icon/serialization/serialization.hpp>
#include <icon/core/transport.hpp>
#include <icon/core/unknown_message.hpp>

namespace icon::details {
template<class Deserializer>
class EndpointRequest : public core::UnknownMessage
{
  using Protocol_t = icon::details::Protocol<
    transport::Raw_t,
    transport::RawBuffer_t,
    icon::details::DataLayout<
      icon::details::fields::Identity,
      icon::details::fields::Header,
      icon::details::fields::Body>>;

public:
  EndpointRequest(core::Identity identity, core::Header header, transport::Raw_t body)
    : core::UnknownMessage(std::move(body)), identity_{ std::move(identity) }, header_{ std::move(header) }
  {}

  static EndpointRequest create(transport::RawBuffer_t buffer)
  {
    auto parser = Parser<Protocol_t>(std::move(buffer));
    auto [identity, header, body] = std::move(parser).template extract<icon::details::fields::Identity, icon::details::fields::Header, icon::details::fields::Body>();

    return EndpointRequest{
      core::Identity{ identity.to_string() },
      Deserializer::template deserialize<core::Header>(header),
      std::move(body)
    };
  }

  const core::Identity& identity() const
  {
    return identity_;
  }

  const core::Header& header() const
  {
    return header_;
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
  core::Identity identity_;
  core::Header header_;
};
}// namespace icon::details
