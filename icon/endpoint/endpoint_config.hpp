#pragma once

#include <icon/core/protocol.hpp>
#include <icon/endpoint/basic_endpoint/basic_endpoint.hpp>
#include <icon/endpoint/basic_endpoint/basic_endpoint_builder.hpp>

namespace icon::details {
template<class Message, class Consumer>
struct add_consumer_config
{
  template<class Builder>
  void operator()(Builder& builder)
  {
    builder.template add_consumer<Message>(std::move(c));
  }
  Consumer c;
};

struct add_address_config
{
  template<class Builder>
  void operator()(Builder& builder)
  {
    builder.add_address(std::move(address));
  }
  std::string address;
};

template<class Service>
struct use_service
{
  template<class Builder>
  void operator()(Builder& builder)
  {
    builder.use_service(service);
  }

  Service service;
};

template<class S1, class S2> //TODO: Consuder to use fold expressions and std::tupe, it allows to support variadic list of services
struct use_services
{
  template<class Builder>
  void operator()(Builder& builder)
  {
    builder.use_service(std::forward<S1>(s1));
    builder.use_service(std::forward<S2>(s2));
  }

  S1 s1;
  S2 s2;
};

}// namespace icon::details

namespace icon {
template<class... Config>
auto setup_default_endpoint(Config&&... configs)
{
  auto builder = icon::details::BasicEndpointBuilder{};

  (configs(builder), ...);

  return builder;
}


template<class Message, class Consumer>
auto consumer(Consumer&& consumer)
{
  return icon::details::add_consumer_config<Message, Consumer>{
    std::forward<Consumer>(consumer)
  };
}

inline auto address(std::string address)
{
  return icon::details::add_address_config{ std::move(address) };
}

template<class Service>
auto use_service(Service&& service) //TODO: Use service and use services are likely to be merged
{
  return icon::details::use_service{std::forward<Service>(service)};
}

template<class... Services>
auto use_services(Services&&... services)
{
  return icon::details::use_services<Services...>{std::forward<Services>(services)...};
}

}// namespace icon