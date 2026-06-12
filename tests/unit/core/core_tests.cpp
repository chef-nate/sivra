#include <sivra/core/budget.hpp>
#include <sivra/core/diagnostic.hpp>
#include <sivra/core/owner_token.hpp>
#include <sivra/core/result.hpp>
#include <sivra/core/trace.hpp>

#include <doctest/doctest.h>

#include <limits>
#include <string>
#include <vector>

namespace {

class recording_trace_sink final : public sivra::core::trace_sink {
public:
  void emit(
    const sivra::core::trace_event& event
  ) override {
    events.push_back(event);
  }

  std::vector<sivra::core::trace_event> events;
};

} // namespace

TEST_CASE(
  "owner tokens are non-zero and process-unique"
) {
  const auto lhs = sivra::core::owner_token_source::next();
  const auto rhs = sivra::core::owner_token_source::next();

  CHECK(lhs.value() != 0);
  CHECK(rhs.value() != 0);
  CHECK(lhs != rhs);
}

TEST_CASE(
  "budget counters reject consumption beyond the limit atomically"
) {
  sivra::core::budget_counter budget(3);

  CHECK(budget.try_consume(2));
  CHECK(!budget.try_consume(2));
  CHECK(budget.consumed() == 2);
  CHECK(budget.remaining() == 1);
  CHECK(budget.try_consume());
  CHECK(budget.remaining() == 0);
}

TEST_CASE(
  "budget counters define zero and overflow-safe exact-limit behavior"
) {
  sivra::core::budget_counter zero(sivra::core::budget_limit{.maximum = 0});
  CHECK(zero.try_consume(0));
  CHECK(!zero.try_consume());
  CHECK(zero.consumed() == 0);

  sivra::core::budget_counter maximum(std::numeric_limits<std::size_t>::max());
  CHECK(maximum.try_consume(std::numeric_limits<std::size_t>::max()));
  CHECK(!maximum.try_consume());
  CHECK(maximum.consumed() == std::numeric_limits<std::size_t>::max());
  CHECK(maximum.remaining() == 0);

  const auto diagnostic =
    maximum.exhaustion_diagnostic("core.test_budget", "test budget exhausted");
  CHECK(diagnostic.code == "core.test_budget");
  REQUIRE(diagnostic.notes.size() == 1);
  CHECK(diagnostic.notes.front().message.find("consumed") != std::string::npos);
}

TEST_CASE(
  "core failures carry structured diagnostics"
) {
  const auto result = sivra::core::fail<int>("test.failure", "expected failure");

  REQUIRE(!result.has_value());
  REQUIRE(result.error().size() == 1);
  CHECK(result.error().front().code == "test.failure");
  CHECK(result.error().front().severity == sivra::core::diagnostic_severity::error);
  CHECK(result.error().front().message == "expected failure");
}

TEST_CASE(
  "diagnostic codes expose stable domains and retain related notes"
) {
  const sivra::core::source_span source{
    .source = sivra::core::source_id::from_index(2),
    .begin = {.byte_offset = 4, .line = 1, .column = 2},
    .end = {.byte_offset = 9, .line = 1, .column = 7},
  };
  sivra::core::diagnostic diagnostic{
    .code = "ir.invalid_node",
    .severity = sivra::core::diagnostic_severity::error,
    .stage = "validation",
    .message = "invalid node",
    .source = source,
  };
  diagnostic.with_note({.message = "created here", .source = source});

  CHECK(diagnostic.code.domain() == "ir");
  CHECK(diagnostic.code.value() == "ir.invalid_node");
  CHECK_FALSE((diagnostic.code == sivra::core::diagnostic_code("core.invalid_node")));
  REQUIRE(diagnostic.notes.size() == 1);
  CHECK(diagnostic.notes.front().source == source);
}

TEST_CASE(
  "source spans are valid half-open ranges"
) {
  const auto source = sivra::core::source_id::from_index(0);
  const sivra::core::source_span span{
    .source = source,
    .begin = {.byte_offset = 3, .line = 0, .column = 3},
    .end = {.byte_offset = 8, .line = 0, .column = 8},
  };
  const sivra::core::source_span reversed{
    .source = source,
    .begin = {.byte_offset = 8},
    .end = {.byte_offset = 3},
  };

  CHECK(span.is_valid());
  CHECK(span.contains({.byte_offset = 3}));
  CHECK(span.contains({.byte_offset = 7}));
  CHECK(!span.contains({.byte_offset = 8}));
  CHECK(!reversed.is_valid());
  CHECK(!sivra::core::source_span{}.is_valid());
}

TEST_CASE(
  "stage results enforce documented artifact and status combinations"
) {
  const sivra::core::stage_result<int> complete{
    .artifact = 1,
    .status = sivra::core::analysis_status::complete,
  };
  const sivra::core::stage_result<int> partial{
    .artifact = 1,
    .status = sivra::core::analysis_status::partial,
  };
  const sivra::core::stage_result<int> invalid{
    .status = sivra::core::analysis_status::invalid_input,
  };
  const sivra::core::stage_result<int> missing_complete{
    .status = sivra::core::analysis_status::complete,
  };
  const sivra::core::stage_result<int> invalid_with_artifact{
    .artifact = 1,
    .status = sivra::core::analysis_status::invalid_input,
  };

  CHECK(complete.is_consistent());
  CHECK(partial.is_consistent());
  CHECK(invalid.is_consistent());
  CHECK(!missing_complete.is_consistent());
  CHECK(!invalid_with_artifact.is_consistent());
}

TEST_CASE(
  "null trace sinks accept trace events"
) {
  sivra::core::null_trace_sink sink;
  sink.emit(
    {
      .domain = sivra::core::trace_domain::core,
      .kind = "event",
      .detail = "ignored",
    }
  );
}

TEST_CASE(
  "trace sinks observe deterministic event order without changing semantic state"
) {
  recording_trace_sink sink;
  int semantic_value = 7;
  sink.emit(
    {
      .domain = sivra::core::trace_domain::ir,
      .kind = "first",
      .detail = "a",
    }
  );
  sink.emit(
    {
      .domain = sivra::core::trace_domain::ir,
      .kind = "second",
      .detail = "b",
    }
  );

  REQUIRE(sink.events.size() == 2);
  CHECK(sink.events[0].kind == "first");
  CHECK(sink.events[1].kind == "second");
  CHECK(semantic_value == 7);
}
