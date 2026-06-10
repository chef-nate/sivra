#pragma once

#include "operation.hpp"

#include <sivra/core/result.hpp>

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sivra::ir {

struct operation_registration {
  std::string key;
  std::string name;
  operation_signature signature;
  operation_semantics semantics;
};

class operation_catalogue {
public:
  const operation_def& operation(
    operation_id id
  ) const;

  const operation_def& at(
    operation_id id
  ) const;

  const operation_def& at(
    std::string_view key
  ) const;

  bool contains(
    std::string_view key
  ) const;

  std::span<const operation_def> operations() const;
  core::owner_token owner() const;

private:
  friend class operation_catalogue_builder;

  operation_catalogue(
    core::owner_token owner,
    std::vector<operation_def> definitions,
    std::unordered_map<
      std::string,
      operation_id
    > by_key
  );

  core::owner_token m_owner;
  std::vector<operation_def> m_definitions;
  std::unordered_map<std::string, operation_id> m_by_key;
};

class operation_catalogue_builder {
public:
  operation_catalogue_builder();

  core::result_t<std::vector<operation_id>> register_operations(
    std::span<const operation_registration> registrations
  );

  core::result_t<operation_id> register_operation(
    operation_registration registration
  );

  core::result_t<std::shared_ptr<const operation_catalogue>> freeze() &&;
  core::owner_token owner() const;

private:
  core::owner_token m_owner;
  std::vector<operation_def> m_definitions;
  std::unordered_map<std::string, operation_id> m_by_key;
  bool m_frozen = false;
};

} // namespace sivra::ir
