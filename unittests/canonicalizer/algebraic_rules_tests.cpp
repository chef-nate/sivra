#include "../support/expression_format.hpp"

#include <sivra/canonicalizer/engine.hpp>
#include <sivra/canonicalizer/options.hpp>
#include <sivra/canonicalizer/rule.hpp>
#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/ir_context.hpp>
#include <sivra/ir/leaf.hpp>
#include <sivra/ir/operation.hpp>
#include <sivra/ir/scalar_type.hpp>

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct algebraic_fixture {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph;
  sivra::ir::builtin_operation_ids builtins;
  sivra::ir::operation_id custom;

  algebraic_fixture()
      : graph(context),
        builtins(sivra::ir::register_builtin_operations(context.operations())),
        custom(context.operations().register_operation("custom")) {}

  const sivra::ir::scalar_type_def& f32() {
    return context.types().scalar(sivra::ir::scalar_type::f32);
  }

  sivra::ir::node_id add_symbol(
    std::string name
  ) {
    return graph.add_node(
      builtins.symbol, f32(), {}, sivra::ir::symbol_ref{.name = std::move(name)}
    );
  }

  sivra::ir::node_id add_constant(
    float value
  ) {
    return graph.add_constant(
      builtins.constant,
      sivra::ir::constant_value::scalar(f32(), sivra::ir::f32_constant::from_value(value))
    );
  }

  sivra::ir::node_id add_operation(
    sivra::ir::operation_id operation,
    std::vector<sivra::ir::node_id> children
  ) {
    return graph.add_node(operation, f32(), std::move(children));
  }
};

void check_canonical_expression(
  std::string_view input,
  std::string_view expected,
  std::string_view actual
) {
  const auto message =
    std::string("\ncanonicalization mismatch") + "\n  input:    " + std::string(input) +
    "\n  expected: " + std::string(expected) + "\n  actual:   " + std::string(actual);

  CHECK_MESSAGE(actual == expected, message);
}

std::string format_source(
  const algebraic_fixture& fixture,
  sivra::ir::node_id root
) {
  return sivra::test_support::format_expression(fixture.graph, root);
}

std::string format_result(
  const sivra::canonicalizer::single_result& result
) {
  return sivra::test_support::format_expression(result.graph, result.root);
}

} // namespace

TEST_CASE(
  "identity elimination removes trailing identity operand"
) {
  algebraic_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto zero = fixture.add_constant(0.0);
  const auto root = fixture.add_operation(fixture.builtins.add, {x, zero});
  const auto input = format_source(fixture, root);

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "x", format_result(result));
}

TEST_CASE(
  "identity elimination removes leading identity operand"
) {
  algebraic_fixture fixture;
  const auto zero = fixture.add_constant(0.0);
  const auto x = fixture.add_symbol("x");
  const auto root = fixture.add_operation(fixture.builtins.add, {zero, x});
  const auto input = format_source(fixture, root);

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "x", format_result(result));
}

TEST_CASE(
  "identity elimination preserves non-identity constants"
) {
  algebraic_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto two = fixture.add_constant(2.0);
  const auto root = fixture.add_operation(fixture.builtins.add, {x, two});
  const auto input = format_source(fixture, root);

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "add(x, 2)", format_result(result));
}

TEST_CASE(
  "identity elimination returns an identity when all operands are identities"
) {
  algebraic_fixture fixture;
  const auto lhs = fixture.add_constant(0.0);
  const auto rhs = fixture.add_constant(0.0);
  const auto root = fixture.add_operation(fixture.builtins.add, {lhs, rhs});
  const auto input = format_source(fixture, root);

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "0", format_result(result));
}

TEST_CASE(
  "identity elimination can be disabled"
) {
  algebraic_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto zero = fixture.add_constant(0.0);
  const auto root = fixture.add_operation(fixture.builtins.add, {x, zero});
  const auto input = format_source(fixture, root);

  sivra::canonicalizer::options options;
  options.disable_rule(sivra::canonicalizer::rule::identity_elimination);

  const sivra::canonicalizer::engine engine(options);
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "add(x, 0)", format_result(result));
}

TEST_CASE(
  "identity elimination only applies to operations with identity metadata"
) {
  algebraic_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto zero = fixture.add_constant(0.0);
  const auto root = fixture.add_operation(fixture.custom, {x, zero});
  const auto input = format_source(fixture, root);

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "custom(x, 0)", format_result(result));
}

TEST_CASE(
  "annihilator collapse removes trailing annihilator operand"
) {
  algebraic_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto zero = fixture.add_constant(0.0);
  const auto root = fixture.add_operation(fixture.builtins.multiply, {x, zero});
  const auto input = format_source(fixture, root);

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "0", format_result(result));
}

TEST_CASE(
  "annihilator collapse removes leading annihilator operand"
) {
  algebraic_fixture fixture;
  const auto zero = fixture.add_constant(0.0);
  const auto x = fixture.add_symbol("x");
  const auto root = fixture.add_operation(fixture.builtins.multiply, {zero, x});
  const auto input = format_source(fixture, root);

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "0", format_result(result));
}

TEST_CASE(
  "annihilator collapse preserves non-annihilator constants"
) {
  algebraic_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto two = fixture.add_constant(2.0);
  const auto root = fixture.add_operation(fixture.builtins.multiply, {x, two});
  const auto input = format_source(fixture, root);

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "multiply(x, 2)", format_result(result));
}

TEST_CASE(
  "annihilator collapse can be disabled"
) {
  algebraic_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto zero = fixture.add_constant(0.0);
  const auto root = fixture.add_operation(fixture.builtins.multiply, {x, zero});
  const auto input = format_source(fixture, root);

  sivra::canonicalizer::options options;
  options.disable_rule(sivra::canonicalizer::rule::annihilator_collapse);

  const sivra::canonicalizer::engine engine(options);
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "multiply(x, 0)", format_result(result));
}

TEST_CASE(
  "annihilator collapse only applies to operations with annihilator metadata"
) {
  algebraic_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto zero = fixture.add_constant(0.0);
  const auto root = fixture.add_operation(fixture.custom, {x, zero});
  const auto input = format_source(fixture, root);

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  check_canonical_expression(input, "custom(x, 0)", format_result(result));
}
