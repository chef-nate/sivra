#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/expression_graph.hpp>
#include <sivra/ir/graph_builder.hpp>
#include <sivra/recovery/recovery.hpp>
#include <sivra/x86/x86.hpp>

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace {

struct recovery_fixture {
  sivra::x86::semantic_provider provider;
  sivra::program::decoded_program program;
  sivra::recovery::state_index state;
  std::shared_ptr<const sivra::ir::operation_catalogue> operations;
  sivra::ir::expression_graph graph;
  sivra::ir::graph_builder builder;
  sivra::recovery::object_annotation_set annotations;
  sivra::recovery::conservative_memory_alias_analysis alias_analysis;
  sivra::recovery::provenance_store provenance;
  sivra::recovery::recovery_engine engine;

  explicit recovery_fixture(
    sivra::program::decoded_program decoded
  )
      : program(std::move(decoded)),
        state(*sivra::recovery::state_index_builder::build(
          program,
          provider
        )),
        operations(make_operations()),
        graph(operations),
        builder(graph),
        engine(
          program,
          provider,
          state,
          annotations,
          alias_analysis,
          graph,
          builder,
          provenance
        ) {}

  static std::shared_ptr<const sivra::ir::operation_catalogue> make_operations() {
    sivra::ir::operation_catalogue_builder operation_builder;
    const auto ids = sivra::ir::register_builtin_operations(operation_builder);
    REQUIRE(ids.has_value());
    auto catalogue = std::move(operation_builder).freeze();
    REQUIRE(catalogue.has_value());
    return *catalogue;
  }
};

sivra::program::decoded_program decode_program(
  std::string_view assembly
) {
  const sivra::x86::tokenizer tokenizer;
  const sivra::x86::parser parser;
  auto tokens = tokenizer.tokenize(sivra::core::source_id::from_index(0), assembly);
  REQUIRE(tokens.has_value());
  auto parsed = parser.parse(*tokens);
  REQUIRE(parsed.has_value());

  sivra::x86::form_resolver resolver(
    sivra::x86::builtin_register_catalogue(),
    sivra::x86::builtin_sse1_instruction_catalogue().catalogue
  );
  auto program = resolver.resolve(*parsed);
  REQUIRE(program.has_value());
  return std::move(*program);
}

std::string read_fixture(
  std::string_view name
) {
  const auto path = std::filesystem::path(SIVRA_ASM_FIXTURE_DIRECTORY) / name;
  std::ifstream stream(path);
  REQUIRE(stream);
  return {
    std::istreambuf_iterator<char>(stream),
    std::istreambuf_iterator<char>(),
  };
}

sivra::program::machine_location xmm_lane(
  const sivra::x86::semantic_provider& provider,
  std::string_view name,
  std::uint32_t lane
) {
  const auto* reg = provider.registers().find(name);
  REQUIRE(reg != nullptr);
  return sivra::program::register_slice{
    .reg = reg->definition.id,
    .bits = {.offset = lane * 32U, .width = 32},
    .lane =
      sivra::program::lane_descriptor{
        .index = lane,
        .element_width = 32,
        .lane_count = 4,
      },
  };
}

sivra::program::program_point after_last_instruction(
  const sivra::program::decoded_program& program
) {
  REQUIRE(program.blocks().size() == 1);
  const auto& block = program.blocks().front();
  REQUIRE(!block.instructions.empty());
  return {
    .block = block.id,
    .instruction = block.instructions.back(),
    .phase = sivra::program::point_phase::after,
  };
}

sivra::recovery::recovery_query lane_query(
  const recovery_fixture& fixture,
  std::string_view name,
  std::uint32_t lane
) {
  return {
    .location = xmm_lane(fixture.provider, name, lane),
    .point = after_last_instruction(fixture.program),
    .expected_type = sivra::ir::value_type::f32(),
  };
}

class cfg_semantic_provider final : public sivra::program::semantic_provider {
public:
  cfg_semantic_provider()
      : m_register_owner(sivra::core::owner_token_source::next()),
        m_form_owner(sivra::core::owner_token_source::next()),
        m_nop_form(
          sivra::program::instruction_form_id::unsafe_from_index(
            0,
            m_form_owner
          )
        ),
        m_zero_form(
          sivra::program::instruction_form_id::unsafe_from_index(
            0,
            m_form_owner
          )
        ),
        m_copy_form(
          sivra::program::instruction_form_id::unsafe_from_index(
            0,
            m_form_owner
          )
        ),
        m_add_form(
          sivra::program::instruction_form_id::unsafe_from_index(
            0,
            m_form_owner
          )
        ),
        m_load_form(
          sivra::program::instruction_form_id::unsafe_from_index(
            0,
            m_form_owner
          )
        ),
        m_store_form(
          sivra::program::instruction_form_id::unsafe_from_index(
            0,
            m_form_owner
          )
        ) {
    m_registers = {
      sivra::program::register_definition{
        .id = sivra::program::register_id::unsafe_from_index(0, m_register_owner),
        .key = "test.xmm0",
        .name = "xmm0",
        .width = 128,
      },
      sivra::program::register_definition{
        .id = sivra::program::register_id::unsafe_from_index(1, m_register_owner),
        .key = "test.xmm1",
        .name = "xmm1",
        .width = 128,
      },
    };
    m_nop_form = add_form("test.nop", "nop");
    m_zero_form = add_form("test.zero_xmm0", "zero_xmm0");
    m_copy_form = add_form("test.copy_xmm0", "copy_xmm0");
    m_add_form = add_form("test.add_xmm0", "add_xmm0");
    m_load_form = add_form("test.load_xmm0", "load_xmm0");
    m_store_form = add_form("test.store", "store");
  }

  [[nodiscard]] sivra::program::instruction_form_id nop_form() const { return m_nop_form; }
  [[nodiscard]] sivra::program::instruction_form_id zero_form() const { return m_zero_form; }
  [[nodiscard]] sivra::program::instruction_form_id copy_form() const { return m_copy_form; }
  [[nodiscard]] sivra::program::instruction_form_id add_form() const { return m_add_form; }
  [[nodiscard]] sivra::program::instruction_form_id load_form() const { return m_load_form; }
  [[nodiscard]] sivra::program::instruction_form_id store_form() const { return m_store_form; }
  [[nodiscard]] sivra::program::register_id xmm0() const { return m_registers[0].id; }
  [[nodiscard]] sivra::program::register_id xmm1() const { return m_registers[1].id; }

  [[nodiscard]] sivra::program::architecture_id architecture() const override {
    return sivra::program::architecture_id("test");
  }

  [[nodiscard]] sivra::program::architecture_profile_id profile() const override {
    return sivra::program::architecture_profile_id("cfg");
  }

  [[nodiscard]] const sivra::program::register_definition& register_definition(
    sivra::program::register_id id
  ) const override {
    if (id.owner() != m_register_owner || id.index() >= m_registers.size()) {
      throw std::out_of_range("test register id is invalid");
    }
    return m_registers[id.index()];
  }

  [[nodiscard]] const sivra::program::instruction_form_definition& form(
    sivra::program::instruction_form_id id
  ) const override {
    if (id.owner() != m_form_owner || id.index() >= m_forms.size()) {
      throw std::out_of_range("test instruction form id is invalid");
    }
    return m_forms[id.index()];
  }

  [[nodiscard]] sivra::core::result_t<sivra::program::instruction_semantics> semantics(
    const sivra::program::decoded_instruction& instruction
  ) const override {
    if (instruction.form == m_nop_form) {
      return sivra::program::instruction_semantics{.form = instruction.form};
    }
    if (instruction.form == m_zero_form) {
      return register_write(instruction.form, lane_vector(sivra::program::lane_operation::zero));
    }
    if (instruction.form == m_copy_form) {
      return register_write(instruction.form, lane_vector(sivra::program::lane_operation::copy));
    }
    if (instruction.form == m_add_form) {
      return register_write(instruction.form, add_vector());
    }
    if (instruction.form == m_load_form) {
      return register_write(instruction.form, lane_vector(sivra::program::lane_operation::copy));
    }
    if (instruction.form == m_store_form) {
      if (instruction.operands.empty()) {
        return sivra::core::fail<sivra::program::instruction_semantics>(
          "test.semantic.missing_operand", "store form requires a memory operand"
        );
      }
      const auto* memory = std::get_if<sivra::program::memory_operand>(&instruction.operands[0]);
      if (memory == nullptr) {
        return sivra::core::fail<sivra::program::instruction_semantics>(
          "test.semantic.invalid_operand", "store form requires a memory operand"
        );
      }
      return sivra::program::instruction_semantics{
        .form = instruction.form,
        .effects =
          {
            sivra::program::memory_write_effect{
              .address = *memory,
              .value = lane_vector(sivra::program::lane_operation::zero),
              .width = 128,
            },
          },
      };
    }
    return sivra::core::fail<sivra::program::instruction_semantics>(
      "test.semantic.invalid_form", "test instruction form is unknown"
    );
  }

  [[nodiscard]] sivra::program::location_relation relate(
    const sivra::program::machine_location& lhs,
    const sivra::program::machine_location& rhs
  ) const override {
    const auto* lhs_register = std::get_if<sivra::program::register_slice>(&lhs);
    const auto* rhs_register = std::get_if<sivra::program::register_slice>(&rhs);
    if (lhs_register == nullptr || rhs_register == nullptr) {
      return lhs == rhs ? sivra::program::location_relation::equal
                        : sivra::program::location_relation::disjoint;
    }
    if (lhs_register->reg != rhs_register->reg) {
      return sivra::program::location_relation::disjoint;
    }
    if (lhs_register->bits == rhs_register->bits) {
      return sivra::program::location_relation::equal;
    }
    if (lhs_register->bits.contains(rhs_register->bits)) {
      return sivra::program::location_relation::contains;
    }
    if (rhs_register->bits.contains(lhs_register->bits)) {
      return sivra::program::location_relation::contained_by;
    }
    return lhs_register->bits.overlaps(rhs_register->bits)
             ? sivra::program::location_relation::overlaps
             : sivra::program::location_relation::disjoint;
  }

private:
  [[nodiscard]] sivra::program::instruction_form_id add_form(
    std::string key,
    std::string mnemonic
  ) {
    const auto id = sivra::program::instruction_form_id::unsafe_from_index(
      static_cast<std::uint32_t>(m_forms.size()), m_form_owner
    );
    m_forms.push_back(
      {
        .id = id,
        .key = std::move(key),
        .mnemonic = std::move(mnemonic),
        .semantic_key = "",
      }
    );
    m_forms.back().semantic_key = m_forms.back().key;
    return id;
  }

  [[nodiscard]] sivra::program::register_slice destination() const {
    return {
      .reg = xmm0(),
      .bits = {.offset = 0, .width = 128},
    };
  }

  [[nodiscard]] static sivra::program::vector_value lane_vector(
    sivra::program::lane_operation operation
  ) {
    auto type = sivra::ir::value_type::vector(sivra::ir::scalar_category::floating_point, 32, 4);
    REQUIRE(type.has_value());
    std::vector<sivra::program::lane_expression> lanes;
    lanes.reserve(4);
    for (std::uint32_t lane = 0; lane < 4; ++lane) {
      std::vector<sivra::program::lane_operand_ref> inputs;
      if (operation == sivra::program::lane_operation::copy) {
        inputs.push_back({.role = sivra::program::lane_operand_role::source, .lane = lane});
      }
      lanes.push_back({.operation = operation, .inputs = std::move(inputs)});
    }
    return {.type = *type, .lanes = std::move(lanes)};
  }

  [[nodiscard]] static sivra::program::vector_value add_vector() {
    auto type = sivra::ir::value_type::vector(sivra::ir::scalar_category::floating_point, 32, 4);
    REQUIRE(type.has_value());
    std::vector<sivra::program::lane_expression> lanes;
    lanes.reserve(4);
    for (std::uint32_t lane = 0; lane < 4; ++lane) {
      lanes.push_back(
        {
          .operation = sivra::program::lane_operation::add_f32,
          .inputs =
            {
              {.role = sivra::program::lane_operand_role::old_destination, .lane = lane},
              {.role = sivra::program::lane_operand_role::source, .lane = lane},
            },
        }
      );
    }
    return {.type = *type, .lanes = std::move(lanes)};
  }

  [[nodiscard]] sivra::program::instruction_semantics register_write(
    sivra::program::instruction_form_id form,
    sivra::program::vector_value value
  ) const {
    return {
      .form = form,
      .effects =
        {
          sivra::program::semantic_write{
            .destination = destination(),
            .value = std::move(value),
          },
        },
    };
  }

  sivra::core::owner_token m_register_owner;
  sivra::core::owner_token m_form_owner;
  std::vector<sivra::program::register_definition> m_registers;
  std::vector<sivra::program::instruction_form_definition> m_forms;
  sivra::program::instruction_form_id m_nop_form;
  sivra::program::instruction_form_id m_zero_form;
  sivra::program::instruction_form_id m_copy_form;
  sivra::program::instruction_form_id m_add_form;
  sivra::program::instruction_form_id m_load_form;
  sivra::program::instruction_form_id m_store_form;
};

struct cfg_recovery_fixture {
  cfg_semantic_provider provider;
  sivra::program::decoded_program program;
  sivra::recovery::state_index state;
  std::shared_ptr<const sivra::ir::operation_catalogue> operations;
  sivra::ir::expression_graph graph;
  sivra::ir::graph_builder builder;
  sivra::recovery::object_annotation_set annotations;
  sivra::recovery::conservative_memory_alias_analysis alias_analysis;
  sivra::recovery::provenance_store provenance;
  sivra::recovery::recovery_engine engine;

  cfg_recovery_fixture(
    cfg_semantic_provider semantic_provider,
    sivra::program::decoded_program decoded
  )
      : provider(std::move(semantic_provider)),
        program(std::move(decoded)),
        state(*sivra::recovery::state_index_builder::build(
          program,
          provider
        )),
        operations(recovery_fixture::make_operations()),
        graph(operations),
        builder(graph),
        engine(
          program,
          provider,
          state,
          annotations,
          alias_analysis,
          graph,
          builder,
          provenance
        ) {}
};

sivra::program::register_operand cfg_register_operand(
  sivra::program::register_id reg
) {
  return {
    .reg = reg,
    .slice = {.offset = 0, .width = 128},
  };
}

sivra::program::machine_location cfg_xmm0_lane(
  const cfg_semantic_provider& provider,
  std::uint32_t lane
) {
  return sivra::program::register_slice{
    .reg = provider.xmm0(),
    .bits = {.offset = lane * 32U, .width = 32},
    .lane =
      sivra::program::lane_descriptor{
        .index = lane,
        .element_width = 32,
        .lane_count = 4,
      },
  };
}

sivra::program::program_point after_block(
  const sivra::program::decoded_program& program,
  sivra::program::basic_block_id block_id
) {
  const auto& block = program.block(block_id);
  REQUIRE(!block.instructions.empty());
  return {
    .block = block.id,
    .instruction = block.instructions.back(),
    .phase = sivra::program::point_phase::after,
  };
}

sivra::recovery::recovery_query cfg_lane_query(
  const cfg_recovery_fixture& fixture,
  sivra::program::basic_block_id block_id
) {
  return {
    .location = cfg_xmm0_lane(fixture.provider, 0),
    .point = after_block(fixture.program, block_id),
    .expected_type = sivra::ir::value_type::f32(),
  };
}

} // namespace

