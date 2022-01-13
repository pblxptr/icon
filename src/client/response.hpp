#pragma once

#include <core/header.hpp>
#include <core/identity.hpp>
#include <serialization/serialization.hpp>
#include <core/protocol.hpp>

// TODO: Code duplication

namespace icon::details {
template<class Deserializer>
class Response
{
  using Protocol_t = icon::details::Protocol<
    icon::details::protocol::Raw_t,
    icon::details::protocol::RawBuffer_t,
    icon::details::DataLayout<
      icon::details::fields::Header,
      icon::details::fields::Body>>;

public:
  explicit Response(icon::details::protocol::RawBuffer_t buffer)
  {
    auto parser = Parser<Protocol_t>(std::move(buffer));
    auto [header, body] = std::move(parser).template extract<icon::details::fields::Header, icon::details::fields::Body>();

    header_ = Deserializer::template deserialize<core::Header>(header);
    body_ = std::move(body);
  }

  template<class Message>
  bool is() const
  {
    return Deserializer::template message_number_for<Message>() == header_.message_number();
  }

private:
  core::Header header_;
  icon::details::protocol::Raw_t body_;
};


// template<Deserializable Message>
// class BaseResponse
// {
// public:
//   BaseResponse(Message&& message) : message_{ std::move(message) } {}

//   template<class T>
//   bool message_number_match_for(const size_t message_number) const
//   {
//     return message_.template message_number_match_for<T>(message_number);
//   }

//   template<class T>
//   T message() const
//   {
//     return message_.template deserialize<T>();
//   }

//   template<class T>
//   T message_safe(const size_t message_number) const
//   {
//     if (!message_number_match_for<T>(message_number)) {
//       throw std::runtime_error("Cannot deserialize");
//     }

//     return message_.template deserialize<T>();
//   }

// private:
//   Message message_;
// };

// template<Deserializable Message>
// class InternalResponse : public BaseResponse<Message>
// {
// public:
//   InternalResponse(core::Header&& header, Message&& message)
//     : BaseResponse<Message>(std::move(message)), header_{ std::move(header) } {}

//   using BaseResponse<Message>::message;

//   const core::Header& header() const { return header_; }

//   template<class T>
//   bool is() const
//   {
//     return BaseResponse<Message>::template message_number_match_for<T>(
//       header_.message_number());
//   }

// private:
//   core::Header header_;
// };

// template<Deserializable Message>
// class Response : public BaseResponse<Message>
// {
// public:
//   Response(core::Header&& header, Message&& message)
//     : BaseResponse<Message>(std::move(message)),
//       message_number_{ header.message_number() } {}

//   using BaseResponse<Message>::message;

//   template<class T>
//   bool is() const
//   {
//     return BaseResponse<Message>::template message_number_match_for<T>(
//       message_number_);
//   }

// private:
//   size_t message_number_;
// };
}// namespace icon::details