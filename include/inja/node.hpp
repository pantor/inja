// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_NODE_HPP_
#define INCLUDE_INJA_NODE_HPP_

#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "function_storage.hpp"
#include "string_view.hpp"


namespace inja {

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
  nlohmann::json value;

  explicit LiteralNode(const nlohmann::json& value, size_t pos): value(value), ExpressionNode(pos) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class JsonNode : public ExpressionNode {
public:
  std::string json_ptr;

  explicit JsonNode(const std::string& json_ptr, size_t pos): json_ptr(json_ptr), ExpressionNode(pos) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class FunctionNode : public ExpressionNode {
  using Op = FunctionStorage::Operation;

public:
  enum class Associativity {
    Left,
    Right,
  };

  unsigned int precedence;
  Associativity associativity;

  Op operation;

  std::string name;
  unsigned int number_args;
  CallbackFunction callback;

  explicit FunctionNode(nonstd::string_view name, size_t pos) : operation(Op::Callback), name(name), precedence(5), associativity(Associativity::Left), number_args(1), ExpressionNode(pos) { }
  explicit FunctionNode(Op operation, size_t pos) : operation(operation), number_args(1), ExpressionNode(pos) {
    switch (operation) {
      case Op::Not: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::And: {
        precedence = 1;
        associativity = Associativity::Left;
      } break;
      case Op::Or: {
        precedence = 1;
        associativity = Associativity::Left;
      } break;
      case Op::In: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::Equal: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::NotEqual: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::Greater: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::GreaterEqual: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::Less: {
        precedence = 2;
        associativity = Associativity::Left;
      } break;
      case Op::LessEqual: {
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
  explicit ExpressionListNode(size_t pos) : AstNode(pos) { }

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

  explicit ForArrayStatementNode(nonstd::string_view value, size_t pos) : value(value), ForStatementNode(pos) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class ForObjectStatementNode : public ForStatementNode {
public:
  nonstd::string_view key;
  nonstd::string_view value;

  explicit ForObjectStatementNode(nonstd::string_view key, nonstd::string_view value, size_t pos) : key(key), value(value), ForStatementNode(pos) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class IfStatementNode : public StatementNode {
public:
  ExpressionListNode condition;
  BlockNode true_statement;
  BlockNode false_statement;
  BlockNode *parent;

  bool is_nested;
  bool has_false_statement {false};

  explicit IfStatementNode(size_t pos) : is_nested(false), StatementNode(pos) { }
  explicit IfStatementNode(bool is_nested, size_t pos) : is_nested(is_nested), StatementNode(pos) { }

  void accept(NodeVisitor& v) const {
    v.visit(*this);
  }
};

class IncludeStatementNode : public StatementNode {
public:
  std::string file;

  explicit IncludeStatementNode(const std::string& file, size_t pos) : file(file), StatementNode(pos) { }

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
