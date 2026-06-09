#include "../support/expression_format.hpp"

#include <sivra/canonicalizer/engine.hpp>
#include <sivra/canonicalizer/options.hpp>
#include <sivra/canonicalizer/rule.hpp>
#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/ir_context.hpp>
#include <sivra/ir/leaf.hpp>
#include <sivra/ir/scalar_type.hpp>

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

struct rule_pipeline_fixture {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph;
  sivra::ir::builtin_operation_ids builtins;

  rule_pipeline_fixture()
      : graph(context),
        builtins(sivra::ir::register_builtin_operations(context.operations())) {}

  sivra::ir::node_id add_symbol(
    std::string name
  ) {
    return graph.add_node(
      builtins.symbol,
      context.types().scalar(sivra::ir::scalar_type::f32),
      {},
      sivra::ir::symbol_ref{.name = std::move(name)}
    );
  }

  sivra::ir::node_id add_constant(
    float value
  ) {
    return graph.add_constant(
      builtins.constant,
      sivra::ir::constant_value::scalar(
        context.types().scalar(sivra::ir::scalar_type::f32),
        sivra::ir::f32_constant::from_value(value)
      )
    );
  }

  sivra::ir::node_id add_multiply(
    std::vector<sivra::ir::node_id> children
  ) {
    return graph.add_node(
      builtins.multiply, context.types().scalar(sivra::ir::scalar_type::f32), std::move(children)
    );
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

std::string canonicalize_expression(
  const rule_pipeline_fixture& fixture,
  sivra::ir::node_id root,
  sivra::canonicalizer::options options = {}
) {
  const sivra::canonicalizer::engine engine(options);
  const auto result = engine.canonicalize(fixture.graph, root);
  return sivra::test_support::format_expression(result.graph, result.root);
}

std::string format_source(
  const rule_pipeline_fixture& fixture,
  sivra::ir::node_id root
) {
  return sivra::test_support::format_expression(fixture.graph, root);
}

} // namespace

TEST_CASE(
  "identity elimination feeds annihilator collapse"
) {
  rule_pipeline_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto zero = fixture.add_constant(0.0);
  const auto one = fixture.add_constant(1.0);
  const auto root = fixture.add_multiply({x, zero, one});
  const auto input = format_source(fixture, root);

  check_canonical_expression(input, "0", canonicalize_expression(fixture, root));
}

TEST_CASE(
  "rule composition is independent of operand order"
) {
  rule_pipeline_fixture fixture;
  const auto one = fixture.add_constant(1.0);
  const auto x = fixture.add_symbol("x");
  const auto zero = fixture.add_constant(0.0);
  const auto root = fixture.add_multiply({one, x, zero});
  const auto input = format_source(fixture, root);

  check_canonical_expression(input, "0", canonicalize_expression(fixture, root));
}

TEST_CASE(
  "identity elimination rebuilds an operation with remaining children"
) {
  rule_pipeline_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto one = fixture.add_constant(1.0);
  const auto y = fixture.add_symbol("y");
  const auto root = fixture.add_multiply({x, one, y});
  const auto input = format_source(fixture, root);

  check_canonical_expression(input, "multiply(x, y)", canonicalize_expression(fixture, root));
}

TEST_CASE(
  "multiple identities are removed before a later rule"
) {
  rule_pipeline_fixture fixture;
  const auto first_one = fixture.add_constant(1.0);
  const auto x = fixture.add_symbol("x");
  const auto second_one = fixture.add_constant(1.0);
  const auto zero = fixture.add_constant(0.0);
  const auto third_one = fixture.add_constant(1.0);
  const auto root = fixture.add_multiply({first_one, x, second_one, zero, third_one});
  const auto input = format_source(fixture, root);

  check_canonical_expression(input, "0", canonicalize_expression(fixture, root));
}

TEST_CASE(
  "rule pipeline respects independently disabled rules"
) {
  rule_pipeline_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto zero = fixture.add_constant(0.0);
  const auto one = fixture.add_constant(1.0);
  const auto root = fixture.add_multiply({x, zero, one});
  const auto input = format_source(fixture, root);

  SUBCASE("both rules enabled") {
    check_canonical_expression(input, "0", canonicalize_expression(fixture, root));
  }

  SUBCASE("identity elimination disabled") {
    sivra::canonicalizer::options options;
    options.disable_rule(sivra::canonicalizer::rule::identity_elimination);

    check_canonical_expression(input, "0", canonicalize_expression(fixture, root, options));
  }

  SUBCASE("annihilator collapse disabled") {
    sivra::canonicalizer::options options;
    options.disable_rule(sivra::canonicalizer::rule::annihilator_collapse);

    check_canonical_expression(
      input, "multiply(x, 0)", canonicalize_expression(fixture, root, options)
    );
  }

  SUBCASE("both rules disabled") {
    sivra::canonicalizer::options options;
    options.disable_rule(
      sivra::canonicalizer::rule::identity_elimination |
      sivra::canonicalizer::rule::annihilator_collapse
    );

    check_canonical_expression(
      input, "multiply(x, 0, 1)", canonicalize_expression(fixture, root, options)
    );
  }
}

TEST_CASE(
  "identity elimination replaces an operation when one child remains"
) {
  rule_pipeline_fixture fixture;
  const auto x = fixture.add_symbol("x");
  const auto one = fixture.add_constant(1.0);
  const auto root = fixture.add_multiply({x, one});
  const auto input = format_source(fixture, root);

  check_canonical_expression(input, "x", canonicalize_expression(fixture, root));
}

TEST_CASE(
  "identity elimination reuses an identity when all children are identities"
) {
  rule_pipeline_fixture fixture;
  const auto lhs = fixture.add_constant(1.0);
  const auto rhs = fixture.add_constant(1.0);
  const auto root = fixture.add_multiply({lhs, rhs});
  const auto input = format_source(fixture, root);

  check_canonical_expression(input, "1", canonicalize_expression(fixture, root));
}
