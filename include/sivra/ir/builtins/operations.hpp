#pragma once

#include <sivra/core/result.hpp>
#include <sivra/ir/id.hpp>
#include <sivra/ir/operation_catalogue.hpp>

namespace sivra::ir {

struct builtin_operation_ids {
  operation_id add;
  operation_id multiply;
  operation_id subtract;
  operation_id divide;
  operation_id maximum;
  operation_id minimum;
  operation_id sqrt;
  operation_id reciprocal;
  operation_id reciprocal_sqrt;
  operation_id square;
  operation_id bit_and;
  operation_id bit_and_not;
  operation_id bit_or;
  operation_id bit_xor;
  operation_id copy;
};

core::result_t<builtin_operation_ids> register_builtin_operations(
  operation_catalogue_builder& builder
);

} // namespace sivra::ir
