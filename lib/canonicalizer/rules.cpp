#include "rules.hpp"

#include <sivra/ir/constant.hpp>
#include <sivra/ir/leaf.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <variant>
#include <vector>

namespace {

const sivra::ir::constant_value* constant_leaf(
  const sivra::ir::expression_node& node
) {
  if (!node.leaf_value().has_value()) {
    return nullptr;
  }

  return std::get_if<sivra::ir::constant_value>(&*node.leaf_value());
}

const sivra::ir::scalar_type_def* scalar_element_type(
  const sivra::ir::type& result_type
) {
  switch (result_type.kind()) {
  case sivra::ir::type_kind::scalar:
    return &static_cast<const sivra::ir::scalar_type_def&>(result_type);

  case sivra::ir::type_kind::vector: {
    const auto& element_type =
      static_cast<const sivra::ir::vector_type_def&>(result_type).element_type();
    if (element_type.kind() != sivra::ir::type_kind::scalar) {
      return nullptr;
    }
    return &static_cast<const sivra::ir::scalar_type_def&>(element_type);
  }

  case sivra::ir::type_kind::matrix: {
    const auto& element_type =
      static_cast<const sivra::ir::matrix_type_def&>(result_type).element_type();
    if (element_type.kind() != sivra::ir::type_kind::scalar) {
      return nullptr;
    }
    return &static_cast<const sivra::ir::scalar_type_def&>(element_type);
  }

  case sivra::ir::type_kind::unknown:
    return nullptr;
  }

  return nullptr;
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
  const sivra::ir::type& result_type
) {
  const auto* element_type = scalar_element_type(result_type);
  if (element_type == nullptr) {
    return false;
  }

  if (const auto* well_known = std::get_if<sivra::ir::well_known_constant>(&expected.element)) {
    return matches_well_known_constant(actual, *well_known);
  }

  const auto& explicit_value = std::get<sivra::ir::scalar_constant_t>(expected.element);
  const bool matches_type = (element_type->scalar() == sivra::ir::scalar_type::f32 &&
                             std::holds_alternative<sivra::ir::f32_constant>(explicit_value)) ||
                            (element_type->scalar() == sivra::ir::scalar_type::i32 &&
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
  if (&source_node.result_type() != &child_node.result_type()) {
    return false;
  }

  const auto* constant = constant_leaf(child_node);
  if (constant == nullptr || &constant->result_type() != &source_node.result_type()) {
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
  return child.operation() == parent.operation() && &child.result_type() == &parent.result_type() &&
         child.children().size() >= 2 && !child.leaf_value().has_value();
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

    flattened.insert(flattened.end(), child_node.children().begin(), child_node.children().end());
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
  return m_rebuilt.context().operations().at(source_node.operation());
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
  return m_rebuilt.add_node(
    source_node.operation(),
    source_node.result_type(),
    std::move(copied_children),
    source_node.leaf_value()
  );
}

std::span<const rule_entry> rule_pipeline() {
  return rule_entries;
}

} // namespace sivra::canonicalizer
