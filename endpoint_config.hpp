#pragma once

#include "protocol.hpp"
#include "endpoint_builder.hpp"
#include "data_types.hpp"

template<class Message, class Consumer>
struct add_consumer_config
{
  void operator()(EndpointBuilder& builder)
  {
    builder.add_consumer<Message, Consumer>(std::forward<Consumer>(c));
  }

  Consumer c;
};

struct add_address_config
{
  void operator()(EndpointBuilder& builder)
  {
    builder.add_address(std::move(address));
  }
  std::string address;
};
}

namespace icon::api {
template<class... Config>
EndpointBuilder setup_default_endpoint(Config&&... configs)
{
  auto builder = icon::details::EndpointBuilder<BasicEndpoint>(std::move(address));

  (configs(builder), ...);

  return builder;
}

template<class Message, class Consumer>
auto consumer(Consumer&& consumer)
{
  return add_consumer_config<Message, Consumer>{std::forward<Consumer>(consumer)};
}

auto address(std::string address)
{
  return add_address_config{std::move(address)};
}
};