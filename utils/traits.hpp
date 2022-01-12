#pragma once

#include <tuple>

namespace icon::traits {
template <class T, class Tuple> struct IndexOf;

template <class T, class... Types> struct IndexOf<T, std::tuple<T, Types...>> {
  static constexpr size_t value() { return 0; }
};

template <class T, class U, class... Types>
struct IndexOf<T, std::tuple<U, Types...>> {
  static constexpr size_t value() {
    return 1 + IndexOf<T, std::tuple<Types...>>::value();
  }
};
} // namespace icon::traits
