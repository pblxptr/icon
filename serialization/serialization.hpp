#pragma once

namespace icon::details {

template <class T>
concept Deserializable = requires(T a) {
  a.template deserialize<void>();
};

template <class T>
concept Serializable = requires(T a) {
  a.serialize();
};
} // namespace icon::details