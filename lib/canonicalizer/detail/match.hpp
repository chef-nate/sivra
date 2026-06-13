#pragma once

#include <sivra/canonicalizer/rewrite.hpp>

#include <cstdint>
#include <optional>

namespace sivra::canonicalizer::detail {

bool constant_operand_matches(
  const rewrite_subject& subject,
  const ir::expression_node& child,
  const ir::operation_constant& expected
);

bool operand_matches(
  const rewrite_subject& subject,
  rewrite_context& context,
  std::size_t index,
  const std::optional<ir::operation_constant>& expected
);

[[nodiscard]] core::result_t<ir::constant_value> integer_constant(
  const ir::value_type& type,
  std::int32_t value
);

[[nodiscard]] core::result_t<ir::node_id> make_integer_node(
  rewrite_context& context,
  const ir::value_type& type,
  std::int32_t value
);

} // namespace sivra::canonicalizer::detail
