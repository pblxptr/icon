#pragma once

namespace icon::details {
  enum class ErrorCode {
    SendTimeout,
    ReceiveTimeout,
    InternalError
  };
}