#pragma once

#include <core/header.hpp>
#include <core/identity.hpp>
#include <serialization/serialization.hpp>
#include <core/transport.hpp>
#include <core/unknown_message.hpp>

namespace icon::details {
template<Deserializable Message>
class InternalRequest
{
public:
  InternalRequest(core::Identity&& identity, core::Header&& header, Message&& message)
    : identity_{ std::move(identity) }, header_{ std::move(header) },
      message_{ std::move(message) } {}

  // Todo: Consider returning by value
  const core::Identity& identity() const { return identity_; }

  const core::Header& header() const { return header_; }

  template<class T>
  bool is()
  {
    return message_.template message_number_match_for<T>(
      header_.message_number());
  }

  template<class T>
  T body()
  {
    if (!is<T>())// TODO: Code duplication with Response
    {
      throw std::runtime_error("Cannot deserialize");
    }

    return message_.template deserialize<T>();
  }

private:
  core::Identity identity_;
  core::Header header_;
  Message message_;
};


template<class Deserializer>
class IncomingRequest : public core::UnknownMessage
{
  using Protocol_t = icon::details::Protocol<
    transport::Raw_t,
    transport::RawBuffer_t,
    icon::details::DataLayout<
      icon::details::fields::Identity,
      icon::details::fields::Header,
      icon::details::fields::Body
    >
  >;

public:
  IncomingRequest(core::Identity identity, core::Header header, transport::Raw_t body)
    : core::UnknownMessage(std::move(body))
    , identity_{std::move(identity)}
    , header_{std::move(header)}
  {}

  static IncomingRequest create(transport::RawBuffer_t buffer)
  {
    auto parser = Parser<Protocol_t>(std::move(buffer));
    auto [identity, header, body] = std::move(parser).template extract<
      icon::details::fields::Identity,
      icon::details::fields::Header,
      icon::details::fields::Body
    >();

    return IncomingRequest{
      core::Identity{identity.to_string()},
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


