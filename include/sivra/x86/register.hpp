#pragma once

#include <sivra/program/semantic_provider.hpp>

#include <memory>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sivra::x86 {

enum class register_bank {
  simd,
  gpr,
  flags,
  mxcsr,
};

struct register_info {
  program::register_definition definition;
  register_bank bank = register_bank::simd;
};

class register_catalogue {
public:
  [[nodiscard]] const register_info& at(
    program::register_id id
  ) const;
  [[nodiscard]] const register_info* find(
    std::string_view name
  ) const;
  [[nodiscard]] std::span<const register_info> registers() const;
  [[nodiscard]] core::owner_token owner() const;

private:
  friend std::shared_ptr<const register_catalogue> builtin_register_catalogue();

  register_catalogue(
    core::owner_token owner,
    std::vector<register_info> registers,
    std::unordered_map<
      std::string,
      program::register_id
    > by_name
  );

  core::owner_token m_owner;
  std::vector<register_info> m_registers;
  std::unordered_map<std::string, program::register_id> m_by_name;
};

[[nodiscard]] std::shared_ptr<const register_catalogue> builtin_register_catalogue();

} // namespace sivra::x86
