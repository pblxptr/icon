#pragma once

#include <icon/core/identity.hpp>
#include <icon/core/header.hpp>
#include <icon/protobuf/icon.pb.h>
#include <icon/metadata/metadata.pb.h>

// TODO: Add basic serializer, and move therer serialization of core::Identity
namespace icon::details::serialization::protobuf {
class ProtobufData
{
public:
  template<class T>
  requires(!std::is_same_v<T, core::Header>) static size_t message_number_for()
  {
    return T{}.GetDescriptor()->options().GetExtension(icon::metadata::MESSAGE_NUMBER);
  }
};

class ProtobufSerializer : public ProtobufData
{
public:
  using ProtobufData::message_number_for;

  template<class T>
  static zmq::message_t serialize(const T& t)
  {
    auto serialized = zmq::message_t{ t.ByteSizeLong() };
    t.SerializeToArray(serialized.data(), serialized.size());

    return serialized;
  }

  static zmq::message_t serialize(const core::Header& header)
  {
    auto th = icon::Header{};
    th.set_message_number(header.message_number());

    return serialize(th);
  }

  static zmq::message_t serialize(const core::Identity& identity)
  {
    return zmq::message_t{ identity.value() };
  }
};

class ProtobufDeserializer : public ProtobufData
{
public:
  using ProtobufData::message_number_for;

  template<class Destination>
  static Destination deserialize(const zmq::message_t& src)
  {
    auto dst = Destination{};
    dst.ParseFromArray(src.data(), src.size());

    return dst;
  }
};
template<>
inline core::Header ProtobufDeserializer::deserialize<core::Header>(const zmq::message_t& src)
{
  auto th = deserialize<icon::Header>(src);

  return core::Header{ th.message_number() };
}
}// namespace icon::details::serialization::protobuf
