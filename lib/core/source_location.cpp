#include <sivra/core/source_location.hpp>

namespace sivra::core {

bool source_span::is_valid() const {
  return source.is_valid() && begin.byte_offset <= end.byte_offset;
}

bool source_span::contains(
  source_position position
) const {
  return is_valid() && begin.byte_offset <= position.byte_offset &&
         position.byte_offset < end.byte_offset;
}

} // namespace sivra::core
