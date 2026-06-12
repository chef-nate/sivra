#pragma once

#include "constant.hpp"
#include "id.hpp"

#include <string>

namespace sivra::ir {

struct constant_node {
  constant_value value;
};

struct symbol_node {
  symbol_id symbol;
};

struct external_value_node {
  external_value_id value;
};

struct unknown_node {
  std::string reason;
};

} // namespace sivra::ir