TEST_CASE(
  "recovery state index records register definitions and memory versions"
) {
  recovery_fixture fixture(decode_program("movaps [rax], xmm1\naddps xmm0, xmm2"));

  CHECK(fixture.state.definitions().size() == 1);
  CHECK(fixture.state.memory_transitions().size() == 1);
  const auto& store = fixture.program.instructions().front();
  const auto store_point = sivra::program::program_point{
    .block = fixture.program.blocks().front().id,
    .instruction = store.id,
    .phase = sivra::program::point_phase::after,
  };
  CHECK(
    fixture.state.memory_before(store_point).index() <
    fixture.state.memory_after(store_point).index()
  );
  CHECK(fixture.state.effects(fixture.program.instructions().back().id).size() == 1);
}

TEST_CASE(
  "recovery lowers straight-line packed arithmetic into faithful IR"
) {
  recovery_fixture fixture(decode_program("movaps xmm0, [rax]\naddps xmm0, xmm1"));

  const auto result = fixture.engine.recover(lane_query(fixture, "xmm0", 0));

  REQUIRE(result.root.has_value());
  CHECK(result.status == sivra::core::analysis_status::partial);
  const auto& node = fixture.graph.at(*result.root);
  const auto* operation = node.get_if_operation();
  REQUIRE(operation != nullptr);
  CHECK(fixture.graph.catalogue().operation(operation->operation).name() == "add");
  REQUIRE(operation->operands.size() == 2);
  CHECK(fixture.graph.at(operation->operands[0]).get_if_external_value() != nullptr);
  CHECK(fixture.graph.at(operation->operands[1]).get_if_external_value() != nullptr);
  REQUIRE(result.provenance.has_value());
  CHECK(fixture.provenance.at(*result.provenance).inputs.size() == 2);
}

