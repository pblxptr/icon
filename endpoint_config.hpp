#pragma once

#include "protocol.hpp"
#include "endpoint_builder.hpp"
#include "data_types.hpp"

namespace icon::details {
template<class Message, class Consumer>
struct add_consumer_config
{
  template<class Endpoint>
  void operator()(EndpointBuilder<Endpoint>& builder)
  {
    builder.template add_consumer<Message>(std::move(c));
  }
  Consumer c;
};

struct add_address_config
{
  template<class Endpoint>
  void operator()(EndpointBuilder<Endpoint>& builder)
  {
    builder.add_address(std::move(address));
  }
  std::string address;
};
}

namespace icon::api {
template<class... Config>
auto setup_default_endpoint(Config&&... configs)
{
  auto builder = icon::details::EndpointBuilder<icon::details::BasicEndpoint>{};

  (configs(builder), ...);

  return builder;
}

template<class Message, class Consumer>
auto consumer(Consumer&& consumer)
{
  return icon::details::add_consumer_config<Message, Consumer>{std::forward<Consumer>(consumer)};
}

auto address(std::string address)
{
  return icon::details::add_address_config{std::move(address)};
}
}