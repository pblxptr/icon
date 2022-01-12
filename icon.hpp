#pragma once

#include <zmq.hpp>
#include <concepts>

namespace icon
{
  template<class Message>
  concept MessageToSend = !std::is_lvalue_reference_v<Message>;
}