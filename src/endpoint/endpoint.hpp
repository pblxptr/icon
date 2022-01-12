#pragma once

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>

namespace icon::details {
class Endpoint {
public:
  virtual ~Endpoint() = default;
  virtual boost::asio::awaitable<void> run() = 0;
};
} // namespace icon::details