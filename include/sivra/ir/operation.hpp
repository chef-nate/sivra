#pragma once

#include "constant.hpp"
#include "id.hpp"

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

struct operation_signature {
  std::uint32_t minimum_operands = 0;
  std::optional<std::uint32_t> maximum_operands;
  bool operands_match_result = true;
};

class operation_def {
public:
  operation_def(
    operation_id id,
    std::string key,
    std::string name,
    operation_signature signature,
    operation_semantics semantics = {}
  );

  operation_id id() const;
  std::string_view key() const;
  std::string_view name() const;
  const operation_signature& signature() const;
  const operation_semantics& semantics() const;
  bool has_trait(
    operation_trait trait
  ) const;

private:
  operation_id m_id;
  std::string m_key;
  std::string m_name;
  operation_signature m_signature;
  operation_semantics m_semantics;
};

} // namespace sivra::ir
