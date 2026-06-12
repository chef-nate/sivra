#include <sivra/program/operand.hpp>

namespace sivra::program {

std::uint32_t bit_range::end() const {
  return offset + width;
}

bool bit_range::contains(
  bit_range other
) const {
  return width != 0 && other.width != 0 && offset <= other.offset && end() >= other.end();
}

bool bit_range::overlaps(
  bit_range other
) const {
  return width != 0 && other.width != 0 && offset < other.end() && other.offset < end();
}

core::result_t<void> bit_range::validate() const {
  if (width == 0) {
    return core::fail<void>("program.bit_range.empty", "bit range width must be non-zero");
  }
  if (end() < offset) {
    return core::fail<void>("program.bit_range.overflow", "bit range end overflows");
  }
  return {};
}

} // namespace sivra::program
