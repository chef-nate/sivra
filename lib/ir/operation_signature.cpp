#include <sivra/ir/operation_signature.hpp>

namespace sivra::ir {

core::result_t<void> arity_constraint::validate() const {
  if (maximum.has_value() && *maximum < minimum) {
    return core::fail<void>(
      "ir.signature.invalid_arity", "operation maximum arity is below minimum arity"
    );
  }
  return {};
}

bool arity_constraint::accepts(
  std::size_t count
) const {
  return count >= minimum && (!maximum.has_value() || count <= *maximum);
}

core::result_t<void> operation_signature::validate_definition() const {
  return arity.validate();
}

core::result_t<void> operation_signature::validate_application(
  const value_type& result_type,
  std::span<const value_type> actual_operand_types,
  const operation_attributes& attributes,
  const operation_attribute_schema& attribute_schema
) const {
  if (!arity.accepts(actual_operand_types.size())) {
    return core::fail<void>(
      "ir.graph.invalid_arity", "operation operand count does not match its signature"
    );
  }

  if (operand_types == operand_type_constraint::same_as_result) {
    for (const auto& operand_type : actual_operand_types) {
      if (operand_type != result_type) {
        return core::fail<void>(
          "ir.graph.type_mismatch", "operation operand type does not match result type"
        );
      }
    }
  }

  auto validated_attributes = attribute_schema.validate(attributes);
  if (!validated_attributes.has_value()) {
    return std::unexpected(std::move(validated_attributes.error()));
  }
  return {};
}

} // namespace sivra::ir
