#pragma once

namespace icon::details::core
{
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