#pragma once

#include <vector>
#include <zmq.hpp>

#include "traits.hpp"

namespace icon {

namespace fields {
  struct Identity {};
  struct Header {};
  struct Body {};
}

template<class Raw>
Raw serialize();

template<class Destination, class Source>
Destination deserialize(const Source&);

template<class... Fields>
using DataLayout = std::tuple<Fields...>;

template<
  class TRaw,
  class TRawBuffer,
  class TDataLayout
>
struct Protocol
{
  using Raw = TRaw;
  using RawBuffer = TRawBuffer;
  using DataLayout = TDataLayout;
};

template<class Protocol>
struct Parser
{
  using Raw = typename Protocol::Raw;
  using RawBuffer = typename Protocol::RawBuffer;
  using DataLayout = typename Protocol::DataLayout;

  Parser()
  {
    buffer_.resize(std::tuple_size<DataLayout>::value);
  }

  Parser(RawBuffer&& buffer)
  {
    buffer_ = std::move(buffer);
  }

  Parser(const Parser&) = delete;
  Parser& operator=(const Parser&) = delete;
  Parser(Parser&&) = default;
  Parser& operator=(Parser&&) = default;

  template<class Field, class FieldData> requires std::is_base_of_v<Field, std::decay_t<FieldData>>
  FieldData get() &
  {
    constexpr auto index = icon::traits::IndexOf<std::decay_t<Field>, DataLayout>::value();
    auto raw = Raw{};
    raw.copy(buffer_[index]);

    return FieldData{std::move(raw)};
  }

 template<class Field, class FieldData> requires std::is_base_of_v<Field, std::decay_t<FieldData>>
  auto get() &&
  {
    constexpr auto index = icon::traits::IndexOf<std::decay_t<Field>, DataLayout>::value();

    return FieldData{Raw{std::move(buffer_[index])}};
  }

  template<class T>
  void set(Raw raw)
  {
    constexpr auto index = icon::traits::IndexOf<std::decay_t<T>, DataLayout>::value();

    buffer_[index] = std::move(raw);
  }

  template<class Field, class FieldData> requires std::is_base_of_v<Field, std::decay_t<FieldData>>
  void set(FieldData&& data)
  {
    constexpr auto index = icon::traits::IndexOf<std::decay_t<Field>, DataLayout>::value();

    buffer_[index] = serialize<Raw>(std::forward<FieldData>(data));
  }

  RawBuffer parse() &
  {
    return buffer_;
  }

  RawBuffer parse() &&
  {
    return std::move(buffer_);
  }

  void clear()
  {
    buffer_.clear();
  }

  RawBuffer buffer_;
};
}