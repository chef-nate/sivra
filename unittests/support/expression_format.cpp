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

std::string_view format_scalar_type(
  sivra::ir::scalar_type type
) {
  switch (type) {
  case sivra::ir::scalar_type::f32:
    return "f32";
  case sivra::ir::scalar_type::i32:
    return "i32";
  case sivra::ir::scalar_type::unknown:
    return "unknown";
  }

  return "unknown";
}

std::string format_aggregate_type(
  const sivra::ir::type& type
) {
  switch (type.kind()) {
  case sivra::ir::type_kind::vector: {
    const auto& vector = static_cast<const sivra::ir::vector_type_def&>(type);
    const auto& element = static_cast<const sivra::ir::scalar_type_def&>(vector.element_type());
    return "vec<" + std::string(format_scalar_type(element.scalar())) + ", " +
           std::to_string(vector.elements()) + ">";
  }

  case sivra::ir::type_kind::matrix: {
    const auto& matrix = static_cast<const sivra::ir::matrix_type_def&>(type);
    const auto& element = static_cast<const sivra::ir::scalar_type_def&>(matrix.element_type());
    return "matrix<" + std::string(format_scalar_type(element.scalar())) + ", " +
           std::to_string(matrix.rows()) + "x" + std::to_string(matrix.columns()) + ">";
  }

  case sivra::ir::type_kind::unknown:
  case sivra::ir::type_kind::scalar:
    return {};
  }

  return {};
}

std::string format_constant(
  const sivra::ir::constant_value& value
) {
  if (value.result_type().kind() == sivra::ir::type_kind::scalar) {
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

std::string format_memory_ref(
  const sivra::ir::memory_ref& value
) {
  auto formatted = std::string("mem[") + value.base_register;
  if (value.offset > 0) {
    formatted += "+" + std::to_string(value.offset);
  } else if (value.offset < 0) {
    formatted += std::to_string(value.offset);
  }
  formatted += "]";
  return formatted;
}

struct leaf_formatter {
  std::string operator()(
    const sivra::ir::memory_ref& value
  ) const {
    return format_memory_ref(value);
  }

  std::string operator()(
    const sivra::ir::constant_value& value
  ) const {
    return format_constant(value);
  }

  std::string operator()(
    const sivra::ir::symbol_ref& value
  ) const {
    return value.name;
  }
};

} // namespace

namespace sivra::test_support {

std::string format_expression(
  const ir::expression_graph& graph,
  ir::node_id root
) {
  const auto& node = graph.at(root);
  if (node.leaf_value().has_value()) {
    return std::visit(leaf_formatter{}, *node.leaf_value());
  }

  auto formatted = std::string(graph.context().operations().at(node.operation()).name()) + "(";
  bool first = true;
  for (const auto child : node.children()) {
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
