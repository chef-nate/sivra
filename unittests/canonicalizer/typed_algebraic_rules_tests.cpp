#include <sivra/canonicalizer/engine.hpp>
#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/ir_context.hpp>
#include <sivra/ir/leaf.hpp>
#include <sivra/ir/operation.hpp>
#include <sivra/ir/scalar_type.hpp>
#include <sivra/ir/type.hpp>

#include <doctest/doctest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {

struct typed_algebraic_fixture {
  sivra::ir::ir_context context;
  sivra::ir::expression_graph graph;
  sivra::ir::builtin_operation_ids builtins;
  sivra::ir::operation_id all_bits_or;
  sivra::ir::operation_id explicit_annihilator;

  typed_algebraic_fixture()
      : graph(context),
        builtins(sivra::ir::register_builtin_operations(context.operations())),
        all_bits_or(context.operations().register_operation(
          "all_bits_or",
          sivra::ir::operation_semantics{
            .annihilator =
              sivra::ir::operation_constant{sivra::ir::well_known_constant::all_bits_set},
          }
        )),
        explicit_annihilator(context.operations().register_operation(
          "explicit_annihilator",
          sivra::ir::operation_semantics{
            .annihilator = sivra::ir::operation_constant{sivra::ir::scalar_constant_t{
              sivra::ir::i32_constant::from_value(7)}},
          }
        )) {}

  const sivra::ir::scalar_type_def& f32() {
    return context.types().scalar(sivra::ir::scalar_type::f32);
  }

  const sivra::ir::scalar_type_def& i32() {
    return context.types().scalar(sivra::ir::scalar_type::i32);
  }

  const sivra::ir::vector_type_def& vector_f32() { return context.types().vector(f32(), 4); }

  sivra::ir::node_id add_symbol(
    const sivra::ir::type& result_type,
    std::string name
  ) {
    return graph.add_node(
      builtins.symbol, result_type, {}, sivra::ir::symbol_ref{.name = std::move(name)}
    );
  }

  sivra::ir::node_id add_f32(
    float value
  ) {
    return graph.add_constant(
      builtins.constant,
      sivra::ir::constant_value::scalar(f32(), sivra::ir::f32_constant::from_value(value))
    );
  }

  sivra::ir::node_id add_i32(
    std::int32_t value
  ) {
    return graph.add_constant(
      builtins.constant,
      sivra::ir::constant_value::scalar(i32(), sivra::ir::i32_constant::from_value(value))
    );
  }

  sivra::ir::node_id add_vector_f32_splat(
    float value
  ) {
    return graph.add_constant(
      builtins.constant,
      sivra::ir::constant_value::splat(vector_f32(), sivra::ir::f32_constant::from_value(value))
    );
  }

  sivra::ir::node_id add_operation(
    sivra::ir::operation_id operation,
    const sivra::ir::type& result_type,
    std::vector<sivra::ir::node_id> children
  ) {
    return graph.add_node(operation, result_type, std::move(children));
  }
};

const sivra::ir::constant_value& constant_value(
  const sivra::ir::expression_node& node
) {
  REQUIRE(node.leaf_value().has_value());
  const auto* value = std::get_if<sivra::ir::constant_value>(&*node.leaf_value());
  REQUIRE(value != nullptr);
  return *value;
}

} // namespace

TEST_CASE(
  "identity elimination preserves the remaining child type"
) {
  typed_algebraic_fixture fixture;
  const auto x = fixture.add_symbol(fixture.f32(), "x");
  const auto zero = fixture.add_f32(0.0F);
  const auto root = fixture.add_operation(fixture.builtins.add, fixture.f32(), {x, zero});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  CHECK(result_node.operation() == fixture.builtins.symbol);
  CHECK(&result_node.result_type() == &fixture.f32());
}

