#pragma once

#include <icon/endpoint/consumer_handler.hpp>
#include <icon/endpoint/message_context.hpp>
namespace icon::details {

template<class Endpoint, class Request, class Message, class Consumer>
class BasicEndpointConsumerHandler : public ConsumerHandler<Endpoint, Request>
{

public:
  explicit BasicEndpointConsumerHandler(Consumer consumer)
    : consumer_{ std::move(consumer) }
  {}

  awaitable<void> handle(Endpoint& endpoint, Request&& request)
  {
    auto context =
      MessageContext<Message>{ endpoint, request.identity(), request.header(), request.template get<Message>() };
    co_await consumer_(context);
  }

private:
  Consumer consumer_;
};
}// namespace icon::details