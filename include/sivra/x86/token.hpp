#pragma once

#include <sivra/core/source_location.hpp>

#include <string>

namespace sivra::x86 {

enum class token_kind {
  identifier,
  integer,
  comma,
  plus,
  minus,
  left_bracket,
  right_bracket,
  newline,
};

struct token {
  token_kind kind = token_kind::identifier;
  std::string text;
  core::source_span source;
};

} // namespace sivra::x86
