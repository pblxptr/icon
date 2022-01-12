#pragma once

#include <core/header.hpp>
#include <core/identity.hpp>
#include <serialization/serialization.hpp>

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
}// namespace icon::details