#include <sivra/ir/operation_registry.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace sivra::ir {

std::vector<operation_id> operation_registry::register_operations(
  std::span<const operation_registration> registrations
) {
  if (registrations.empty()) {
    return {};
  }

  std::unordered_set<std::string_view> batch_names;
  batch_names.reserve(registrations.size());

  for (const auto& registration : registrations) {
    if (m_by_name.contains(registration.name) || !batch_names.emplace(registration.name).second) {
      throw std::invalid_argument("operation already registered: " + registration.name);
    }
  }

  const auto maximum_id = static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());

  if (m_operations.size() > maximum_id ||
      registrations.size() - 1 > maximum_id - m_operations.size()) {
    throw std::length_error("operation_registry operation_id limit exceeded");
  }

  if (registrations.size() > std::numeric_limits<std::size_t>::max() - m_operations.size()) {
    throw std::length_error("operation_registry size limit exceeded");
  }

  const auto combined_size = m_operations.size() + registrations.size();
  auto staged_operations = m_operations;
  auto staged_by_name = m_by_name;

  staged_operations.reserve(combined_size);
  staged_by_name.reserve(combined_size);

  std::vector<operation_id> identifiers;
  identifiers.reserve(registrations.size());

  for (const auto& registration : registrations) {
    const operation_id id(static_cast<std::uint32_t>(staged_operations.size()));

    staged_operations.emplace_back(id, registration.name, registration.semantics);
    staged_by_name.emplace(registration.name, id);
    identifiers.push_back(id);
  }

  m_operations.swap(staged_operations);
  m_by_name.swap(staged_by_name);
  return identifiers;
}

operation_id operation_registry::register_operation(
  std::string name,
  operation_semantics semantics
) {
  const std::array registrations{
    operation_registration{
      .name = std::move(name),
      .semantics = std::move(semantics),
    },
  };

  return register_operations(registrations).front();
}

operation_id operation_registry::register_operation(
  std::string name,
  operation_trait traits
) {
  return register_operation(std::move(name), operation_semantics{.traits = traits});
}

const operation_def& operation_registry::at(
  operation_id id
) const {
  return m_operations.at(id.value());
}

const operation_def& operation_registry::at(
  std::string_view name
) const {
  const auto found = m_by_name.find(std::string(name));
  if (found == m_by_name.end()) {
    throw std::out_of_range("operation not registered");
  }

  return at(found->second);
}

bool operation_registry::contains(
  std::string_view name
) const {
  return m_by_name.contains(std::string(name));
}

std::span<const operation_def> operation_registry::operations() const {
  return m_operations;
}

} // namespace sivra::ir
