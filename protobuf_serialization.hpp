#pragma once

#include "icon.pb.h"
#include <spdlog/spdlog.h>
#include "protocol.hpp"

namespace {
  using DomainHeader_t =    icon::details::fields2::Header;
  using TransportHeader_t = icon::transport::Header;
}

namespace icon::details::serialization
{
template<class RawData>
class ProtobufData
{
public:
  ProtobufData() = default;
  explicit ProtobufData(RawData data)
    : data_{std::move(data)}
  {};

  ProtobufData(const ProtobufData&) = delete;
  ProtobufData& operator=(const ProtobufData&) = delete;
  ProtobufData(ProtobufData&&) = default;
  ProtobufData& operator=(ProtobufData&&) = default;

  bool has_value() const
  {
    return data_.has_value();
  }

  const RawData& data() const
  {
    return *data_;
  }

  std::optional<RawData> data_;
};

template<class Message, class Body>
uint32_t get_message_number(const ProtobufData<Body>& msg)
{
  return Message::GetDescriptor()->options().GetExtension(icon::metadata::MESSAGE_NUMBER);
}

template<class Message>
auto get_header_for_message(const ProtobufData<Message>& data)
{
  auto req = icon::transport::Header{};
  req.set_message_number(get_message_number<Message>(data));

  return ProtobufData<icon::transport::Header>{std::move(req)};
}

template<class Destination, class Source>
Destination serialize(const ProtobufData<Source>& field) requires (not std::is_same_v<Destination, Source>)
{
  const auto& data = field.data();
  auto dst = Destination{data.ByteSizeLong()};
  data.SerializeToArray(dst.data(), dst.size());

  return dst;
}

template<class Destination>
Destination serialize(const ProtobufData<DomainHeader_t>& header_field)
{
  const auto& header = header_field.data();
  auto proto_header = TransportHeader_t{};
  proto_header.set_message_number(header.message_number());

  return serialize<Destination>(ProtobufData{std::move(proto_header)});
}

template<class Destination, class Source>
Destination deserialize(const ProtobufData<Source>& field) requires (not std::is_same_v<Destination, DomainHeader_t>)
{
  const auto& src = field.data();
  auto dst = Destination{};
  dst.ParseFromArray(src.data(), src.size());

  return dst;
}

template<class Destination, class Source>
Destination deserialize(const ProtobufData<Source>& field) requires std::is_same_v<Destination, DomainHeader_t>
{
  auto proto_header = deserialize<TransportHeader_t>(field);
    
  return DomainHeader_t{proto_header.message_number()};
}

}


