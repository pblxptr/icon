#pragma once

#include "protocol.hpp"

namespace icon::details {
  template<Deserializable Body>
  class GenericBody
  {
  public:
    GenericBody(Body&& b)
      : body_{std::move(b)}
    {}

    template<class Message, class Number>
    bool is(const Number& number) const
    {
      return get_message_number<Message>(body_) == number && body_.has_value();
    }

    template<class Destination, class Number>
    Destination get(const Number& number) const &
    {
      if (not is<Destination>(number)) {
        throw std::invalid_argument("Destination type does not match to the source type");
      }

      return deserialize<Destination>(body_);
    }

    Body body_;
  };

  template<Deserialized Header>
  class GenericHeader
  {
  public:
    GenericHeader(Header&& header)
      : header_{std::move(header)}
    {}

    auto msg_number() const
    {
      return header_.message_number();
    }

    Header header_;
  };

  template<
    Deserialized Header,
    Deserializable Body
  >
  class BasicResponse : protected GenericHeader<Header>, protected GenericBody<Body>
  {
    using Header_t = GenericHeader<Header>;
    using Body_t = GenericBody<Body>;
  public:
    BasicResponse(Header&& header, Body&& body)
      : Header_t(std::move(header))
      , Body_t(std::move(body))
    {}

    template<class Message>
    bool is() const
    {
      return Body_t::template is<Message>(Header_t::msg_number());
    }

    template<class Message>
    Message get()
    {
      return Body_t::template get<Message>(Header_t::msg_number());
    }
  };

  template<
    Deserialized Header,
    Deserializable Body
  >
  class InternalResponse : public GenericHeader<Header>, public GenericBody<Body>
  {
    using Header_t = GenericHeader<Header>;
    using Body_t = GenericBody<Body>;
  public:
    InternalResponse(Header&& header, Body&& body)
      : Header_t(std::move(header))
      , Body_t(std::move(body))
    {}

    template<class Message>
    bool is() const
    {
      return Body_t::template is<Message>(Header_t::msg_number());
    }
  };

  template<
    class Identity,
    Deserialized Header,
    Deserializable Body
  >
  class InternalRequest : public GenericHeader<Header>, public GenericBody<Body>
  {
    using Header_t = GenericHeader<Header>;
    using Body_t = GenericBody<Body>;
  public:
    InternalRequest(Identity, Header&& header, Body&& body)
      : Header_t(std::move(header))
      , Body_t(std::move(body))
    {}

    template<class Message>
    bool is() const
    {
      return Body_t::template is<Message>(Header_t::msg_number());
    }
  };

}