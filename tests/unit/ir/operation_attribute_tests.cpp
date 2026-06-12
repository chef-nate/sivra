#include <sivra/ir/operation_attribute.hpp>

#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <vector>

namespace {

sivra::ir::operation_attributes attributes(
  std::initializer_list<sivra::ir::operation_attribute> entries
) {
  const std::vector copied(entries);
  const auto result = sivra::ir::operation_attributes::create(copied);
  REQUIRE(result.has_value());
  return *result;
}

sivra::ir::operation_attribute_schema test_schema() {
  const std::array fields{
    sivra::ir::operation_attribute_field{
      .key = "mode",
      .kind = sivra::ir::operation_attribute_kind::enum_key,
      .required = true,
      .allowed_enum_values =
        {
          sivra::ir::operation_enum_value("low"),
          sivra::ir::operation_enum_value("high"),
        },
    },
    sivra::ir::operation_attribute_field{
      .key = "lane",
      .kind = sivra::ir::operation_attribute_kind::integer,
      .default_value = std::int64_t{0},
      .minimum_integer = 0,
      .maximum_integer = 3,
    },
  };
  const auto result = sivra::ir::operation_attribute_schema::create(fields);
  REQUIRE(result.has_value());
  return *result;
}

} // namespace

TEST_CASE(
  "operation attributes normalize independently of insertion order"
) {
  const auto lhs = attributes(
    {
      {.key = "mode", .value = sivra::ir::operation_enum_value("high")},
      {.key = "lane", .value = std::int64_t{2}},
    }
  );
  const auto rhs = attributes(
    {
      {.key = "lane", .value = std::int64_t{2}},
      {.key = "mode", .value = sivra::ir::operation_enum_value("high")},
    }
  );

  CHECK(lhs == rhs);
  CHECK(
    std::hash<sivra::ir::operation_attributes>{}(lhs) ==
    std::hash<sivra::ir::operation_attributes>{}(rhs)
  );
  REQUIRE(lhs.entries().size() == 2);
  CHECK(lhs.entries()[0].key == "lane");
  CHECK(lhs.entries()[1].key == "mode");
}

TEST_CASE(
  "operation attribute schemas insert defaults and retain deterministic field order"
) {
  const auto schema = test_schema();
  const auto validated = schema.validate(attributes(
    {
      {.key = "mode", .value = sivra::ir::operation_enum_value("low")},
    }
  ));

  REQUIRE(validated.has_value());
  REQUIRE(schema.fields().size() == 2);
  CHECK(schema.fields()[0].key == "lane");
  CHECK(schema.fields()[1].key == "mode");
  REQUIRE(validated->find("lane") != nullptr);
  CHECK(std::get<std::int64_t>(*validated->find("lane")) == 0);
}

TEST_CASE(
  "operation attribute schemas reject missing, unknown, wrong-kind, range, and enum values"
) {
  const auto schema = test_schema();

  const auto missing = schema.validate({});
  REQUIRE(!missing.has_value());
  CHECK(missing.error().front().code == "ir.attribute.required");

  const auto unknown = schema.validate(attributes(
    {
      {.key = "mode", .value = sivra::ir::operation_enum_value("low")},
      {.key = "other", .value = true},
    }
  ));
  REQUIRE(!unknown.has_value());
  CHECK(unknown.error().front().code == "ir.attribute.unknown");

  const auto wrong_kind = schema.validate(attributes(
    {
      {.key = "mode", .value = sivra::ir::operation_enum_value("low")},
      {.key = "lane", .value = true},
    }
  ));
  REQUIRE(!wrong_kind.has_value());
  CHECK(wrong_kind.error().front().code == "ir.attribute.kind");

  const auto out_of_range = schema.validate(attributes(
    {
      {.key = "mode", .value = sivra::ir::operation_enum_value("low")},
      {.key = "lane", .value = std::int64_t{4}},
    }
  ));
  REQUIRE(!out_of_range.has_value());
  CHECK(out_of_range.error().front().code == "ir.attribute.range");

  const auto invalid_enum = schema.validate(attributes(
    {
      {.key = "mode", .value = sivra::ir::operation_enum_value("other")},
    }
  ));
  REQUIRE(!invalid_enum.has_value());
  CHECK(invalid_enum.error().front().code == "ir.attribute.enum");
}

TEST_CASE(
  "operation attribute and schema construction reject malformed definitions atomically"
) {
  const std::array duplicate_attributes{
    sivra::ir::operation_attribute{.key = "lane", .value = std::int64_t{0}},
    sivra::ir::operation_attribute{.key = "lane", .value = std::int64_t{1}},
  };
  CHECK(!sivra::ir::operation_attributes::create(duplicate_attributes).has_value());

  const std::array invalid_fields{
    sivra::ir::operation_attribute_field{
      .key = "lane",
      .kind = sivra::ir::operation_attribute_kind::integer,
      .default_value = std::int64_t{5},
      .minimum_integer = 0,
      .maximum_integer = 3,
    },
  };
  const auto invalid_schema = sivra::ir::operation_attribute_schema::create(invalid_fields);
  REQUIRE(!invalid_schema.has_value());
  CHECK(invalid_schema.error().front().code == "ir.attribute_schema.invalid_default");
}
