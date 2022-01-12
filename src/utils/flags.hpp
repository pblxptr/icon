#pragma once

#include <bitset>

namespace icon::details {
template <typename T> class Flags {
  static_assert(std::is_enum_v<T>,
                "Flags can only be specialized for enum types");

  using Underlying_t = std::make_unsigned_t<std::underlying_type_t<T>>;

public:
  constexpr void set(T e, bool value = true) noexcept {
    bits_.set(underlying(e), value);
  }

  constexpr void clear(T e) noexcept { set(e, false); }

  constexpr bool test(T e) noexcept { return bits_.test(underlying(e)); }

  constexpr void reset() noexcept { bits_.reset(); }

private:
  static constexpr Underlying_t underlying(T e) {
    return static_cast<Underlying_t>(e);
  }

private:
  std::bitset<underlying(T::_size)> bits_;
};
} // namespace icon::details