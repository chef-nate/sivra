#include <sivra/x86/register.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>
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

} // namespace

namespace sivra::x86 {

register_catalogue::register_catalogue(
  core::owner_token owner,
  std::vector<register_info> registers,
  std::unordered_map<
    std::string,
    program::register_id
  > by_name
)
    : m_owner(owner),
      m_registers(std::move(registers)),
      m_by_name(std::move(by_name)) {
}

const register_info& register_catalogue::at(
  program::register_id id
) const {
  if (id.owner() != m_owner || id.index() >= m_registers.size()) {
    throw std::out_of_range("register_id does not belong to this x86 register catalogue");
  }
  return m_registers[id.index()];
}

const register_info* register_catalogue::find(
  std::string_view name
) const {
  const auto found = m_by_name.find(lower(std::string(name)));
  return found == m_by_name.end() ? nullptr : &at(found->second);
}

std::span<const register_info> register_catalogue::registers() const {
  return m_registers;
}

core::owner_token register_catalogue::owner() const {
  return m_owner;
}

std::shared_ptr<const register_catalogue> builtin_register_catalogue() {
  static const auto catalogue = [] {
    const auto owner = core::owner_token_source::next();
    std::vector<register_info> registers;
    std::unordered_map<std::string, program::register_id> by_name;

    const auto add_register = [&](
                                std::string key,
                                std::string name,
                                std::uint32_t width,
                                register_bank bank,
                                std::optional<program::register_id> parent = std::nullopt,
                                program::bit_range parent_range = {}
                              ) {
      const auto id = program::register_id::unsafe_from_index(
        static_cast<std::uint32_t>(registers.size()), owner
      );
      registers.push_back(
        {
          .definition =
            {
              .id = id,
              .key = key,
              .name = name,
              .width = width,
              .parent = parent,
              .parent_range = parent_range,
            },
          .bank = bank,
        }
      );
      by_name.emplace(lower(std::move(key)), id);
      by_name.emplace(lower(std::move(name)), id);
      return id;
    };

    for (int index = 0; index < 16; ++index) {
      add_register(
        "xmm" + std::to_string(index), "xmm" + std::to_string(index), 128, register_bank::simd
      );
    }
    const std::array legacy_gprs{
      std::pair{"eax", "rax"},
      std::pair{"ebx", "rbx"},
      std::pair{"ecx", "rcx"},
      std::pair{"edx", "rdx"},
      std::pair{"esi", "rsi"},
      std::pair{"edi", "rdi"},
      std::pair{"esp", "rsp"},
      std::pair{"ebp", "rbp"},
    };
    std::vector<program::register_id> legacy_32_ids;
    legacy_32_ids.reserve(legacy_gprs.size());
    for (const auto& [reg32, reg64] : legacy_gprs) {
      legacy_32_ids.push_back(add_register(reg32, reg32, 32, register_bank::gpr));
    }
    for (std::size_t index = 0; index < legacy_gprs.size(); ++index) {
      const auto parent =
        add_register(legacy_gprs[index].second, legacy_gprs[index].second, 64, register_bank::gpr);
      auto& child = registers[legacy_32_ids[index].index()].definition;
      child.parent = parent;
      child.parent_range = {.offset = 0, .width = 32};
    }
    for (int index = 8; index < 16; ++index) {
      const auto parent = add_register(
        "r" + std::to_string(index), "r" + std::to_string(index), 64, register_bank::gpr
      );
      add_register(
        "r" + std::to_string(index) + "d",
        "r" + std::to_string(index) + "d",
        32,
        register_bank::gpr,
        parent,
        {.offset = 0, .width = 32}
      );
    }
    add_register("mxcsr", "mxcsr", 32, register_bank::mxcsr);
    return std::shared_ptr<const register_catalogue>(
      new register_catalogue(owner, std::move(registers), std::move(by_name))
    );
  }();
  return catalogue;
}

} // namespace sivra::x86
