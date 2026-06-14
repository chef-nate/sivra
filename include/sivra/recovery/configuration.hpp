#pragma once

#include <cstddef>

namespace sivra::recovery {

struct recovery_configuration {
  std::size_t max_recursion_depth = 64;
  bool treat_may_alias_store_as_barrier = true;
};

} // namespace sivra::recovery
