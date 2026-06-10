#include <sivra/ir/operation.hpp>

#include <utility>

namespace sivra::ir {

operation_def::operation_def(
  operation_id id,
  std::string key,
  std::string name,
  operation_signature signature,
  operation_semantics semantics
)
    : m_id(id),
      m_key(std::move(key)),
      m_name(std::move(name)),
      m_signature(signature),
      m_semantics(std::move(semantics)) {
}

operation_id operation_def::id() const {
  return m_id;
}

std::string_view operation_def::key() const {
  return m_key;
}

std::string_view operation_def::name() const {
  return m_name;
}

const operation_signature& operation_def::signature() const {
  return m_signature;
}

const operation_semantics& operation_def::semantics() const {
  return m_semantics;
}

bool operation_def::has_trait(
  operation_trait trait
) const {
  return (m_semantics.traits & trait) == trait;
}

} // namespace sivra::ir
