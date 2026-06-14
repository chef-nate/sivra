#pragma once

#include "id.hpp"

#include <sivra/core/result.hpp>
#include <sivra/ir/value_type.hpp>
#include <sivra/program/machine_location.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace sivra::recovery {

enum class annotation_source {
  user,
  debug_information,
  decompiler_database,
  abi,
  sivra_inference,
};

enum class annotation_strength {
  required,
  advisory,
};

enum class object_shape_kind {
  scalar,
  vector,
  matrix,
  fixed_array,
  structured,
  opaque,
};

enum class object_layout {
  row_major,
  column_major,
  packed,
  interleaved,
  explicit_strides,
  unknown,
};

struct object_shape {
  object_shape_kind kind = object_shape_kind::scalar;
  std::vector<std::uint32_t> dimensions;
};

struct object_annotation {
  std::optional<object_id> object;
  std::string name;
  std::optional<program::machine_location> storage;
  ir::value_type element_type = ir::value_type::unknown();
  object_shape shape;
  object_layout layout = object_layout::unknown;
  annotation_source source = annotation_source::sivra_inference;
  annotation_strength strength = annotation_strength::advisory;
};

class object_annotation_set {
public:
  object_annotation_set();

  [[nodiscard]] core::result_t<object_id> add(
    object_annotation annotation
  );
  [[nodiscard]] std::span<const object_annotation> annotations() const;
  [[nodiscard]] const object_annotation* find(
    object_id object
  ) const;
  [[nodiscard]] core::owner_token owner() const;

private:
  core::owner_token m_owner;
  std::vector<object_annotation> m_annotations;
};

} // namespace sivra::recovery
