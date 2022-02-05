#pragma once

#include <icon/endpoint/basic_endpoint/basic_endpoint_consumer_handler.hpp>

namespace icon::details {
class BasicEndpointBuilder
{
public:
  void use_service(boost::asio::io_context& ctx)
  {
    bctx_ = std::ref(ctx);
  }

  void use_service(zmq::context_t& ctx)
  {
    zctx_ = std::ref(ctx);
  }

  void use_services(boost::asio::io_context& bctx, zmq::context_t& zctx)
  {
    use_service(bctx);
    use_service(zctx);
  }

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
    if (!bctx_.has_value() || !zctx_.has_value()) {
      throw std::runtime_error("Configuration incomplete. One of required services is missing: boost::asio::io_context, zmq::context_t");
    }

    return std::make_unique<BasicEndpoint>(zctx_.value(), bctx_.value(), std::move(addresses_), std::move(handlers_));
  }

private:
  std::optional<std::reference_wrapper<boost::asio::io_context>> bctx_;
  std::optional<std::reference_wrapper<zmq::context_t>> zctx_;
  std::vector<std::string> addresses_;
  std::unordered_map<size_t,
    std::unique_ptr<BasicEndpoint::ConsumerHandlerBase_t>>
    handlers_;
};
}// namespace icon::details
