#pragma once

#include <concepts>
#include <zmq.hpp>

namespace icon {
template<class Message>
concept MessageToSend = !std::is_lvalue_reference_v<Message>;

}
