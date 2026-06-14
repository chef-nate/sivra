#include <sivra/recovery/recovered_object.hpp>

#include <utility>

namespace sivra::recovery {

std::span<const recovered_aggregate_view> recovered_object_model::objects() const {
  return m_objects;
}

recovered_object_builder::recovered_object_builder(
  core::owner_token owner
)
    : m_owner(owner) {
}

object_id recovered_object_builder::add_object(
  std::vector<recovered_element> elements
) {
  const auto id =
    object_id::unsafe_from_index(static_cast<std::uint32_t>(m_objects.size()), m_owner);
  m_objects.push_back({.object = id, .elements = std::move(elements)});
  return id;
}

recovered_object_model recovered_object_builder::freeze() && {
  recovered_object_model model;
  model.m_objects = std::move(m_objects);
  return model;
}

} // namespace sivra::recovery
