#pragma once

#include <core/protocol.hpp>
#include <endpoint/basic_endpoint/basic_endpoint.hpp>
#include <endpoint/basic_endpoint/basic_endpoint_builder.hpp>

namespace icon::details {
template<class Message, class Consumer>
struct add_consumer_config
{
  template<class Builder>
  void operator()(Builder &builder)
  {
    builder.template add_consumer<Message>(std::move(c));
  }
  Consumer c;
};

struct add_address_config
{
  template<class Builder>
  void operator()(Builder &builder)
  {
    builder.add_address(std::move(address));
  }
  std::string address;
};
}// namespace icon::details

namespace icon::api {
template<class... Config>
auto setup_default_endpoint(Config &&...configs)
{
  auto builder = icon::details::BasicEndpointBuilder{};

  (configs(builder), ...);

  return builder;
}

template<class Message, class Consumer>
auto consumer(Consumer &&consumer)
{
  return icon::details::add_consumer_config<Message, Consumer>{
    std::forward<Consumer>(consumer)
  };
}

auto address(std::string address)
{
  return icon::details::add_address_config{ std::move(address) };
}
}// namespace icon::api