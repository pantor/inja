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
class StatementNode;
class ForStatementNode;
class IfStatementNode;
class IncludeStatementNode;


class AstNode {
public:
  virtual void accept(NodeVisitor&) = 0;
};

class NodeVisitor {
public:
  virtual void visit(BlockNode& node);
  virtual void visit(TextNode& node);
  virtual void visit(ExpressionNode& node);
  virtual void visit(LiteralNode& node);
  virtual void visit(JsonNode& node);
  virtual void visit(FunctionNode& node);
  virtual void visit(StatementNode& node);
  virtual void visit(ForStatementNode& node);
  virtual void visit(IfStatementNode& node);
  virtual void visit(IncludeStatementNode& node);
};


class BlockNode : public AstNode {
public:
  std::vector<std::shared_ptr<AstNode>> nodes;

  void accept(NodeVisitor& v) {
    v.visit(*this);

    for (auto& n : nodes) {
      n->accept(v);
    }
  }
};

class ExpressionNode : public AstNode {
public:
  void accept(NodeVisitor& v) {
    v.visit(*this);
  }
};

class LiteralNode : public ExpressionNode {
public:
  json data;

  LiteralNode(const json& data): data(data) { }

  void accept(NodeVisitor& v) {
    v.visit(*this);
  }
};

class JsonNode : public ExpressionNode {
public:
  std::string json_ptr;

  JsonNode(const std::string& json_ptr): json_ptr(json_ptr) { }

  void accept(NodeVisitor& v) {
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
    Other,
  };

  Operation operation;
  std::vector<std::shared_ptr<ExpressionNode>> args;

  FunctionNode(Operation operation) : operation(operation) { }

  void accept(NodeVisitor& v) {
    v.visit(*this);

    for (auto& arg : args) {
      arg->accept(v);
    }
  }
};

class StatementNode : public AstNode {
public:
  virtual void accept(NodeVisitor& v) = 0;
};

class ForStatementNode : public StatementNode {
public:
  void accept(NodeVisitor& v) {
    v.visit(*this);
  };

  ExpressionNode condition;
  BlockNode body;
};

class IfStatementNode : public StatementNode {
public:
  void accept(NodeVisitor& v) {
    v.visit(*this);

    condition.accept(v);
    true_statement.accept(v);

    if (has_false_statement) {
      false_statement.accept(v);
    }
  };

  ExpressionNode condition;
  BlockNode true_statement;
  BlockNode false_statement;

  bool has_false_statement {false};
};

class IncludeStatementNode : public StatementNode {
public:
  std::string file;

  IncludeStatementNode(const std::string& file) : file(file) { }
  void accept(NodeVisitor& v) {
    v.visit(*this);
  };
};

class TextNode : public AstNode {
public:
  std::string content;

  TextNode(const std::string& content): content(content) { }

  void accept(NodeVisitor& v) {
    v.visit(*this);
  }
};

inline void NodeVisitor::visit(BlockNode& node) {
  std::cout << "<block (" << node.nodes.size() << ")>" << std::endl;
}

inline void NodeVisitor::visit(TextNode& node) {
  std::cout << node.content << std::endl;
}

inline void NodeVisitor::visit(ExpressionNode& node) {
  
}

inline void NodeVisitor::visit(LiteralNode& node) {
  std::cout << "<json literal> " << node.data << std::endl;
}

inline void NodeVisitor::visit(JsonNode& node) {
  std::cout << "<json ptr> " << node.json_ptr << std::endl;
}

inline void NodeVisitor::visit(FunctionNode& node) {
  std::cout << "<function> " << std::endl;
}

inline void NodeVisitor::visit(StatementNode& node) {
  
}

inline void NodeVisitor::visit(ForStatementNode& node) {
  std::cout << "<if>" << std::endl;
}

inline void NodeVisitor::visit(IfStatementNode& node) {
  std::cout << "<if>" << std::endl;
}

inline void NodeVisitor::visit(IncludeStatementNode& node) {
  std::cout << "<include " << node.file << ">" << std::endl;
}



} // namespace inja

#endif // INCLUDE_INJA_NODE_HPP_
