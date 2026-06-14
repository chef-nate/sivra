#pragma once

#include <sivra/core/hash.hpp>
#include <sivra/core/owner_token.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>

namespace sivra::recovery {

class memory_version {
public:
  static constexpr memory_version unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return memory_version(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const memory_version&
  ) const = default;

private:
  constexpr memory_version(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index = 0;
  core::owner_token m_owner;
};

class provenance_id {
public:
  static constexpr provenance_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return provenance_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const provenance_id&
  ) const = default;

private:
  constexpr provenance_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index = 0;
  core::owner_token m_owner;
};

class object_id {
public:
  static constexpr object_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return object_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const object_id&
  ) const = default;

private:
  constexpr object_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index = 0;
  core::owner_token m_owner;
};

class type_variable_id {
public:
  static constexpr type_variable_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return type_variable_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const type_variable_id&
  ) const = default;

private:
  constexpr type_variable_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index = 0;
  core::owner_token m_owner;
};

} // namespace sivra::recovery

template <>
struct std::hash<sivra::recovery::memory_version> {
  std::size_t operator()(
    sivra::recovery::memory_version value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(seed, value.owner().value(), value.index());
    return seed;
  }
};

template <>
struct std::hash<sivra::recovery::provenance_id> {
  std::size_t operator()(
    sivra::recovery::provenance_id value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(seed, value.owner().value(), value.index());
    return seed;
  }
};

template <>
struct std::hash<sivra::recovery::object_id> {
  std::size_t operator()(
    sivra::recovery::object_id value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(seed, value.owner().value(), value.index());
    return seed;
  }
};

template <>
struct std::hash<sivra::recovery::type_variable_id> {
  std::size_t operator()(
    sivra::recovery::type_variable_id value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(seed, value.owner().value(), value.index());
    return seed;
  }
};
