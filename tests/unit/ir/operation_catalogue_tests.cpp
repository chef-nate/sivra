#include "../../support/graph_builder_fixture.hpp"

#include <sivra/ir/operation_catalogue.hpp>

#include <doctest/doctest.h>

#include <array>
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
  "operation catalogue validates signatures"
) {
  sivra::ir::operation_catalogue_builder builder;
  const auto invalid = builder.register_operation(
    {
      .key = "invalid",
      .name = "invalid",
      .signature =
        {
          .minimum_operands = 3,
          .maximum_operands = 2,
          .operands_match_result = true,
        },
      .semantics = {},
    }
  );

  REQUIRE(!invalid.has_value());
  CHECK(invalid.error().front().code == "ir.catalogue.invalid_signature");
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
  CHECK(operations.catalogue->contains("maximum"));
  CHECK(!operations.catalogue->contains("constant"));
  CHECK(!operations.catalogue->contains("symbol"));
  CHECK(!operations.catalogue->contains("memory_load"));
}