TEST_CASE(
  "recovery preserves scalar upper lanes by querying the old destination"
) {
  recovery_fixture fixture(decode_program("addss xmm0, xmm1"));

  const auto result = fixture.engine.recover(lane_query(fixture, "xmm0", 2));

  REQUIRE(result.root.has_value());
  CHECK(fixture.graph.at(*result.root).get_if_external_value() != nullptr);
}

TEST_CASE(
  "recovery treats possible memory store dependencies as barriers"
) {
  recovery_fixture fixture(decode_program("movaps [rax], xmm1\nmovaps xmm0, [rax]"));

  const auto result = fixture.engine.recover(lane_query(fixture, "xmm0", 0));

  REQUIRE(result.root.has_value());
  CHECK(result.status == sivra::core::analysis_status::partial);
  CHECK(fixture.graph.at(*result.root).get_if_unknown() != nullptr);
  REQUIRE(!result.diagnostics.empty());
  CHECK(result.diagnostics.front().code == "recovery.memory.store_barrier");
}

TEST_CASE(
  "recovery memoizes repeated queries"
) {
  recovery_fixture fixture(decode_program("mulps xmm0, xmm1"));
  const auto query = lane_query(fixture, "xmm0", 1);

  const auto first = fixture.engine.recover(query);
  const auto second = fixture.engine.recover(query);

  REQUIRE(first.root.has_value());
  REQUIRE(second.root.has_value());
  CHECK(*first.root == *second.root);
  CHECK(fixture.engine.statistics().cache_hits == 1);
}

