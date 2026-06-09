#pragma once

#include "operation.hpp"

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sivra::ir {

/**
 * @struct operation_registration
 * @brief Describes an operation to add to an operation_registry.
 */
struct operation_registration {
  std::string name;
  operation_semantics semantics;
};

/**
 * @class operation_registry
 * @brief Owns operation definitions and assigns operation_id values.
 *
 * operation_registry provides lookup by operation_id or name. Registered names
 * are unique within the registry.
 */
class operation_registry {
public:
  /**
   * @brief Atomically registers a batch of operation definitions.
   *
   * The returned operation_id values correspond to registrations in input
   * order. If registration fails, the registry remains unchanged.
   *
   * @param registrations Operation definitions to register.
   * @return Assigned operation_id values in input order.
   */
  std::vector<operation_id> register_operations(
    std::span<const operation_registration> registrations
  );

  /**
   * @brief Registers a new operation definition.
   *
   * @param name Unique operation name.
   * @param semantics Algebraic semantics associated with the operation.
   * @return operation_id assigned to the registered operation.
   */
  operation_id register_operation(
    std::string name,
    operation_semantics semantics = {}
  );

  /**
   * @brief Registers a new operation definition from a trait mask.
   *
   * @param name Unique operation name.
   * @param traits Algebraic traits associated with the operation.
   * @return operation_id assigned to the registered operation.
   */
  operation_id register_operation(
    std::string name,
    operation_trait traits
  );

  /**
   * @brief Looks up an operation by operation_id.
   *
   * @param id operation_id returned by register_operation().
   * @return Operation definition associated with the id.
   */
  const operation_def& at(
    operation_id id
  ) const;

  /**
   * @brief Looks up an operation by name.
   *
   * @param name Name passed to register_operation().
   * @return Operation definition associated with the name.
   */
  const operation_def& at(
    std::string_view name
  ) const;

  /**
   * @brief Returns true if an operation with the given name is registered.
   */
  bool contains(
    std::string_view name
  ) const;

  /**
   * @brief Returns all registered operation definitions in operation_id order.
   */
  std::span<const operation_def> operations() const;

private:
  std::vector<operation_def> m_operations;
  std::unordered_map<std::string, operation_id> m_by_name;
};

} // namespace sivra::ir
