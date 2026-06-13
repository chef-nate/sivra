#include <sivra/x86/parser.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <optional>
#include <utility>

namespace {

std::string lower(
  std::string value
) {
  std::ranges::transform(value, value.begin(), [](unsigned char character) {
    return static_cast<char>(std::tolower(character));
  });
  return value;
}

std::optional<std::uint64_t> parse_integer(
  std::string_view text
) {
  std::uint64_t value = 0;
  const int base = text.starts_with("0x") || text.starts_with("0X") ? 16 : 10;
  if (base == 16) {
    text.remove_prefix(2);
  }
  if (text.empty()) {
    return std::nullopt;
  }
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, base);
  if (error != std::errc{} || end != text.data() + text.size()) {
    return std::nullopt;
  }
  return value;
}

} // namespace

namespace sivra::x86 {

core::result_t<std::vector<unresolved_instruction>> parser::parse(
  std::span<const token> tokens
) const {
  std::vector<unresolved_instruction> instructions;
  std::size_t index = 0;
  while (index < tokens.size()) {
    while (index < tokens.size() && tokens[index].kind == token_kind::newline) {
      ++index;
    }
    if (index >= tokens.size()) {
      break;
    }
    if (tokens[index].kind != token_kind::identifier) {
      return core::fail<std::vector<unresolved_instruction>>(
        "x86.parser.expected_mnemonic", "x86 parser expected an instruction mnemonic"
      );
    }

    unresolved_instruction instruction{
      .mnemonic = lower(tokens[index].text),
      .source = tokens[index].source,
    };
    ++index;

    bool expect_operand = true;
    while (index < tokens.size() && tokens[index].kind != token_kind::newline) {
      if (!expect_operand) {
        if (tokens[index].kind != token_kind::comma) {
          return core::fail<std::vector<unresolved_instruction>>(
            "x86.parser.expected_comma", "x86 parser expected a comma between operands"
          );
        }
        ++index;
        expect_operand = true;
        continue;
      }

      if (tokens[index].kind == token_kind::identifier) {
        instruction.operands.push_back(
          unresolved_register_operand{.name = lower(tokens[index].text),
                                      .source = tokens[index].source}
        );
        instruction.source.end = tokens[index].source.end;
        ++index;
        expect_operand = false;
        continue;
      }
      if (tokens[index].kind == token_kind::integer) {
        auto value = parse_integer(tokens[index].text);
        if (!value.has_value()) {
          return core::fail<std::vector<unresolved_instruction>>(
            "x86.parser.invalid_integer", "x86 parser could not parse integer operand"
          );
        }
        instruction.operands.push_back(
          unresolved_immediate_operand{.value = *value, .source = tokens[index].source}
        );
        instruction.source.end = tokens[index].source.end;
        ++index;
        expect_operand = false;
        continue;
      }
      if (tokens[index].kind == token_kind::left_bracket) {
        const auto begin = tokens[index].source;
        ++index;
        if (index >= tokens.size() || tokens[index].kind != token_kind::identifier) {
          return core::fail<std::vector<unresolved_instruction>>(
            "x86.parser.expected_memory_base", "x86 parser expected a memory base register"
          );
        }
        unresolved_memory_operand memory{.base = lower(tokens[index].text), .source = begin};
        ++index;
        if (index < tokens.size() &&
            (tokens[index].kind == token_kind::plus || tokens[index].kind == token_kind::minus)) {
          const bool negate = tokens[index].kind == token_kind::minus;
          ++index;
          if (index >= tokens.size() || tokens[index].kind != token_kind::integer) {
            return core::fail<std::vector<unresolved_instruction>>(
              "x86.parser.expected_displacement", "x86 parser expected a memory displacement"
            );
          }
          auto parsed_displacement = parse_integer(tokens[index].text);
          if (!parsed_displacement.has_value() ||
              *parsed_displacement >
                static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
            return core::fail<std::vector<unresolved_instruction>>(
              "x86.parser.invalid_integer", "x86 parser could not parse memory displacement"
            );
          }
          const auto displacement = static_cast<std::int64_t>(*parsed_displacement);
          memory.displacement = negate ? -displacement : displacement;
          ++index;
        }
        if (index >= tokens.size() || tokens[index].kind != token_kind::right_bracket) {
          return core::fail<std::vector<unresolved_instruction>>(
            "x86.parser.expected_memory_end", "x86 parser expected the end of a memory operand"
          );
        }
        memory.source.end = tokens[index].source.end;
        instruction.source.end = tokens[index].source.end;
        ++index;
        instruction.operands.push_back(std::move(memory));
        expect_operand = false;
        continue;
      }

      return core::fail<std::vector<unresolved_instruction>>(
        "x86.parser.invalid_operand", "x86 parser could not parse operand syntax"
      );
    }
    if (expect_operand && !instruction.operands.empty()) {
      return core::fail<std::vector<unresolved_instruction>>(
        "x86.parser.trailing_comma", "x86 parser found a trailing comma"
      );
    }
    instructions.push_back(std::move(instruction));
  }
  return instructions;
}

} // namespace sivra::x86
