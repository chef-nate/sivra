#include <sivra/canonicalizer/equivalence_contract.hpp>

#include <utility>

namespace sivra::canonicalizer {

equivalence_contract_id::equivalence_contract_id(
  std::string key
)
    : m_key(std::move(key)) {
}

std::string_view equivalence_contract_id::key() const {
  return m_key;
}

const equivalence_contract_id& algebraic_equivalence_contract() {
  static const equivalence_contract_id contract("sivra.algebraic.v1");
  return contract;
}

} // namespace sivra::canonicalizer
