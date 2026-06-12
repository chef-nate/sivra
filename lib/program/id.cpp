#include <sivra/program/id.hpp>

#include <utility>

namespace sivra::program {

architecture_id::architecture_id(
  std::string value
)
    : m_value(std::move(value)) {
}

architecture_id::architecture_id(
  const char* value
)
    : m_value(value) {
}

std::string_view architecture_id::value() const {
  return m_value;
}

bool architecture_id::empty() const {
  return m_value.empty();
}

architecture_profile_id::architecture_profile_id(
  std::string value
)
    : m_value(std::move(value)) {
}

architecture_profile_id::architecture_profile_id(
  const char* value
)
    : m_value(value) {
}

std::string_view architecture_profile_id::value() const {
  return m_value;
}

bool architecture_profile_id::empty() const {
  return m_value.empty();
}

} // namespace sivra::program
