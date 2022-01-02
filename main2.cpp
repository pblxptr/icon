#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <spdlog/spdlog.h>
#include <cassert>
#include "co_stream_watcher.hpp"
#include "zmq_client.hpp"
#include "icon.pb.h"
#include "protobuf_serialization.hpp"
#include "protocol.hpp"
#include "data_types.hpp"

using namespace icon::details;
using Raw = zmq::message_t;
template<class Message>
using DataType = icon::details::serialization::ProtobufData<Message>;

template<class Message>
void send(Message&& msg)
{
  auto header = fields2::Header{1};
  auto raw_header = serialize<Raw>(DataType{std::move(header)});
  auto raw_body   = serialize<Raw>(DataType{std::forward<Message>(msg)});
}

void receive()
{
  auto raw_header = Raw{};
  auto raw_body   = Raw{};

  auto header = deserialize<fields2::Header>(DataType{std::move(raw_header)});

}

// icon::fields2::Header -> icon::transport::Header -> zmq::message_t

int main()
{
  zmq::socket_type::router
  send(icon::transport::ConnectionEstablishReq{});
  receive();

}