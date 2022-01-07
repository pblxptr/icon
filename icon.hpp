#pragma once

#include <zmq.hpp>
#include <concepts>

namespace icon::details 
{
  template<class Message>
  concept MessageToSend = !std::is_lvalue_reference_v<Message>;
}