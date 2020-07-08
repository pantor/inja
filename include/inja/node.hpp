// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_NODE_HPP_
#define INCLUDE_INJA_NODE_HPP_

#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "string_view.hpp"

namespace inja {

using json = nlohmann::json;

struct Node {
  enum class Op : uint8_t {
    Nop,
    // print StringRef (always immediate)
    PrintText,
    // print value
    PrintValue,
    // push value onto stack (always immediate)
    Push,

    // builtin functions
    // result is pushed to stack
    // args specify number of arguments
    // all functions can take their "last" argument either immediate
    // or popped off stack (e.g. if immediate, it's like the immediate was
    // just pushed to the stack)
    Not,
    And,
    Or,
    In,
    Equal,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    At,
    Different,
    DivisibleBy,
    Even,
    First,
    Float,
    Int,
    Last,
    Length,
    Lower,
    Max,
    Min,
    Odd,
    Range,
    Result,
    Round,
    Sort,
    Upper,
    Exists,
    ExistsInObject,
    IsBoolean,
    IsNumber,
    IsInteger,
    IsFloat,
    IsObject,
    IsArray,
    IsString,
    Default,

    // include another template
    // value is the template name
    Include,

    // callback function
    // str is the function name (this means it cannot be a lookup)
    // args specify number of arguments
    // as with builtin functions, "last" argument can be immediate
    Callback,

    // unconditional jump
    // args is the index of the node to jump to.
    Jump,

    // conditional jump
    // value popped off stack is checked for truthyness
    // if false, args is the index of the node to jump to.
    // if true, no action is taken (falls through)
    ConditionalJump,

    // start loop
    // value popped off stack is what is iterated over
    // args is index of node after end loop (jumped to if iterable is empty)
    // immediate value is key name (for maps)
    // str is value name
    StartLoop,

    // end a loop
    // args is index of the first node in the loop body
    EndLoop,
  };

  enum Flag {
    // location of value for value-taking ops (mask)
    ValueMask = 0x03,
    // pop value off stack
    ValuePop = 0x00,
    // value is immediate rather than on stack
    ValueImmediate = 0x01,
    // lookup immediate str (dot notation)
    ValueLookupDot = 0x02,
    // lookup immediate str (json pointer notation)
    ValueLookupPointer = 0x03,
  };

  Op op {Op::Nop};
  uint32_t args : 30;
  uint32_t flags : 2;

  json value;
  std::string str;
  size_t pos;

  explicit Node(Op op, unsigned int args, size_t pos) : op(op), args(args), flags(0), pos(pos) {}
  explicit Node(Op op, nonstd::string_view str, unsigned int flags, size_t pos) : op(op), args(0), flags(flags), str(str), pos(pos) {}
  explicit Node(Op op, json &&value, unsigned int flags, size_t pos) : op(op), args(0), flags(flags), value(std::move(value)), pos(pos) {}
};




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
    // Add,
    // Subtract,
    // Multiplication,
    // Division,
    // Power,
    // Modulo,
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
    At,
    Different,
    DivisibleBy,
    Even,
    First,
    Float,
    Int,
    Last,
    Length,
    Lower,
    Max,
    Min,
    Odd,
    Range,
    Result,
    Round,
    Sort,
    Upper,
    Exists,
    ExistsInObject,
    IsBoolean,
    IsNumber,
    IsInteger,
    IsFloat,
    IsObject,
    IsArray,
    IsString,
    Default,
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

  explicit FunctionNode(Operation operation) : operation(operation), ExpressionNode(0) {
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

  explicit FunctionNode(nonstd::string_view name) : operation(Operation::Callback), name(name), precedence(1), associativity(Associativity::Left), ExpressionNode(0) { }

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
