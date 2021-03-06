#pragma once

#include <icon/endpoint/basic_endpoint/basic_endpoint.hpp>
#include <icon/icon.hpp>

// TODO: Disable copy and move operation
// TODO: Consider changing all properites to request

namespace icon {
template<class Message>
class MessageContext
{
public:
  MessageContext(icon::details::BasicEndpoint& endpoint,
    icon::details::core::Identity identity,
    icon::details::core::Header header,
    Message message)
    : endpoint_{ endpoint }, identity_{ std::move(identity) },
      header_{ std::move(header) }, message_{ std::move(message) } {}

  const Message& message() { return message_; }

  template<MessageToSend ResponseMessage>
  awaitable<void> async_respond(ResponseMessage&& message)
  {
    co_await endpoint_.async_respond(identity_, std::forward<ResponseMessage>(message));
  }

private:
  icon::details::BasicEndpoint& endpoint_;
  icon::details::core::Identity identity_;
  icon::details::core::Header header_;
  Message message_;
};
}// namespace icon