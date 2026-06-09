#include <sivra/canonicalizer/engine.hpp>
#include <sivra/canonicalizer/options.hpp>
#include <sivra/canonicalizer/rule.hpp>
#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/ir_context.hpp>
#include <sivra/ir/leaf.hpp>
#include <sivra/ir/operation.hpp>
#include <sivra/ir/scalar_type.hpp>
#include <sivra/ir/type.hpp>

#include <doctest/doctest.h>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct associative_fixture {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph;
  sivra::ir::builtin_operation_ids builtins;
  sivra::ir::operation_id associative;
  sivra::ir::operation_id other_associative;
  sivra::ir::operation_id non_associative;

  associative_fixture()
      : graph(context),
        builtins(sivra::ir::register_builtin_operations(context.operations())),
        associative(context.operations().register_operation(
          "associative",
          sivra::ir::operation_trait::associative
        )),
        other_associative(context.operations().register_operation(
          "other_associative",
          sivra::ir::operation_trait::associative
        )),
        non_associative(context.operations().register_operation("non_associative")) {}

  const sivra::ir::scalar_type_def& f32() {
    return context.types().scalar(sivra::ir::scalar_type::f32);
  }

  const sivra::ir::scalar_type_def& i32() {
    return context.types().scalar(sivra::ir::scalar_type::i32);
  }

  sivra::ir::node_id add_symbol(
    const sivra::ir::type& result_type,
    std::string name
  ) {
    return graph.add_node(
      builtins.symbol, result_type, {}, sivra::ir::symbol_ref{.name = std::move(name)}
    );
  }

  sivra::ir::node_id add_operation(
    sivra::ir::operation_id operation,
    const sivra::ir::type& result_type,
    std::vector<sivra::ir::node_id> children,
    std::optional<sivra::ir::leaf_type_t> leaf_value = std::nullopt
  ) {
    return graph.add_node(operation, result_type, std::move(children), std::move(leaf_value));
  }
};

std::string_view symbol_name(
  const sivra::ir::expression_node& node
) {
  REQUIRE(node.leaf_value().has_value());
  const auto* symbol = std::get_if<sivra::ir::symbol_ref>(&*node.leaf_value());
  REQUIRE(symbol != nullptr);
  return symbol->name;
}

void check_symbol_children(
  const sivra::ir::expression_graph& graph,
  const sivra::ir::expression_node& node,
  const std::vector<std::string_view>& expected
) {
  REQUIRE(node.children().size() == expected.size());
  for (std::size_t index = 0; index < expected.size(); ++index) {
    CHECK(symbol_name(graph.at(node.children()[index])) == expected[index]);
  }
}

} // namespace

TEST_CASE(
  "associative flattening replaces a nested operation with its children"
) {
  associative_fixture fixture;
  const auto a = fixture.add_symbol(fixture.f32(), "a");
  const auto b = fixture.add_symbol(fixture.f32(), "b");
  const auto c = fixture.add_symbol(fixture.f32(), "c");
  const auto d = fixture.add_symbol(fixture.f32(), "d");
  const auto nested = fixture.add_operation(fixture.associative, fixture.f32(), {b, c});
  const auto root = fixture.add_operation(fixture.associative, fixture.f32(), {a, nested, d});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  CHECK(result_node.operation() == fixture.associative);
  check_symbol_children(result.graph, result_node, {"a", "b", "c", "d"});
}

TEST_CASE(
  "associative flattening handles deep and repeated nesting in operand order"
) {
  associative_fixture fixture;
  const auto a = fixture.add_symbol(fixture.f32(), "a");
  const auto b = fixture.add_symbol(fixture.f32(), "b");
  const auto c = fixture.add_symbol(fixture.f32(), "c");
  const auto d = fixture.add_symbol(fixture.f32(), "d");
  const auto e = fixture.add_symbol(fixture.f32(), "e");
  const auto f = fixture.add_symbol(fixture.f32(), "f");
  const auto g = fixture.add_symbol(fixture.f32(), "g");
  const auto inner = fixture.add_operation(fixture.associative, fixture.f32(), {b, c});
  const auto left = fixture.add_operation(fixture.associative, fixture.f32(), {a, inner, d});
  const auto right = fixture.add_operation(fixture.associative, fixture.f32(), {e, f});
  const auto root = fixture.add_operation(fixture.associative, fixture.f32(), {left, right, g});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  check_symbol_children(result.graph, result_node, {"a", "b", "c", "d", "e", "f", "g"});
}

TEST_CASE(
  "associative flattening can be disabled as a rule"
) {
  associative_fixture fixture;
  const auto a = fixture.add_symbol(fixture.f32(), "a");
  const auto b = fixture.add_symbol(fixture.f32(), "b");
  const auto c = fixture.add_symbol(fixture.f32(), "c");
  const auto nested = fixture.add_operation(fixture.associative, fixture.f32(), {b, c});
  const auto root = fixture.add_operation(fixture.associative, fixture.f32(), {a, nested});

  sivra::canonicalizer::options options;
  options.disable_rule(sivra::canonicalizer::rule::associative_flattening);

  const sivra::canonicalizer::engine engine(options);
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  REQUIRE(result_node.children().size() == 2);
  CHECK(symbol_name(result.graph.at(result_node.children()[0])) == "a");
  const auto& nested_result = result.graph.at(result_node.children()[1]);
  CHECK(nested_result.operation() == fixture.associative);
  check_symbol_children(result.graph, nested_result, {"b", "c"});
}

