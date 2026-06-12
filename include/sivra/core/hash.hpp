#pragma once

#include <cstddef>
#include <functional>
#include <type_traits>

namespace sivra::core {

inline void hash_combine(
  std::size_t&
) noexcept {
}

template <
  typename T,
  typename... Rest
>
void hash_combine(
  std::size_t& seed,
  const T& value,
  const Rest&... rest
) noexcept {
  constexpr std::size_t mix_constant = 0x9e3779b9U;
  seed ^= std::hash<std::remove_cvref_t<T>>{}(value) + mix_constant + (seed << 6) + (seed >> 2);
  hash_combine(seed, rest...);
}

} // namespace sivra::core
