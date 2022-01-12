#pragma once

namespace icon::details::serialization::protobuf
{
  template<class T>
  size_t protobuf_message_number()
  {
    return T{}.GetDescriptor()->options().GetExtension(icon::metadata::MESSAGE_NUMBER);
  }
}