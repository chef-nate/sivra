#include <sivra/ir/id.hpp>

namespace sivra::ir {

operation_id::operation_id(
  std::uint32_t value
)
    : m_value(value) {
}

std::uint32_t operation_id::value() const {
  return m_value;
}

bool operation_id::operator==(
  const operation_id& other
) const {
  return m_value == other.m_value;
}

bool operation_id::operator<(
  const operation_id& other
) const {
  return m_value < other.m_value;
}

node_id::node_id(
  std::uint32_t value
)
    : m_value(value) {
}

std::uint32_t node_id::value() const {
  return m_value;
}

bool node_id::operator==(
  const node_id& other
) const {
  return m_value == other.m_value;
}

bool node_id::operator<(
  const node_id& other
) const {
  return m_value < other.m_value;
}

} // namespace sivra::ir
