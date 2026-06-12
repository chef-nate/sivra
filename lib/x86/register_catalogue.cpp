#include <sivra/x86/register.hpp>

#include <algorithm>
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

    const auto add_register =
      [&](std::string key, std::string name, std::uint32_t width, register_bank bank) {
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
              },
            .bank = bank,
          }
        );
        by_name.emplace(lower(std::move(key)), id);
        by_name.emplace(lower(std::move(name)), id);
      };

    for (int index = 0; index < 16; ++index) {
      add_register(
        "xmm" + std::to_string(index), "xmm" + std::to_string(index), 128, register_bank::simd
      );
    }
    for (const auto* name : {"eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp"}) {
      add_register(name, name, 32, register_bank::gpr);
    }
    for (const auto* name : {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp"}) {
      add_register(name, name, 64, register_bank::gpr);
    }
    for (int index = 8; index < 16; ++index) {
      add_register(
        "r" + std::to_string(index), "r" + std::to_string(index), 64, register_bank::gpr
      );
      add_register(
        "r" + std::to_string(index) + "d", "r" + std::to_string(index) + "d", 32, register_bank::gpr
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
