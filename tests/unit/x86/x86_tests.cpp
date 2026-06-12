#include <sivra/x86/x86.hpp>

#include <doctest/doctest.h>

#if defined(__SSE__)
#include <xmmintrin.h>
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

namespace {

using lane_bits_t = std::array<std::uint32_t, 4>;

float as_float(
  std::uint32_t bits
) {
  return std::bit_cast<float>(bits);
}

std::uint32_t as_bits(
  float value
) {
  return std::bit_cast<std::uint32_t>(value);
}

std::uint32_t eval_lane(
  const sivra::program::lane_expression& expression,
  const lane_bits_t& old_destination,
  const lane_bits_t& source
) {
  const auto read = [&](const sivra::program::lane_operand_ref& input) {
    return input.role == sivra::program::lane_operand_role::old_destination
             ? old_destination[input.lane]
             : source[input.lane];
  };
  using enum sivra::program::lane_operation;
  switch (expression.operation) {
  case zero:
    return 0;
  case copy:
    return read(expression.inputs.front());
  case add_f32:
    return as_bits(as_float(read(expression.inputs[0])) + as_float(read(expression.inputs[1])));
  case subtract_f32:
    return as_bits(as_float(read(expression.inputs[0])) - as_float(read(expression.inputs[1])));
  case multiply_f32:
    return as_bits(as_float(read(expression.inputs[0])) * as_float(read(expression.inputs[1])));
  case divide_f32:
    return as_bits(as_float(read(expression.inputs[0])) / as_float(read(expression.inputs[1])));
  case minimum_f32:
    return as_bits(
      std::min(as_float(read(expression.inputs[0])), as_float(read(expression.inputs[1])))
    );
  case maximum_f32:
    return as_bits(
      std::max(as_float(read(expression.inputs[0])), as_float(read(expression.inputs[1])))
    );
  case sqrt_f32:
    return as_bits(std::sqrt(as_float(read(expression.inputs.front()))));
  case reciprocal_f32:
    return as_bits(1.0F / as_float(read(expression.inputs.front())));
  case reciprocal_sqrt_f32:
    return as_bits(1.0F / std::sqrt(as_float(read(expression.inputs.front()))));
  case bit_and:
    return read(expression.inputs[0]) & read(expression.inputs[1]);
  case bit_and_not:
    return ~read(expression.inputs[0]) & read(expression.inputs[1]);
  case bit_or:
    return read(expression.inputs[0]) | read(expression.inputs[1]);
  case bit_xor:
    return read(expression.inputs[0]) ^ read(expression.inputs[1]);
  }
  return 0;
}

lane_bits_t evaluate_written_vector(
  const sivra::program::instruction_semantics& semantics,
  const lane_bits_t& old_destination,
  const lane_bits_t& source
) {
  const auto write_it = std::ranges::find_if(semantics.effects, [](const auto& effect) {
    return std::holds_alternative<sivra::program::semantic_write>(effect);
  });
  REQUIRE(write_it != semantics.effects.end());
  const auto& write = std::get<sivra::program::semantic_write>(*write_it);
  const auto& value = std::get<sivra::program::vector_value>(write.value);
  lane_bits_t result{};
  for (std::size_t lane = 0; lane < value.lanes.size(); ++lane) {
    result[lane] = eval_lane(value.lanes[lane], old_destination, source);
  }
  return result;
}

sivra::program::decoded_instruction decode_instruction(
  std::string_view text
) {
  const sivra::x86::tokenizer tokenizer;
  const sivra::x86::parser parser;
  const auto tokens = tokenizer.tokenize(sivra::core::source_id::from_index(0), text);
  REQUIRE(tokens.has_value());
  const auto parsed = parser.parse(*tokens);
  REQUIRE(parsed.has_value());
  sivra::x86::form_resolver resolver(
    sivra::x86::builtin_register_catalogue(),
    sivra::x86::builtin_sse1_instruction_catalogue().catalogue
  );
  const auto program = resolver.resolve(*parsed);
  REQUIRE(program.has_value());
  REQUIRE(program->instructions().size() == 1);
  const auto instructions = program->instructions();
  return instructions.front();
}

sivra::program::instruction_semantics semantics_for(
  std::string_view text
) {
  sivra::x86::semantic_provider provider;
  const auto instruction = decode_instruction(text);
  auto semantics = provider.semantics(instruction);
  REQUIRE(semantics.has_value());
  return *semantics;
}

#if defined(__SSE__)
lane_bits_t store_bits(
  __m128 value
) {
  alignas(16) std::array<float, 4> stored{};
  _mm_storeu_ps(stored.data(), value);
  return {
    as_bits(stored[0]),
    as_bits(stored[1]),
    as_bits(stored[2]),
    as_bits(stored[3]),
  };
}

__m128 load_bits(
  const lane_bits_t& bits
) {
  const std::array values{
    as_float(bits[0]), as_float(bits[1]), as_float(bits[2]), as_float(bits[3])};
  return _mm_loadu_ps(values.data());
}
#endif

} // namespace

