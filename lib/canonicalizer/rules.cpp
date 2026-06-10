#include "rules.hpp"

#include <sivra/ir/constant.hpp>
#include <sivra/ir/leaf.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace {

template <typename T>
T value_or_throw(
  sivra::core::result_t<T> result
) {
  if (!result.has_value()) {
    const auto message =
      result.error().empty() ? "IR graph construction failed" : result.error().front().message;
    throw std::logic_error(message);
  }
  return std::move(*result);
}

const sivra::ir::constant_value* constant_leaf(
  const sivra::ir::expression_node& node
) {
  const auto* constant = node.get_if_constant();
  return constant == nullptr ? nullptr : &constant->value;
}

bool matches_well_known_constant(
  const sivra::ir::scalar_constant_t& actual,
  sivra::ir::well_known_constant expected
) {
  return std::visit(
    [expected](const auto& value) {
      switch (expected) {
      case sivra::ir::well_known_constant::zero:
        return value.value() == 0;
      case sivra::ir::well_known_constant::one:
        return value.value() == 1;
      case sivra::ir::well_known_constant::all_bits_set:
        return value.bits == std::numeric_limits<std::uint32_t>::max();
      }

      return false;
    },
    actual
  );
}

bool matches_operation_constant(
  const sivra::ir::scalar_constant_t& actual,
  const sivra::ir::operation_constant& expected,
  const sivra::ir::value_type& result_type
) {
  if (result_type.kind() != sivra::ir::value_type_kind::scalar &&
      result_type.kind() != sivra::ir::value_type_kind::vector) {
    return false;
  }

  if (const auto* well_known = std::get_if<sivra::ir::well_known_constant>(&expected.element)) {
    return matches_well_known_constant(actual, *well_known);
  }

  const auto& explicit_value = std::get<sivra::ir::scalar_constant_t>(expected.element);
  const bool matches_type = (result_type.category() == sivra::ir::scalar_category::floating_point &&
                             result_type.element_bit_width() == 32 &&
                             std::holds_alternative<sivra::ir::f32_constant>(explicit_value)) ||
                            (result_type.category() == sivra::ir::scalar_category::signed_integer &&
                             result_type.element_bit_width() == 32 &&
                             std::holds_alternative<sivra::ir::i32_constant>(explicit_value));

  if (!matches_type) {
    return false;
  }

  return actual == explicit_value;
}

bool constant_operand_matches(
  const sivra::ir::expression_node& source_node,
  const sivra::ir::expression_node& child_node,
  const sivra::ir::operation_constant& expected
) {
  if (source_node.result_type() != child_node.result_type()) {
    return false;
  }

  const auto* constant = constant_leaf(child_node);
  if (constant == nullptr || constant->result_type() != source_node.result_type()) {
    return false;
  }

  for (std::size_t index = 0; index < constant->element_count(); ++index) {
    if (!matches_operation_constant(
          constant->element(index), expected, source_node.result_type()
        )) {
      return false;
    }
  }

  return true;
}

bool is_identity_operand(
  const sivra::ir::expression_node& source_node,
  const sivra::ir::operation_def& operation,
  const sivra::ir::expression_node& child_node
) {
  const auto& identity = operation.semantics().identity;
  return identity.has_value() && constant_operand_matches(source_node, child_node, *identity);
}

bool is_annihilator_operand(
  const sivra::ir::expression_node& source_node,
  const sivra::ir::operation_def& operation,
  const sivra::ir::expression_node& child_node
) {
  const auto& annihilator = operation.semantics().annihilator;
  return annihilator.has_value() && constant_operand_matches(source_node, child_node, *annihilator);
}

bool is_flattenable_associative_child(
  const sivra::ir::expression_node& parent,
  const sivra::ir::expression_node& child
) {
  const auto* parent_application = parent.get_if_operation();
  const auto* child_application = child.get_if_operation();
  return parent_application != nullptr && child_application != nullptr &&
         child_application->operation == parent_application->operation &&
         child.result_type() == parent.result_type() && child.operands().size() >= 2;
}

std::optional<sivra::ir::node_id> remove_identity_operands(
  sivra::canonicalizer::rewrite_context& context,
  const sivra::ir::expression_node& source_node,
  const sivra::ir::operation_def& operation,
  std::vector<sivra::ir::node_id>& children
) {
  std::optional<sivra::ir::node_id> first_identity;

  std::erase_if(children, [&](const auto child) {
    if (!is_identity_operand(source_node, operation, context.rebuilt_node(child))) {
      return false;
    }

    if (!first_identity.has_value()) {
      first_identity = child;
    }

    return true;
  });

  return first_identity;
}

