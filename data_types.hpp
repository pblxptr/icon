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

  template<Deserializable Body>
  class InternalRequest
  {
  public:
    InternalRequest(
      fields2::Identity&& identity,
      fields2::Header&& header,
      Body&& body
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

    const Body& body() const
    {
      return body_;
    }

  private:
    fields2::Identity identity_;
    fields2::Header header_;
    Body body_;
  };

}