TEST_CASE(
  "recovery handles supported straight-line vector fixture from assembly"
) {
  recovery_fixture fixture(decode_program(read_fixture("vec4_supported_mix.asm")));

  const auto result = fixture.engine.recover(lane_query(fixture, "xmm0", 3));

  REQUIRE(result.root.has_value());
  const auto& root = fixture.graph.at(*result.root);
  const auto* operation = root.get_if_operation();
  REQUIRE(operation != nullptr);
  CHECK(fixture.graph.catalogue().operation(operation->operation).name() == "multiply");
  CHECK(result.status == sivra::core::analysis_status::partial);
}

TEST_CASE(
  "recovery handles supported scalar row fixture from assembly"
) {
  recovery_fixture fixture(decode_program(read_fixture("scalar_row_supported.asm")));

  const auto lane_zero = fixture.engine.recover(lane_query(fixture, "xmm0", 0));
  const auto lane_three = fixture.engine.recover(lane_query(fixture, "xmm0", 3));

  REQUIRE(lane_zero.root.has_value());
  const auto* lane_zero_operation = fixture.graph.at(*lane_zero.root).get_if_operation();
  REQUIRE(lane_zero_operation != nullptr);
  CHECK(fixture.graph.catalogue().operation(lane_zero_operation->operation).name() == "add");

  REQUIRE(lane_three.root.has_value());
  const auto* lane_three_constant = fixture.graph.at(*lane_three.root).get_if_constant();
  REQUIRE(lane_three_constant != nullptr);
  CHECK(lane_three_constant->value.element_count() == 1);
}

