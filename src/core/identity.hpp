#pragma once
#include <string>
namespace icon::details::core {
class Identity {
public:
  explicit Identity(std::string identity) : identity_{std::move(identity)} {}

  std::string value() const { return identity_; }

private:
  std::string identity_;
};
} // namespace icon::details::core