#include <endpoint/basic_endpoint/basic_endpoint.hpp>

namespace {
    template<
    class RawBuffer,
    class Protocol,
    class Request,
    class RawMessageData
  >
  auto extract_request(RawBuffer&& buffer)
  {
    auto parser = icon::details::Parser<Protocol>{std::move(buffer)};
    auto identity = std::move(parser)
      .template get<icon::details::fields::Identity>()
      .to_string();

    auto header = RawMessageData{std::move(parser)
      .template get<icon::details::fields::Header>()}
      .template deserialize<icon::transport::Header>();

    auto message = RawMessageData{std::move(parser)
      .template get<icon::details::fields::Body>()};

    return Request{
      icon::details::core::Identity{std::move(identity)},
      icon::details::core::Header{header.message_number()},
      std::move(message)
    };
  }
}


namespace icon::details
{
  BasicEndpoint::BasicEndpoint(
      zmq::context_t& zctx,
      boost::asio::io_context& bctx,
      std::vector<std::string> addresses,
      std::unordered_map<size_t, std::unique_ptr<ConsumerHandlerBase_t>> handlers
    )
    : BaseEndpoint(zmq::socket_t{zctx, zmq::socket_type::router}, bctx)
    , handlers_{std::move(handlers)}
  {
    for (const auto& addr : addresses)
    {
      socket_.bind(addr);
    }
  }

  awaitable<void> BasicEndpoint::run()
  {
    while (true)
    {
      auto buffer = co_await async_recv_base();
      handle_recv(std::move(buffer));
    }
  }

  void BasicEndpoint::handle_recv(RawBuffer_t&& buffer)
  {
    auto request = extract_request<
      RawBuffer_t,
      Protocol_t,
      Request_t,
      Deserializable_t
    >(std::move(buffer));

    const auto& header = request.header();
    auto consumer = find_consumer(header.message_number());
    if (!consumer)
      return;

    consumer->handle(*this, std::move(request));
  }

  BasicEndpoint::ConsumerHandlerBase_t* BasicEndpoint::find_consumer(const size_t message_number)
  {
    if (!handlers_.contains(message_number)) {
      return nullptr;
    } else {
      return handlers_[message_number].get();
    }

    return nullptr;
  }

}