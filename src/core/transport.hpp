#pragma once

#include <zmq.hpp>

namespace icon::details::transport {
using Raw_t = zmq::message_t;
using RawBuffer_t = std::vector<Raw_t>;
}// namespace icon::details::transport