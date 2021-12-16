#pragma once

#include "icon.pb.h"

namespace icon::proto
{

template<class Message>
auto get_message_number()
{
  return Message::GetDescriptor()->options().GetExtension(icon::metadata::MESSAGE_NUMBER);
}

template<class Message>
auto get_header_for_message()
{
  auto header = icon::transport::Header{};
  header.set_message_number(get_message_number<Message>());

  return header;
}

template<class BaseField, class Data>
class ProtobufField : BaseField
{
public:
  ProtobufField() {}
  ProtobufField(Data data)
    : data_{std::move(data)}
  {};

  ProtobufField(const ProtobufField&) = delete;
  ProtobufField& operator=(const ProtobufField&) = delete;
  ProtobufField(ProtobufField&&) = default;
  ProtobufField& operator=(ProtobufField&&) = default;

  bool has_value() const
  {
    return data_.has_value() && not (*data_).empty();
  }

  const Data& data() const
  {
    return *data_;
  }

  std::optional<Data> data_;
};

template<class Raw, class BaseField, class Source>
Raw serialize(ProtobufField<BaseField, Source> field) requires (not std::is_same_v<Raw, Source>)
{
  const auto& data = field.data();
  auto raw = Raw{data.ByteSizeLong()};
  data.SerializeToArray(raw.data(), raw.size());

  return raw;
}

template<class Destination, class BaseField, class Raw>
Destination deserialize(const ProtobufField<BaseField, Raw>& field) requires (not std::is_same_v<Destination, Raw>)
{
  const auto& data = field.data();
  auto message = Destination{};
  message.ParseFromArray(data.data(), data.size());

  return message;
}
}


