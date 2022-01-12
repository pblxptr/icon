#pragma once

#include <serialization/protobuf_message_number.hpp>

#include "icon.pb.h"
#include <spdlog/spdlog.h>
#include <core/protocol.hpp>
#include <core/header.hpp>

namespace icon::details::serialization::protobuf
{
/**
 * @brief Basic serialiable class for Protobuf data
 *
 * @tparam Data to serialize
 */
template<class Data>
class BasicSerializable
{
public:
  BasicSerializable(Data message)
    : data_{std::move(message)}
  {}

  BasicSerializable(const BasicSerializable&) = delete;
  BasicSerializable& operator=(const BasicSerializable&) = delete;
  BasicSerializable(BasicSerializable&&) = default;
  BasicSerializable& operator=(BasicSerializable&&) = default;

  zmq::message_t serialize() const
  {
    auto serialized = zmq::message_t{data_.ByteSizeLong()};
    data_.SerializeToArray(serialized.data(), serialized.size());

    return serialized;
  }
private:
  Data data_;
};

/**
 * @brief Serializable for every message from .proto
 *
 * @tparam Data message to serializae
 */
template<class T>
class ProtobufSerializable : public BasicSerializable<T>
{
public:
  using BasicSerializable<T>::serialize;

  explicit ProtobufSerializable(T data)
    : BasicSerializable<T>(std::move(data))
  {}

  static size_t message_number()
  {
    return protobuf_message_number<T>();
  }
};

/**
 * @brief Serializable specialization for core::Header
 *
 * @tparam specialized for core::Header
 */
template<>
class ProtobufSerializable<core::Header> : public BasicSerializable<icon::transport::Header>
{
public:
  using BasicSerializable<icon::transport::Header>::serialize;

  explicit ProtobufSerializable(const core::Header& header)
    : BasicSerializable<icon::transport::Header>(convert(header))
{}
private:
/**
 * @brief Converts core::Header to transport haeder
 *
 * @param header Header to serialize
 * @return icon::transport::Header
 */
  icon::transport::Header convert(const core::Header& header)
  {
    auto th = icon::transport::Header{};
    th.set_message_number(header.message_number());

    return th;
  }
};

}