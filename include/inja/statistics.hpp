// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_STATISTICS_HPP_
#define INCLUDE_INJA_STATISTICS_HPP_

#include "node.hpp"


namespace inja {

/*!
 * \brief A class for counting statistics on a Template.
 */
struct StatisticsVisitor : public NodeVisitor {
  unsigned int variable_counter;

  explicit StatisticsVisitor() : variable_counter(0) { }

  void visit(const BlockNode& node) {
    for (auto& n : node.nodes) {
      n->accept(*this);
    }
  }

  void visit(const TextNode& node) { }
  void visit(const ExpressionNode& node) { }
  void visit(const LiteralNode& node) { }

  void visit(const JsonNode& node) {
    variable_counter += 1;
  }

  void visit(const FunctionNode& node) { }

  void visit(const ExpressionListNode& node) {
    for (auto& n : node.rpn_output) {
      n->accept(*this);
    }
  }

  void visit(const StatementNode& node) { }
  void visit(const ForStatementNode& node) { }

  void visit(const ForArrayStatementNode& node) {
    node.condition.accept(*this);
    node.body.accept(*this);
  }

  void visit(const ForObjectStatementNode& node) {
    node.condition.accept(*this);
    node.body.accept(*this);
  }

  void visit(const IfStatementNode& node) {
    node.condition.accept(*this);
    node.true_statement.accept(*this);
    node.false_statement.accept(*this);
  }

  void visit(const IncludeStatementNode& node) { }
};

} // namespace inja

#endif // INCLUDE_INJA_STATISTICS_HPP_