TEST_CASE(
  "annihilator collapse preserves scalar constant type and value"
) {
  typed_algebraic_fixture fixture;
  const auto x = fixture.add_symbol(fixture.f32(), "x");
  const auto zero = fixture.add_f32(0.0F);
  const auto root = fixture.add_operation(fixture.builtins.multiply, fixture.f32(), {x, zero});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);
  const auto& value = constant_value(result_node);

  CHECK(result_node.operation() == fixture.builtins.constant);
  CHECK(&result_node.result_type() == &fixture.f32());
  CHECK(&value.result_type() == &fixture.f32());
  CHECK(
    std::get<sivra::ir::f32_constant>(value.element(0)) == sivra::ir::f32_constant::from_value(0.0F)
  );
}

TEST_CASE(
  "annihilator collapse ignores a scalar constant for a vector operation"
) {
  typed_algebraic_fixture fixture;
  const auto x = fixture.add_symbol(fixture.vector_f32(), "x");
  const auto scalar_zero = fixture.add_f32(0.0F);
  const auto root =
    fixture.add_operation(fixture.builtins.multiply, fixture.vector_f32(), {x, scalar_zero});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  CHECK(result_node.operation() == fixture.builtins.multiply);
  CHECK(&result_node.result_type() == &fixture.vector_f32());
  REQUIRE(result_node.children().size() == 2);
  CHECK(&result.graph.at(result_node.children()[1]).result_type() == &fixture.f32());
}

TEST_CASE(
  "annihilator collapse accepts an exact vector splat constant"
) {
  typed_algebraic_fixture fixture;
  const auto x = fixture.add_symbol(fixture.vector_f32(), "x");
  const auto zero = fixture.add_vector_f32_splat(0.0F);
  const auto root =
    fixture.add_operation(fixture.builtins.multiply, fixture.vector_f32(), {x, zero});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);
  const auto& value = constant_value(result_node);

  CHECK(result_node.operation() == fixture.builtins.constant);
  CHECK(&result_node.result_type() == &fixture.vector_f32());
  CHECK(&value.result_type() == &fixture.vector_f32());
  CHECK(value.is_splat());
  CHECK(value.element_count() == 4);
  CHECK(
    std::get<sivra::ir::f32_constant>(value.element(3)) == sivra::ir::f32_constant::from_value(0.0F)
  );
}

TEST_CASE(
  "identity elimination ignores a constant with a different scalar type"
) {
  typed_algebraic_fixture fixture;
  const auto x = fixture.add_symbol(fixture.i32(), "x");
  const auto f32_zero = fixture.add_f32(0.0F);
  const auto root = fixture.add_operation(fixture.builtins.add, fixture.i32(), {x, f32_zero});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);

  CHECK(result_node.operation() == fixture.builtins.add);
  CHECK(&result_node.result_type() == &fixture.i32());
  REQUIRE(result_node.children().size() == 2);
  CHECK(&result.graph.at(result_node.children()[1]).result_type() == &fixture.f32());
}

TEST_CASE(
  "annihilator collapse supports all-bits-set constants"
) {
  typed_algebraic_fixture fixture;
  const auto x = fixture.add_symbol(fixture.i32(), "x");
  const auto all_bits_set = fixture.add_i32(-1);
  const auto root = fixture.add_operation(fixture.all_bits_or, fixture.i32(), {x, all_bits_set});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);
  const auto& value = constant_value(result_node);

  CHECK(result_node.operation() == fixture.builtins.constant);
  CHECK(&result_node.result_type() == &fixture.i32());
  CHECK(
    std::get<sivra::ir::i32_constant>(value.element(0)).bits ==
    std::numeric_limits<std::uint32_t>::max()
  );
}

TEST_CASE(
  "annihilator collapse supports explicit non-zero constants"
) {
  typed_algebraic_fixture fixture;
  const auto x = fixture.add_symbol(fixture.i32(), "x");
  const auto seven = fixture.add_i32(7);
  const auto root = fixture.add_operation(fixture.explicit_annihilator, fixture.i32(), {x, seven});

  const sivra::canonicalizer::engine engine;
  const auto result = engine.canonicalize(fixture.graph, root);
  const auto& result_node = result.graph.at(result.root);
  const auto& value = constant_value(result_node);

  CHECK(result_node.operation() == fixture.builtins.constant);
  CHECK(&result_node.result_type() == &fixture.i32());
  CHECK(std::get<sivra::ir::i32_constant>(value.element(0)).value() == 7);
}
