#pragma once

#include <compare>
#include <string>
#include <string_view>

namespace sivra::canonicalizer {

class rule_id {
public:
  explicit rule_id(
    std::string key
  );

  [[nodiscard]] std::string_view key() const;

  auto operator<=>(
    const rule_id&
  ) const = default;

private:
  std::string m_key;
};

using rewrite_rule_id = rule_id;

class equivalence_contract_id {
public:
  explicit equivalence_contract_id(
    std::string key
  );

  [[nodiscard]] std::string_view key() const;

  auto operator<=>(
    const equivalence_contract_id&
  ) const = default;

private:
  std::string m_key;
};

} // namespace sivra::canonicalizer
