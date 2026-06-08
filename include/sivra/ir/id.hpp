#pragma once

#include <cstdint>

namespace sivra::ir {

/**
 * @class operation_id
 * @brief Identifies an operation_def stored in an operation_registry.
 */
class operation_id {
public:
  /**
   * @brief Creates an operation_id from its numeric value.
   */
  explicit operation_id(
    std::uint32_t value
  );

  /**
   * @brief Returns the numeric value of this operation_id.
   */
  std::uint32_t value() const;

  bool operator==(
    const operation_id&
  ) const;
  bool operator<(
    const operation_id&
  ) const;

private:
  std::uint32_t m_value;
};

/**
 * @class node_id
 * @brief Identifies an expression_node stored in an expression_graph.
 */
class node_id {
public:
  /**
   * @brief Creates a node_id from its numeric value.
   */
  explicit node_id(
    std::uint32_t value
  );

  /**
   * @brief Returns the numeric value of this node_id.
   */
  std::uint32_t value() const;

  bool operator==(
    const node_id&
  ) const;
  bool operator<(
    const node_id&
  ) const;

private:
  std::uint32_t m_value;
};

} // namespace sivra::ir
