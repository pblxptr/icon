#pragma once

#include "consumer_handler.hpp"
#include "endpoint.hpp"
#include "data_types.hpp"
#include "protobuf_serialization.hpp"
#include "protocol.hpp"
#include "icon.hpp"
#include "context.hpp"

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

    auto body = RawMessageData{std::move(parser)
      .template get<icon::details::fields::Body>()};

    return Request{
      icon::details::fields2::Identity{std::move(identity)},
      icon::details::fields2::Header{header.message_number()},
      icon::details::fields2::Body{std::move(body)}
    };
  }
  template<
    class Endpoint,
    class Message
  >
  auto build_request(const icon::details::fields2::Identity& identity, Message&& message)
  {
    // auto parser = icon::details::Parser<typename Endpoint::Protocol_t>{};
    // parser.set<icon::details::fields::Identity>(serialize<Endpoint::Raw_t>(Endpoint::ZmqData(identity)));
    // // parser.set<icon::details::fields::Header>()
    // parser.set<icon::details::fields::Body>(serialize<Endpoint::Raw_t>(message));

    // return parser.parse();
    return 1;
  }
}
namespace icon::details {


class BasicEndpoint : public BaseEndpoint
{
public:
  using BaseEndpoint::Raw_t;
  using BaseEndpoint::RawBuffer_t;
  template<class Message>
  using MessageData_t    = icon::details::serialization::protobuf::ProtobufMessage<Message>;
  using RawMessageData_t = icon::details::serialization::protobuf::ProtobufRawMessage;

  using Request_t           = icon::details::InternalRequest<RawMessageData_t>;
  using Response_t          = icon::details::InternalResponse<RawMessageData_t>;
  using ConsumerHandlerBase_t = ConsumerHandler<BasicEndpoint, Request_t>;

  using Protocol_t = icon::details::Protocol<
    Raw_t,
    RawBuffer_t,
    icon::details::DataLayout<
      icon::details::fields::Identity,
      icon::details::fields::Header,
      icon::details::fields::Body
    >
  >;

  BasicEndpoint
    (
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

  template<class Message>
  awaitable<void> async_send(icon::details::fields2::Identity& identity, Message&& message)
  {
    auto request = build_request<Endpoint, Message>(identity, std::forward<Message>(message));
    co_await async_send_base(std::move(request));
  }

  virtual awaitable<void> run() override
  {
    while (true)
    {
      auto buffer = co_await async_recv_base();
      handle_recv(std::move(buffer));
    }
  }

  void handle_recv(RawBuffer_t&& buffer)
  {
    auto request = extract_request<
      RawBuffer_t,
      Protocol_t,
      Request_t,
      RawMessageData_t
    >(std::move(buffer));

    const auto& header = request.header();
    auto consumer = find_consumer(header.message_number());
    if (!consumer) 
      return;

    consumer->handle(*this, std::move(request));
  }

  ConsumerHandlerBase_t* find_consumer(const size_t message_number)
  {
    if (!handlers_.contains(message_number)) {
      return nullptr;
    } else {
      return handlers_[message_number].get();
    }

    return nullptr;
  }

private:
  std::unordered_map<size_t, std::unique_ptr<ConsumerHandlerBase_t>> handlers_;
};

template<class Message>
class MessageContext
{
public:
  MessageContext(
    BasicEndpoint& endpoint,
    icon::details::fields2::Identity identity,
    icon::details::fields2::Header header,
    Message message
  )
  : endpoint_{endpoint}
  , identity_{std::move(identity)}
  , header_{std::move(header)}
  , message_{std::move(message)}
  {}

  const Message& message()
  {
    return message_;
  }

  template<MessageToSend ResponseMessage>
  awaitable<void> async_respond(ResponseMessage&& message)
  {
    co_await endpoint_.async_send(std::forward<ResponseMessage>(message));
  }

private:
  BasicEndpoint& endpoint_;
  icon::details::fields2::Identity identity_;
  icon::details::fields2::Header header_;
  Message message_;
};

template<
  class Endpoint,
  class Request,
  class Message,
  class Consumer
> 
class BasicEndpointConsumerHandler : public ConsumerHandler<Endpoint, Request>
{
public:
    explicit BasicEndpointConsumerHandler(Consumer consumer)
    : consumer_{std::move(consumer)} //TODO: move or forward?????
  {}

  void handle(Endpoint& endpoint, Request&& request)
  {
    auto context = MessageContext<Message>{
      endpoint,
      request.identity(),
      request.header(),
      request.template body<Message>()
    };
    consumer_(context);
  }

private:
  Consumer consumer_;
};

class BasicEndpointBuilder
{
public:
  void add_address(std::string&& address)
  {
    addresses_.push_back(std::move(address));
  }

  template<class Message, class Consumer>
  void add_consumer(Consumer&& consumer)
  {
    const auto msg_number = BasicEndpoint::MessageData_t<Message>::message_number();

    if (handlers_.contains(msg_number)) {
      throw std::invalid_argument("Handler for requested message has been already registered.");
    }

    handlers_[msg_number] = std::make_unique<
      BasicEndpointConsumerHandler<
        BasicEndpoint,
        BasicEndpoint::Request_t,
        Message, 
        Consumer
      >
    >(std::forward<Consumer>(consumer));
  }

  template<class... Args>
  auto build(Args... args)
  {
    return std::make_unique<BasicEndpoint>(
      context::zmq(),
      context::boost(),
      std::move(addresses_),
      std::move(handlers_)
    );
  }

private:
  std::vector<std::string> addresses_;
  std::unordered_map<size_t, std::unique_ptr<BasicEndpoint::ConsumerHandlerBase_t>> handlers_;
};
}