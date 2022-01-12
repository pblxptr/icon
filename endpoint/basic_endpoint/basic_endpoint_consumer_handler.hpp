#pragma once 

#include <endpoint/consumer_handler.hpp>
#include <endpoint/message_context.hpp>
namespace icon::details
{
  template<
    class Endpoint,
    class Request,
    class Message,
    class Consumer
  >
  class BasicEndpointConsumerHandler : public ConsumerHandler<Endpoint, Request>
  {
  public:
      explicit BasicEndpointConsumerHandler(Consumer consumer)
      : consumer_{std::move(consumer)} //TODO: move or forward?????
    {}

    void handle(Endpoint& endpoint, Request&& request)
    {
      auto context = MessageContext<Message>{
        endpoint,
        request.identity(),
        request.header(),
        request.template body<Message>()
      };
      consumer_(context);
    }

  private:
    Consumer consumer_;
  };
}