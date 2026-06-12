#include "examples.hpp"

#include <array>
#include <stdexcept>

#ifndef SIVRA_RAW_EXPRESSION_FIXTURE_DIRECTORY
#error "SIVRA_RAW_EXPRESSION_FIXTURE_DIRECTORY must be defined"
#endif

namespace {

constexpr std::array names{
  std::string_view("dot-product"),
  std::string_view("weighted-clamp"),
  std::string_view("mat4-transform"),
};

constexpr std::array dot_product{
  sivra::tool::example_expression{
    .output = "xmm0",
    .file_name = "dot_product.json",
  },
};

constexpr std::array weighted_clamp{
  sivra::tool::example_expression{
    .output = "xmm0",
    .file_name = "weighted_clamp.json",
  },
};

constexpr std::array mat4_transform{
  sivra::tool::example_expression{
    .output = "xmm0",
    .file_name = "mat4_transform_xmm0.json",
  },
  sivra::tool::example_expression{
    .output = "xmm1",
    .file_name = "mat4_transform_xmm1.json",
  },
  sivra::tool::example_expression{
    .output = "xmm2",
    .file_name = "mat4_transform_xmm2.json",
  },
};

} // namespace

namespace sivra::tool {

std::string_view fixture_directory() {
  return SIVRA_RAW_EXPRESSION_FIXTURE_DIRECTORY;
}

std::span<const std::string_view> example_names() {
  return names;
}

std::span<const example_expression> example_expressions(
  std::string_view example
) {
  if (example == "dot-product") {
    return dot_product;
  }
  if (example == "weighted-clamp") {
    return weighted_clamp;
  }
  if (example == "mat4-transform") {
    return mat4_transform;
  }

  throw std::invalid_argument("unknown example");
}

} // namespace sivra::tool
