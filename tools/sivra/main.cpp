#include "examples.hpp"

#include <raw_expression_json.hpp>

#include <sivra/canonicalizer/engine.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/leaf.hpp>
#include <sivra/ir/value_type.hpp>

#include <CLI/CLI.hpp>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <print>
#include <span>
#include <stdexcept>
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

std::string format_memory_operand(
  const sivra::compat::raw_memory_operand& value
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

std::string format_value_type(
  const sivra::ir::value_type& type
) {
  if (type.kind() == sivra::ir::value_type_kind::unknown) {
    return "unknown";
  }
  auto scalar =
    std::string(format_scalar_category(type.category())) + std::to_string(type.element_bit_width());
  if (type.kind() == sivra::ir::value_type_kind::vector) {
    return "vec<" + scalar + ", " + std::to_string(type.lane_count()) + ">";
  }
  return scalar;
}

std::string format_constant(
  const sivra::ir::constant_value& value
) {
  const auto format_element = [](const sivra::ir::scalar_constant_t& element) {
    return std::visit(
      []<typename T>(const T& scalar) { return std::to_string(scalar.value()); }, element
    );
  };

  if (value.result_type().kind() == sivra::ir::value_type_kind::scalar) {
    return format_element(value.element(0));
  }
  const auto type = format_value_type(value.result_type());
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

std::string format_expression(
  const sivra::ir::expression_graph& graph,
  sivra::ir::node_id root,
  std::span<const sivra::compat::raw_memory_operand> external_values
) {
  const auto& node = graph.at(root);
  if (const auto* constant = node.get_if_constant()) {
    return format_constant(constant->value);
  }
  if (const auto* symbol = node.get_if_symbol()) {
    return std::string(graph.symbol_name(symbol->symbol));
  }
  if (const auto* external = node.get_if_external_value()) {
    if (external->value.index() >= external_values.size()) {
      throw std::out_of_range("external value metadata is missing");
    }
    return format_memory_operand(external_values[external->value.index()]);
  }
  if (const auto* unknown = node.get_if_unknown()) {
    return "unknown(" + unknown->reason + ")";
  }
  if (const auto* merge = node.get_if_merge()) {
    auto formatted = std::string("merge(");
    bool first = true;
    for (const auto incoming : merge->incoming) {
      if (!first) {
        formatted += ", ";
      }
      first = false;
      formatted += format_expression(graph, incoming, external_values);
    }
    formatted += ")";
    return formatted;
  }

  const auto& application = std::get<sivra::ir::operation_application>(node.payload());
  auto formatted = display_name(graph.catalogue().operation(application.operation).name()) + "(";
  bool first = true;
  for (const auto child : application.operands) {
    if (!first) {
      formatted += ", ";
    }
    first = false;
    formatted += format_expression(graph, child, external_values);
  }
  formatted += ")";
  return formatted;
}

std::string read_text_file(
  const std::filesystem::path& path
) {
  std::ifstream stream(path);
  if (!stream) {
    throw std::runtime_error("failed to open fixture: " + path.string());
  }
  return {
    std::istreambuf_iterator<char>(stream),
    std::istreambuf_iterator<char>(),
  };
}

std::string diagnostic_message(
  const sivra::core::diagnostic_bundle_t& diagnostics
) {
  if (diagnostics.empty()) {
    return "unknown analysis failure";
  }
  return diagnostics.front().message;
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
      const auto path =
        std::filesystem::path(sivra::tool::fixture_directory()) / expression.file_name;
      const auto json = read_text_file(path);
      const auto loaded = sivra::compat::parse_raw_expression_json(json);
      if (!loaded.has_value()) {
        throw std::runtime_error(diagnostic_message(loaded.error()));
      }
      const sivra::canonicalizer::engine canonicalizer;
      const auto canonicalized = canonicalizer.canonicalize(loaded->graph, loaded->root);
      if (!canonicalized.root.has_value()) {
        throw std::runtime_error(diagnostic_message(canonicalized.diagnostics));
      }
      std::println(
        "{}: {}",
        expression.output,
        format_expression(canonicalized.graph, *canonicalized.root, loaded->external_values)
      );
    }
  } catch (const std::exception& error) {
    std::println(stderr, "error: {}", error.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
