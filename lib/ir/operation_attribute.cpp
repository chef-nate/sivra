#include <sivra/ir/operation_attribute.hpp>

#include <sivra/core/hash.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

namespace {

bool enum_value_allowed(
  const sivra::ir::operation_attribute_field& field,
  const sivra::ir::operation_enum_value& value
) {
  return field.allowed_enum_values.empty() ||
         std::ranges::find(field.allowed_enum_values, value) != field.allowed_enum_values.end();
}

bool value_is_valid_for_field(
  const sivra::ir::operation_attribute_field& field,
  const sivra::ir::operation_attribute_value& value
) {
  if (sivra::ir::kind_of(value) != field.kind) {
    return false;
  }
  if (const auto* integer = std::get_if<std::int64_t>(&value)) {
    return (!field.minimum_integer.has_value() || *integer >= *field.minimum_integer) &&
           (!field.maximum_integer.has_value() || *integer <= *field.maximum_integer);
  }
  if (const auto* enum_value = std::get_if<sivra::ir::operation_enum_value>(&value)) {
    return !enum_value->key().empty() && enum_value_allowed(field, *enum_value);
  }
  if (const auto* type = std::get_if<sivra::ir::value_type>(&value)) {
    return type->validate().has_value();
  }
  return true;
}

void hash_attribute_value(
  std::size_t& seed,
  const sivra::ir::operation_attribute_value& value
) {
  sivra::core::hash_combine(seed, value.index());
  std::visit(
    [&seed](const auto& entry) {
      using entry_t = std::remove_cvref_t<decltype(entry)>;
      if constexpr (std::is_same_v<entry_t, std::vector<std::uint32_t>>) {
        for (const auto index : entry) {
          sivra::core::hash_combine(seed, index);
        }
      } else {
        sivra::core::hash_combine(seed, entry);
      }
    },
    value
  );
}

} // namespace

