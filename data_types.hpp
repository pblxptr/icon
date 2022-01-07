#pragma once

#include "protocol.hpp"

namespace icon::details {
  template<Deserializable Body>
  class InternalResponse
  {
  public:
    InternalResponse
    (
      fields2::Header&& header, 
      Body&& body
    )
    : header_{std::move(header)}
    , body_{std::move(body)}
    {}

  private:
    fields2::Header header_;
    Body body_;
  };

  template<Deserializable Payload>
  class InternalRequest
  {
  public:
    InternalRequest(
      fields2::Identity&& identity,
      fields2::Header&& header,
      fields2::Body<Payload>&& body
    )
    : identity_{std::move(identity)}
    , header_{std::move(header)}
    , body_{std::move(body)}
    {}

    //Todo: Consider returning by value
    const fields2::Identity& identity() const
    {
      return identity_;
    }

    const fields2::Header& header() const
    {
      return header_;
    }

    template<class Message>
    Message body()
    {
      return body_.template extract_with_message_number<Message>(header_.message_number());
    }

  private:
    fields2::Identity identity_;
    fields2::Header header_;
    fields2::Body<Payload> body_;
  };

}