#pragma once

#include <sivra/core/diagnostic.hpp>
#include <sivra/core/result.hpp>
#include <sivra/ir/constant.hpp>
#include <sivra/ir/operation.hpp>

#include <memory>
#include <span>
#include <variant>
#include <vector>

namespace sivra::canonicalizer {

struct no_evaluation {};

struct evaluated_constant {
  ir::constant_value value;
};

struct invalid_evaluation {
  core::diagnostic diagnostic;
};

using evaluation_result = std::variant<no_evaluation, evaluated_constant, invalid_evaluation>;

using evaluate_operation_fn = evaluation_result (*)(
  std::span<const ir::constant_value> operands,
  const ir::operation_attributes& attributes,
  ir::value_type result_type
);

struct operation_evaluator {
  ir::operation_key operation;
  evaluate_operation_fn evaluate = nullptr;
};

class evaluator_catalogue {
public:
  [[nodiscard]] static core::result_t<std::shared_ptr<const evaluator_catalogue>> create(
    std::vector<operation_evaluator> evaluators
  );

  [[nodiscard]] const operation_evaluator* find(
    const ir::operation_key& operation
  ) const;
  [[nodiscard]] std::span<const operation_evaluator> evaluators() const;

private:
  explicit evaluator_catalogue(
    std::vector<operation_evaluator> evaluators
  );

  std::vector<operation_evaluator> m_evaluators;
};

[[nodiscard]] std::shared_ptr<const evaluator_catalogue> builtin_evaluator_catalogue();

} // namespace sivra::canonicalizer
