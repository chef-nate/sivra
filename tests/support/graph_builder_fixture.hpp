#pragma once

#include <sivra/core/result.hpp>
#include <sivra/ir/builtins/operations.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/graph_builder.hpp>
#include <sivra/ir/operation_catalogue.hpp>

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sivra::test_support {

template <typename T>
T require_value(
  core::result_t<T> result
) {
  if (!result.has_value()) {
    const auto message =
      result.error().empty() ? "test IR construction failed" : result.error().front().message;
    throw std::runtime_error(message);
  }
  return std::move(*result);
}

struct test_catalogue {
  std::shared_ptr<const ir::operation_catalogue> catalogue;
  ir::builtin_operation_ids builtins;
  std::vector<ir::operation_id> custom;
};

inline test_catalogue make_test_catalogue(
  std::vector<ir::operation_registration> custom = {}
) {
  ir::operation_catalogue_builder builder;
  const auto builtins = require_value(ir::register_builtin_operations(builder));

  std::vector<ir::operation_id> custom_ids;
  custom_ids.reserve(custom.size());
  for (auto& registration : custom) {
    custom_ids.push_back(require_value(builder.register_operation(std::move(registration))));
  }

  return {
    .catalogue = require_value(std::move(builder).freeze()),
    .builtins = builtins,
    .custom = std::move(custom_ids),
  };
}

inline ir::operation_registration test_operation(
  std::string key,
  ir::operation_semantics semantics = {},
  ir::operation_signature signature = {
    .minimum_operands = 0,
    .maximum_operands = std::nullopt,
    .operands_match_result = true,
  }
) {
  return {
    .key = key,
    .name = std::move(key),
    .signature = signature,
    .semantics = std::move(semantics),
  };
}

class graph_builder_fixture {
public:
  explicit graph_builder_fixture(
    std::vector<ir::operation_registration> custom = {}
  )
      : operations(make_test_catalogue(std::move(custom))),
        graph(operations.catalogue),
        builder(graph) {}

  ir::node_id symbol(
    std::string name,
    ir::value_type type = ir::value_type::f32()
  ) {
    return require_value(builder.make_symbol(std::move(name), std::move(type)));
  }

  ir::node_id f32(
    float value
  ) {
    return require_value(builder.make_constant(
      ir::constant_value::scalar(ir::value_type::f32(), ir::f32_constant::from_value(value))
    ));
  }

  ir::node_id i32(
    std::int32_t value
  ) {
    return require_value(builder.make_constant(
      ir::constant_value::scalar(ir::value_type::i32(), ir::i32_constant::from_value(value))
    ));
  }

  ir::node_id apply(
    ir::operation_id operation,
    std::initializer_list<ir::node_id> operands,
    ir::value_type result_type = ir::value_type::f32()
  ) {
    const std::vector copied(operands);
    return require_value(builder.apply(operation, copied, std::move(result_type)));
  }

  test_catalogue operations;
  ir::expression_graph graph;
  ir::graph_builder builder;
};

} // namespace sivra::test_support
