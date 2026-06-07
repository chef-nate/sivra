#include <sivra/ir/operation.hpp>

#include <utility>

namespace sivra {

operation_def::operation_def(
  operation_id id,
  std::string name,
  operation_trait traits
)
    : m_id(id),
      m_name(std::move(name)),
      m_traits(traits) {
}

operation_id operation_def::id() const {
  return m_id;
}

std::string_view operation_def::name() const {
  return m_name;
}

bool operation_def::has_trait(
  operation_trait trait
) const {
  const auto requested = static_cast<std::uint32_t>(trait);
  const auto available = static_cast<std::uint32_t>(m_traits);
  return (available & requested) == requested;
}

} // namespace sivra
