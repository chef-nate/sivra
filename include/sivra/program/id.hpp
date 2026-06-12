#pragma once

#include <sivra/core/hash.hpp>
#include <sivra/core/owner_token.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace sivra::program {

class architecture_id {
public:
  architecture_id() = default;
  architecture_id(
    std::string value
  );
  architecture_id(
    const char* value
  );

  [[nodiscard]] std::string_view value() const;
  [[nodiscard]] bool empty() const;

  auto operator<=>(
    const architecture_id&
  ) const = default;

private:
  std::string m_value;
};

class architecture_profile_id {
public:
  architecture_profile_id() = default;
  architecture_profile_id(
    std::string value
  );
  architecture_profile_id(
    const char* value
  );

  [[nodiscard]] std::string_view value() const;
  [[nodiscard]] bool empty() const;

  auto operator<=>(
    const architecture_profile_id&
  ) const = default;

private:
  std::string m_value;
};

class register_id {
public:
  static constexpr register_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return register_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const register_id&
  ) const = default;

private:
  constexpr register_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index = 0;
  core::owner_token m_owner;
};

class instruction_form_id {
public:
  static constexpr instruction_form_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return instruction_form_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const instruction_form_id&
  ) const = default;

private:
  constexpr instruction_form_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index = 0;
  core::owner_token m_owner;
};

class function_id {
public:
  static constexpr function_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return function_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const function_id&
  ) const = default;

private:
  constexpr function_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index = 0;
  core::owner_token m_owner;
};

class basic_block_id {
public:
  static constexpr basic_block_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return basic_block_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const basic_block_id&
  ) const = default;

private:
  constexpr basic_block_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index = 0;
  core::owner_token m_owner;
};

class instruction_id {
public:
  static constexpr instruction_id unsafe_from_index(
    std::uint32_t index,
    core::owner_token owner
  ) {
    return instruction_id(index, owner);
  }

  [[nodiscard]] constexpr std::uint32_t index() const { return m_index; }
  [[nodiscard]] constexpr core::owner_token owner() const { return m_owner; }

  auto operator<=>(
    const instruction_id&
  ) const = default;

private:
  constexpr instruction_id(
    std::uint32_t index,
    core::owner_token owner
  )
      : m_index(index),
        m_owner(owner) {}

  std::uint32_t m_index = 0;
  core::owner_token m_owner;
};

} // namespace sivra::program

template <>
struct std::hash<sivra::program::register_id> {
  std::size_t operator()(
    sivra::program::register_id value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(seed, value.owner().value(), value.index());
    return seed;
  }
};

template <>
struct std::hash<sivra::program::instruction_form_id> {
  std::size_t operator()(
    sivra::program::instruction_form_id value
  ) const noexcept {
    std::size_t seed = 0;
    sivra::core::hash_combine(seed, value.owner().value(), value.index());
    return seed;
  }
};
