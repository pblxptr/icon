#pragma once

#include <boost/asio.hpp>
#include <zmq.hpp>

namespace icon::details::context {
boost::asio::io_context &boost();
zmq::context_t &zmq();
}// namespace icon::details::context