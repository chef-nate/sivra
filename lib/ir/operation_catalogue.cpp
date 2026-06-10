#include <sivra/ir/operation_catalogue.hpp>

#include <array>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace sivra::ir {

operation_catalogue::operation_catalogue(
  core::owner_token owner,
  std::vector<operation_def> definitions,
  std::unordered_map<
    std::string,
    operation_id
  > by_key
)
    : m_owner(owner),
      m_definitions(std::move(definitions)),
      m_by_key(std::move(by_key)) {
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
  const auto found = m_by_key.find(std::string(key));
  if (found == m_by_key.end()) {
    throw std::out_of_range("operation not registered");
  }
  return operation(found->second);
}

bool operation_catalogue::contains(
  std::string_view key
) const {
  return m_by_key.contains(std::string(key));
}

std::span<const operation_def> operation_catalogue::operations() const {
  return m_definitions;
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

  std::unordered_set<std::string_view> batch_keys;
  for (const auto& registration : registrations) {
    if (registration.key.empty() || registration.name.empty()) {
      return core::fail<std::vector<operation_id>>(
        "ir.catalogue.invalid_operation", "operation key and name must not be empty"
      );
    }
    if (m_by_key.contains(registration.key) || !batch_keys.emplace(registration.key).second) {
      return core::fail<std::vector<operation_id>>(
        "ir.catalogue.duplicate_operation", "operation key already registered: " + registration.key
      );
    }
    if (registration.signature.maximum_operands.has_value() &&
        *registration.signature.maximum_operands < registration.signature.minimum_operands) {
      return core::fail<std::vector<operation_id>>(
        "ir.catalogue.invalid_signature", "operation maximum arity is below minimum arity"
      );
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
  std::vector<operation_id> identifiers;
  identifiers.reserve(registrations.size());

  for (const auto& registration : registrations) {
    const auto id =
      operation_id::unsafe_from_index(static_cast<std::uint32_t>(definitions.size()), m_owner);
    definitions.emplace_back(
      id, registration.key, registration.name, registration.signature, registration.semantics
    );
    by_key.emplace(registration.key, id);
    identifiers.push_back(id);
  }

  m_definitions.swap(definitions);
  m_by_key.swap(by_key);
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

core::result_t<std::shared_ptr<const operation_catalogue>>
operation_catalogue_builder::freeze() && {
  if (m_frozen) {
    return core::fail<std::shared_ptr<const operation_catalogue>>(
      "ir.catalogue.frozen", "operation catalogue builder is frozen"
    );
  }
  m_frozen = true;
  return std::shared_ptr<const operation_catalogue>(
    new operation_catalogue(m_owner, std::move(m_definitions), std::move(m_by_key))
  );
}

core::owner_token operation_catalogue_builder::owner() const {
  return m_owner;
}

} // namespace sivra::ir
