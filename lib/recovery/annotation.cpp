#include <sivra/recovery/annotation.hpp>

#include <algorithm>
#include <utility>

namespace sivra::recovery {

object_annotation_set::object_annotation_set() : m_owner(core::owner_token_source::next()) {
}

core::result_t<object_id> object_annotation_set::add(
  object_annotation annotation
) {
  if (annotation.name.empty()) {
    return core::fail<object_id>(
      "recovery.annotation.invalid_object", "object annotation name must not be empty"
    );
  }
  const auto id =
    object_id::unsafe_from_index(static_cast<std::uint32_t>(m_annotations.size()), m_owner);
  annotation.object = id;
  m_annotations.push_back(std::move(annotation));
  return id;
}

std::span<const object_annotation> object_annotation_set::annotations() const {
  return m_annotations;
}

const object_annotation* object_annotation_set::find(
  object_id object
) const {
  if (object.owner() != m_owner || object.index() >= m_annotations.size()) {
    return nullptr;
  }
  return &m_annotations[object.index()];
}

core::owner_token object_annotation_set::owner() const {
  return m_owner;
}

} // namespace sivra::recovery
