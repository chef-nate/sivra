#include <sivra/ir/operation.hpp>

#include <utility>

namespace sivra::ir {

operation_key::operation_key(
  std::string value,
  std::uint32_t version
)
    : m_value(std::move(value)),
      m_version(version) {
}

operation_key::operation_key(
  const char* value,
  std::uint32_t version
)
    : m_value(value),
      m_version(version) {
}

std::string_view operation_key::value() const {
  return m_value;
}

std::uint32_t operation_key::version() const {
  return m_version;
}

bool operation_key::empty() const {
  return m_value.empty();
}

operation_def::operation_def(
  operation_id id,
  operation_key key,
  std::string name,
  operation_signature signature,
  operation_attribute_schema attribute_schema,
  operation_semantics semantics
)
    : m_id(id),
      m_key(std::move(key)),
      m_name(std::move(name)),
      m_signature(std::move(signature)),
      m_attribute_schema(std::move(attribute_schema)),
      m_semantics(std::move(semantics)) {
}

operation_id operation_def::id() const {
  return m_id;
}

const operation_key& operation_def::stable_key() const {
  return m_key;
}

std::string_view operation_def::key() const {
  return m_key.value();
}

std::string_view operation_def::name() const {
  return m_name;
}

const operation_signature& operation_def::signature() const {
  return m_signature;
}

const operation_attribute_schema& operation_def::attribute_schema() const {
  return m_attribute_schema;
}

const operation_semantics& operation_def::semantics() const {
  return m_semantics;
}

const std::optional<operation_key>& operation_def::evaluator_key() const {
  return m_semantics.evaluator_key;
}

bool operation_def::has_trait(
  operation_trait trait
) const {
  return (m_semantics.traits & trait) == trait;
}

} // namespace sivra::ir
