#include <icon/endpoint/basic_endpoint/basic_endpoint.hpp>
#include <icon/utils/support.hpp>
#include <icon/endpoint/request.hpp>

namespace icon::details {
BasicEndpoint::BasicEndpoint(
  zmq::context_t& zctx,
  boost::asio::io_context& bctx,
  std::vector<std::string> addresses,
  std::unordered_map<size_t, std::unique_ptr<ConsumerHandlerBase_t>> handlers)
  : BaseEndpoint(zmq::socket_t{ zctx, zmq::socket_type::router }, bctx),
    handlers_{ std::move(handlers) }
{
  for (const auto& addr : addresses) {
    socket_.bind(addr);
  }
}

awaitable<void> BasicEndpoint::run()
{
  while (true) {
    auto buffer = co_await async_recv_base();
    co_await handle_recv(std::move(buffer));
  }
}

awaitable<void> BasicEndpoint::handle_recv(RawBuffer_t&& buffer)
{
  auto request = Request_t::create(std::move(buffer));

  const auto& header = request.header();
  auto consumer = find_consumer(header.message_number());
  if (!consumer)
    co_return;

  co_await consumer->handle(*this, std::move(request));
}

BasicEndpoint::ConsumerHandlerBase_t*
  BasicEndpoint::find_consumer(const size_t message_number)
{
  if (!handlers_.contains(message_number)) {
    return nullptr;
  } else {
    return handlers_[message_number].get();
  }

  return nullptr;
}

}// namespace icon::details