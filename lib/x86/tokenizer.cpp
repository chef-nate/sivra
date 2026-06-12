#include <sivra/x86/tokenizer.hpp>

#include <cctype>

namespace {

sivra::core::source_span span(
  sivra::core::source_id source,
  std::size_t begin,
  std::size_t end
) {
  return {
    .source = source,
    .begin = {.byte_offset = begin, .line = 0, .column = begin},
    .end = {.byte_offset = end, .line = 0, .column = end},
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
  while (index < text.size()) {
    const auto character = static_cast<unsigned char>(text[index]);
    if (std::isspace(character)) {
      if (text[index] == '\n') {
        tokens.push_back(
          {.kind = token_kind::newline, .text = "\n", .source = span(source, index, index + 1)}
        );
      }
      ++index;
      continue;
    }

    const auto single = [&](token_kind kind) {
      tokens.push_back(
        {.kind = kind,
         .text = std::string(1, text[index]),
         .source = span(source, index, index + 1)}
      );
      ++index;
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
    if (std::isdigit(character)) {
      if (index + 1 < text.size() && text[index] == '0' &&
          (text[index + 1] == 'x' || text[index + 1] == 'X')) {
        index += 2;
        while (index < text.size() && std::isxdigit(static_cast<unsigned char>(text[index]))) {
          ++index;
        }
      } else {
        while (index < text.size() && std::isdigit(static_cast<unsigned char>(text[index]))) {
          ++index;
        }
      }
      tokens.push_back(
        {.kind = token_kind::integer,
         .text = std::string(text.substr(begin, index - begin)),
         .source = span(source, begin, index)}
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
      }
      tokens.push_back(
        {.kind = token_kind::identifier,
         .text = std::string(text.substr(begin, index - begin)),
         .source = span(source, begin, index)}
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
