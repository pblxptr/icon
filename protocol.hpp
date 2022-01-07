#pragma once

#include <vector>
#include <zmq.hpp>

#include "traits.hpp"
namespace icon::details {

namespace fields2
{
  class Identity
  {
  public:
    explicit Identity(std::string identity)
      : identity_{std::move(identity)}
    {}

    std::string value() const
    {
      return identity_;
    }
  private:
    std::string identity_;
  };

  class Header
  {
  public:
    explicit Header(size_t msg_number)
      : message_number_{msg_number}
    {}

    size_t message_number() const
    {
      return message_number_;
    }

  private:
    size_t message_number_;
  };

  template<class Data = void>
  class Body
  {
  public:
    Body(Data&& data)
      : data_{std::move(data)}
    {}

    template<class Message>
    auto extract_with_message_number(size_t message_number)
    {
      return data_.template deserialize_safe<Message>(message_number);
    }


  private:
    Data data_;
  };
}

namespace fields {
  struct Identity {};
  struct Header {};
  struct Body {};
}

// template<class T>
// concept Deserializable =
// requires (T a) {
//     deserialize<void>(a);
// };

// template<class T>
// concept Serializable =
// requires (T a) {
//     serialize<void>(a);
// };

// template<class T>
// concept Deserialized = (not Deserializable<T>);

template<class T>
concept Deserializable =
requires (T a) {
    a.template deserialize<void>();
};

template<class T>
concept Serializable =
requires (T a) {
    a.serialize();
};

// template<class T>
// concept Deserialized = (not Deserializable<T>);


template<class... Fields>
struct DataLayout
{
  using Types = std::tuple<Fields...>;
  
  static constexpr auto size()
  {
    return std::tuple_size<Types>::value;
  }
};

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
  using DataLayoutTypes = typename DataLayout::Types;

  Parser()
  {
    buffer_.resize(DataLayout::size());
  }

  Parser(RawBuffer&& buffer)
  {
    assert(buffer.size() == DataLayout::size());
  
    buffer_ = std::move(buffer);
  }

  Parser(const Parser&) = delete;
  Parser& operator=(const Parser&) = delete;
  Parser(Parser&&) = default;
  Parser& operator=(Parser&&) = default;

  template<class Field>
  Raw get() &
  {
    constexpr auto index = icon::traits::IndexOf<std::decay_t<Field>, DataLayoutTypes>::value();
    auto raw = Raw{};
    raw.copy(buffer_[index]);

    return Raw{std::move(raw)};
  }

  template<class Field>
  Raw get() &&
  {
     constexpr auto index = icon::traits::IndexOf<std::decay_t<Field>, DataLayoutTypes>::value();

    return Raw{std::move(buffer_[index])};
  }

  template<class Field>
  void set(Raw raw)
  {
    constexpr auto index = icon::traits::IndexOf<std::decay_t<Field>, DataLayoutTypes>::value();

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