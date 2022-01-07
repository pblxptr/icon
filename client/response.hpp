#pragma once

#include <core/identity.hpp>
#include <core/header.hpp>
#include <core/body.hpp>
#include <serialization/serialization.hpp>

//TODO: Code duplication

namespace icon::details
{
  template<Deserializable Payload>
  class InternalResponse
  {
  public:
    InternalResponse(
      core::Header&& header,
      core::DeserializableBody<Payload>&& body
    )
    : header_{std::move(header)}
    , body_{std::move(body)}
    {}

    const core::Header& header() const
    {
      return header_;
    }

    template<class Message>
    bool is() const
    {
      return body_.template message_number_match_for<Message>(header_.message_number());
    }

    template<class Message>
    Message body()
    {
      if (!is<Message>())
      {
        throw std::runtime_error("Cannot deserialize");
      }

      return body_.template deserialize<Message>();
    }

  private:
    core::Header header_;
    core::DeserializableBody<Payload> body_;
  };

  template<Deserializable Payload>
  class Response
  {
  public:
    Response(
      core::Header&& header,
      core::DeserializableBody<Payload>&& body
    )
    : message_number_{header.message_number()}
    , body_{std::move(body)}
    {}

    template<class Message>
    bool is()
    {
      return body_.template message_number_match_for<Message>(message_number_);
    }

    template<class Message>
    Message body()
    {
      if (!is<Message>())
      {
        throw std::runtime_error("Cannot deserialize");
      }

      return body_.template deserialize<Message>();
    }

  private:
    size_t message_number_;
    core::DeserializableBody<Payload> body_;
  };
}