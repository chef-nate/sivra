#pragma once

#include <span>
#include <string_view>

namespace sivra::tool {

struct example_expression {
  std::string_view output;
  std::string_view file_name;
};

std::string_view fixture_directory();

std::span<const std::string_view> example_names();

std::span<const example_expression> example_expressions(
  std::string_view example
);

} // namespace sivra::tool
