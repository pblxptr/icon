#pragma once

#include <concepts>

template<class T>
concept Rvalue = !std::is_lvalue_reference_v<T>;