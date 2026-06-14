#include <sivra/recovery/memory.hpp>

namespace sivra::recovery {

alias_relation conservative_memory_alias_analysis::relate(
  const program::memory_operand& lhs,
  const program::memory_operand& rhs
) const {
  if (lhs.base == rhs.base && lhs.displacement == rhs.displacement && lhs.width == rhs.width) {
    return alias_relation::must_alias;
  }
  if (lhs.base == rhs.base && lhs.base.has_value() && lhs.width != 0 && rhs.width != 0) {
    const auto lhs_begin = lhs.displacement;
    const auto lhs_end = lhs_begin + static_cast<std::int64_t>(lhs.width / 8U);
    const auto rhs_begin = rhs.displacement;
    const auto rhs_end = rhs_begin + static_cast<std::int64_t>(rhs.width / 8U);
    if (lhs_end <= rhs_begin || rhs_end <= lhs_begin) {
      return alias_relation::no_alias;
    }
  }
  return alias_relation::may_alias;
}

} // namespace sivra::recovery
