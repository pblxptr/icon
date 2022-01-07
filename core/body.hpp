#pragma once

#include <serialization/serialization.hpp>

namespace icon::details::core
{
  template<Deserializable Data>
  class DeserializableBody
  {
  public:
    DeserializableBody(Data&& data)
      : data_{std::move(data)}
    {}

    template<class Message>
    bool message_number_match_for(const size_t message_number) const
    {
      return data_.template message_number_match_for<Message>(message_number);
    }

    template<class Message>
    auto deserialize() const
    {
      return data_.template deserialize<Message>();
    }

  private:
    Data data_;
  };

  template<Serializable Data>
  class SerializableBody
  {
  public:
    SerializableBody(Data&& data)
      : data_{std::move(data)}
    {}

    auto serialize() const
    {
      return data_.template serialize();
    }

  private:
    Data data_;
  };
}