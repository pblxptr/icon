#pragma once

#include <zmqpp/zmqpp.hpp>
#include "event_bus.hpp"

struct ZmqSocketDescriptor;

struct ZmqMessageReceived
{
  ZmqSocketDescriptor& descriptor;
  zmqpp::message_t message;
};

struct ZmqMessageSent
{
  ZmqSocketDescriptor& descriptor;
};

struct ZmqSocketError
{
  ZmqSocketDescriptor& descriptor;
};


class ZmqSocketEventBus : public EventBus<
  ZmqMessageReceived,
  ZmqMessageSent
>
{

};