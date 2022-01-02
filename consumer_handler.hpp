#pragma once

namespace icon::details {
template<
  class Endpoint,
  class Request
>
class ConsumerHandler
{
public:
  virtual ~ConsumerHandler() = default;
  virtual void handle(Endpoint&, Request&&) = 0;
};

template<class Message>
class MessageContext
{
public:
  MessageContext
  (
    icon::details::fields2::Identity identity,
    icon::details::fields2::Header header,
    Message message
  )
    : identity_{std::move(identity)}
    , header_{std::move(header)}
    , message_{std::move(message)}
  {}
  MessageContext(const MessageContext&) = delete;
  MessageContext& operator=(const MessageContext&) = delete;
  MessageContext(MessageContext&&) = delete;
  MessageContext& operator=(MessageContext&&) = delete;

  Message& message() const
  {
    return message_;
  }
  
  template<class Message>
  awaitable<void> async_respond(Message&& msg)
  {
    co_await endpoint_.asyc_send(std::forward<Message>());
  }
private:
  icon::details::fields2::Identity identity_;
  icon::details::fields2::Header header_;
  Message message_;
};

template<
  class Endpoint,
  class Request,
  class Message,
  class Consumer
>
class BasicConsumerHandler : public ConsumerHandler<Endpoint, Request>
{
public:
  explicit BasicEndpointConsumerHandler(Consumer consumer)
    : consumer_{std::move(consumer)} //TODO: move or forward?????
  {}

  void handle(Endpoint& endpoint, Request&& req)
  {
    auto context = MessageContext{
      endpoint, 
      req.identity(), 
      req.header(), 
      deserialize<Message>(req.body())
    };
    consumer_(context);
  }
private:
  Consumer consumer_;
};
}