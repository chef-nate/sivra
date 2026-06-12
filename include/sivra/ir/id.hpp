#pragma once

#include <sivra/core/hash.hpp>
#include <sivra/core/owner_token.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>

namespace sivra::ir {

class operation_id {
public:
  static constexpr operation_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return operation_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }

  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const operation_id&
  ) const = default;

private:
  constexpr operation_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index;
  core::owner_token m_owner;
};

class node_id {
public:
  static constexpr node_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return node_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }

  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const node_id&
  ) const = default;

private:
  constexpr node_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index;
  core::owner_token m_owner;
};

class external_value_id {
public:
  static constexpr external_value_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return external_value_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }

  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const external_value_id&
  ) const = default;

private:
  constexpr external_value_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index;
  core::owner_token m_owner;
};

class symbol_id {
public:
  static constexpr symbol_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return symbol_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }

  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const symbol_id&
  ) const = default;

private:
  constexpr symbol_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index;
  core::owner_token m_owner;
};

} // namespace sivra::ir

template <>
struct std::hash<sivra::ir::operation_id> {
  std::size_t operator()(
    sivra::ir::operation_id value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(seed, value.owner().value(), value.index());
    return seed;
  }
};

template <>
struct std::hash<sivra::ir::node_id> {
  std::size_t operator()(
    sivra::ir::node_id value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(seed, value.owner().value(), value.index());
    return seed;
  }
};

template <>
struct std::hash<sivra::ir::external_value_id> {
  std::size_t operator()(
    sivra::ir::external_value_id value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(seed, value.owner().value(), value.index());
    return seed;
  }
};

template <>
struct std::hash<sivra::ir::symbol_id> {
  std::size_t operator()(
    sivra::ir::symbol_id value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(seed, value.owner().value(), value.index());
    return seed;
  }
};
