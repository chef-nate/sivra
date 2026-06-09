#pragma once

#include <span>
#include <string_view>

namespace sivra::tool {

struct embedded_expression {
  std::string_view output;
  std::string_view json;
};

std::span<const std::string_view> example_names();

std::span<const embedded_expression> example_expressions(
  std::string_view example
);

} // namespace sivra::tool
