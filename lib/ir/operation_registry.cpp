#include <sivra/ir/operation_registry.hpp>

#include <stdexcept>
#include <utility>

namespace sivra {

operation_id operation_registry::register_operation(
  std::string name,
  operation_trait traits
) {
  if (m_by_name.contains(name)) {
    throw std::invalid_argument("operation already registered");
  }

  const operation_id id(static_cast<std::uint32_t>(m_operations.size()));
  m_operations.emplace_back(id, name, traits);
  m_by_name.emplace(std::move(name), id);
  return id;
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

} // namespace sivra