sivra::canonicalizer::rewrite_result apply_associative_flattening(
  sivra::canonicalizer::rewrite_context& context,
  const sivra::ir::expression_node& source_node,
  std::vector<sivra::ir::node_id>& children
) {
  const auto& operation = context.operation_for(source_node);
  if (!context.is_trait_enabled(sivra::ir::operation_trait::associative) ||
      !operation.has_trait(sivra::ir::operation_trait::associative) || children.size() < 2) {
    return {};
  }

  std::vector<sivra::ir::node_id> flattened;
  flattened.reserve(children.size());

  bool changed = false;
  for (const auto child : children) {
    const auto& child_node = context.rebuilt_node(child);
    if (!is_flattenable_associative_child(source_node, child_node)) {
      flattened.push_back(child);
      continue;
    }

    flattened.insert(flattened.end(), child_node.operands().begin(), child_node.operands().end());
    changed = true;
  }

  if (!changed) {
    return {};
  }

  children = std::move(flattened);
  return {
    .action = sivra::canonicalizer::rewrite_action::children_changed,
  };
}

sivra::canonicalizer::rewrite_result apply_identity_elimination(
  sivra::canonicalizer::rewrite_context& context,
  const sivra::ir::expression_node& source_node,
  std::vector<sivra::ir::node_id>& children
) {
  const auto& operation = context.operation_for(source_node);

  if (!operation.semantics().identity.has_value()) {
    return {};
  }

  const auto first_identity = remove_identity_operands(context, source_node, operation, children);

  // no identity operands found, so this rule does not apply
  if (!first_identity.has_value()) {
    return {};
  }

  // when all children are identities, reuse one existing identity value
  if (children.empty()) {
    return {
      .action = sivra::canonicalizer::rewrite_action::replaced,
      .replacement = first_identity,
    };
  }

  // a single remaining child represents the operation result directly
  if (children.size() == 1) {
    return {
      .action = sivra::canonicalizer::rewrite_action::replaced,
      .replacement = children.front(),
    };
  }

  return {
    .action = sivra::canonicalizer::rewrite_action::children_changed,
  };
}

sivra::canonicalizer::rewrite_result apply_annihilator_collapse(
  sivra::canonicalizer::rewrite_context& context,
  const sivra::ir::expression_node& source_node,
  std::vector<sivra::ir::node_id>& children
) {
  const auto& operation = context.operation_for(source_node);
  if (!operation.semantics().annihilator.has_value()) {
    return {};
  }

  for (const auto child : children) {
    if (is_annihilator_operand(source_node, operation, context.rebuilt_node(child))) {
      return {
        .action = sivra::canonicalizer::rewrite_action::replaced,
        .replacement = child,
      };
    }
  }

  return {};
}

const std::array rule_entries{
#define SIVRA_CANONICALIZER_RULE(name, value, enabled_by_default, description)                     \
  sivra::canonicalizer::rule_entry{                                                                \
    sivra::canonicalizer::rule::name, #name, description, apply_##name},
#include <sivra/canonicalizer/rule.def>
#undef SIVRA_CANONICALIZER_RULE
};

} // namespace

namespace sivra::canonicalizer {

rewrite_context::rewrite_context(
  ir::expression_graph& rebuilt,
  const options& config
)
    : m_rebuilt(rebuilt),
      m_builder(rebuilt),
      m_options(config) {
}

bool rewrite_context::is_trait_enabled(
  ir::operation_trait trait
) const {
  return m_options.is_trait_enabled(trait);
}

const ir::operation_def& rewrite_context::operation_for(
  const ir::expression_node& source_node
) const {
  const auto* application = source_node.get_if_operation();
  if (application == nullptr) {
    throw std::logic_error("canonicalizer rule requires an operation node");
  }
  return m_rebuilt.catalogue().operation(application->operation);
}

const ir::expression_node& rewrite_context::rebuilt_node(
  ir::node_id node
) const {
  return m_rebuilt.at(node);
}

ir::node_id rewrite_context::copy_node(
  const ir::expression_node& source_node,
  std::vector<ir::node_id> copied_children
) {
  if (const auto* constant = source_node.get_if_constant()) {
    return value_or_throw(m_builder.make_constant(constant->value));
  }
  if (const auto* symbol = source_node.get_if_symbol()) {
    return value_or_throw(m_builder.make_symbol(symbol->name, source_node.result_type()));
  }
  if (const auto* external = source_node.get_if_external_value()) {
    return value_or_throw(
      m_builder.make_external_value(external->value, source_node.result_type())
    );
  }
  if (const auto* unknown = source_node.get_if_unknown()) {
    return value_or_throw(m_builder.make_unknown(unknown->reason, source_node.result_type()));
  }

  const auto* application = source_node.get_if_operation();
  if (application == nullptr) {
    throw std::logic_error("unsupported expression node kind");
  }
  return value_or_throw(
    m_builder.apply(application->operation, copied_children, source_node.result_type())
  );
}

std::span<const rule_entry> rule_pipeline() {
  return rule_entries;
}

} // namespace sivra::canonicalizer
