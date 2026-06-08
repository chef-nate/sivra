#include <sivra/ir/operation.hpp>

#include <utility>

namespace sivra::ir {

operation_def::operation_def(
  operation_id id,
  std::string name,
  operation_semantics semantics
)
    : m_id(id),
      m_name(std::move(name)),
      m_semantics(std::move(semantics)) {
}

operation_id operation_def::id() const {
  return m_id;
}

std::string_view operation_def::name() const {
  return m_name;
}

const operation_semantics& operation_def::semantics() const {
  return m_semantics;
}

bool operation_def::has_trait(
  operation_trait trait
) const {
  const auto requested = static_cast<std::uint32_t>(trait);
  const auto available = static_cast<std::uint32_t>(m_semantics.traits);
  return (available & requested) == requested;
}

} // namespace sivra::ir
