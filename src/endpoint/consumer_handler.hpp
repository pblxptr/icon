#pragma once

namespace icon::details {
template<class Endpoint, class Request>
class ConsumerHandler
{
public:
  virtual ~ConsumerHandler() = default;
  virtual void handle(Endpoint&, Request&&) = 0;
};
}// namespace icon::details