TEST_CASE(
  "recovery collapses structurally equal predecessor definitions at a join"
) {
  cfg_semantic_provider provider;
  sivra::program::decoded_program_builder program_builder(provider.architecture());
  const auto function = program_builder.add_function("entry");
  REQUIRE(function.has_value());
  const auto entry = program_builder.add_block(*function);
  const auto left = program_builder.add_block(*function);
  const auto right = program_builder.add_block(*function);
  const auto join = program_builder.add_block(*function);
  REQUIRE(entry.has_value());
  REQUIRE(left.has_value());
  REQUIRE(right.has_value());
  REQUIRE(join.has_value());
  REQUIRE(program_builder.add_instruction(*entry, provider.nop_form(), {}).has_value());
  REQUIRE(program_builder.add_instruction(*left, provider.zero_form(), {}).has_value());
  REQUIRE(program_builder.add_instruction(*right, provider.zero_form(), {}).has_value());
  REQUIRE(program_builder.add_instruction(*join, provider.nop_form(), {}).has_value());
  REQUIRE(program_builder.add_edge(*entry, *left).has_value());
  REQUIRE(program_builder.add_edge(*entry, *right).has_value());
  REQUIRE(program_builder.add_edge(*left, *join).has_value());
  REQUIRE(program_builder.add_edge(*right, *join).has_value());
  auto program = std::move(program_builder).freeze();
  REQUIRE(program.has_value());
  cfg_recovery_fixture fixture(std::move(provider), std::move(*program));

  const auto result = fixture.engine.recover(cfg_lane_query(fixture, *join));

  REQUIRE(result.root.has_value());
  CHECK(fixture.state.predecessors(*join).size() == 2);
  CHECK(fixture.graph.at(*result.root).get_if_constant() != nullptr);
}