TEST_CASE(
  "x86 register catalogue exposes SSE and x86-64 address registers"
) {
  const auto registers = sivra::x86::builtin_register_catalogue();
  const auto* xmm0 = registers->find("xmm0");
  const auto* xmm15 = registers->find("xmm15");
  const auto* rax = registers->find("rax");
  const auto* r8d = registers->find("r8d");

  REQUIRE(xmm0 != nullptr);
  REQUIRE(xmm15 != nullptr);
  REQUIRE(rax != nullptr);
  REQUIRE(r8d != nullptr);
  CHECK(xmm0->definition.width == 128);
  CHECK(xmm15->definition.width == 128);
  CHECK(rax->definition.width == 64);
  CHECK(r8d->definition.width == 32);
}

TEST_CASE(
  "x86 SSE1 catalogue declares supported forms with stable form ids"
) {
  const auto instructions = sivra::x86::builtin_sse1_instruction_catalogue();

  CHECK(instructions.catalogue->forms().size() == 31);
  CHECK(instructions.catalogue->find("sse.addps.xmm.xmm_m128")->id == instructions.ids.addps);
  CHECK(instructions.catalogue->find("sse.movss.m32.xmm")->id == instructions.ids.movss_store);
  CHECK(
    instructions.catalogue->find("sse.shufps.xmm.xmm_m128.imm8")->id == instructions.ids.shufps
  );
}

