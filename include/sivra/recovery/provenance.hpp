#pragma once

#include "id.hpp"

#include <sivra/core/diagnostic.hpp>
#include <sivra/ir/id.hpp>
#include <sivra/program/instruction.hpp>
#include <sivra/program/machine_location.hpp>

#include <optional>
#include <span>
#include <string>
#include <vector>

namespace sivra::recovery {

enum class provenance_kind {
  instruction_write,
  external_input,
  memory_boundary,
  control_flow_merge,
  unsupported_boundary,
  unknown_boundary,
};

struct provenance_record {
  std::optional<provenance_id> id;
  provenance_kind kind = provenance_kind::unknown_boundary;
  std::optional<program::instruction_id> instruction;
  std::optional<program::program_point> point;
  std::optional<program::machine_location> location;
  std::optional<ir::node_id> node;
  std::vector<provenance_id> inputs;
  std::string detail;
  core::diagnostic_bundle_t diagnostics;
};

class provenance_store {
public:
  provenance_store();

  [[nodiscard]] provenance_id append(
    provenance_record record
  );
  [[nodiscard]] const provenance_record& at(
    provenance_id id
  ) const;
  [[nodiscard]] std::span<const provenance_record> records() const;
  [[nodiscard]] core::owner_token owner() const;

private:
  core::owner_token m_owner;
  std::vector<provenance_record> m_records;
};

} // namespace sivra::recovery