TEST_CASE(
  "recovery emits an explicit merge for unequal predecessor definitions"
) {
  cfg_semantic_provider provider;
  sivra::program::decoded_program_builder program_builder(provider.architecture());
  const auto function = program_builder.add_function("entry");
  REQUIRE(function.has_value());
  const auto entry = program_builder.add_block(*function);
  const auto left = program_builder.add_block(*function);
  const auto right = program_builder.add_block(*function);
  const auto join = program_builder.add_block(*function);
  REQUIRE(entry.has_value());
  REQUIRE(left.has_value());
  REQUIRE(right.has_value());
  REQUIRE(join.has_value());
  REQUIRE(program_builder.add_instruction(*entry, provider.nop_form(), {}).has_value());
  REQUIRE(program_builder.add_instruction(*left, provider.zero_form(), {}).has_value());
  REQUIRE(program_builder
            .add_instruction(
              *right,
              provider.copy_form(),
              {sivra::program::operand(cfg_register_operand(provider.xmm1()))}
            )
            .has_value());
  REQUIRE(program_builder.add_instruction(*join, provider.nop_form(), {}).has_value());
  REQUIRE(program_builder.add_edge(*entry, *left).has_value());
  REQUIRE(program_builder.add_edge(*entry, *right).has_value());
  REQUIRE(program_builder.add_edge(*left, *join).has_value());
  REQUIRE(program_builder.add_edge(*right, *join).has_value());
  auto program = std::move(program_builder).freeze();
  REQUIRE(program.has_value());
  cfg_recovery_fixture fixture(std::move(provider), std::move(*program));

  const auto result = fixture.engine.recover(cfg_lane_query(fixture, *join));

  REQUIRE(result.root.has_value());
  const auto* merge = fixture.graph.at(*result.root).get_if_merge();
  REQUIRE(merge != nullptr);
  CHECK(merge->incoming.size() == 2);
  REQUIRE(result.provenance.has_value());
  CHECK(
    fixture.provenance.at(*result.provenance).kind ==
    sivra::recovery::provenance_kind::control_flow_merge
  );
}

