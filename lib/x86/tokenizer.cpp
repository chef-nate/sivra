#include <sivra/x86/tokenizer.hpp>

#include <cctype>
#include <cstdint>

namespace {

sivra::core::source_span span(
  sivra::core::source_id source,
  std::size_t begin,
  std::size_t end,
  std::uint32_t begin_line,
  std::uint32_t begin_column,
  std::uint32_t end_line,
  std::uint32_t end_column
) {
  return {
    .source = source,
    .begin = {.byte_offset = begin, .line = begin_line, .column = begin_column},
    .end = {.byte_offset = end, .line = end_line, .column = end_column},
  };
}

} // namespace

namespace sivra::x86 {

core::result_t<std::vector<token>> tokenizer::tokenize(
  core::source_id source,
  std::string_view text
) const {
  std::vector<token> tokens;
  std::size_t index = 0;
  std::uint32_t line = 0;
  std::uint32_t column = 0;
  const auto token_span =
    [&](std::size_t begin, std::size_t end, std::uint32_t begin_line, std::uint32_t begin_column) {
      return span(source, begin, end, begin_line, begin_column, line, column);
    };
  while (index < text.size()) {
    const auto character = static_cast<unsigned char>(text[index]);
    if (std::isspace(character)) {
      if (text[index] == '\n') {
        const auto begin = index;
        const auto begin_line = line;
        const auto begin_column = column;
        ++index;
        ++line;
        column = 0;
        tokens.push_back(
          {.kind = token_kind::newline,
           .text = "\n",
           .source = span(source, begin, index, begin_line, begin_column, line, column)}
        );
        continue;
      }
      ++index;
      ++column;
      continue;
    }

    const auto single = [&](token_kind kind) {
      const auto begin = index;
      const auto begin_line = line;
      const auto begin_column = column;
      ++index;
      ++column;
      tokens.push_back(
        {.kind = kind,
         .text = std::string(1, text[begin]),
         .source = token_span(begin, index, begin_line, begin_column)}
      );
    };
    switch (text[index]) {
    case ',':
      single(token_kind::comma);
      continue;
    case '+':
      single(token_kind::plus);
      continue;
    case '-':
      single(token_kind::minus);
      continue;
    case '[':
      single(token_kind::left_bracket);
      continue;
    case ']':
      single(token_kind::right_bracket);
      continue;
    default:
      break;
    }

    const auto begin = index;
    const auto begin_line = line;
    const auto begin_column = column;
    if (std::isdigit(character)) {
      if (index + 1 < text.size() && text[index] == '0' &&
          (text[index + 1] == 'x' || text[index + 1] == 'X')) {
        index += 2;
        column += 2;
        while (index < text.size() && std::isxdigit(static_cast<unsigned char>(text[index]))) {
          ++index;
          ++column;
        }
      } else {
        while (index < text.size() && std::isdigit(static_cast<unsigned char>(text[index]))) {
          ++index;
          ++column;
        }
      }
      tokens.push_back(
        {.kind = token_kind::integer,
         .text = std::string(text.substr(begin, index - begin)),
         .source = token_span(begin, index, begin_line, begin_column)}
      );
      continue;
    }
    if (std::isalpha(character) || text[index] == '_') {
      while (index < text.size()) {
        const auto current = static_cast<unsigned char>(text[index]);
        if (!std::isalnum(current) && text[index] != '_') {
          break;
        }
        ++index;
        ++column;
      }
      tokens.push_back(
        {.kind = token_kind::identifier,
         .text = std::string(text.substr(begin, index - begin)),
         .source = token_span(begin, index, begin_line, begin_column)}
      );
      continue;
    }

    return core::fail<std::vector<token>>(
      "x86.tokenizer.invalid_character", "x86 tokenizer encountered an unsupported character"
    );
  }
  return tokens;
}

} // namespace sivra::x86
