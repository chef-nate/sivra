#pragma once

#include "token.hpp"

#include <sivra/core/result.hpp>

#include <string_view>
#include <vector>

namespace sivra::x86 {

class tokenizer {
public:
  [[nodiscard]] core::result_t<std::vector<token>> tokenize(
    core::source_id source,
    std::string_view text
  ) const;
};

} // namespace sivra::x86