TEST_CASE(
  "recovery ignores definitions in unreachable blocks"
) {
  cfg_semantic_provider provider;
  sivra::program::decoded_program_builder program_builder(provider.architecture());
  const auto function = program_builder.add_function("entry");
  REQUIRE(function.has_value());
  const auto entry = program_builder.add_block(*function);
  const auto join = program_builder.add_block(*function);
  const auto unreachable = program_builder.add_block(*function);
  REQUIRE(entry.has_value());
  REQUIRE(join.has_value());
  REQUIRE(unreachable.has_value());
  REQUIRE(program_builder.add_instruction(*entry, provider.nop_form(), {}).has_value());
  REQUIRE(program_builder.add_instruction(*join, provider.nop_form(), {}).has_value());
  REQUIRE(program_builder.add_instruction(*unreachable, provider.zero_form(), {}).has_value());
  REQUIRE(program_builder.add_edge(*entry, *join).has_value());
  auto program = std::move(program_builder).freeze();
  REQUIRE(program.has_value());
  cfg_recovery_fixture fixture(std::move(provider), std::move(*program));

  const auto result = fixture.engine.recover(cfg_lane_query(fixture, *join));

  REQUIRE(result.root.has_value());
  CHECK(fixture.graph.at(*result.root).get_if_external_value() != nullptr);
}

TEST_CASE(
  "recovery reports conservative loop cycles instead of ignoring loop-carried values"
) {
  cfg_semantic_provider provider;
  sivra::program::decoded_program_builder program_builder(provider.architecture());
  const auto function = program_builder.add_function("entry");
  REQUIRE(function.has_value());
  const auto loop = program_builder.add_block(*function);
  REQUIRE(loop.has_value());
  REQUIRE(
    program_builder
      .add_instruction(
        *loop, provider.add_form(), {sivra::program::operand(cfg_register_operand(provider.xmm1()))}
      )
      .has_value()
  );
  REQUIRE(program_builder.add_edge(*loop, *loop).has_value());
  auto program = std::move(program_builder).freeze();
  REQUIRE(program.has_value());
  cfg_recovery_fixture fixture(std::move(provider), std::move(*program));

  const auto result = fixture.engine.recover(cfg_lane_query(fixture, *loop));

  REQUIRE(result.root.has_value());
  CHECK(result.status == sivra::core::analysis_status::partial);
  REQUIRE(!result.diagnostics.empty());
  CHECK(result.diagnostics.front().code == "recovery.loop.cycle");
}

TEST_CASE(
  "recovery treats reaching predecessor stores as memory barriers"
) {
  cfg_semantic_provider provider;
  const sivra::program::memory_operand memory{.displacement = 16, .width = 128};
  sivra::program::decoded_program_builder program_builder(provider.architecture());
  const auto function = program_builder.add_function("entry");
  REQUIRE(function.has_value());
  const auto store_block = program_builder.add_block(*function);
  const auto load_block = program_builder.add_block(*function);
  REQUIRE(store_block.has_value());
  REQUIRE(load_block.has_value());
  REQUIRE(program_builder
            .add_instruction(*store_block, provider.store_form(), {sivra::program::operand(memory)})
            .has_value());
  REQUIRE(program_builder
            .add_instruction(*load_block, provider.load_form(), {sivra::program::operand(memory)})
            .has_value());
  REQUIRE(program_builder.add_edge(*store_block, *load_block).has_value());
  auto program = std::move(program_builder).freeze();
  REQUIRE(program.has_value());
  cfg_recovery_fixture fixture(std::move(provider), std::move(*program));

  const auto result = fixture.engine.recover(cfg_lane_query(fixture, *load_block));

  REQUIRE(result.root.has_value());
  CHECK(fixture.graph.at(*result.root).get_if_unknown() != nullptr);
  REQUIRE(!result.diagnostics.empty());
  CHECK(result.diagnostics.front().code == "recovery.memory.store_barrier");
}
