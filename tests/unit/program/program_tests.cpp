#include <sivra/program/program.hpp>

#include <doctest/doctest.h>

#include <array>

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

TEST_CASE(
  "decoded program builder records reciprocal control-flow edges"
) {
  sivra::program::instruction_catalogue_builder forms;
  const auto form = forms.register_form("test.nop", "nop", {}, "test.nop");
  REQUIRE(form.has_value());

  sivra::program::decoded_program_builder builder(sivra::program::architecture_id("test"));
  const auto function = builder.add_function("entry");
  REQUIRE(function.has_value());
  const auto source = builder.add_block(*function);
  REQUIRE(source.has_value());
  const auto target = builder.add_block(*function);
  REQUIRE(target.has_value());
  REQUIRE(builder.add_instruction(*source, *form, {}).has_value());
  REQUIRE(builder.add_instruction(*target, *form, {}).has_value());

  REQUIRE(builder.add_edge(*source, *target).has_value());
  CHECK(!builder.add_edge(*source, *target).has_value());

  auto program = std::move(builder).freeze();

  REQUIRE(program.has_value());
  CHECK(program->block(*source).successors.front() == *target);
  CHECK(program->block(*target).predecessors.front() == *source);
}

TEST_CASE(
  "decoded program validation rejects functions without blocks"
) {
  sivra::program::decoded_program_builder builder(sivra::program::architecture_id("test"));
  const auto function = builder.add_function("entry");
  REQUIRE(function.has_value());

  const auto program = std::move(builder).freeze();

  REQUIRE(!program.has_value());
  CHECK(program.error().front().code == "program.decoded_program.invalid_function");
}

TEST_CASE(
  "instruction operand validation checks count kind and width"
) {
  sivra::program::instruction_catalogue_builder forms;
  const auto form = forms.register_form(
    "test.load",
    "load",
    {
      sivra::program::operand_constraint{
        .kind = sivra::program::operand_constraint_kind::register_operand,
        .access = sivra::program::operand_access::write,
        .width = 128,
        .register_class = "test.vector",
      },
      sivra::program::operand_constraint{
        .kind = sivra::program::operand_constraint_kind::memory,
        .access = sivra::program::operand_access::read,
        .width = 128,
      },
    },
    "test.load"
  );
  REQUIRE(form.has_value());
  auto catalogue = std::move(forms).freeze();
  REQUIRE(catalogue.has_value());
  const auto& definition = (*catalogue)->form(*form);

  const auto wrong_count = sivra::program::validate_instruction_operands(definition, {});
  REQUIRE(!wrong_count.has_value());
  CHECK(wrong_count.error().front().code == "program.instruction_form.invalid_operand_count");

  const std::array wrong_kind{
    sivra::program::operand(sivra::program::immediate_operand{.bits = 0, .width = 8}),
    sivra::program::operand(sivra::program::memory_operand{.width = 128}),
  };
  const auto wrong_kind_result =
    sivra::program::validate_instruction_operands(definition, wrong_kind);
  REQUIRE(!wrong_kind_result.has_value());
  CHECK(wrong_kind_result.error().front().code == "program.instruction_form.invalid_operand");

  const std::array wrong_width{
    sivra::program::operand(
      sivra::program::register_operand{
        .reg = sivra::program::register_id::unsafe_from_index(0, {}),
        .slice = {.offset = 0, .width = 32},
      }
    ),
    sivra::program::operand(sivra::program::memory_operand{.width = 128}),
  };
  const auto wrong_width_result =
    sivra::program::validate_instruction_operands(definition, wrong_width);
  REQUIRE(!wrong_width_result.has_value());
  CHECK(
    wrong_width_result.error().front().code == "program.instruction_form.invalid_operand_width"
  );
}
