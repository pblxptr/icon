#pragma once

#include "icon.pb.h"
#include <spdlog/spdlog.h>

namespace icon::proto
{
template<class RawData>
class ProtobufData
{
public:
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
Destination serialize(ProtobufData<Source> field) requires (not std::is_same_v<Destination, Source>)
{
  const auto& data = field.data();
  auto dst = Destination{data.ByteSizeLong()};
  data.SerializeToArray(dst.data(), dst.size());

  return dst;
}

template<class Destination, class Source>
Destination deserialize(const ProtobufData<Source>& field) requires (not std::is_same_v<Destination, Source>)
{
  const auto& data = field.data();
  auto message = Destination{};
  message.ParseFromArray(data.data(), data.size());

  return message;
}
}


