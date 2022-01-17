#include "context.hpp"

//TODO: Possible to be removed in favor of the use_serivce method.

namespace {
boost::asio::io_context boost_context{};
zmq::context_t zmq_context{};
}// namespace

namespace icon::details::context {
boost::asio::io_context& boost() { return boost_context; }
zmq::context_t& zmq() { return zmq_context; }
}// namespace icon::details::context