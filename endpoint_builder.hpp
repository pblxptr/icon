#pragma once

#include "consumer_handler.hpp"
#include "endpoint.hpp"

namespace icon::details {

template<class Endpoint>
class EndpointBuilder
{
public:
  void add_address(std::string address)
  {
    addresses_.push_back(std::move(address));
  }

  template<class Message, class Consumer>
  void add_consumer(Consumer&& consumer)
  {
    auto [number, handler] = create_message_handler<Message>(std::forward<Consumer>(consumer));
    handlers[number] = std::move(handler);
  }

  std::unique_ptr<Context> build()
  {
    return std::make_unique<BasicEndpoint>(
      std::move(addresses_),
      std::move(handlers_)
    );
  }
private:
  template<class Message, class Consumer>
  auto create_message_handler(Consumer&& consumer)
  {
    auto handler = GenericHandler<Message, Consumer>{};
  }

private:
  std::vector<std::string> addresses_;
  std::unordered_map<size_t, std::unique_ptr<ConsumerHandler>> handlers_;
};

}