#include "expression_format.hpp"

#include <sivra/ir/constant.hpp>
#include <sivra/ir/leaf.hpp>

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace {

std::string format_f32(
  float value
) {
  std::ostringstream stream;
  stream << std::setprecision(15) << value;
  return stream.str();
}

std::string format_scalar_constant(
  const sivra::ir::scalar_constant_t& value
) {
  return std::visit(
    []<typename T>(const T& scalar) -> std::string {
      if constexpr (std::is_same_v<T, sivra::ir::f32_constant>) {
        return format_f32(scalar.value());
      } else {
        return std::to_string(scalar.value());
      }
    },
    value
  );
}

std::string_view format_scalar_category(
  sivra::ir::scalar_category category
) {
  switch (category) {
  case sivra::ir::scalar_category::floating_point:
    return "f";
  case sivra::ir::scalar_category::signed_integer:
    return "i";
  case sivra::ir::scalar_category::unsigned_integer:
    return "u";
  case sivra::ir::scalar_category::unknown:
    return "unknown";
  }

  return "unknown";
}

std::string format_aggregate_type(
  const sivra::ir::value_type& type
) {
  switch (type.kind()) {
  case sivra::ir::value_type_kind::vector:
    return "vec<" + std::string(format_scalar_category(type.category())) +
           std::to_string(type.element_bit_width()) + ", " + std::to_string(type.lane_count()) +
           ">";
  case sivra::ir::value_type_kind::unknown:
  case sivra::ir::value_type_kind::scalar:
    return {};
  }

  return {};
}

std::string format_constant(
  const sivra::ir::constant_value& value
) {
  if (value.result_type().kind() == sivra::ir::value_type_kind::scalar) {
    return format_scalar_constant(value.element(0));
  }

  const auto type = format_aggregate_type(value.result_type());
  if (value.is_splat()) {
    return type + "(" + format_scalar_constant(value.element(0)) + ")";
  }

  auto formatted = type + "{";
  for (std::size_t index = 0; index < value.element_count(); ++index) {
    if (index != 0) {
      formatted += ", ";
    }
    formatted += format_scalar_constant(value.element(index));
  }
  formatted += "}";
  return formatted;
}

} // namespace

namespace sivra::test_support {

std::string format_expression(
  const ir::expression_graph& graph,
  ir::node_id root
) {
  const auto& node = graph.at(root);
  if (const auto* constant = node.get_if_constant()) {
    return format_constant(constant->value);
  }
  if (const auto* symbol = node.get_if_symbol()) {
    return symbol->name;
  }
  if (const auto* external = node.get_if_external_value()) {
    return "external" + std::to_string(external->value.index());
  }
  if (const auto* unknown = node.get_if_unknown()) {
    return "unknown(" + unknown->reason + ")";
  }

  const auto& application = std::get<ir::operation_application>(node.payload());
  auto formatted = std::string(graph.catalogue().operation(application.operation).name()) + "(";
  bool first = true;
  for (const auto child : application.operands) {
    if (!first) {
      formatted += ", ";
    }
    first = false;
    formatted += format_expression(graph, child);
  }
  formatted += ")";
  return formatted;
}

} // namespace sivra::test_support
