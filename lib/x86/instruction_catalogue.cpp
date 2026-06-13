#include <sivra/x86/instruction_catalogue.hpp>

#include <array>
#include <utility>

namespace {

using sivra::program::operand_access;
using sivra::program::operand_constraint;
using sivra::program::operand_constraint_kind;

operand_constraint xmm_rw() {
  return {
    .kind = operand_constraint_kind::register_operand,
    .access = operand_access::read_write,
    .width = 128,
    .register_class = "x86.xmm",
  };
}

operand_constraint xmm_write() {
  return {
    .kind = operand_constraint_kind::register_operand,
    .access = operand_access::write,
    .width = 128,
    .register_class = "x86.xmm",
  };
}

operand_constraint xmm_read() {
  return {
    .kind = operand_constraint_kind::register_operand,
    .access = operand_access::read,
    .width = 128,
    .register_class = "x86.xmm",
  };
}

operand_constraint xmm_or_memory(
  std::uint32_t width
) {
  return {
    .kind = operand_constraint_kind::register_or_memory,
    .access = operand_access::read,
    .width = width,
    .register_class = "x86.xmm",
  };
}

operand_constraint memory(
  std::uint32_t width
) {
  return {
    .kind = operand_constraint_kind::memory,
    .access = operand_access::write,
    .width = width,
  };
}

operand_constraint imm8() {
  return {
    .kind = operand_constraint_kind::immediate,
    .access = operand_access::read,
    .width = 8,
    .immediate_width = 8,
  };
}

} // namespace

namespace sivra::x86 {

builtin_instruction_catalogue builtin_sse1_instruction_catalogue() {
  static const auto catalogue = [] {
    program::instruction_catalogue_builder builder;
    const auto add = [&](
                       std::string key,
                       std::string mnemonic,
                       std::vector<operand_constraint> operands,
                       std::string semantic_key
                     ) {
      return *builder.register_form(
        std::move(key), std::move(mnemonic), std::move(operands), std::move(semantic_key)
      );
    };

    builtin_instruction_ids ids{
      .addps = add("sse.addps.xmm.xmm_m128", "addps", {xmm_rw(), xmm_or_memory(128)}, "sse.addps"),
      .addss = add("sse.addss.xmm.xmm_m32", "addss", {xmm_rw(), xmm_or_memory(32)}, "sse.addss"),
      .subps = add("sse.subps.xmm.xmm_m128", "subps", {xmm_rw(), xmm_or_memory(128)}, "sse.subps"),
      .subss = add("sse.subss.xmm.xmm_m32", "subss", {xmm_rw(), xmm_or_memory(32)}, "sse.subss"),
      .mulps = add("sse.mulps.xmm.xmm_m128", "mulps", {xmm_rw(), xmm_or_memory(128)}, "sse.mulps"),
      .mulss = add("sse.mulss.xmm.xmm_m32", "mulss", {xmm_rw(), xmm_or_memory(32)}, "sse.mulss"),
      .divps = add("sse.divps.xmm.xmm_m128", "divps", {xmm_rw(), xmm_or_memory(128)}, "sse.divps"),
      .divss = add("sse.divss.xmm.xmm_m32", "divss", {xmm_rw(), xmm_or_memory(32)}, "sse.divss"),
      .minps = add("sse.minps.xmm.xmm_m128", "minps", {xmm_rw(), xmm_or_memory(128)}, "sse.minps"),
      .minss = add("sse.minss.xmm.xmm_m32", "minss", {xmm_rw(), xmm_or_memory(32)}, "sse.minss"),
      .maxps = add("sse.maxps.xmm.xmm_m128", "maxps", {xmm_rw(), xmm_or_memory(128)}, "sse.maxps"),
      .maxss = add("sse.maxss.xmm.xmm_m32", "maxss", {xmm_rw(), xmm_or_memory(32)}, "sse.maxss"),
      .sqrtps =
        add("sse.sqrtps.xmm.xmm_m128", "sqrtps", {xmm_write(), xmm_or_memory(128)}, "sse.sqrtps"),
      .sqrtss =
        add("sse.sqrtss.xmm.xmm_m32", "sqrtss", {xmm_rw(), xmm_or_memory(32)}, "sse.sqrtss"),
      .rcpps =
        add("sse.rcpps.xmm.xmm_m128", "rcpps", {xmm_write(), xmm_or_memory(128)}, "sse.rcpps"),
      .rcpss = add("sse.rcpss.xmm.xmm_m32", "rcpss", {xmm_rw(), xmm_or_memory(32)}, "sse.rcpss"),
      .rsqrtps = add(
        "sse.rsqrtps.xmm.xmm_m128", "rsqrtps", {xmm_write(), xmm_or_memory(128)}, "sse.rsqrtps"
      ),
      .rsqrtss =
        add("sse.rsqrtss.xmm.xmm_m32", "rsqrtss", {xmm_rw(), xmm_or_memory(32)}, "sse.rsqrtss"),
      .andps = add("sse.andps.xmm.xmm_m128", "andps", {xmm_rw(), xmm_or_memory(128)}, "sse.andps"),
      .andnps =
        add("sse.andnps.xmm.xmm_m128", "andnps", {xmm_rw(), xmm_or_memory(128)}, "sse.andnps"),
      .orps = add("sse.orps.xmm.xmm_m128", "orps", {xmm_rw(), xmm_or_memory(128)}, "sse.orps"),
      .xorps = add("sse.xorps.xmm.xmm_m128", "xorps", {xmm_rw(), xmm_or_memory(128)}, "sse.xorps"),
      .movaps_load = add(
        "sse.movaps.xmm.xmm_m128", "movaps", {xmm_write(), xmm_or_memory(128)}, "sse.movaps.load"
      ),
      .movaps_store =
        add("sse.movaps.m128.xmm", "movaps", {memory(128), xmm_read()}, "sse.movaps.store"),
      .movups_load = add(
        "sse.movups.xmm.xmm_m128", "movups", {xmm_write(), xmm_or_memory(128)}, "sse.movups.load"
      ),
      .movups_store =
        add("sse.movups.m128.xmm", "movups", {memory(128), xmm_read()}, "sse.movups.store"),
      .movss_load =
        add("sse.movss.xmm.xmm_m32", "movss", {xmm_rw(), xmm_or_memory(32)}, "sse.movss.load"),
      .movss_store = add("sse.movss.m32.xmm", "movss", {memory(32), xmm_read()}, "sse.movss.store"),
      .shufps = add(
        "sse.shufps.xmm.xmm_m128.imm8",
        "shufps",
        {xmm_rw(), xmm_or_memory(128), imm8()},
        "sse.shufps"
      ),
      .unpcklps = add(
        "sse.unpcklps.xmm.xmm_m128", "unpcklps", {xmm_rw(), xmm_or_memory(128)}, "sse.unpcklps"
      ),
      .unpckhps = add(
        "sse.unpckhps.xmm.xmm_m128", "unpckhps", {xmm_rw(), xmm_or_memory(128)}, "sse.unpckhps"
      ),
    };

    return builtin_instruction_catalogue{
      .catalogue = *std::move(builder).freeze(),
      .ids = ids,
    };
  }();
  return catalogue;
}

} // namespace sivra::x86
