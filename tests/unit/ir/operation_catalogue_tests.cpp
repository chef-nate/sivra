#include "../../support/graph_builder_fixture.hpp"

#include <sivra/ir/operation_catalogue.hpp>

#include <doctest/doctest.h>

#include <array>
#include <memory>
#include <stdexcept>

namespace {

sivra::ir::operation_registration registration(
  std::string key
) {
  return sivra::test_support::test_operation(std::move(key));
}

} // namespace

TEST_CASE(
  "operation catalogue registration preserves order and freezes immutable definitions"
) {
  sivra::ir::operation_catalogue_builder builder;
  const std::array registrations{
    registration("first"),
    registration("second"),
  };

  const auto identifiers = builder.register_operations(registrations);
  REQUIRE(identifiers.has_value());
  const auto catalogue = std::move(builder).freeze();
  REQUIRE(catalogue.has_value());

  CHECK((*identifiers)[0].index() == 0);
  CHECK((*identifiers)[1].index() == 1);
  CHECK((*catalogue)->at("first").id() == (*identifiers)[0]);
  CHECK((*catalogue)->at("second").name() == "second");
  CHECK((*catalogue)->operations().size() == 2);
  CHECK((*catalogue)->find(sivra::ir::operation_key("first")) != nullptr);
  CHECK((*catalogue)->find(sivra::ir::operation_key("first", 2)) == nullptr);
  CHECK((*catalogue)->find_by_name("second") != nullptr);
  CHECK((*catalogue)->find_by_name("missing") == nullptr);

  const auto mutation = builder.register_operation(registration("third"));
  REQUIRE(!mutation.has_value());
  CHECK(mutation.error().front().code == "ir.catalogue.frozen");
}

TEST_CASE(
  "operation catalogue rejects duplicate batches atomically"
) {
  sivra::ir::operation_catalogue_builder builder;
  REQUIRE(builder.register_operation(registration("existing")).has_value());

  const std::array duplicate_batch{
    registration("new"),
    registration("new"),
  };
  const auto duplicate = builder.register_operations(duplicate_batch);
  CHECK(!duplicate.has_value());

  const auto next = builder.register_operation(registration("after"));
  REQUIRE(next.has_value());
  CHECK(next->index() == 1);
}

TEST_CASE(
  "operation catalogue keys include stable versions"
) {
  sivra::ir::operation_catalogue_builder builder;
  auto first = registration("operation");
  auto second = registration("operation");
  first.key = sivra::ir::operation_key("operation", 1);
  first.name = "operation.v1";
  second.key = sivra::ir::operation_key("operation", 2);
  second.name = "operation.v2";

  const std::array registrations{std::move(first), std::move(second)};
  const auto identifiers = builder.register_operations(registrations);
  REQUIRE(identifiers.has_value());
  const auto catalogue = std::move(builder).freeze();
  REQUIRE(catalogue.has_value());

  CHECK((*catalogue)->find(sivra::ir::operation_key("operation", 1))->id() == (*identifiers)[0]);
  CHECK((*catalogue)->find(sivra::ir::operation_key("operation", 2))->id() == (*identifiers)[1]);
  CHECK((*catalogue)->at("operation").id() == (*identifiers)[0]);
  CHECK((*catalogue)->contains("operation"));
}

TEST_CASE(
  "operation catalogue validates signatures"
) {
  sivra::ir::operation_catalogue_builder builder;
  const auto invalid = builder.register_operation(
    {
      .key = "invalid",
      .name = "invalid",
      .signature =
        {
          .arity =
            {
              .minimum = 3,
              .maximum = 2,
            },
          .operand_types = sivra::ir::operand_type_constraint::same_as_result,
        },
      .attribute_schema = {},
      .semantics = {},
    }
  );

  REQUIRE(!invalid.has_value());
  CHECK(invalid.error().front().code == "ir.catalogue.invalid_signature");
}

TEST_CASE(
  "operation catalogue validates semantic metadata"
) {
  sivra::ir::operation_catalogue_builder unary_builder;
  auto unary = registration("unary");
  unary.signature.arity = {.minimum = 1, .maximum = 1};
  unary.semantics.traits = sivra::ir::operation_trait::commutative;
  const auto invalid_trait = unary_builder.register_operation(std::move(unary));
  REQUIRE(!invalid_trait.has_value());
  CHECK(invalid_trait.error().front().code == "ir.catalogue.invalid_semantics");

  sivra::ir::operation_catalogue_builder conflicting_builder;
  auto conflicting = registration("conflicting");
  conflicting.signature.arity = {.minimum = 2, .maximum = std::nullopt};
  conflicting.semantics.traits = sivra::ir::operation_trait::commutative;
  conflicting.semantics.identity = sivra::ir::operation_constant{
    sivra::ir::well_known_constant::zero,
  };
  conflicting.semantics.left_identity = sivra::ir::operation_constant{
    sivra::ir::well_known_constant::zero,
  };
  const auto invalid_constants = conflicting_builder.register_operation(std::move(conflicting));
  REQUIRE(!invalid_constants.has_value());
  CHECK(invalid_constants.error().front().code == "ir.catalogue.invalid_semantics");
}

