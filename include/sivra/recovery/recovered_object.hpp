#pragma once

#include "id.hpp"

#include <sivra/ir/id.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace sivra::recovery {

struct object_element_ref {
  object_id object;
  std::vector<std::uint32_t> indices;
};

struct recovered_element {
  object_element_ref element;
  ir::node_id root;
  provenance_id provenance;
  bool complete = true;
};

struct recovered_aggregate_view {
  object_id object;
  std::vector<recovered_element> elements;
};

class recovered_object_model {
public:
  [[nodiscard]] std::span<const recovered_aggregate_view> objects() const;

private:
  friend class recovered_object_builder;

  std::vector<recovered_aggregate_view> m_objects;
};

class recovered_object_builder {
public:
  explicit recovered_object_builder(
    core::owner_token owner
  );

  [[nodiscard]] object_id add_object(
    std::vector<recovered_element> elements
  );
  [[nodiscard]] recovered_object_model freeze() &&;

private:
  core::owner_token m_owner;
  std::vector<recovered_aggregate_view> m_objects;
};

} // namespace sivra::recovery
