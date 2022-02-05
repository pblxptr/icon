#pragma once

#include <zmq.hpp>
#include <boost/asio/io_context.hpp>

#include <icon/client/basic_client.hpp>
#include <icon/icon.pb.h>

namespace icon::details {
class Client : public BasicClient
{
public:
  BasicClient(zmq::context_t& zctx, boost::asio::io_context& bctx)
    : BasicClient{ zctx, bctx }
  {}

  awaitable<bool> async_connect(const char* endpoint)
  {
    spdlog::debug("Client: connecting to endpoint: {}, is_connected: {}", endpoint, is_connection_established_);

    if (is_connected()) {
      co_return true;
    }

    // Connection over zmq socket, send init and wait for cfm
    BasicClient::connect(endpoint);

    co_await init_connection_async();

    co_return is_connected_;
  }

  bool is_connected() const
  {
    return BasicClient::is_connected() && is_connection_established_;
  }

private:
  awaitable<void> init_connection_async()
  {
    using ConEstablishReq_t = icon::transport::ConnectionEstablishReq;
    using ConEstablishCfm_t = icon::transport::ConnectionEstablishCfm;

    const auto response = co_await async_send_with_response(ConEstablishReq_t{});

    if (!response.is<ConEstablishCfm_t>()) {
      throw std::runtime_error("Connection establish failed");
    }

    spdlog::debug("Client: connected");

    is_connection_established_ = true;
  }

private:
  bool is_connection_established_{ false };
};
}// namespace icon::details