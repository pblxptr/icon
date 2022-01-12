#pragma once

namespace icon::details::core {
class Header
{
public:
  explicit Header(size_t msg_number) : message_number_{ msg_number } {}

  size_t message_number() const { return message_number_; }

private:
  size_t message_number_;
};
}// namespace icon::details::core