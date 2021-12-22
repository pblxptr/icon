#pragma once

namespace icon::details {
template<class Request>
class ConsumerHandler
{
public:
  virtual ~ConsumerHandler() = default;
  virtual void handle(Request&&) = 0;
};

template<class Message>
class MessageContext
{
public:
  MessageContext(const MessageContext&) = delete;
  MessageContext& operator=(const MessageContext&) = delete;
  MessageContext(MessageContext&&) = delete;
  MessageContext& operator=(MessageContext&&) = delete;

  Message& message() const
  {
    return message_;
  }

  template<Serializable ResponseMessage>
  awaitable<void> async_respond()
  {
    
  }
private:
  Message message_;
};

template<class Message, class Consumer>
class ConsumerHandler
{
public:
  void handle(Request&& req)
  {
    
    auto context = MessageContext<Message>(std::move(req));
    consumer_(context);
    
  }
private:
  Consumer consumer_;
};
}