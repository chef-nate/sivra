#include <sivra/core/budget.hpp>
#include <sivra/core/diagnostic.hpp>
#include <sivra/core/owner_token.hpp>
#include <sivra/core/result.hpp>
#include <sivra/core/trace.hpp>

#include <doctest/doctest.h>

#include <vector>

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
  "null trace sinks accept trace events"
) {
  sivra::core::null_trace_sink sink;
  sink.emit(
    {
      .domain = "test",
      .kind = "event",
      .detail = "ignored",
    }
  );
}
