#include <raw_expression_json.hpp>

#include <doctest/doctest.h>

#include <stdexcept>
#include <string_view>

namespace {

constexpr std::string_view single_memory_load = R"json(
{
  "format": "simd-decompiler.raw-expression-dag.v1",
  "root": 0,
  "nodes": [
    {
      "id": 0,
      "kind": "memory_load",
      "type": {
        "scalar": "floating_point",
        "element_bits": 32,
        "lane_count": 1
      },
      "memory": {
        "base": "rdi",
        "displacement": 8
      },
      "memory_lane": 1,
      "element_bits": 32,
      "children": []
    }
  ]
}
)json";

} // namespace

TEST_CASE(
  "raw expression compatibility importer loads a supported graph"
) {
  const auto loaded = sivra::compat::parse_raw_expression_json(single_memory_load);

  CHECK(loaded.graph.size() == 1);
  CHECK(loaded.root.index() == 0);
  CHECK(loaded.root.owner() == loaded.graph.owner());
  CHECK(loaded.catalogue == loaded.graph.shared_catalogue());
  REQUIRE(loaded.external_values.size() == 1);
  CHECK(loaded.external_values[0].base_register == "rdi");
  CHECK(loaded.external_values[0].offset == 12);
  const auto* external = loaded.graph.at(loaded.root).get_if_external_value();
  REQUIRE(external != nullptr);
  CHECK(external->value.index() == 0);
  CHECK(external->value.owner() == loaded.catalogue->owner());
}

TEST_CASE(
  "raw expression compatibility importer rejects an unsupported format"
) {
  constexpr std::string_view unsupported = R"json(
    {"format":"unsupported","root":0,"nodes":[]}
  )json";

  CHECK_THROWS_AS(sivra::compat::parse_raw_expression_json(unsupported), std::runtime_error);
}
