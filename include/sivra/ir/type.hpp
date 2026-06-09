#pragma once

#include "scalar_type.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace sivra::ir {

class type_context;

/**
 * @enum type_kind
 * @brief Identifies the concrete category of an IR type.
 */
enum class type_kind {
  unknown,
  scalar,
  vector,
  matrix,
};

/**
 * @class type
 * @brief Base class for immutable IR type definitions.
 *
 * type objects are owned by type_context and referenced by IR nodes or analysis
 * facts. Derived classes describe scalar, vector, and matrix-shaped values.
 */
class type {
public:
  virtual ~type() = default;

  const type_context& context() const;
  type_kind kind() const;

protected:
  type(
    const type_context& context,
    type_kind kind
  );

private:
  const type_context* m_context;
  type_kind m_kind;
};

/**
 * @class unknown_type
 * @brief Represents an expression type that has not been recovered.
 */
class unknown_type final : public type {
  friend class type_context;

private:
  explicit unknown_type(
    const type_context& context
  );
};

/**
 * @class scalar_type_def
 * @brief Represents a single scalar value.
 */
class scalar_type_def final : public type {
public:
  scalar_type scalar() const;

private:
  friend class type_context;

  scalar_type_def(
    const type_context& context,
    scalar_type scalar
  );

  scalar_type m_scalar;
};

/**
 * @class vector_type_def
 * @brief Represents a fixed-length sequence of elements with the same type.
 *
 * The element type describes each element, and elements gives the number of
 * elements in the vector.
 */
class vector_type_def final : public type {
public:
  const type& element_type() const;
  std::uint32_t elements() const;

private:
  friend class type_context;

  vector_type_def(
    const type_context& context,
    const type& element_type,
    std::uint32_t elements
  );

  const type* m_element_type;
  std::uint32_t m_elements;
};

/**
 * @class matrix_type_def
 * @brief Represents a fixed-shape two-dimensional collection of elements.
 *
 * The element type describes each matrix entry.
 */
class matrix_type_def final : public type {
public:
  const type& element_type() const;
  std::uint32_t rows() const;
  std::uint32_t columns() const;

private:
  friend class type_context;

  matrix_type_def(
    const type_context& context,
    const type& element_type,
    std::uint32_t rows,
    std::uint32_t columns
  );

  const type* m_element_type;
  std::uint32_t m_rows;
  std::uint32_t m_columns;
};

/**
 * @class type_context
 * @brief Owns IR type definitions and returns stable references to them.
 *
 * Any graph or analysis fact that references a type from this context must not
 * outlive the context.
 */
class type_context {
public:
  type_context() = default;

  type_context(
    const type_context&
  ) = delete;

  type_context(
    type_context&&
  ) = delete;

  type_context& operator=(
    const type_context&
  ) = delete;

  type_context& operator=(
    type_context&&
  ) = delete;

  /**
   * @brief Returns the context-owned unknown type.
   */
  const unknown_type& unknown();

  /**
   * @brief Returns a scalar type for the requested scalar_type.
   */
  const scalar_type_def& scalar(
    scalar_type scalar
  );

  /**
   * @brief Returns a vector type with the requested element type and element count.
   *
   * @throws std::invalid_argument if element_type is not a scalar type owned by
   * this context.
   */
  const vector_type_def& vector(
    const type& element_type,
    std::uint32_t elements
  );

  /**
   * @brief Returns a matrix type with the requested element type and shape.
   *
   * @throws std::invalid_argument if element_type is not a scalar type owned by
   * this context.
   */
  const matrix_type_def& matrix(
    const type& element_type,
    std::uint32_t rows,
    std::uint32_t columns
  );

private:
  struct vector_key {
    const type* element_type;
    std::uint32_t elements;

    bool operator==(
      const vector_key&
    ) const = default;
  };

  struct matrix_key {
    const type* element_type;
    std::uint32_t rows;
    std::uint32_t columns;

    bool operator==(
      const matrix_key&
    ) const = default;
  };

  struct vector_key_hash {
    std::size_t operator()(
      vector_key key
    ) const;
  };

  struct matrix_key_hash {
    std::size_t operator()(
      matrix_key key
    ) const;
  };

  std::unique_ptr<unknown_type> m_unknown;
  std::unordered_map<scalar_type, std::unique_ptr<scalar_type_def>> m_scalars;
  std::unordered_map<vector_key, std::unique_ptr<vector_type_def>, vector_key_hash> m_vectors;
  std::unordered_map<matrix_key, std::unique_ptr<matrix_type_def>, matrix_key_hash> m_matrices;
};

} // namespace sivra::ir
