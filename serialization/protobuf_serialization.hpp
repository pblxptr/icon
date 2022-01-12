#pragma once

#include "icon.pb.h"
#include <spdlog/spdlog.h>
#include <core/protocol.hpp>

namespace icon::details::serialization::protobuf
{

template<class Message>
class ProtobufMessage
{
public:
  ProtobufMessage(Message message)
    : message_{std::move(message)}
  {}

  ProtobufMessage(const ProtobufMessage&) = delete;
  ProtobufMessage& operator=(const ProtobufMessage&) = delete;
  ProtobufMessage(ProtobufMessage&&) = default;
  ProtobufMessage& operator=(ProtobufMessage&&) = default;

  static size_t message_number()
  {
    return Message::GetDescriptor()->options().GetExtension(icon::metadata::MESSAGE_NUMBER);
  }

  zmq::message_t serialize() const
  {
    auto serialized = zmq::message_t{message_.ByteSizeLong()};
    message_.SerializeToArray(serialized.data(), serialized.size());

    return serialized;
  }

private:
  Message message_;
};

class ProtobufRawMessage
{
public:
  explicit ProtobufRawMessage(zmq::message_t raw_message)
    : raw_message_{std::move(raw_message)}
  {}

  ProtobufRawMessage(const ProtobufRawMessage&) = delete;
  ProtobufRawMessage& operator=(const ProtobufRawMessage&) = delete;
  ProtobufRawMessage(ProtobufRawMessage&&) = default;
  ProtobufRawMessage& operator=(ProtobufRawMessage&&) = default;

template<class Message>
bool message_number_match_for(const size_t message_number) const
{
  return ProtobufMessage<Message>::message_number() == message_number;
}

template<class Destination>
Destination deserialize() const
{
  auto dst = Destination{};
  dst.ParseFromArray(raw_message_.data(), raw_message_.size());

  return dst;
}

private:
  zmq::message_t raw_message_;
};

}


