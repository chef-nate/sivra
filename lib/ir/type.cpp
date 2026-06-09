#include <sivra/ir/type.hpp>

#include <functional>
#include <memory>
#include <stdexcept>

namespace sivra::ir {

namespace {

inline void hash_combine(
  std::size_t&
) {
}

template <
  typename T,
  typename... Rest
>
inline void hash_combine(
  std::size_t& seed,
  const T& value,
  Rest... rest
) {
  std::hash<T> hasher;
  seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  hash_combine(seed, rest...);
}

} // namespace

type::type(
  const type_context& context,
  type_kind kind
)
    : m_context(&context),
      m_kind(kind) {
}

const type_context& type::context() const {
  return *m_context;
}

type_kind type::kind() const {
  return m_kind;
}

unknown_type::unknown_type(
  const type_context& context
)
    : type(
        context,
        type_kind::unknown
      ) {
}

scalar_type_def::scalar_type_def(
  const type_context& context,
  scalar_type scalar
)
    : type(
        context,
        type_kind::scalar
      ),
      m_scalar(scalar) {
}

scalar_type scalar_type_def::scalar() const {
  return m_scalar;
}

vector_type_def::vector_type_def(
  const type_context& context,
  const type& element_type,
  std::uint32_t elements
)
    : type(
        context,
        type_kind::vector
      ),
      m_element_type(&element_type),
      m_elements(elements) {
}

const type& vector_type_def::element_type() const {
  return *m_element_type;
}

std::uint32_t vector_type_def::elements() const {
  return m_elements;
}

matrix_type_def::matrix_type_def(
  const type_context& context,
  const type& element_type,
  std::uint32_t rows,
  std::uint32_t columns
)
    : type(
        context,
        type_kind::matrix
      ),
      m_element_type(&element_type),
      m_rows(rows),
      m_columns(columns) {
}

const type& matrix_type_def::element_type() const {
  return *m_element_type;
}

std::uint32_t matrix_type_def::rows() const {
  return m_rows;
}

std::uint32_t matrix_type_def::columns() const {
  return m_columns;
}

std::size_t type_context::vector_key_hash::operator()(
  type_context::vector_key key
) const {
  std::size_t seed = 0;
  hash_combine(seed, key.element_type, key.elements);
  return seed;
}

std::size_t type_context::matrix_key_hash::operator()(
  type_context::matrix_key key
) const {
  std::size_t seed = 0;
  hash_combine(seed, key.element_type, key.rows, key.columns);
  return seed;
}

const unknown_type& type_context::unknown() {
  if (m_unknown == nullptr) {
    m_unknown = std::unique_ptr<unknown_type>(new unknown_type(*this));
  }

  return *m_unknown;
}

const scalar_type_def& type_context::scalar(
  scalar_type scalar
) {
  if (const auto iter = m_scalars.find(scalar); iter != m_scalars.end()) {
    return *iter->second;
  }

  const auto inserted =
    m_scalars.emplace(scalar, std::unique_ptr<scalar_type_def>(new scalar_type_def(*this, scalar)));
  return *inserted.first->second;
}

const vector_type_def& type_context::vector(
  const type& element_type,
  std::uint32_t elements
) {
  if (&element_type.context() != this) {
    throw std::invalid_argument("vector element type belongs to another type_context");
  }

  const vector_key key{.element_type = &element_type, .elements = elements};

  if (const auto iter = m_vectors.find(key); iter != m_vectors.end()) {
    return *iter->second;
  }

  const auto inserted = m_vectors.emplace(
    key, std::unique_ptr<vector_type_def>(new vector_type_def(*this, element_type, elements))
  );
  return *inserted.first->second;
}

const matrix_type_def& type_context::matrix(
  const type& element_type,
  std::uint32_t rows,
  std::uint32_t columns
) {
  if (&element_type.context() != this) {
    throw std::invalid_argument("matrix element type belongs to another type_context");
  }

  const matrix_key key{.element_type = &element_type, .rows = rows, .columns = columns};

  if (const auto iter = m_matrices.find(key); iter != m_matrices.end()) {
    return *iter->second;
  }

  const auto inserted = m_matrices.emplace(
    key, std::unique_ptr<matrix_type_def>(new matrix_type_def(*this, element_type, rows, columns))
  );
  return *inserted.first->second;
}

} // namespace sivra::ir
