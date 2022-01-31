#pragma once

#include <icon/icon.hpp>
#include <icon/core/identity.hpp>
#include <icon/core/protocol.hpp>
#include <icon/endpoint/base_endpoint.hpp>
#include <icon/endpoint/consumer_handler.hpp>
#include <icon/endpoint/endpoint.hpp>
#include <icon/endpoint/request.hpp>
#include <icon/endpoint/response.hpp>
#include <icon/protobuf/protobuf_serialization.hpp>

namespace icon::details {

class BasicEndpoint : public BaseEndpoint
{
public:
  using BaseEndpoint::Raw_t;
  using BaseEndpoint::RawBuffer_t;
  using Serializer_t = icon::details::serialization::protobuf::ProtobufSerializer;
  using Deserializer_t = icon::details::serialization::protobuf::ProtobufDeserializer;
  using Request_t = EndpointRequest<Deserializer_t>;
  using ConsumerHandlerBase_t = ConsumerHandler<BasicEndpoint, Request_t>;

  BasicEndpoint(
    zmq::context_t& zctx,
    boost::asio::io_context& bctx,
    std::vector<std::string> addresses,
    std::unordered_map<size_t, std::unique_ptr<ConsumerHandlerBase_t>>
      handlers);

  template<MessageToSend Message>
  awaitable<void> async_respond(const icon::details::core::Identity& identity,
    Message&& message)
  {
    auto response = EndpointResponse<Message, Serializer_t>{ identity, std::forward<Message>(message) };
    co_await async_send_base(std::move(response).build());
  }

  awaitable<void> run() override;

private:
  awaitable<void> handle_recv(RawBuffer_t&&);
  ConsumerHandlerBase_t* find_consumer(const size_t);

private:
  std::unordered_map<size_t, std::unique_ptr<ConsumerHandlerBase_t>> handlers_;
};
}// namespace icon::details