TEST_CASE(
  "operation identifiers are scoped to one catalogue"
) {
  auto lhs = sivra::test_support::make_test_catalogue();
  auto rhs = sivra::test_support::make_test_catalogue();

  CHECK(lhs.builtins.add.index() == rhs.builtins.add.index());
  CHECK(lhs.builtins.add != rhs.builtins.add);
  CHECK_THROWS_AS(rhs.catalogue->operation(lhs.builtins.add), std::invalid_argument);
}

TEST_CASE(
  "built-in catalogue exposes only value operations"
) {
  const auto operations = sivra::test_support::make_test_catalogue();

  CHECK(operations.catalogue->contains("add"));
  CHECK(operations.catalogue->contains("multiply"));
  CHECK(operations.catalogue->contains("subtract"));
  CHECK(operations.catalogue->contains("divide"));
  CHECK(operations.catalogue->contains("maximum"));
  CHECK(operations.catalogue->contains("minimum"));
  CHECK(operations.catalogue->contains("sqrt"));
  CHECK(operations.catalogue->contains("reciprocal"));
  CHECK(operations.catalogue->contains("reciprocal_sqrt"));
  CHECK(operations.catalogue->contains("square"));
  CHECK(operations.catalogue->contains("bit_and"));
  CHECK(operations.catalogue->contains("bit_and_not"));
  CHECK(operations.catalogue->contains("bit_or"));
  CHECK(operations.catalogue->contains("bit_xor"));
  CHECK(operations.catalogue->contains("copy"));
  CHECK(!operations.catalogue->contains("constant"));
  CHECK(!operations.catalogue->contains("symbol"));
  CHECK(!operations.catalogue->contains("memory_load"));
}

TEST_CASE(
  "catalogue compatibility identifiers are independent of registration order"
) {
  sivra::ir::operation_catalogue_builder lhs_builder;
  sivra::ir::operation_catalogue_builder rhs_builder;
  const std::array lhs_registrations{registration("first"), registration("second")};
  const std::array rhs_registrations{registration("second"), registration("first")};
  REQUIRE(lhs_builder.register_operations(lhs_registrations).has_value());
  REQUIRE(rhs_builder.register_operations(rhs_registrations).has_value());

  const auto lhs = std::move(lhs_builder).freeze();
  const auto rhs = std::move(rhs_builder).freeze();
  REQUIRE(lhs.has_value());
  REQUIRE(rhs.has_value());
  CHECK((*lhs)->compatibility_id() == (*rhs)->compatibility_id());
}

TEST_CASE(
  "catalogue compatibility identifiers include signature and attribute semantics"
) {
  sivra::ir::operation_catalogue_builder lhs_builder;
  sivra::ir::operation_catalogue_builder rhs_builder;
  auto lhs_registration = registration("operation");
  auto rhs_registration = registration("operation");
  rhs_registration.signature.arity.maximum = 1;
  REQUIRE(lhs_builder.register_operation(std::move(lhs_registration)).has_value());
  REQUIRE(rhs_builder.register_operation(std::move(rhs_registration)).has_value());

  const auto lhs = std::move(lhs_builder).freeze();
  const auto rhs = std::move(rhs_builder).freeze();
  REQUIRE(lhs.has_value());
  REQUIRE(rhs.has_value());
  CHECK((*lhs)->compatibility_id() != (*rhs)->compatibility_id());
}

TEST_CASE(
  "graphs retain catalogue lifetime after the builder and caller reference are destroyed"
) {
  const auto make_graph = [] {
    sivra::ir::operation_catalogue_builder builder;
    const auto operation =
      sivra::test_support::require_value(builder.register_operation(registration("operation")));
    auto catalogue = sivra::test_support::require_value(std::move(builder).freeze());
    sivra::ir::expression_graph graph(catalogue);
    sivra::ir::graph_builder graph_builder(graph);
    const auto lhs = sivra::test_support::require_value(
      graph_builder.make_symbol("lhs", sivra::ir::value_type::f32())
    );
    const auto rhs = sivra::test_support::require_value(
      graph_builder.make_symbol("rhs", sivra::ir::value_type::f32())
    );
    const std::array operands{lhs, rhs};
    static_cast<void>(sivra::test_support::require_value(
      graph_builder.apply(operation, operands, sivra::ir::value_type::f32())
    ));
    return graph;
  };

  const auto graph = make_graph();
  CHECK(graph.catalogue().at("operation").name() == "operation");
  CHECK(graph.validate().has_value());
}
