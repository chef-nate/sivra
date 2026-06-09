#include <sivra/canonicalizer/engine.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/ir_context.hpp>
#include <sivra/ir/leaf.hpp>
#include <sivra/ir/operation.hpp>
#include <sivra/ir/scalar_type.hpp>

#include <doctest/doctest.h>

#include <string>
#include <utility>

namespace {

struct context_fixture {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph;
  sivra::ir::operation_id constant;
  sivra::ir::operation_id symbol;
  sivra::ir::operation_id custom;

  explicit context_fixture(
    bool custom_has_identity
  )
      : graph(context),
        constant(context.operations().register_operation("constant")),
        symbol(context.operations().register_operation("symbol")),
        custom(context.operations().register_operation(
          "custom",
          custom_has_identity
            ? sivra::ir::operation_semantics{
                .identity = sivra::ir::operation_constant{
                  .element = sivra::ir::well_known_constant::zero,
                },
              }
            : sivra::ir::operation_semantics{}
        )) {
  }

  const sivra::ir::scalar_type_def& f32() {
    return context.types().scalar(sivra::ir::scalar_type::f32);
  }

  sivra::ir::node_id add_symbol(
    std::string name
  ) {
    return graph.add_node(symbol, f32(), {}, sivra::ir::symbol_ref{.name = std::move(name)});
  }

  sivra::ir::node_id add_zero() {
    return graph.add_constant(
      constant, sivra::ir::constant_value::scalar(f32(), sivra::ir::f32_constant::from_value(0.0F))
    );
  }

  sivra::ir::node_id add_custom(
    sivra::ir::node_id lhs,
    sivra::ir::node_id rhs
  ) {
    return graph.add_node(custom, f32(), {lhs, rhs});
  }
};

} // namespace

TEST_CASE(
  "canonicalizer result graph preserves the source context"
) {
  context_fixture fixture(false);
  const auto root = fixture.add_symbol("x");

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);

  CHECK(&result.graph.context() == &fixture.context);
}

TEST_CASE(
  "one canonicalizer engine handles graphs from separate contexts"
) {
  context_fixture lhs(false);
  context_fixture rhs(false);
  const auto lhs_root = lhs.add_symbol("lhs");
  const auto rhs_root = rhs.add_symbol("rhs");

  const sivra::canonicalizer::engine engine;
  const auto lhs_result = engine.canonicalize(lhs.graph, lhs_root);
  const auto rhs_result = engine.canonicalize(rhs.graph, rhs_root);

  CHECK(&lhs_result.graph.context() == &lhs.context);
  CHECK(&rhs_result.graph.context() == &rhs.context);
  CHECK(lhs_result.graph.at(lhs_result.root).operation() == lhs.symbol);
  CHECK(rhs_result.graph.at(rhs_result.root).operation() == rhs.symbol);
}

TEST_CASE(
  "canonicalizer resolves operation semantics from each graph context"
) {
  context_fixture with_identity(true);
  context_fixture without_identity(false);
  REQUIRE(with_identity.custom == without_identity.custom);

  const auto with_symbol = with_identity.add_symbol("x");
  const auto with_zero = with_identity.add_zero();
  const auto with_root = with_identity.add_custom(with_symbol, with_zero);

  const auto without_symbol = without_identity.add_symbol("x");
  const auto without_zero = without_identity.add_zero();
  const auto without_root = without_identity.add_custom(without_symbol, without_zero);

  const sivra::canonicalizer::engine engine;
  const auto replaced = engine.canonicalize(with_identity.graph, with_root);
  const auto preserved = engine.canonicalize(without_identity.graph, without_root);

  CHECK(replaced.graph.at(replaced.root).operation() == with_identity.symbol);
  CHECK(preserved.graph.at(preserved.root).operation() == without_identity.custom);
}
