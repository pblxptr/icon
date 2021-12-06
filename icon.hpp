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

  template <class Func>
  concept RegularHandler = std::invocable<Func, Message_t&&>;

  template <class Func>
  concept MultipartDynamicHandler = std::invocable<Func, std::vector<Message_t>&&>;

  // template <class Func, size_t Parts>
  // concept MultipartStaticHandler = std::invocable<Func, std::array<Message_t, Parts>&&>;
}