#pragma once

#include <vector>
#include <zmq.hpp>

#include <utils/traits.hpp>

namespace icon::details {

namespace fields {
struct Identity {};
struct Header {};
struct Body {};
} // namespace fields

template <class... Fields> struct DataLayout {
  using Types = std::tuple<Fields...>;

  static constexpr auto size() { return std::tuple_size<Types>::value; }
};

template <class TRaw, class TRawBuffer, class TDataLayout> struct Protocol {
  using Raw = TRaw;
  using RawBuffer = TRawBuffer;
  using DataLayout = TDataLayout;
};

template <class Protocol> struct Parser {
  using Raw = typename Protocol::Raw;
  using RawBuffer = typename Protocol::RawBuffer;
  using DataLayout = typename Protocol::DataLayout;
  using DataLayoutTypes = typename DataLayout::Types;

  Parser() { buffer_.resize(DataLayout::size()); }

  Parser(RawBuffer &&buffer) {
    assert(buffer.size() == DataLayout::size());

    buffer_ = std::move(buffer);
  }

  Parser(const Parser &) = delete;
  Parser &operator=(const Parser &) = delete;
  Parser(Parser &&) = default;
  Parser &operator=(Parser &&) = default;

  template <class Field> Raw get() & {
    constexpr auto index =
        icon::traits::IndexOf<std::decay_t<Field>, DataLayoutTypes>::value();
    auto raw = Raw{};
    raw.copy(buffer_[index]);

    return Raw{std::move(raw)};
  }

  template <class Field> Raw get() && {
    constexpr auto index =
        icon::traits::IndexOf<std::decay_t<Field>, DataLayoutTypes>::value();

    return Raw{std::move(buffer_[index])};
  }

  template <class Field> void set(Raw raw) {
    constexpr auto index =
        icon::traits::IndexOf<std::decay_t<Field>, DataLayoutTypes>::value();

    buffer_[index] = std::move(raw);
  }

  RawBuffer parse() & { return buffer_; }

  RawBuffer parse() && { return std::move(buffer_); }

  void clear() { buffer_.clear(); }

  template<class... Fields>
  constexpr auto extract() &&
  {
    return std::make_tuple( (std::move(*this). template get<Fields>()) ... );
  }

  RawBuffer buffer_;
};
} // namespace icon::details