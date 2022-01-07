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
      core::Body<Payload>&& body
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
    Message body()
    {
      return body_.template extract_with_message_number<Message>(header_.message_number());
    }

  private:
    core::Identity identity_;
    core::Header header_;
    core::Body<Payload> body_;
  };
}