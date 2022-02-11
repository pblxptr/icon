#pragma once

#include <metadata.pb.h>

namespace icon::details::serialization::protobuf {
template<class T>
size_t protobuf_message_number()
{
  return T{}.GetDescriptor()->options().GetExtension(
    shm.iw.proto::MESSAGE_NUMBER);
}
}// namespace icon::details::serialization::protobuf