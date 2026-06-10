#pragma once

#include <sivra/core/result.hpp>
#include <sivra/ir/id.hpp>
#include <sivra/ir/operation_catalogue.hpp>

namespace sivra::ir {

struct builtin_operation_ids {
  operation_id add;
  operation_id multiply;
  operation_id subtract;
  operation_id maximum;
};

core::result_t<builtin_operation_ids> register_builtin_operations(
  operation_catalogue_builder& builder
);

} // namespace sivra::ir
