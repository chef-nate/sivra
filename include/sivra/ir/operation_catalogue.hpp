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

class catalogue_compatibility_id {
public:
  catalogue_compatibility_id() = default;
  catalogue_compatibility_id(
    std::string value
  );

  [[nodiscard]] std::string_view value() const;

  auto operator<=>(
    const catalogue_compatibility_id&
  ) const = default;

private:
  std::string m_value;
};

struct operation_registration {
  operation_key key;
  std::string name;
  operation_signature signature;
  operation_attribute_schema attribute_schema;
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

  const operation_def& named(
    std::string_view name
  ) const;

  [[nodiscard]] const operation_def* find(
    const operation_key& key
  ) const;

  [[nodiscard]] const operation_def* find_by_name(
    std::string_view name
  ) const;

  bool contains(
    std::string_view key
  ) const;

  [[nodiscard]] std::span<const operation_def> operations() const;
  [[nodiscard]] const catalogue_compatibility_id& compatibility_id() const;
  [[nodiscard]] core::owner_token owner() const;

private:
  friend class operation_catalogue_builder;

  operation_catalogue(
    core::owner_token owner,
    catalogue_compatibility_id compatibility_id,
    std::vector<operation_def> definitions,
    std::unordered_map<
      std::string,
      operation_id
    > by_key,
    std::unordered_map<
      std::string,
      operation_id
    > by_name
  );

  core::owner_token m_owner;
  catalogue_compatibility_id m_compatibility_id;
  std::vector<operation_def> m_definitions;
  std::unordered_map<std::string, operation_id> m_by_key;
  std::unordered_map<std::string, operation_id> m_by_name;
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

  [[nodiscard]] core::result_t<void> validate() const;
  core::result_t<std::shared_ptr<const operation_catalogue>> freeze() &&;
  [[nodiscard]] core::owner_token owner() const;

private:
  core::owner_token m_owner;
  std::vector<operation_def> m_definitions;
  std::unordered_map<std::string, operation_id> m_by_key;
  std::unordered_map<std::string, operation_id> m_by_name;
  bool m_frozen = false;
};

} // namespace sivra::ir