namespace sivra::ir {

operation_enum_value::operation_enum_value(
  std::string key
)
    : m_key(std::move(key)) {
}

std::string_view operation_enum_value::key() const {
  return m_key;
}

operation_attribute_kind kind_of(
  const operation_attribute_value& value
) {
  return static_cast<operation_attribute_kind>(value.index());
}

core::result_t<operation_attributes> operation_attributes::create(
  std::span<const operation_attribute> attributes
) {
  std::vector<operation_attribute> normalized(attributes.begin(), attributes.end());
  std::ranges::sort(normalized, {}, &operation_attribute::key);

  for (std::size_t index = 0; index < normalized.size(); ++index) {
    if (normalized[index].key.empty()) {
      return core::fail<operation_attributes>(
        "ir.attribute.empty_key", "operation attribute key must not be empty"
      );
    }
    if (index != 0 && normalized[index - 1].key == normalized[index].key) {
      return core::fail<operation_attributes>(
        "ir.attribute.duplicate", "operation attribute keys must be unique"
      );
    }
  }
  return operation_attributes(std::move(normalized));
}

std::span<const operation_attribute> operation_attributes::entries() const {
  return m_attributes;
}

const operation_attribute_value* operation_attributes::find(
  std::string_view key
) const {
  const auto found = std::ranges::lower_bound(m_attributes, key, {}, &operation_attribute::key);
  return found != m_attributes.end() && found->key == key ? &found->value : nullptr;
}

bool operation_attributes::empty() const {
  return m_attributes.empty();
}

operation_attributes::operation_attributes(
  std::vector<operation_attribute> attributes
)
    : m_attributes(std::move(attributes)) {
}

core::result_t<operation_attribute_schema> operation_attribute_schema::create(
  std::span<const operation_attribute_field> fields
) {
  std::vector<operation_attribute_field> normalized(fields.begin(), fields.end());
  std::ranges::sort(normalized, {}, &operation_attribute_field::key);

  for (std::size_t index = 0; index < normalized.size(); ++index) {
    const auto& field = normalized[index];
    if (field.key.empty()) {
      return core::fail<operation_attribute_schema>(
        "ir.attribute_schema.empty_key", "operation attribute schema key must not be empty"
      );
    }
    if (index != 0 && normalized[index - 1].key == field.key) {
      return core::fail<operation_attribute_schema>(
        "ir.attribute_schema.duplicate", "operation attribute schema keys must be unique"
      );
    }
    if (field.minimum_integer.has_value() && field.maximum_integer.has_value() &&
        *field.minimum_integer > *field.maximum_integer) {
      return core::fail<operation_attribute_schema>(
        "ir.attribute_schema.invalid_range", "operation attribute integer range is invalid"
      );
    }
    if ((field.minimum_integer.has_value() || field.maximum_integer.has_value()) &&
        field.kind != operation_attribute_kind::integer) {
      return core::fail<operation_attribute_schema>(
        "ir.attribute_schema.invalid_range",
        "operation attribute integer ranges require an integer field"
      );
    }
    if (!field.allowed_enum_values.empty() && field.kind != operation_attribute_kind::enum_key) {
      return core::fail<operation_attribute_schema>(
        "ir.attribute_schema.invalid_enum", "allowed enum values require an enum field"
      );
    }
    for (std::size_t enum_index = 0; enum_index < field.allowed_enum_values.size(); ++enum_index) {
      const auto& enum_value = field.allowed_enum_values[enum_index];
      if (enum_value.key().empty()) {
        return core::fail<operation_attribute_schema>(
          "ir.attribute_schema.invalid_enum", "allowed enum values must not be empty"
        );
      }
      if (std::ranges::find(
            field.allowed_enum_values.begin(),
            field.allowed_enum_values.begin() + static_cast<std::ptrdiff_t>(enum_index),
            enum_value
          ) != field.allowed_enum_values.begin() + static_cast<std::ptrdiff_t>(enum_index)) {
        return core::fail<operation_attribute_schema>(
          "ir.attribute_schema.invalid_enum", "allowed enum values must be unique"
        );
      }
    }
    if (field.default_value.has_value() && !value_is_valid_for_field(field, *field.default_value)) {
      return core::fail<operation_attribute_schema>(
        "ir.attribute_schema.invalid_default",
        "operation attribute default does not satisfy its field constraints"
      );
    }
  }
  return operation_attribute_schema(std::move(normalized));
}

core::result_t<operation_attributes> operation_attribute_schema::validate(
  const operation_attributes& attributes
) const {
  std::vector<operation_attribute> normalized(
    attributes.entries().begin(), attributes.entries().end()
  );

  for (const auto& attribute : attributes.entries()) {
    const auto field =
      std::ranges::lower_bound(m_fields, attribute.key, {}, &operation_attribute_field::key);
    if (field == m_fields.end() || field->key != attribute.key) {
      return core::fail<operation_attributes>(
        "ir.attribute.unknown", "operation attribute is not declared by the operation schema"
      );
    }
    if (kind_of(attribute.value) != field->kind) {
      return core::fail<operation_attributes>(
        "ir.attribute.kind", "operation attribute value has the wrong kind"
      );
    }
    if (!value_is_valid_for_field(*field, attribute.value)) {
      if (std::holds_alternative<std::int64_t>(attribute.value)) {
        return core::fail<operation_attributes>(
          "ir.attribute.range", "operation attribute integer is outside the allowed range"
        );
      }
      if (std::holds_alternative<operation_enum_value>(attribute.value)) {
        return core::fail<operation_attributes>(
          "ir.attribute.enum", "operation attribute enum value is not allowed"
        );
      }
      return core::fail<operation_attributes>(
        "ir.attribute.value", "operation attribute value is invalid"
      );
    }
  }

  for (const auto& field : m_fields) {
    if (attributes.find(field.key) != nullptr) {
      continue;
    }
    if (field.default_value.has_value()) {
      normalized.push_back(
        operation_attribute{
          .key = field.key,
          .value = *field.default_value,
        }
      );
      continue;
    }
    if (field.required) {
      return core::fail<operation_attributes>(
        "ir.attribute.required", "required operation attribute is missing"
      );
    }
  }

  return operation_attributes::create(normalized);
}

std::span<const operation_attribute_field> operation_attribute_schema::fields() const {
  return m_fields;
}

bool operation_attribute_schema::empty() const {
  return m_fields.empty();
}

operation_attribute_schema::operation_attribute_schema(
  std::vector<operation_attribute_field> fields
)
    : m_fields(std::move(fields)) {
}

} // namespace sivra::ir

std::size_t std::hash<sivra::ir::operation_enum_value>::operator()(
  const sivra::ir::operation_enum_value& value
) const noexcept {
  return std::hash<std::string_view>{}(value.key());
}

std::size_t std::hash<sivra::ir::operation_attributes>::operator()(
  const sivra::ir::operation_attributes& value
) const noexcept {
  std::size_t seed = 0;
  for (const auto& attribute : value.entries()) {
    sivra::core::hash_combine(seed, attribute.key);
    hash_attribute_value(seed, attribute.value);
  }
  return seed;
}
