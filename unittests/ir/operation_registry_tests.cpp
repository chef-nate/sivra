#include <sivra/ir/constant.hpp>
#include <sivra/ir/operation.hpp>
#include <sivra/ir/operation_registry.hpp>

#include <doctest/doctest.h>

#include <array>
#include <span>
#include <stdexcept>
#include <variant>

TEST_CASE(
  "operation_registry registers a batch in input order"
) {
  sivra::ir::operation_registry operations;
  const std::array registrations{
    sivra::ir::operation_registration{.name = "first"},
    sivra::ir::operation_registration{.name = "second"},
    sivra::ir::operation_registration{.name = "third"},
  };

  const auto identifiers = operations.register_operations(registrations);

  REQUIRE(identifiers.size() == registrations.size());
  CHECK(identifiers[0] == sivra::ir::operation_id(0));
  CHECK(identifiers[1] == sivra::ir::operation_id(1));
  CHECK(identifiers[2] == sivra::ir::operation_id(2));
  CHECK(operations.at(identifiers[0]).name() == "first");
  CHECK(operations.at(identifiers[1]).name() == "second");
  CHECK(operations.at(identifiers[2]).name() == "third");
  CHECK(operations.at("first").id() == identifiers[0]);
  CHECK(operations.at("second").id() == identifiers[1]);
  CHECK(operations.at("third").id() == identifiers[2]);
}

TEST_CASE(
  "operation_registry appends a batch after existing operations"
) {
  sivra::ir::operation_registry operations;
  const auto existing = operations.register_operation("existing");
  const std::array registrations{
    sivra::ir::operation_registration{.name = "first"},
    sivra::ir::operation_registration{.name = "second"},
  };

  const auto identifiers = operations.register_operations(registrations);

  REQUIRE(identifiers.size() == registrations.size());
  CHECK(existing == sivra::ir::operation_id(0));
  CHECK(identifiers[0] == sivra::ir::operation_id(1));
  CHECK(identifiers[1] == sivra::ir::operation_id(2));
  CHECK(operations.at(existing).name() == "existing");
}

TEST_CASE(
  "operation_registry preserves batch semantics"
) {
  sivra::ir::operation_registry operations;
  const std::array registrations{
    sivra::ir::operation_registration{
      .name = "identity",
      .semantics =
        sivra::ir::operation_semantics{
          .traits = sivra::ir::operation_trait::associative,
          .identity = sivra::ir::operation_constant{sivra::ir::well_known_constant::one},
          .notes = "identity operation",
        },
    },
    sivra::ir::operation_registration{
      .name = "annihilator",
      .semantics =
        sivra::ir::operation_semantics{
          .traits = sivra::ir::operation_trait::commutative,
          .annihilator = sivra::ir::operation_constant{sivra::ir::i32_constant::from_value(-1)},
          .notes = "annihilator operation",
        },
    },
  };

  const auto identifiers = operations.register_operations(registrations);

  const auto& identity = operations.at(identifiers[0]);
  CHECK(identity.has_trait(sivra::ir::operation_trait::associative));
  REQUIRE(identity.semantics().identity.has_value());
  CHECK(
    std::get<sivra::ir::well_known_constant>(identity.semantics().identity->element) ==
    sivra::ir::well_known_constant::one
  );
  CHECK(identity.semantics().notes == "identity operation");

  const auto& annihilator = operations.at(identifiers[1]);
  CHECK(annihilator.has_trait(sivra::ir::operation_trait::commutative));
  REQUIRE(annihilator.semantics().annihilator.has_value());
  const auto& annihilator_value =
    std::get<sivra::ir::scalar_constant_t>(annihilator.semantics().annihilator->element);
  CHECK(
    std::get<sivra::ir::i32_constant>(annihilator_value) == sivra::ir::i32_constant::from_value(-1)
  );
  CHECK(annihilator.semantics().notes == "annihilator operation");
}

TEST_CASE(
  "operation_registry rejects an existing name atomically"
) {
  sivra::ir::operation_registry operations;
  const auto existing = operations.register_operation("existing");
  const auto size_before = operations.operations().size();
  const std::array registrations{
    sivra::ir::operation_registration{.name = "first"},
    sivra::ir::operation_registration{.name = "existing"},
    sivra::ir::operation_registration{.name = "last"},
  };

  CHECK_THROWS_AS(operations.register_operations(registrations), std::invalid_argument);
  CHECK(operations.operations().size() == size_before);
  CHECK(operations.at("existing").id() == existing);
  CHECK(!operations.contains("first"));
  CHECK(!operations.contains("last"));
}

TEST_CASE(
  "operation_registry rejects an internal duplicate atomically"
) {
  sivra::ir::operation_registry operations;
  const std::array registrations{
    sivra::ir::operation_registration{.name = "first"},
    sivra::ir::operation_registration{.name = "duplicate"},
    sivra::ir::operation_registration{.name = "duplicate"},
    sivra::ir::operation_registration{.name = "last"},
  };

  CHECK_THROWS_AS(operations.register_operations(registrations), std::invalid_argument);
  CHECK(operations.operations().empty());
  CHECK(!operations.contains("first"));
  CHECK(!operations.contains("duplicate"));
  CHECK(!operations.contains("last"));
}

TEST_CASE(
  "operation_registry accepts an empty batch without changes"
) {
  sivra::ir::operation_registry operations;
  const auto existing = operations.register_operation("existing");
  const auto size_before = operations.operations().size();
  const std::span<const sivra::ir::operation_registration> registrations;

  const auto identifiers = operations.register_operations(registrations);

  CHECK(identifiers.empty());
  CHECK(operations.operations().size() == size_before);
  CHECK(operations.at("existing").id() == existing);
}

TEST_CASE(
  "operation_registry single registration preserves semantics"
) {
  sivra::ir::operation_registry operations;
  const auto identifier = operations.register_operation(
    "single",
    sivra::ir::operation_semantics{
      .traits = sivra::ir::operation_trait::idempotent,
      .identity = sivra::ir::operation_constant{sivra::ir::well_known_constant::zero},
      .notes = "single operation",
    }
  );

  const auto& operation = operations.at(identifier);
  CHECK(identifier == sivra::ir::operation_id(0));
  CHECK(operation.name() == "single");
  CHECK(operation.has_trait(sivra::ir::operation_trait::idempotent));
  REQUIRE(operation.semantics().identity.has_value());
  CHECK(
    std::get<sivra::ir::well_known_constant>(operation.semantics().identity->element) ==
    sivra::ir::well_known_constant::zero
  );
  CHECK(operation.semantics().notes == "single operation");
}
