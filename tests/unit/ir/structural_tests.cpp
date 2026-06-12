#include "../../support/graph_builder_fixture.hpp"

#include <sivra/ir/structural.hpp>
#include <sivra/ir/validation.hpp>

#include <doctest/doctest.h>

#include <new>

TEST_CASE(
  "structural services match expressions across compatible catalogues"
) {
  sivra::test_support::graph_builder_fixture lhs;
  sivra::test_support::graph_builder_fixture rhs;
  const auto lhs_root = lhs.apply(lhs.operations.builtins.add, {lhs.symbol("x"), lhs.f32(1.0F)});
  const auto rhs_root = rhs.apply(rhs.operations.builtins.add, {rhs.symbol("x"), rhs.f32(1.0F)});
  sivra::ir::structural_context structural;

  CHECK(structural.equal(lhs.graph, lhs_root, rhs.graph, rhs_root));
  CHECK(structural.hash(lhs.graph, lhs_root) == structural.hash(rhs.graph, rhs_root));
  CHECK(
    structural.compare(lhs.graph, lhs_root, rhs.graph, rhs_root) == std::strong_ordering::equal
  );
}

TEST_CASE(
  "structural identity ignores graph sharing shape"
) {
  sivra::test_support::graph_builder_fixture shared;
  sivra::test_support::graph_builder_fixture duplicated;
  const auto shared_symbol = shared.symbol("x");
  const auto shared_root =
    shared.apply(shared.operations.builtins.add, {shared_symbol, shared_symbol});
  const auto duplicated_root = duplicated.apply(
    duplicated.operations.builtins.add, {duplicated.symbol("x"), duplicated.symbol("x")}
  );
  sivra::ir::structural_context structural;

  CHECK(structural.equal(shared.graph, shared_root, duplicated.graph, duplicated_root));
}

TEST_CASE(
  "structural comparison is deterministic and distinguishes ordered operands"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto ab =
    fixture.apply(fixture.operations.builtins.subtract, {fixture.symbol("a"), fixture.symbol("b")});
  const auto ba =
    fixture.apply(fixture.operations.builtins.subtract, {fixture.symbol("b"), fixture.symbol("a")});
  sivra::ir::structural_context structural;

  const auto forward = structural.compare(fixture.graph, ab, fixture.graph, ba);
  const auto reverse = structural.compare(fixture.graph, ba, fixture.graph, ab);
  CHECK(forward != std::strong_ordering::equal);
  CHECK(forward == std::strong_ordering::less);
  CHECK(reverse == std::strong_ordering::greater);
  CHECK(!structural.equal(fixture.graph, ab, fixture.graph, ba));
}

TEST_CASE(
  "whole graph validation accepts builder-produced graphs"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto external = sivra::test_support::require_value(
    fixture.builder.make_external_value(sivra::ir::value_type::f32())
  );
  static_cast<void>(
    fixture.apply(fixture.operations.builtins.maximum, {external, fixture.f32(0.0F)})
  );

  CHECK(fixture.graph.validate().has_value());
  CHECK(sivra::ir::validate_graph(fixture.graph).has_value());
}

TEST_CASE(
  "structural identity includes result types and normalized attributes"
) {
  const auto schema = sivra::test_support::require_value(
    sivra::ir::operation_attribute_schema::create(
      std::array{
        sivra::ir::operation_attribute_field{
          .key = "lane",
          .kind = sivra::ir::operation_attribute_kind::integer,
          .required = true,
          .minimum_integer = 0,
          .maximum_integer = 3,
        },
      }
    )
  );
  auto operation = sivra::test_support::test_operation(
    "extract",
    {},
    {
      .arity = {.minimum = 1, .maximum = 1},
      .operand_types = sivra::ir::operand_type_constraint::same_as_result,
    }
  );
  operation.attribute_schema = schema;
  sivra::test_support::graph_builder_fixture fixture({std::move(operation)});
  const auto input = fixture.symbol("input");
  const std::array operands{input};
  const auto lane_zero = sivra::test_support::require_value(
    sivra::ir::operation_attributes::create(
      std::array{
        sivra::ir::operation_attribute{.key = "lane", .value = std::int64_t{0}},
      }
    )
  );
  const auto lane_one = sivra::test_support::require_value(
    sivra::ir::operation_attributes::create(
      std::array{
        sivra::ir::operation_attribute{.key = "lane", .value = std::int64_t{1}},
      }
    )
  );
  const auto first = sivra::test_support::require_value(fixture.builder.apply(
    fixture.operations.custom.front(), operands, lane_zero, sivra::ir::value_type::f32()
  ));
  const auto second = sivra::test_support::require_value(fixture.builder.apply(
    fixture.operations.custom.front(), operands, lane_one, sivra::ir::value_type::f32()
  ));
  sivra::ir::structural_context structural;

  CHECK(!structural.equal(fixture.graph, first, fixture.graph, second));
  CHECK(structural.hash(fixture.graph, first) != structural.hash(fixture.graph, second));

  sivra::test_support::graph_builder_fixture float_fixture;
  sivra::test_support::graph_builder_fixture integer_fixture;
  const auto float_symbol = float_fixture.symbol("x", sivra::ir::value_type::f32());
  const auto integer_symbol = integer_fixture.symbol("x", sivra::ir::value_type::i32());
  CHECK(
    !structural.equal(float_fixture.graph, float_symbol, integer_fixture.graph, integer_symbol)
  );
}

