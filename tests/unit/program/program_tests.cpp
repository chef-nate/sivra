#include <sivra/program/program.hpp>

#include <doctest/doctest.h>

TEST_CASE(
  "program bit ranges validate containment and overlap"
) {
  const sivra::program::bit_range full{.offset = 0, .width = 128};
  const sivra::program::bit_range lane{.offset = 32, .width = 32};
  const sivra::program::bit_range upper{.offset = 96, .width = 32};

  CHECK(full.validate().has_value());
  CHECK(!sivra::program::bit_range{.offset = 0, .width = 0}.validate().has_value());
  CHECK(full.contains(lane));
  CHECK(full.contains(upper));
  CHECK(lane.overlaps({.offset = 48, .width = 16}));
  CHECK(!lane.overlaps(upper));
}

TEST_CASE(
  "decoded program builder creates validated owner-scoped containers"
) {
  sivra::program::instruction_catalogue_builder forms;
  const auto form = forms.register_form("test.nop", "nop", {}, "test.nop");
  REQUIRE(form.has_value());

  sivra::program::decoded_program_builder builder(sivra::program::architecture_id("test"));
  const auto function = builder.add_function("entry");
  REQUIRE(function.has_value());
  const auto block = builder.add_block(*function);
  REQUIRE(block.has_value());
  const auto instruction = builder.add_instruction(*block, *form, {});
  REQUIRE(instruction.has_value());

  auto program = std::move(builder).freeze();

  REQUIRE(program.has_value());
  CHECK(program->validate().has_value());
  CHECK(program->functions().size() == 1);
  CHECK(program->blocks().size() == 1);
  CHECK(program->instructions().size() == 1);
  CHECK(program->block(*block).instructions.front() == *instruction);
}
