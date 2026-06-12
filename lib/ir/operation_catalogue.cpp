#include <sivra/ir/operation_catalogue.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace {

class compatibility_encoder {
public:
  compatibility_encoder() : m_value("sivra.ir.catalogue.v1;") {}

  void append_string(
    std::string_view value
  ) {
    append_integer(value.size());
    m_value.append(value);
    m_value.push_back(';');
  }

  template <typename Integer>
  void append_integer(
    Integer value
  ) {
    std::array<char, std::numeric_limits<Integer>::digits10 + 4> buffer{};
    const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (error != std::errc{}) {
      throw std::logic_error("failed to encode catalogue compatibility descriptor");
    }
    m_value.append(buffer.data(), end);
    m_value.push_back(';');
  }

  [[nodiscard]] std::string finish() && { return std::move(m_value); }

private:
  std::string m_value;
};

void append_type(
  compatibility_encoder& encoder,
  const sivra::ir::value_type& type
) {
  encoder.append_integer(static_cast<std::uint8_t>(type.kind()));
  encoder.append_integer(static_cast<std::uint8_t>(type.category()));
  encoder.append_integer(type.element_bit_width());
  encoder.append_integer(type.lane_count());
}

void append_scalar_constant(
  compatibility_encoder& encoder,
  const sivra::ir::scalar_constant_t& value
) {
  encoder.append_integer(value.index());
  std::visit([&encoder](const auto& constant) { encoder.append_integer(constant.bits); }, value);
}

void append_attribute_value(
  compatibility_encoder& encoder,
  const sivra::ir::operation_attribute_value& value
) {
  encoder.append_integer(value.index());
  std::visit(
    [&encoder](const auto& entry) {
      using entry_t = std::remove_cvref_t<decltype(entry)>;
      if constexpr (std::is_same_v<entry_t, std::int64_t>) {
        encoder.append_integer(entry);
      } else if constexpr (std::is_same_v<entry_t, bool>) {
        encoder.append_integer(static_cast<std::uint8_t>(entry));
      } else if constexpr (std::is_same_v<entry_t, sivra::ir::operation_enum_value>) {
        encoder.append_string(entry.key());
      } else if constexpr (std::is_same_v<entry_t, std::vector<std::uint32_t>>) {
        encoder.append_integer(entry.size());
        for (const auto index : entry) {
          encoder.append_integer(index);
        }
      } else {
        append_type(encoder, entry);
      }
    },
    value
  );
}

void append_operation_constant(
  compatibility_encoder& encoder,
  const std::optional<sivra::ir::operation_constant>& value
) {
  encoder.append_integer(static_cast<std::uint8_t>(value.has_value()));
  if (!value.has_value()) {
    return;
  }
  encoder.append_integer(value->element.index());
  std::visit(
    [&encoder](const auto& element) {
      using element_t = std::remove_cvref_t<decltype(element)>;
      if constexpr (std::is_same_v<element_t, sivra::ir::well_known_constant>) {
        encoder.append_integer(static_cast<std::uint8_t>(element));
      } else {
        append_scalar_constant(encoder, element);
      }
    },
    value->element
  );
}

std::string key_index_value(
  const sivra::ir::operation_key& key
) {
  compatibility_encoder encoder;
  encoder.append_string(key.value());
  encoder.append_integer(key.version());
  return std::move(encoder).finish();
}