TEST_CASE(
  "x86 every declared SSE1 form resolves to its exact form id and supported semantics"
) {
  const auto instructions = sivra::x86::builtin_sse1_instruction_catalogue();
  struct form_case {
    std::string_view text;
    sivra::program::instruction_form_id id;
  };
  const std::vector<form_case> cases{
    {"addps xmm0, xmm1", instructions.ids.addps},
    {"addss xmm0, xmm1", instructions.ids.addss},
    {"subps xmm0, xmm1", instructions.ids.subps},
    {"subss xmm0, xmm1", instructions.ids.subss},
    {"mulps xmm0, xmm1", instructions.ids.mulps},
    {"mulss xmm0, xmm1", instructions.ids.mulss},
    {"divps xmm0, xmm1", instructions.ids.divps},
    {"divss xmm0, xmm1", instructions.ids.divss},
    {"minps xmm0, xmm1", instructions.ids.minps},
    {"minss xmm0, xmm1", instructions.ids.minss},
    {"maxps xmm0, xmm1", instructions.ids.maxps},
    {"maxss xmm0, xmm1", instructions.ids.maxss},
    {"sqrtps xmm0, xmm1", instructions.ids.sqrtps},
    {"sqrtss xmm0, xmm1", instructions.ids.sqrtss},
    {"rcpps xmm0, xmm1", instructions.ids.rcpps},
    {"rcpss xmm0, xmm1", instructions.ids.rcpss},
    {"rsqrtps xmm0, xmm1", instructions.ids.rsqrtps},
    {"rsqrtss xmm0, xmm1", instructions.ids.rsqrtss},
    {"andps xmm0, xmm1", instructions.ids.andps},
    {"andnps xmm0, xmm1", instructions.ids.andnps},
    {"orps xmm0, xmm1", instructions.ids.orps},
    {"xorps xmm0, xmm1", instructions.ids.xorps},
    {"movaps xmm0, xmm1", instructions.ids.movaps_load},
    {"movaps [rax], xmm1", instructions.ids.movaps_store},
    {"movups xmm0, xmm1", instructions.ids.movups_load},
    {"movups [rax], xmm1", instructions.ids.movups_store},
    {"movss xmm0, xmm1", instructions.ids.movss_load},
    {"movss [rax], xmm1", instructions.ids.movss_store},
    {"shufps xmm0, xmm1, 0x1b", instructions.ids.shufps},
    {"unpcklps xmm0, xmm1", instructions.ids.unpcklps},
    {"unpckhps xmm0, xmm1", instructions.ids.unpckhps},
  };

  const sivra::x86::semantic_provider provider;
  for (const auto& entry : cases) {
    CAPTURE(entry.text);
    const auto instruction = decode_instruction(entry.text);
    CHECK(instruction.form == entry.id);
    const auto semantics = provider.semantics(instruction);
    REQUIRE(semantics.has_value());
    CHECK(!semantics->unsupported);
    CHECK(semantics->form == entry.id);
    CHECK(!semantics->effects.empty());
  }
}

TEST_CASE(
  "x86 tokenizer parser and resolver produce decoded instructions from text"
) {
  const sivra::x86::tokenizer tokenizer;
  const sivra::x86::parser parser;
  const auto tokens =
    tokenizer.tokenize(sivra::core::source_id::from_index(4), "shufps xmm0, xmm1, 0x1b\n");
  REQUIRE(tokens.has_value());
  CHECK(tokens->front().source.begin.byte_offset == 0);
  CHECK(tokens->front().source.end.byte_offset == 6);
  const auto parsed = parser.parse(*tokens);
  REQUIRE(parsed.has_value());
  REQUIRE(parsed->size() == 1);
  CHECK(parsed->front().mnemonic == "shufps");
  CHECK(parsed->front().operands.size() == 3);

  sivra::x86::form_resolver resolver(
    sivra::x86::builtin_register_catalogue(),
    sivra::x86::builtin_sse1_instruction_catalogue().catalogue
  );
  const auto program = resolver.resolve(*parsed);
  REQUIRE(program.has_value());
  REQUIRE(program->instructions().size() == 1);
  CHECK(
    program->instructions().front().form ==
    sivra::x86::builtin_sse1_instruction_catalogue().ids.shufps
  );
}

TEST_CASE(
  "x86 resolver accepts x86-64 GPR memory bases for SSE1 operands"
) {
  const sivra::x86::tokenizer tokenizer;
  const sivra::x86::parser parser;
  const auto tokens = tokenizer.tokenize(sivra::core::source_id::from_index(0), "addps xmm0, [r8]");
  REQUIRE(tokens.has_value());
  const auto parsed = parser.parse(*tokens);
  REQUIRE(parsed.has_value());
  sivra::x86::form_resolver resolver(
    sivra::x86::builtin_register_catalogue(),
    sivra::x86::builtin_sse1_instruction_catalogue().catalogue
  );

  const auto program = resolver.resolve(*parsed);

  REQUIRE(program.has_value());
  const auto instructions = program->instructions();
  const auto& source = std::get<sivra::program::memory_operand>(instructions.front().operands[1]);
  CHECK(source.width == 128);
}

