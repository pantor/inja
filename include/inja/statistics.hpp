#ifndef INCLUDE_INJA_STATISTICS_HPP_
#define INCLUDE_INJA_STATISTICS_HPP_

#include "node.hpp"

namespace inja {

/*!
 * \brief A class for counting statistics on a Template.
 */
class StatisticsVisitor : public NodeVisitor {
  void visit(const BlockNode& node) override {
    for (const auto& n : node.nodes) {
      n->accept(*this);
    }
  }

  void visit(const TextNode&) override {}
  void visit(const ExpressionNode&) override {}
  void visit(const LiteralNode&) override {}

  void visit(const DataNode&) override {
    variable_counter += 1;
  }

  void visit(const FunctionNode& node) override {
    for (const auto& n : node.arguments) {
      n->accept(*this);
    }
  }

  void visit(const ExpressionListNode& node) override {
    node.root->accept(*this);
  }

  void visit(const StatementNode&) override {}
  void visit(const ForStatementNode&) override {}

  void visit(const ForArrayStatementNode& node) override {
    node.condition.accept(*this);
    node.body.accept(*this);
  }

  void visit(const ForObjectStatementNode& node) override {
    node.condition.accept(*this);
    node.body.accept(*this);
  }

  void visit(const IfStatementNode& node) override {
    node.condition.accept(*this);
    node.true_statement.accept(*this);
    node.false_statement.accept(*this);
  }

  void visit(const IncludeStatementNode&) override {}

  void visit(const ExtendsStatementNode&) override {}

  void visit(const BlockStatementNode& node) override {
    node.block.accept(*this);
  }

  void visit(const SetStatementNode&) override {}

public:
  size_t variable_counter {0};

  explicit StatisticsVisitor() {}
};

} // namespace inja

#endif // INCLUDE_INJA_STATISTICS_HPP_
