#pragma once

#include <serialization/protobuf_message_number.hpp>

#include "icon.pb.h"
#include <spdlog/spdlog.h>
#include <core/protocol.hpp>
#include <core/header.hpp>

#include <zmq.hpp>

namespace icon::details::serialization::protobuf
{
/**
 * @brief Represents BasicDeserializable data for Protobuf deserialization
 *
 */
class BasicDeserializable
{
public:
  explicit BasicDeserializable(zmq::message_t raw_message)
    : raw_message_{std::move(raw_message)}
  {}

  BasicDeserializable(const BasicDeserializable&) = delete;
  BasicDeserializable& operator=(const BasicDeserializable&) = delete;
  BasicDeserializable(BasicDeserializable&&) = default;
  BasicDeserializable& operator=(BasicDeserializable&&) = default;

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

/**
 * @brief Represents ProtobufDeserializable data
 *
 */
class ProtobufDeserializable : public BasicDeserializable
{
public:
  ProtobufDeserializable(zmq::message_t data)
    : BasicDeserializable(std::move(data))
  {}

  /**
   * @brief Checks whether message number matches to Message type
   *
   * @tparam Message type
   * @param message_number Message number to check
   * @return Returns true if message number matches to Message type, false if not
   */
  template<class T>
  bool message_number_match_for(const size_t message_number) const
  {
    return protobuf_message_number<T>() == message_number;
  }

  /**
   * @brief Deserialize data into Destination
   *
   * @tparam Destination type
   */
  template<class Destination>
  Destination deserialize() const
  {
    return BasicDeserializable::deserialize<Destination>();
  }
};

/**
 * @brief Checks whether message number matches to Message type
 *
 * @tparam Specialization for core::Header
 * @param message_number Message number to check
 * @return Returns true if message number matches to Message type, false if not
 */
template<>
inline bool ProtobufDeserializable::message_number_match_for<core::Header>(const size_t message_number) const
{
  return false;
}

/**
 * @brief Deserialize data into core::Header
 *
 * @tparam  Specialization for core::Header
 */
template<>
inline core::Header ProtobufDeserializable::deserialize<core::Header>() const
{
  auto th = BasicDeserializable::deserialize<icon::transport::Header>();

  return core::Header{th.message_number()};
}

}