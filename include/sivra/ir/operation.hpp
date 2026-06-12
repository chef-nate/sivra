#pragma once

#include "constant.hpp"
#include "id.hpp"
#include "operation_attribute.hpp"
#include "operation_signature.hpp"

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace sivra::ir {

enum class operation_trait : std::uint32_t {
  none = 0,
  associative = 1u << 0,
  commutative = 1u << 1,
  idempotent = 1u << 2,
  pure = 1u << 3,
  comparison = 1u << 4,
  conversion = 1u << 5,
  lane_operation = 1u << 6,
};

constexpr operation_trait operator|(
  operation_trait lhs,
  operation_trait rhs
) {
  return static_cast<operation_trait>(
    static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs)
  );
}

constexpr operation_trait operator&(
  operation_trait lhs,
  operation_trait rhs
) {
  return static_cast<operation_trait>(
    static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs)
  );
}

constexpr operation_trait operator~(
  operation_trait value
) {
  return static_cast<operation_trait>(~static_cast<std::uint32_t>(value));
}

enum class well_known_constant {
  zero,
  one,
  all_bits_set,
};

using operation_constant_value = std::variant<well_known_constant, scalar_constant_t>;

struct operation_constant {
  operation_constant_value element;
};

struct operation_semantics {
  operation_trait traits = operation_trait::none;
  std::optional<operation_constant> identity;
  std::optional<operation_constant> annihilator;
  std::string notes;
};

class operation_key {
public:
  operation_key() = default;
  operation_key(
    std::string value,
    std::uint32_t version = 1
  );
  operation_key(
    const char* value,
    std::uint32_t version = 1
  );

  [[nodiscard]] std::string_view value() const;
  [[nodiscard]] std::uint32_t version() const;
  [[nodiscard]] bool empty() const;

  auto operator<=>(
    const operation_key&
  ) const = default;

private:
  std::string m_value;
  std::uint32_t m_version = 1;
};

class operation_def {
public:
  operation_def(
    operation_id id,
    operation_key key,
    std::string name,
    operation_signature signature,
    operation_attribute_schema attribute_schema,
    operation_semantics semantics = {}
  );

  [[nodiscard]] operation_id id() const;
  [[nodiscard]] const operation_key& stable_key() const;
  [[nodiscard]] std::string_view key() const;
  [[nodiscard]] std::string_view name() const;
  [[nodiscard]] const operation_signature& signature() const;
  [[nodiscard]] const operation_attribute_schema& attribute_schema() const;
  [[nodiscard]] const operation_semantics& semantics() const;
  [[nodiscard]] bool has_trait(
    operation_trait trait
  ) const;

private:
  operation_id m_id;
  operation_key m_key;
  std::string m_name;
  operation_signature m_signature;
  operation_attribute_schema m_attribute_schema;
  operation_semantics m_semantics;
};

} // namespace sivra::ir
