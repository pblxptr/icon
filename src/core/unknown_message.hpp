#pragma once

#include <cstdint>
#include <core/transport.hpp>
#include <optional>

namespace icon::details::core {

// TODO: Consider adding additional field size_t message number and template parameter Deserializer.
// Then 'is' and 'get' functions could be refactored and there would be
// no need to implement 'is' and 'get' functions in derived classes.
class UnknownMessage
{
public:
  UnknownMessage() {}

  explicit UnknownMessage(transport::Raw_t data)
    : data_{ std::move(data) }
  {}
  template<class Deserializer, class Message>
  bool is(const size_t msg_number) const
  {
    return Deserializer::template message_number_for<Message>() == msg_number;
  }

  template<class Deserializer, class Destination>
  Destination get() const
  {
    if (!data_) {
      throw std::runtime_error("Cannot deserializer. Message is empty");
    }

    return Deserializer::template deserialize<Destination>(*data_);
  }

private:
  std::optional<transport::Raw_t> data_;
};
}// namespace icon::details::core