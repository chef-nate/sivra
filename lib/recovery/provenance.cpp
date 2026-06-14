#include <sivra/recovery/provenance.hpp>

#include <stdexcept>
#include <utility>

namespace sivra::recovery {

provenance_store::provenance_store() : m_owner(core::owner_token_source::next()) {
}

provenance_id provenance_store::append(
  provenance_record record
) {
  const auto id =
    provenance_id::unsafe_from_index(static_cast<std::uint32_t>(m_records.size()), m_owner);
  record.id = id;
  m_records.push_back(std::move(record));
  return id;
}

const provenance_record& provenance_store::at(
  provenance_id id
) const {
  if (id.owner() != m_owner || id.index() >= m_records.size()) {
    throw std::out_of_range("provenance_id does not belong to this provenance_store");
  }
  return m_records[id.index()];
}

std::span<const provenance_record> provenance_store::records() const {
  return m_records;
}

core::owner_token provenance_store::owner() const {
  return m_owner;
}

} // namespace sivra::recovery
