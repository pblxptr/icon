#pragma once

#include <endpoint/basic_endpoint/basic_endpoint_consumer_handler.hpp>

namespace icon::details {
class BasicEndpointBuilder
{
public:
  void add_address(std::string&& address)
  {
    addresses_.push_back(std::move(address));
  }

  template<class Message, class Consumer>
  void add_consumer(Consumer&& consumer)
  {
    const auto msg_number =
      BasicEndpoint::Serializer_t::message_number_for<Message>();

    if (handlers_.contains(msg_number)) {
      throw std::invalid_argument(
        "Handler for requested message has been already registered.");
    }

    handlers_[msg_number] = std::make_unique<BasicEndpointConsumerHandler<
      BasicEndpoint,
      BasicEndpoint::Request_t,
      Message,
      Consumer>>(
      std::forward<Consumer>(consumer));
  }

  auto build()
  {
    return std::make_unique<BasicEndpoint>(context::zmq(), context::boost(), std::move(addresses_), std::move(handlers_));
  }

private:
  std::vector<std::string> addresses_;
  std::unordered_map<size_t,
    std::unique_ptr<BasicEndpoint::ConsumerHandlerBase_t>>
    handlers_;
};
}// namespace icon::details