TEST_CASE(
  "structural equality hash and ordering obey basic laws"
) {
  sivra::test_support::graph_builder_fixture fixture;
  const auto a = fixture.symbol("a");
  const auto b = fixture.symbol("b");
  const auto c = fixture.symbol("c");
  sivra::ir::structural_context structural;

  CHECK(structural.equal(fixture.graph, a, fixture.graph, a));
  CHECK(structural.equal(fixture.graph, a, fixture.graph, a));
  CHECK(structural.hash(fixture.graph, a) == structural.hash(fixture.graph, a));
  CHECK(structural.compare(fixture.graph, a, fixture.graph, b) == std::strong_ordering::less);
  CHECK(structural.compare(fixture.graph, b, fixture.graph, a) == std::strong_ordering::greater);
  CHECK(structural.compare(fixture.graph, b, fixture.graph, c) == std::strong_ordering::less);
  CHECK(structural.compare(fixture.graph, a, fixture.graph, c) == std::strong_ordering::less);
}

TEST_CASE(
  "structural services support merge nodes deterministically"
) {
  sivra::test_support::graph_builder_fixture lhs;
  sivra::test_support::graph_builder_fixture rhs;
  const std::array lhs_incoming{lhs.symbol("a"), lhs.symbol("b")};
  const std::array rhs_incoming{rhs.symbol("a"), rhs.symbol("b")};
  const auto lhs_merge = sivra::test_support::require_value(
    lhs.builder.make_merge(lhs_incoming, sivra::ir::value_type::f32())
  );
  const auto rhs_merge = sivra::test_support::require_value(
    rhs.builder.make_merge(rhs_incoming, sivra::ir::value_type::f32())
  );
  sivra::ir::structural_context structural;

  CHECK(structural.equal(lhs.graph, lhs_merge, rhs.graph, rhs_merge));
  CHECK(structural.hash(lhs.graph, lhs_merge) == structural.hash(rhs.graph, rhs_merge));
}

TEST_CASE(
  "structural context does not reuse cache entries for graphs at the same address"
) {
  const auto operations = sivra::test_support::make_test_catalogue();
  using graph_t = sivra::ir::expression_graph;
  void* storage = ::operator new(sizeof(graph_t), std::align_val_t{alignof(graph_t)});
  sivra::ir::structural_context structural;

  auto* first = new (storage) graph_t(operations.catalogue);
  sivra::ir::node_id first_symbol = [&] {
    sivra::ir::graph_builder builder(*first);
    return sivra::test_support::require_value(
      builder.make_symbol("first", sivra::ir::value_type::f32())
    );
  }();
  const auto first_hash = structural.hash(*first, first_symbol);
  first->~graph_t();

  auto* second = new (storage) graph_t(operations.catalogue);
  sivra::ir::node_id second_symbol = [&] {
    sivra::ir::graph_builder builder(*second);
    return sivra::test_support::require_value(
      builder.make_symbol("second", sivra::ir::value_type::f32())
    );
  }();
  const auto second_hash = structural.hash(*second, second_symbol);
  second->~graph_t();
  ::operator delete(storage, std::align_val_t{alignof(graph_t)});

  CHECK(first_hash != second_hash);
}

TEST_CASE(
  "structural services handle deeply shared DAGs without recursive encoding growth"
) {
  sivra::test_support::graph_builder_fixture fixture;
  auto root = fixture.symbol("x");
  for (int index = 0; index < 256; ++index) {
    root = fixture.apply(fixture.operations.builtins.add, {root, root});
  }
  sivra::ir::structural_context structural;

  CHECK(structural.equal(fixture.graph, root, fixture.graph, root));
  CHECK(structural.hash(fixture.graph, root) == structural.hash(fixture.graph, root));
}
