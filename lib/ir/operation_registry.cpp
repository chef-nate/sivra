#include <sivra/ir/operation_registry.hpp>

#include <limits>
#include <stdexcept>
#include <utility>

namespace sivra::ir {

operation_id operation_registry::register_operation(
  std::string name,
  operation_semantics semantics
) {
  if (m_by_name.contains(name)) {
    throw std::invalid_argument("operation already registered");
  }

  // std::vector::size() may exceed uint32_t on 64-bit targets, so check before
  // narrowing the size into an operation_id.
  if (m_operations.size() > std::numeric_limits<std::uint32_t>::max()) {
    throw std::length_error("operation_registry operation_id limit exceeded");
  }

  const operation_id id(static_cast<std::uint32_t>(m_operations.size()));
  m_operations.emplace_back(id, name, std::move(semantics));
  m_by_name.emplace(std::move(name), id);
  return id;
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
