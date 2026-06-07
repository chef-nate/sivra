#include <sivra/ir/type.hpp>

namespace sivra {

type::type(
  scalar_type scalar
)
    : m_scalar(scalar) {
}

scalar_type type::scalar() const {
  return m_scalar;
}

bool type::operator==(
  const type& other
) const {
  return m_scalar == other.m_scalar;
}

} // namespace sivra
