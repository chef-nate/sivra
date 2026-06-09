#include "embedded_examples.hpp"
#include "raw_expression_json.hpp"

#include <sivra/canonicalizer/engine.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/leaf.hpp>
#include <sivra/ir/operation_registry.hpp>
#include <sivra/ir/scalar_type.hpp>
#include <sivra/ir/type.hpp>

#include <CLI/CLI.hpp>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <print>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace {

std::string display_name(
  std::string_view operation
) {
  if (operation == "multiply") {
    return "mul";
  }

  return std::string(operation);
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
    const auto format_scalar_type = [](sivra::ir::scalar_type type) {
      switch (type) {
      case sivra::ir::scalar_type::f32:
        return std::string_view("f32");
      case sivra::ir::scalar_type::i32:
        return std::string_view("i32");
      case sivra::ir::scalar_type::unknown:
        return std::string_view("unknown");
      }

      return std::string_view("unknown");
    };

    const auto format_element = [](const sivra::ir::scalar_constant_t& element) {
      return std::visit(
        []<typename T>(const T& scalar) { return std::to_string(scalar.value()); }, element
      );
    };

    if (value.result_type().kind() == sivra::ir::type_kind::scalar) {
      return format_element(value.element(0));
    }

    const auto format_aggregate_type = [&](const sivra::ir::type& type) {
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
        return std::string();
      }

      return std::string();
    };

    const auto type = format_aggregate_type(value.result_type());
    if (value.is_splat()) {
      return type + "(" + format_element(value.element(0)) + ")";
    }

    auto formatted = type + "{";
    for (std::size_t index = 0; index < value.element_count(); ++index) {
      if (index != 0) {
        formatted += ", ";
      }
      formatted += format_element(value.element(index));
    }
    formatted += "}";
    return formatted;
  }

  std::string operator()(
    const sivra::ir::symbol_ref& value
  ) const {
    return value.name;
  }
};

std::string format_expression(
  const sivra::ir::expression_graph& graph,
  sivra::ir::node_id root
) {
  const auto& node = graph.at(root);
  if (node.leaf_value().has_value()) {
    return std::visit(leaf_formatter{}, *node.leaf_value());
  }

  auto formatted = display_name(graph.context().operations().at(node.operation()).name()) + "(";
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

} // namespace

int main(
  int argc,
  char* argv[]
) {
  CLI::App app{"SIVRA"};
  std::string example;
  std::vector<std::string> example_choices;
  for (const auto name : sivra::tool::example_names()) {
    example_choices.emplace_back(name);
  }

  app.add_option("example", example, "Embedded example")
    ->required()
    ->check(CLI::IsMember(example_choices));
  CLI11_PARSE(app, argc, argv);

  try {
    for (const auto& expression : sivra::tool::example_expressions(example)) {
      const auto loaded = sivra::tool::parse_raw_expression_json(expression.json);
      const sivra::canonicalizer::engine canonicalizer;
      const auto canonicalized = canonicalizer.canonicalize(loaded.graph, loaded.root);
      std::println(
        "{}: {}", expression.output, format_expression(canonicalized.graph, canonicalized.root)
      );
    }
  } catch (const std::exception& error) {
    std::println(stderr, "error: {}", error.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
