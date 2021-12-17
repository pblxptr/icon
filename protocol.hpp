#pragma once

#include <vector>
#include <zmq.hpp>

#include "traits.hpp"

namespace icon::details {

namespace fields {
  struct Identity {};
  struct Header {};
  struct Body {};
}

template<class T>
concept Deserializable =
requires (T a) {
    deserialize<void>(a);
};

template<class T>
concept Serializable =
requires (T a) {
    serialize<void>(a);
};

template<class T>
concept Deserialized = (not Deserializable<T>);

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

  template<class Field>
  Raw get() &
  {
    constexpr auto index = icon::traits::IndexOf<std::decay_t<Field>, DataLayout>::value();
    auto raw = Raw{};
    raw.copy(buffer_[index]);

    return Raw{std::move(raw)};
  }

  template<class Field>
  Raw get() &&
  {
     constexpr auto index = icon::traits::IndexOf<std::decay_t<Field>, DataLayout>::value();

    return Raw{std::move(buffer_[index])};
  }

  template<class T>
  void set(Raw raw)
  {
    constexpr auto index = icon::traits::IndexOf<std::decay_t<T>, DataLayout>::value();

    buffer_[index] = std::move(raw);
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