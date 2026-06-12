#pragma once

#include "value_type.hpp"

#include <sivra/core/result.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace sivra::ir {

class operation_enum_value {
public:
  operation_enum_value() = default;
  operation_enum_value(
    std::string key
  );

  [[nodiscard]] std::string_view key() const;

  auto operator<=>(
    const operation_enum_value&
  ) const = default;

private:
  std::string m_key;
};

using operation_attribute_value =
  std::variant<std::int64_t, bool, operation_enum_value, std::vector<std::uint32_t>, value_type>;

enum class operation_attribute_kind {
  integer,
  boolean,
  enum_key,
  index_vector,
  value_type,
};

struct operation_attribute {
  std::string key;
  operation_attribute_value value;

  auto operator<=>(
    const operation_attribute&
  ) const = default;
};

struct operation_attribute_field {
  std::string key;
  operation_attribute_kind kind = operation_attribute_kind::integer;
  bool required = false;
  std::optional<operation_attribute_value> default_value;
  std::optional<std::int64_t> minimum_integer;
  std::optional<std::int64_t> maximum_integer;
  std::vector<operation_enum_value> allowed_enum_values;
};

class operation_attributes {
public:
  operation_attributes() = default;

  [[nodiscard]] static core::result_t<operation_attributes> create(
    std::span<const operation_attribute> attributes
  );

  [[nodiscard]] std::span<const operation_attribute> entries() const;
  [[nodiscard]] const operation_attribute_value* find(
    std::string_view key
  ) const;
  [[nodiscard]] bool empty() const;

  auto operator<=>(
    const operation_attributes&
  ) const = default;

private:
  explicit operation_attributes(
    std::vector<operation_attribute> attributes
  );

  std::vector<operation_attribute> m_attributes;
};

class operation_attribute_schema {
public:
  operation_attribute_schema() = default;

  [[nodiscard]] static core::result_t<operation_attribute_schema> create(
    std::span<const operation_attribute_field> fields
  );

  [[nodiscard]] core::result_t<operation_attributes> validate(
    const operation_attributes& attributes
  ) const;

  [[nodiscard]] std::span<const operation_attribute_field> fields() const;
  [[nodiscard]] bool empty() const;

private:
  explicit operation_attribute_schema(
    std::vector<operation_attribute_field> fields
  );

  std::vector<operation_attribute_field> m_fields;
};

[[nodiscard]] operation_attribute_kind kind_of(
  const operation_attribute_value& value
);

} // namespace sivra::ir

template <>
struct std::hash<sivra::ir::operation_enum_value> {
  std::size_t operator()(
    const sivra::ir::operation_enum_value& value
  ) const noexcept;
};

template <>
struct std::hash<sivra::ir::operation_attributes> {
  std::size_t operator()(
    const sivra::ir::operation_attributes& value
  ) const noexcept;
};
