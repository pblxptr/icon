#pragma once

#include <core/identity.hpp>
#include <core/header.hpp>
#include <core/body.hpp>

namespace icon::details
{
  template<Deserializable Payload>
  class InternalRequest
  {
  public:
    InternalRequest(
      core::Identity&& identity,
      core::Header&& header,
      core::DeserializableBody<Payload>&& body
    )
    : identity_{std::move(identity)}
    , header_{std::move(header)}
    , body_{std::move(body)}
    {}

    //Todo: Consider returning by value
    const core::Identity& identity() const
    {
      return identity_;
    }

    const core::Header& header() const
    {
      return header_;
    }

    template<class Message>
    bool is()
    {
      return body_.template message_number_match_for<Message>(header_.message_number());
    }

    template<class Message>
    Message body()
    {
      if (!is<Message>()) //TODO: Code duplication with Response
      {
        throw std::runtime_error("Cannot deserialize");
      }

      return body_.template deserialize<Message>();
    }

  private:
    core::Identity identity_;
    core::Header header_;
    core::DeserializableBody<Payload> body_;
  };
}