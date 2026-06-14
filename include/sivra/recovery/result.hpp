#pragma once

#include "id.hpp"

#include <sivra/core/result.hpp>
#include <sivra/ir/id.hpp>
#include <sivra/ir/value_type.hpp>

#include <optional>

namespace sivra::recovery {

struct recovery_result {
  std::optional<ir::node_id> root;
  ir::value_type type = ir::value_type::unknown();
  std::optional<provenance_id> provenance;
  core::analysis_status status = core::analysis_status::complete;
  core::diagnostic_bundle_t diagnostics;

  [[nodiscard]] bool has_root() const { return root.has_value(); }
};

} // namespace sivra::recovery
