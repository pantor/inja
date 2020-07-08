// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_NODE_HPP_
#define INCLUDE_INJA_NODE_HPP_

#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "string_view.hpp"

namespace inja {

using json = nlohmann::json;

class NodeVisitor;
class BlockNode;
class TextNode;
class ExpressionNode;
class LiteralNode;
class JsonNode;
class FunctionNode;
class ExpressionListNode;
class StatementNode;
class ForStatementNode;
class ForArrayStatementNode;
class ForObjectStatementNode;
class IfStatementNode;
class IncludeStatementNode;


class NodeVisitor {
public:
  virtual void visit(const BlockNode& node);
  virtual void visit(const TextNode& node);
  virtual void visit(const ExpressionNode& node);
  virtual void visit(const LiteralNode& node);
  virtual void visit(const JsonNode& node);
  virtual void visit(const FunctionNode& node);
  virtual void visit(const ExpressionListNode& node);
  virtual void visit(const StatementNode& node);
  virtual void visit(const ForStatementNode& node);
  virtual void visit(const ForArrayStatementNode& node);
  virtual void visit(const ForObjectStatementNode& node);
  virtual void visit(const IfStatementNode& node);
  virtual void visit(const IncludeStatementNode& node);
};


class AstNode {
public:
  virtual void accept(NodeVisitor& v) const = 0;

  size_t pos;

  AstNode(size_t pos) : pos(pos) { }
};


class BlockNode : public AstNode {
public:
  std::vector<std::shared_ptr<AstNode>> nodes;

  explicit BlockNode() : AstNode(0) {}

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class TextNode : public AstNode {
public:
  std::string content;

  explicit TextNode(const std::string& content, size_t pos): content(content), AstNode(pos) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class ExpressionNode : public AstNode {
public:
  explicit ExpressionNode(size_t pos) : AstNode(pos) {}

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class LiteralNode : public ExpressionNode {
public:
  json value;

  explicit LiteralNode(const json& value): value(value), ExpressionNode(0) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class JsonNode : public ExpressionNode {
public:
  std::string json_ptr;

  explicit JsonNode(const std::string& json_ptr): json_ptr(json_ptr), ExpressionNode(0) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class FunctionNode : public ExpressionNode {
public:
  enum class Operation {
    Not,
    And,
    Or,
    In,
    Equal,
    NotEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Add,
    Subtract,
    Multiplication,
    Division,
    Power,
    Modulo,
    At,
    Default,
    DivisibleBy,
    Even,
    Exists,
    ExistsInObject,
    First,
    Float,
    Int,
    IsArray,
    IsBoolean,
    IsFloat,
    IsInteger,
    IsNumber,
    IsObject,
    IsString,
    Last,
    Length,
    Lower,
    Max,
    Min,
    Odd,
    Range,
    Round,
    Sort,
    Upper,
    Callback,
    ParenLeft,
    ParenRight,
    None,
  };

  enum class Associativity {
    Left,
    Right,
  };

  Operation operation;
  std::string name;
  unsigned int precedence;
  Associativity associativity;
  
  unsigned int number_args;

  explicit FunctionNode(nonstd::string_view name) : operation(Operation::Callback), name(name), precedence(1), associativity(Associativity::Left), number_args(1), ExpressionNode(0) { }
  explicit FunctionNode(Operation operation) : operation(operation), number_args(1), ExpressionNode(0) {
    switch (operation) {
      case Operation::Not: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Operation::And: {
        precedence = 1;
        associativity = Associativity::Left;
      } break;
      case Operation::Or: {
        precedence = 1;
        associativity = Associativity::Left;
      } break;
      case Operation::In: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Operation::Equal: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Operation::NotEqual: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Operation::Greater: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Operation::GreaterEqual: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Operation::Less: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Operation::LessEqual: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      default: {
        precedence = 1;
        associativity = Associativity::Left;
      }
    }
  }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class ExpressionListNode : public AstNode {
public:
  std::vector<std::shared_ptr<ExpressionNode>> rpn_output;

  explicit ExpressionListNode() : AstNode(0) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class StatementNode : public AstNode {
public:
  StatementNode(size_t pos) : AstNode(pos) { }

  virtual void accept(NodeVisitor& v) const = 0;
};

class ForStatementNode : public StatementNode {
public:
  ExpressionListNode condition;
  BlockNode body;
  BlockNode *parent;

  ForStatementNode(size_t pos) : StatementNode(pos) { }

  virtual void accept(NodeVisitor& v) const = 0;
};

class ForArrayStatementNode : public ForStatementNode {
public:
  nonstd::string_view value;

  explicit ForArrayStatementNode(nonstd::string_view value) : value(value), ForStatementNode(0) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class ForObjectStatementNode : public ForStatementNode {
public:
  nonstd::string_view key;
  nonstd::string_view value;

  explicit ForObjectStatementNode(nonstd::string_view key, nonstd::string_view value) : key(key), value(value), ForStatementNode(0) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class IfStatementNode : public StatementNode {
public:
  ExpressionListNode condition;
  BlockNode true_statement;
  BlockNode false_statement;
  bool has_false_statement {false};
  BlockNode *parent;

  explicit IfStatementNode() : StatementNode(0) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class IncludeStatementNode : public StatementNode {
public:
  std::string file;

  explicit IncludeStatementNode(const std::string& file) : file(file), StatementNode(0) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  };
};


inline void NodeVisitor::visit(const BlockNode& node) {
  std::cout << "<block (" << node.nodes.size() << ")>" << std::endl;

  for (auto& n : node.nodes) {
    std::cout << "  ";
    n->accept(*this);
  }
}

inline void NodeVisitor::visit(const TextNode& node) {
  std::cout << node.content << std::endl;
}

inline void NodeVisitor::visit(const ExpressionNode& node) {

}

inline void NodeVisitor::visit(const LiteralNode& node) {
  std::cout << "<json " << node.value << ">" << std::endl;
}

inline void NodeVisitor::visit(const JsonNode& node) {
  std::cout << "<json ptr " << node.json_ptr << ">" << std::endl;
}

inline void NodeVisitor::visit(const FunctionNode& node) {
  std::cout << "<function " << node.name << "> " << std::endl;
}

inline void NodeVisitor::visit(const ExpressionListNode& node) {
  for (auto& n : node.rpn_output) {
    std::cout << "    ";
    n->accept(*this);
  }
}

inline void NodeVisitor::visit(const StatementNode& node) {

}

inline void NodeVisitor::visit(const ForStatementNode& node) {

}

inline void NodeVisitor::visit(const ForArrayStatementNode& node) {
  std::cout << "<for array>" << std::endl;
}

inline void NodeVisitor::visit(const ForObjectStatementNode& node) {
  std::cout << "<for object>" << std::endl;
}

inline void NodeVisitor::visit(const IfStatementNode& node) {
  std::cout << "<if>" << std::endl;

  node.condition.accept(*this);
  node.true_statement.accept(*this);
}

inline void NodeVisitor::visit(const IncludeStatementNode& node) {
  std::cout << "<include " << node.file << ">" << std::endl;
}



} // namespace inja

#endif // INCLUDE_INJA_NODE_HPP_
