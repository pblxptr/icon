// #pragma once

// #include <zmq.hpp>
// #include <zmq_addon.hpp>
// #include <spdlog/spdlog.h>
// #include "zmq_stream_descriptor.hpp"
// #include <optional>
// #include <cassert>

// class AcceptMonitor
// {
//   static constexpr const char* addr = "inproc://accept_mon";

// public:
//   AcceptMonitor(zmq::context_t& zctx, boost::asio::io_context& bctx, zmq::socket_t& socket)
//     : socket_{socket}
//     , monitor_socket_{zctx, zmq::socket_type::pair}
//     , streamd_{bctx}
//   {}

//   ~AcceptMonitor()
//   {
//     close();
//   }

//   void run(const char* addr, const int events = ZMQ_EVENT_ALL)
//   {
//     spdlog::debug("AcceptMonitor::run()");

//     if (zmq_socket_monitor(socket_.handle(), addr, events)) {
//         throw error_t();
//     }

//     monitor_socket_.connect(addr);

//     spdlog::debug("Acceptor monitor on socket: {}", monitor_socket_.get(zmq::sockopt::fd));

//     streamd_.assign(monitor_socket_.get(zmq::sockopt::fd));

//     async_wait();
//   }

//   void close()
//   {
//     spdlog::debug("AcceptMonitor::close()");
    
//     if (socket_) {
//       zmq_socket_monitor(socket_.handle(), ZMQ_NULLPTR, 0);
//     }
//     monitor_socket_.close();
//   }

// private:
//   void async_wait()
//   {
//     spdlog::debug("AcceptMonitor::async_wait()");
    
//     streamd_.async_wait(boost::asio::posix::stream_descriptor::wait_type::wait_error,
//       [this](const auto& ec) { 
//         if (ec) {
//           spdlog::debug("AcceptMonitor::async_wait::wait_for_error: {}", ec.message());
//           return;
//         }
//         check_event(); 
//       }
//     );
//     streamd_.async_wait(boost::asio::posix::stream_descriptor::wait_type::wait_read,
//       [this](const auto& ec) { 
//         if (ec) {
//           spdlog::debug("AcceptMonitor::async_wait::wait_read {}", ec.message());
//           return;
//         }
//         check_event(); 
//       }
//     );
//   }

//   bool check_event()
//   {
//     spdlog::debug("AcceptMonitor::check_event");
//     assert(monitor_socket_);

//     auto messages = std::vector<zmq::message_t>{};
//     zmq::pollitem_t items[] = {
//       { monitor_socket_.handle(), 0, ZMQ_POLLIN, 0 },
//     };
    
//     zmq::poll(&items[0], 1, std::chrono::milliseconds(200));
  
//     if (items[0].revents & ZMQ_POLLIN) {
//       const auto number_of_messages = zmq::recv_multipart(monitor_socket_, std::back_inserter(messages));

//       if (!number_of_messages) {
//         spdlog::debug("No message has been received");
//         return false;
//       }
//     } else {
//       return false;
//     }

//     assert(messages.size() == 2);

//     const auto event = get_event(messages[0]);
//     const auto address = get_address(messages[1]);

//     spdlog::debug("Processing monitor messages");

//     return process_event(event, address);
//   }

//   zmq_event_t get_event(zmq::message_t& msg)
//   {
//     const char *data = static_cast<const char *>(msg.data());
//     auto msg_event = zmq_event_t{};
//     memcpy(&msg_event.event, data, sizeof(uint16_t));
//     data += sizeof(uint16_t);
//     memcpy(&msg_event.value, data, sizeof(int32_t));

//     return msg_event;
//   }

//   std::string get_address(zmq::message_t& msg)
//   {
//     return msg.to_string();
//   }
  
//   bool process_event(const zmq_event_t& event, const std::string& address)
//   {
//     spdlog::debug("process_event");

//     switch (event.event)
//     {
//       case ZMQ_EVENT_MONITOR_STOPPED:
//         spdlog::debug("ZMQ_EVENT_MONITOR_STOPPED");
//         return false;
//         break;
//       case ZMQ_EVENT_CONNECT_DELAYED:
//         spdlog::debug("ZMQ_EVENT_CONNECT_DELAYED");
//         break;
//       case ZMQ_EVENT_ACCEPTED:
//         spdlog::debug("ZMQ_EVENT_ACCEPTED");
//         break;
//       case ZMQ_EVENT_DISCONNECTED:
//         spdlog::debug("ZMQ_EVENT_DISCONNECTED");
//         break;
//       default:
//         spdlog::debug("Unknown event");
//     }

//     return true;
//   }

// private:
//   zmq::socket_t& socket_;
//   zmq::socket_t monitor_socket_;
//   boost::asio::posix::stream_descriptor streamd_;
// };