#pragma once

/**
 * @file icon.hpp
 * @author your name (you@domain.com)
 * @brief Instant connection
 * @version 0.1
 * @date 2021-12-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <zmq.hpp>
#include <concepts>

namespace icon::details {
  using Message_t = zmq::message_t;
  using RegularMessage_t = Message_t;
  using MultipartDynamicMessage_t = std::vector<Message_t>;

  template<class Message>
  concept RegularMessage = std::is_same_v<Message, RegularMessage_t>;

  template<class Message>
  concept MultipartDynamicMessage = std::is_same_v<std::decay_t<Message>, MultipartDynamicMessage_t>;

  template<class Message>
  concept MessageToSend = !std::is_lvalue_reference_v<Message>;

  template <class Func>
  concept RegularRecvHandler = std::invocable<Func, RegularMessage_t&&>;

  template <class Func>
  concept MultipartDynamicRecvHandler = std::invocable<Func, MultipartDynamicMessage_t&&>;
}