TEST_CASE(
  "associative flattening respects the enabled trait mask"
) {
  associative_fixture fixture;
  const auto a = fixture.add_symbol(fixture.f32(), "a");
  const auto b = fixture.add_symbol(fixture.f32(), "b");
  const auto c = fixture.add_symbol(fixture.f32(), "c");
  const auto nested = fixture.add_operation(fixture.associative, fixture.f32(), {b, c});
  const auto root = fixture.add_operation(fixture.associative, fixture.f32(), {a, nested});

  sivra::canonicalizer::options options;
  options.disable_trait(sivra::ir::operation_trait::associative);

  const sivra::canonicalizer::engine engine(options);
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  REQUIRE(result_node.children().size() == 2);
  CHECK(result.graph.at(result_node.children()[1]).operation() == fixture.associative);
}

TEST_CASE(
  "associative flattening ignores operations without associative metadata"
) {
  associative_fixture fixture;
  const auto a = fixture.add_symbol(fixture.f32(), "a");
  const auto b = fixture.add_symbol(fixture.f32(), "b");
  const auto c = fixture.add_symbol(fixture.f32(), "c");
  const auto nested = fixture.add_operation(fixture.non_associative, fixture.f32(), {b, c});
  const auto root = fixture.add_operation(fixture.non_associative, fixture.f32(), {a, nested});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  REQUIRE(result_node.children().size() == 2);
  CHECK(result.graph.at(result_node.children()[1]).operation() == fixture.non_associative);
}

TEST_CASE(
  "associative flattening requires the same nested operation"
) {
  associative_fixture fixture;
  const auto a = fixture.add_symbol(fixture.f32(), "a");
  const auto b = fixture.add_symbol(fixture.f32(), "b");
  const auto c = fixture.add_symbol(fixture.f32(), "c");
  const auto nested = fixture.add_operation(fixture.other_associative, fixture.f32(), {b, c});
  const auto root = fixture.add_operation(fixture.associative, fixture.f32(), {a, nested});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  REQUIRE(result_node.children().size() == 2);
  CHECK(result.graph.at(result_node.children()[1]).operation() == fixture.other_associative);
}

TEST_CASE(
  "associative flattening requires an exact nested result type"
) {
  associative_fixture fixture;
  const auto a = fixture.add_symbol(fixture.f32(), "a");
  const auto b = fixture.add_symbol(fixture.i32(), "b");
  const auto c = fixture.add_symbol(fixture.i32(), "c");
  const auto nested = fixture.add_operation(fixture.associative, fixture.i32(), {b, c});
  const auto root = fixture.add_operation(fixture.associative, fixture.f32(), {a, nested});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  REQUIRE(result_node.children().size() == 2);
  CHECK(&result.graph.at(result_node.children()[1]).result_type() == &fixture.i32());
}

TEST_CASE(
  "associative flattening preserves unary nested operations"
) {
  associative_fixture fixture;
  const auto a = fixture.add_symbol(fixture.f32(), "a");
  const auto b = fixture.add_symbol(fixture.f32(), "b");
  const auto unary = fixture.add_operation(fixture.associative, fixture.f32(), {b});
  const auto root = fixture.add_operation(fixture.associative, fixture.f32(), {a, unary});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  REQUIRE(result_node.children().size() == 2);
  CHECK(result.graph.at(result_node.children()[1]).operation() == fixture.associative);
}

TEST_CASE(
  "associative flattening preserves unary parent operations"
) {
  associative_fixture fixture;
  const auto a = fixture.add_symbol(fixture.f32(), "a");
  const auto b = fixture.add_symbol(fixture.f32(), "b");
  const auto nested = fixture.add_operation(fixture.associative, fixture.f32(), {a, b});
  const auto root = fixture.add_operation(fixture.associative, fixture.f32(), {nested});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  REQUIRE(result_node.children().size() == 1);
  CHECK(result.graph.at(result_node.children()[0]).operation() == fixture.associative);
}

TEST_CASE(
  "associative flattening preserves nested nodes with leaf values"
) {
  associative_fixture fixture;
  const auto a = fixture.add_symbol(fixture.f32(), "a");
  const auto b = fixture.add_symbol(fixture.f32(), "b");
  const auto c = fixture.add_symbol(fixture.f32(), "c");
  const auto nested = fixture.add_operation(
    fixture.associative, fixture.f32(), {b, c}, sivra::ir::symbol_ref{.name = "annotated"}
  );
  const auto root = fixture.add_operation(fixture.associative, fixture.f32(), {a, nested});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  REQUIRE(result_node.children().size() == 2);
  const auto& nested_result = result.graph.at(result_node.children()[1]);
  CHECK(nested_result.operation() == fixture.associative);
  CHECK(symbol_name(nested_result) == "annotated");
}