TEST_CASE(
  "x86 resolver rejects near-miss invalid SSE forms"
) {
  const sivra::x86::tokenizer tokenizer;
  const sivra::x86::parser parser;
  const auto tokens =
    tokenizer.tokenize(sivra::core::source_id::from_index(0), "addps [rax], xmm0");
  REQUIRE(tokens.has_value());
  const auto parsed = parser.parse(*tokens);
  REQUIRE(parsed.has_value());
  sivra::x86::form_resolver resolver(
    sivra::x86::builtin_register_catalogue(),
    sivra::x86::builtin_sse1_instruction_catalogue().catalogue
  );

  const auto program = resolver.resolve(*parsed);

  CHECK(!program.has_value());
  CHECK(program.error().front().code == "x86.resolver.unsupported_form");
}

#if defined(__SSE__)
TEST_CASE(
  "x86 SSE arithmetic semantics match native packed and scalar intrinsics"
) {
  const lane_bits_t old_destination{as_bits(8.0F), as_bits(6.0F), as_bits(4.0F), as_bits(2.0F)};
  const lane_bits_t source{as_bits(2.0F), as_bits(3.0F), as_bits(4.0F), as_bits(5.0F)};
  const auto old_native = load_bits(old_destination);
  const auto source_native = load_bits(source);

  CHECK(
    evaluate_written_vector(semantics_for("addps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_add_ps(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("subps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_sub_ps(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("mulps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_mul_ps(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("divps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_div_ps(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("sqrtps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_sqrt_ps(source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("addss xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_add_ss(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("mulss xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_mul_ss(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("sqrtss xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_move_ss(old_native, _mm_sqrt_ss(source_native)))
  );
}

TEST_CASE(
  "x86 SSE bitwise shuffle unpack and move semantics match native intrinsics"
) {
  const lane_bits_t old_destination{0x3F80'0000U, 0x4000'0000U, 0x4040'0000U, 0x4080'0000U};
  const lane_bits_t source{0x40A0'0000U, 0x40C0'0000U, 0x40E0'0000U, 0x4100'0000U};
  const auto old_native = load_bits(old_destination);
  const auto source_native = load_bits(source);
  alignas(16) float scalar = as_float(source[0]);

  CHECK(
    evaluate_written_vector(semantics_for("andps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_and_ps(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("andnps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_andnot_ps(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("orps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_or_ps(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("xorps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_xor_ps(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("shufps xmm0, xmm1, 0x1b"), old_destination, source) ==
    store_bits(_mm_shuffle_ps(old_native, source_native, 0x1B))
  );
  CHECK(
    evaluate_written_vector(semantics_for("unpcklps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_unpacklo_ps(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("unpckhps xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_unpackhi_ps(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("movaps xmm0, xmm1"), old_destination, source) ==
    store_bits(source_native)
  );
  CHECK(
    evaluate_written_vector(semantics_for("movss xmm0, xmm1"), old_destination, source) ==
    store_bits(_mm_move_ss(old_native, source_native))
  );
  CHECK(
    evaluate_written_vector(semantics_for("movss xmm0, [rax]"), old_destination, source) ==
    store_bits(_mm_load_ss(&scalar))
  );
}
#endif

TEST_CASE(
  "x86 memory source and store semantics expose precise memory effects"
) {
  const auto load = semantics_for("addps xmm0, [rax]");
  REQUIRE(load.effects.size() == 2);
  const auto* read = std::get_if<sivra::program::memory_read_effect>(&load.effects.front());
  REQUIRE(read != nullptr);
  CHECK(read->width == 128);

  const auto store = semantics_for("movss [rax], xmm1");
  REQUIRE(store.effects.size() == 1);
  const auto* write = std::get_if<sivra::program::memory_write_effect>(&store.effects.front());
  REQUIRE(write != nullptr);
  CHECK(write->width == 32);
  const auto& stored = std::get<sivra::program::vector_value>(write->value);
  REQUIRE(stored.lanes.size() == 1);
  CHECK(stored.lanes.front().inputs.front().operand_index == 1);
  CHECK(stored.lanes.front().inputs.front().lane == 0);
}