std::string compatibility_value(
  std::span<const sivra::ir::operation_def> definitions
) {
  std::vector<const sivra::ir::operation_def*> ordered;
  ordered.reserve(definitions.size());
  for (const auto& definition : definitions) {
    ordered.push_back(&definition);
  }
  std::ranges::sort(ordered, [](const auto* lhs, const auto* rhs) {
    return lhs->stable_key() < rhs->stable_key();
  });

  compatibility_encoder encoder;
  encoder.append_integer(ordered.size());
  for (const auto* definition : ordered) {
    encoder.append_string(definition->key());
    encoder.append_integer(definition->stable_key().version());
    encoder.append_integer(definition->signature().arity.minimum);
    encoder.append_integer(
      definition->signature().arity.maximum.value_or(std::numeric_limits<std::uint32_t>::max())
    );
    encoder.append_integer(static_cast<std::uint8_t>(definition->signature().operand_types));
    encoder.append_integer(definition->attribute_schema().fields().size());
    for (const auto& field : definition->attribute_schema().fields()) {
      encoder.append_string(field.key);
      encoder.append_integer(static_cast<std::uint8_t>(field.kind));
      encoder.append_integer(static_cast<std::uint8_t>(field.required));
      encoder.append_integer(static_cast<std::uint8_t>(field.default_value.has_value()));
      if (field.default_value.has_value()) {
        append_attribute_value(encoder, *field.default_value);
      }
      encoder.append_integer(static_cast<std::uint8_t>(field.minimum_integer.has_value()));
      if (field.minimum_integer.has_value()) {
        encoder.append_integer(*field.minimum_integer);
      }
      encoder.append_integer(static_cast<std::uint8_t>(field.maximum_integer.has_value()));
      if (field.maximum_integer.has_value()) {
        encoder.append_integer(*field.maximum_integer);
      }
      encoder.append_integer(field.allowed_enum_values.size());
      for (const auto& enum_value : field.allowed_enum_values) {
        encoder.append_string(enum_value.key());
      }
    }
    encoder.append_integer(static_cast<std::uint32_t>(definition->semantics().traits));
    append_operation_constant(encoder, definition->semantics().identity);
    append_operation_constant(encoder, definition->semantics().annihilator);
  }
  return std::move(encoder).finish();
}

} // namespace

