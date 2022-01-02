#pragma once

#include "consumer_handler.hpp"
#include "endpoint.hpp"
#include "context.hpp"

namespace icon::details {

template<class Endpoint>
class EndpointBuilder
{
public:
  using ConsumerHandlerBase_t = typename Endpoint::ConsumerHandlerBase_t;
  template<
    class Message,
    class Consumer
  > 
  using ConsumerHandler_t     = typename Endpoint::ConsumerHandler_t<Message, Consumer>;

  void add_address(std::string address)
  {
    addresses_.push_back(std::move(address));
  }

  template<class Message, class Consumer>
  void add_consumer(Consumer&& consumer)
  {
    auto number = 1;
    auto handler = std::make_unique<ConsumerHandler_t<Message, Consumer>>(std::forward<Consumer>(consumer));
    handlers_[number] = std::move(handler);
  }

  auto build()
  {
    return std::make_unique<Endpoint>(
      context::zmq(),
      context::boost(),
      std::move(addresses_),
      std::move(handlers_)
    );
  }

private:
  std::vector<std::string> addresses_;
  std::unordered_map<size_t, std::unique_ptr<ConsumerHandlerBase_t>> handlers_;
};

}




struct BasicEndpointConfiguration
{
  using Endpoint = BasicEndpoint;
  using Builder = BasicEndpointBuilder;
};


template<class Impl> 
class CRTP
{
public:
  Impl& impl()
  {
    return static_cast<Impl&>(*this);
  }
};