namespace sivra::ir {

catalogue_compatibility_id::catalogue_compatibility_id(
  std::string value
)
    : m_value(std::move(value)) {
}

std::string_view catalogue_compatibility_id::value() const {
  return m_value;
}

operation_catalogue::operation_catalogue(
  core::owner_token owner,
  catalogue_compatibility_id compatibility_id,
  std::vector<operation_def> definitions,
  std::unordered_map<
    std::string,
    operation_id
  > by_key,
  std::unordered_map<
    std::string,
    operation_id
  > by_name
)
    : m_owner(owner),
      m_compatibility_id(std::move(compatibility_id)),
      m_definitions(std::move(definitions)),
      m_by_key(std::move(by_key)),
      m_by_name(std::move(by_name)) {
}

const operation_def& operation_catalogue::operation(
  operation_id id
) const {
  if (id.owner() != m_owner) {
    throw std::invalid_argument("operation_id belongs to another catalogue");
  }
  return m_definitions.at(id.index());
}

const operation_def& operation_catalogue::at(
  operation_id id
) const {
  return operation(id);
}

const operation_def& operation_catalogue::at(
  std::string_view key
) const {
  const auto found = m_by_key.find(key_index_value(operation_key(std::string(key))));
  if (found == m_by_key.end()) {
    throw std::out_of_range("operation not registered");
  }
  return operation(found->second);
}

const operation_def& operation_catalogue::named(
  std::string_view name
) const {
  const auto found = m_by_name.find(std::string(name));
  if (found == m_by_name.end()) {
    throw std::out_of_range("operation name not registered");
  }
  return operation(found->second);
}

const operation_def* operation_catalogue::find(
  const operation_key& key
) const {
  const auto found = m_by_key.find(key_index_value(key));
  if (found == m_by_key.end()) {
    return nullptr;
  }
  return &operation(found->second);
}

const operation_def* operation_catalogue::find_by_name(
  std::string_view name
) const {
  const auto found = m_by_name.find(std::string(name));
  return found == m_by_name.end() ? nullptr : &operation(found->second);
}

bool operation_catalogue::contains(
  std::string_view key
) const {
  return m_by_key.contains(key_index_value(operation_key(std::string(key))));
}

std::span<const operation_def> operation_catalogue::operations() const {
  return m_definitions;
}

const catalogue_compatibility_id& operation_catalogue::compatibility_id() const {
  return m_compatibility_id;
}

core::owner_token operation_catalogue::owner() const {
  return m_owner;
}

operation_catalogue_builder::operation_catalogue_builder()
    : m_owner(core::owner_token_source::next()) {
}

core::result_t<std::vector<operation_id>> operation_catalogue_builder::register_operations(
  std::span<const operation_registration> registrations
) {
  if (m_frozen) {
    return core::fail<std::vector<operation_id>>(
      "ir.catalogue.frozen", "operation catalogue builder is frozen"
    );
  }

  std::unordered_set<std::string> batch_keys;
  std::unordered_set<std::string_view> batch_names;
  for (const auto& registration : registrations) {
    if (registration.key.empty() || registration.key.version() == 0 || registration.name.empty()) {
      return core::fail<std::vector<operation_id>>(
        "ir.catalogue.invalid_operation", "operation key and name must not be empty"
      );
    }
    const auto indexed_key = key_index_value(registration.key);
    if (m_by_key.contains(indexed_key) || !batch_keys.emplace(indexed_key).second) {
      return core::fail<std::vector<operation_id>>(
        "ir.catalogue.duplicate_operation",
        "operation key already registered: " + std::string(registration.key.value())
      );
    }
    if (m_by_name.contains(registration.name) || !batch_names.emplace(registration.name).second) {
      return core::fail<std::vector<operation_id>>(
        "ir.catalogue.duplicate_name", "operation name already registered: " + registration.name
      );
    }
    if (auto validated = registration.signature.validate_definition(); !validated.has_value()) {
      auto diagnostics = std::move(validated.error());
      for (auto& diagnostic : diagnostics) {
        diagnostic.code = core::diagnostic_code("ir.catalogue.invalid_signature");
      }
      return std::unexpected(std::move(diagnostics));
    }
  }

  const auto maximum = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
  if (m_definitions.size() > maximum ||
      (!registrations.empty() && registrations.size() - 1 > maximum - m_definitions.size())) {
    return core::fail<std::vector<operation_id>>(
      "ir.catalogue.capacity", "operation catalogue identifier capacity exceeded"
    );
  }

  auto definitions = m_definitions;
  auto by_key = m_by_key;
  auto by_name = m_by_name;
  std::vector<operation_id> identifiers;
  identifiers.reserve(registrations.size());

  for (const auto& registration : registrations) {
    const auto id =
      operation_id::unsafe_from_index(static_cast<std::uint32_t>(definitions.size()), m_owner);
    definitions.emplace_back(
      id,
      registration.key,
      registration.name,
      registration.signature,
      registration.attribute_schema,
      registration.semantics
    );
    by_key.emplace(key_index_value(registration.key), id);
    by_name.emplace(registration.name, id);
    identifiers.push_back(id);
  }

  m_definitions.swap(definitions);
  m_by_key.swap(by_key);
  m_by_name.swap(by_name);
  return identifiers;
}

core::result_t<operation_id> operation_catalogue_builder::register_operation(
  operation_registration registration
) {
  const std::array registrations{std::move(registration)};
  auto result = register_operations(registrations);
  if (!result.has_value()) {
    return std::unexpected(std::move(result.error()));
  }
  return result->front();
}

core::result_t<void> operation_catalogue_builder::validate() const {
  if (m_frozen) {
    return core::fail<void>("ir.catalogue.frozen", "operation catalogue builder is frozen");
  }
  for (const auto& definition : m_definitions) {
    if (auto validated = definition.signature().validate_definition(); !validated.has_value()) {
      return std::unexpected(std::move(validated.error()));
    }
  }
  return {};
}

core::result_t<std::shared_ptr<const operation_catalogue>>
operation_catalogue_builder::freeze() && {
  if (m_frozen) {
    return core::fail<std::shared_ptr<const operation_catalogue>>(
      "ir.catalogue.frozen", "operation catalogue builder is frozen"
    );
  }
  if (auto validated = validate(); !validated.has_value()) {
    return std::unexpected(std::move(validated.error()));
  }
  m_frozen = true;
  return std::shared_ptr<const operation_catalogue>(new operation_catalogue(
    m_owner,
    catalogue_compatibility_id(compatibility_value(m_definitions)),
    std::move(m_definitions),
    std::move(m_by_key),
    std::move(m_by_name)
  ));
}

core::owner_token operation_catalogue_builder::owner() const {
  return m_owner;
}

} // namespace sivra::ir
