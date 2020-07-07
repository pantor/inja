// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_PARSER_HPP_
#define INCLUDE_INJA_PARSER_HPP_

#include <limits>
#include <string>
#include <utility>
#include <queue>
#include <vector>

#include "config.hpp"
#include "exceptions.hpp"
#include "function_storage.hpp"
#include "lexer.hpp"
#include "template.hpp"
#include "node.hpp"
#include "token.hpp"
#include "utils.hpp"

#include <nlohmann/json.hpp>

namespace inja {

class ParserStatic {
  using Operation = FunctionNode::Operation;

  ParserStatic() {
    function_storage.add_function("at", 2, Operation::At);
    function_storage.add_function("default", 2, Operation::Default);
    function_storage.add_function("divisibleBy", 2, Operation::DivisibleBy);
    function_storage.add_function("even", 1, Operation::Even);
    function_storage.add_function("first", 1, Operation::First);
    function_storage.add_function("float", 1, Operation::Float);
    function_storage.add_function("int", 1, Operation::Int);
    function_storage.add_function("last", 1, Operation::Last);
    function_storage.add_function("length", 1, Operation::Length);
    function_storage.add_function("lower", 1, Operation::Lower);
    function_storage.add_function("max", 1, Operation::Max);
    function_storage.add_function("min", 1, Operation::Min);
    function_storage.add_function("odd", 1, Operation::Odd);
    function_storage.add_function("range", 1, Operation::Range);
    function_storage.add_function("round", 2, Operation::Round);
    function_storage.add_function("sort", 1, Operation::Sort);
    function_storage.add_function("upper", 1, Operation::Upper);
    function_storage.add_function("exists", 1, Operation::Exists);
    function_storage.add_function("existsIn", 2, Operation::ExistsInObject);
    function_storage.add_function("isBoolean", 1, Operation::IsBoolean);
    function_storage.add_function("isNumber", 1, Operation::IsNumber);
    function_storage.add_function("isInteger", 1, Operation::IsInteger);
    function_storage.add_function("isFloat", 1, Operation::IsFloat);
    function_storage.add_function("isObject", 1, Operation::IsObject);
    function_storage.add_function("isArray", 1, Operation::IsArray);
    function_storage.add_function("isString", 1, Operation::IsString);
  }

public:
  ParserStatic(const ParserStatic &) = delete;
  ParserStatic &operator=(const ParserStatic &) = delete;

  static const ParserStatic &get_instance() {
    static ParserStatic instance;
    return instance;
  }

  FunctionStorage function_storage;
};


/*!
 * \brief Class for parsing an inja Template.
 */
class Parser {
  const ParserStatic &parser_static;
  const ParserConfig &config;

  Lexer lexer;
  TemplateStorage &template_storage;

  Token tok;
  Token peek_tok;
  bool have_peek_tok {false};

  std::vector<size_t> loop_stack;

  BlockNode *current_block {nullptr};
  ExpressionListNode *current_expression_list {nullptr};
  std::stack<std::shared_ptr<FunctionNode>> operator_stack;

  std::stack<IfStatementNode*> if_statement_stack;
  std::stack<ForStatementNode*> for_statement_stack;

  void throw_parser_error(const std::string &message) {
    throw ParserError(message, lexer.current_position());
  }

  void get_next_token() {
    if (have_peek_tok) {
      tok = peek_tok;
      have_peek_tok = false;
    } else {
      tok = lexer.scan();
    }
  }

  void get_peek_token() {
    if (!have_peek_tok) {
      peek_tok = lexer.scan();
      have_peek_tok = true;
    }
  }


public:
  explicit Parser(const ParserConfig &parser_config, const LexerConfig &lexer_config,
                  TemplateStorage &included_templates)
      : config(parser_config), lexer(lexer_config), template_storage(included_templates),
        parser_static(ParserStatic::get_instance()) {}

  bool parse_expression(Template &tmpl) {
    while (tok.kind != Token::Kind::ExpressionClose && tok.kind != Token::Kind::StatementClose) {
      // Literals
      if (tok.kind == Token::Kind::Number || tok.kind == Token::Kind::String) {
        current_expression_list->rpn_output.emplace_back(std::make_shared<LiteralNode>(static_cast<std::string>(tok.text)));
      
      } else if (tok.kind == Token::Kind::Id) {
        get_peek_token();

        // Functions
        if (peek_tok.kind == Token::Kind::LeftParen) {
          operator_stack.emplace(std::make_shared<FunctionNode>(static_cast<std::string>(tok.text)));

        // Operator
        } else if (tok.text == "and" || tok.text == "or" || tok.text == "in") {
          goto parse_operator;
        
        // Variables
        } else {
          current_expression_list->rpn_output.emplace_back(std::make_shared<JsonNode>(static_cast<std::string>(tok.text)));
        }

      // Operators
      } else if (tok.kind == Token::Kind::Equal || tok.kind == Token::Kind::NotEqual || tok.kind == Token::Kind::GreaterThan || tok.kind == Token::Kind::GreaterEqual || tok.kind == Token::Kind::LessThan || tok.kind == Token::Kind::LessEqual) {

  parse_operator:
        FunctionNode::Operation operation;
        switch (tok.kind) {
          case Token::Kind::Id: {
            if (tok.text == "and") {
              operation = FunctionNode::Operation::And;
            } else if (tok.text == "or") {
              operation = FunctionNode::Operation::Or;
            } else if (tok.text == "in") {
              operation = FunctionNode::Operation::In;
            } else {
              throw_parser_error("unknown operator in parser.");
            }
          } break;
          case Token::Kind::Equal: {
            operation = FunctionNode::Operation::Equal;
          } break;
          case Token::Kind::NotEqual: {
            operation = FunctionNode::Operation::NotEqual;
          } break;
          case Token::Kind::GreaterThan: {
            operation = FunctionNode::Operation::Greater;
          } break;
          case Token::Kind::GreaterEqual: {
            operation = FunctionNode::Operation::GreaterEqual;
          } break;
          case Token::Kind::LessThan: {
            operation = FunctionNode::Operation::Less;
          } break;
          case Token::Kind::LessEqual: {
            operation = FunctionNode::Operation::LessEqual;
          } break;
          default: {
            throw_parser_error("unknown operator in parser.");
          }
        }
        auto function_node = std::make_shared<FunctionNode>(operation);

        while (!operator_stack.empty() && ((operator_stack.top()->precedence > function_node->precedence) || (operator_stack.top()->precedence == function_node->precedence && function_node->associativity == FunctionNode::Associativity::Left)) && (operator_stack.top()->operation != FunctionNode::Operation::ParenLeft)) {
          current_expression_list->rpn_output.emplace_back(operator_stack.top());
          operator_stack.pop();
        }

        current_expression_list->rpn_output.emplace_back(function_node);

      // Parens
      } else if (tok.kind == Token::Kind::LeftParen) {
        operator_stack.emplace(std::make_shared<FunctionNode>(FunctionNode::Operation::ParenLeft));

      } else if (tok.kind == Token::Kind::RightParen) {
        while (operator_stack.top()->operation != FunctionNode::Operation::ParenLeft) {
          current_expression_list->rpn_output.emplace_back(operator_stack.top());
          operator_stack.pop();
        }

        if (operator_stack.top()->operation == FunctionNode::Operation::ParenLeft) {
          operator_stack.pop();
        }
      }

      get_next_token();
    }

    while (!operator_stack.empty()) {
      current_expression_list->rpn_output.emplace_back(operator_stack.top());
      operator_stack.pop();
    }

    return true;
  }

  bool parse_statement(Template &tmpl, nonstd::string_view path) {
    if (tok.kind != Token::Kind::Id) {
      return false;
    }

    if (tok.text == static_cast<decltype(tok.text)>("if")) {
      get_next_token();

      // evaluate expression
      
      // conditional jump; destination will be filled in by else or endif
      tmpl.nodes.emplace_back(Node::Op::ConditionalJump, 0, tok.text.data() - tmpl.content.c_str());

      auto if_statement_node = std::make_shared<IfStatementNode>();
      current_block->nodes.emplace_back(if_statement_node);
      if_statement_node->parent = current_block;
      if_statement_stack.emplace(if_statement_node.get());
      current_block = &if_statement_node->true_statement;
      current_expression_list = &if_statement_node->condition;

      if (!parse_expression(tmpl)) {
        return false;
      }

    } else if (tok.text == static_cast<decltype(tok.text)>("endif")) {
      if (if_statement_stack.empty()) {
        throw_parser_error("endif without matching if");
      }
      auto &if_statement_data = if_statement_stack.top();
      get_next_token();

      // // previous conditional jump jumps here
      // if (if_data.prev_cond_jump != std::numeric_limits<unsigned int>::max()) {
      //   tmpl.nodes[if_data.prev_cond_jump].args = tmpl.nodes.size();
      // }

      // // update all previous unconditional jumps to here
      // for (size_t i : if_data.uncond_jumps) {
      //   tmpl.nodes[i].args = tmpl.nodes.size();
      // }

      // // pop if stack

      current_block = if_statement_data->parent;
      if_statement_stack.pop();
    } else if (tok.text == static_cast<decltype(tok.text)>("else")) {
      if (if_statement_stack.empty()) {
        throw_parser_error("else without matching if");
      }
      auto &if_statement_data = if_statement_stack.top();
      get_next_token();

      // end previous block with unconditional jump to endif; destination will be
      // filled in by endif
      tmpl.nodes.emplace_back(Node::Op::Jump, 0, tok.text.data() - tmpl.content.c_str());

      // previous conditional jump jumps here
      // tmpl.nodes[if_data.prev_cond_jump].args = tmpl.nodes.size();

      if_statement_data->has_false_statement = true;
      current_block = &if_statement_data->false_statement;

      // chained else if
      if (tok.kind == Token::Kind::Id && tok.text == static_cast<decltype(tok.text)>("if")) {
        get_next_token();

        // evaluate expression
        if (!parse_expression(tmpl)) {
          return false;
        }

        // conditional jump; destination will be filled in by else or endif
        tmpl.nodes.emplace_back(Node::Op::ConditionalJump, 0, tok.text.data() - tmpl.content.c_str());

        auto if_statement_node = std::make_shared<IfStatementNode>();
        current_block->nodes.emplace_back(if_statement_node);
        if_statement_node->parent = current_block;
        if_statement_stack.emplace(if_statement_node.get());
        current_block = &if_statement_node->true_statement;
      }
    } else if (tok.text == static_cast<decltype(tok.text)>("for")) {
      get_next_token();

      // options: for a in arr; for a, b in obj
      if (tok.kind != Token::Kind::Id) {
        throw_parser_error("expected id, got '" + tok.describe() + "'");
      }
      Token value_token = tok;
      get_next_token();

      Token key_token;
      if (tok.kind == Token::Kind::Comma) {
        get_next_token();
        if (tok.kind != Token::Kind::Id) {
          throw_parser_error("expected id, got '" + tok.describe() + "'");
        }
        key_token = std::move(value_token);
        value_token = tok;
        get_next_token();
      }

      if (tok.kind != Token::Kind::Id || tok.text != static_cast<decltype(tok.text)>("in")) {
        throw_parser_error("expected 'in', got '" + tok.describe() + "'");
      }
      get_next_token();

      if (!parse_expression(tmpl)) {
        return false;
      }

      loop_stack.push_back(tmpl.nodes.size());

      tmpl.nodes.emplace_back(Node::Op::StartLoop, 0, tok.text.data() - tmpl.content.c_str());
      if (!key_token.text.empty()) {
        tmpl.nodes.back().value = key_token.text;
      }
      tmpl.nodes.back().str = static_cast<std::string>(value_token.text);
    } else if (tok.text == static_cast<decltype(tok.text)>("endfor")) {
      get_next_token();
      if (loop_stack.empty()) {
        throw_parser_error("endfor without matching for");
      }

      // update loop with EndLoop index (for empty case)
      tmpl.nodes[loop_stack.back()].args = tmpl.nodes.size();

      tmpl.nodes.emplace_back(Node::Op::EndLoop, 0, tok.text.data() - tmpl.content.c_str());
      tmpl.nodes.back().args = loop_stack.back() + 1; // loop body
      loop_stack.pop_back();
    } else if (tok.text == static_cast<decltype(tok.text)>("include")) {
      get_next_token();

      if (tok.kind != Token::Kind::String) {
        throw_parser_error("expected string, got '" + tok.describe() + "'");
      }

      // build the relative path
      json json_name = json::parse(tok.text);
      std::string pathname = static_cast<std::string>(path);
      pathname += json_name.get_ref<const std::string &>();
      if (pathname.compare(0, 2, "./") == 0) {
        pathname.erase(0, 2);
      }
      // sys::path::remove_dots(pathname, true, sys::path::Style::posix);

      if (config.search_included_templates_in_files && template_storage.find(pathname) == template_storage.end()) {
        auto include_template = Template(load_file(pathname));
        template_storage.emplace(pathname, include_template);
        parse_into_template(template_storage.at(pathname), pathname);
      }

      // generate a reference node
      tmpl.nodes.emplace_back(Node::Op::Include, json(pathname), Node::Flag::ValueImmediate, tok.text.data() - tmpl.content.c_str());
      current_block->nodes.emplace_back(std::make_shared<IncludeStatementNode>(pathname));

      get_next_token();
    } else {
      return false;
    }
    return true;
  }

  /* void append_function(Template &tmpl, Node::Op op, unsigned int num_args) {
    // we can merge with back-to-back push
    if (!tmpl.nodes.empty()) {
      Node &last = tmpl.nodes.back();
      if (last.op == Node::Op::Push) {
        last.op = op;
        last.args = num_args;
        return;
      }
    }

    // otherwise just add it to the end
    tmpl.nodes.emplace_back(op, num_args, tok.text.data() - tmpl.content.c_str());

    if (expression_stack.empty()) {
      current_block->nodes.emplace_back(std::make_shared<FunctionNode>(FunctionNode::Operation::Less));
    }
  }

  void append_callback(Template &tmpl, nonstd::string_view name, unsigned int num_args) {
    // we can merge with back-to-back push value (not lookup)
    if (!tmpl.nodes.empty()) {
      Node &last = tmpl.nodes.back();
      if (last.op == Node::Op::Push && (last.flags & Node::Flag::ValueMask) == Node::Flag::ValueImmediate) {
        last.op = Node::Op::Callback;
        last.args = num_args;
        last.str = static_cast<std::string>(name);
        last.pos = name.data() - tmpl.content.c_str();
        return;
      }
    }

    // otherwise just add it to the end
    tmpl.nodes.emplace_back(Node::Op::Callback, num_args, tok.text.data() - tmpl.content.c_str());
    tmpl.nodes.back().str = static_cast<std::string>(name);

    if (expression_stack.empty()) {
      current_block->nodes.emplace_back(std::make_shared<FunctionNode>(FunctionNode::Operation::Named));
    }
  } */

  void parse_into(Template &tmpl, nonstd::string_view path) {
    lexer.start(tmpl.content);
    current_block = &tmpl.root;

    for (;;) {
      get_next_token();
      switch (tok.kind) {
      case Token::Kind::Eof:
        if (!if_statement_stack.empty()) {
          throw_parser_error("unmatched if");
        }
        if (!for_statement_stack.empty()) {
          throw_parser_error("unmatched for");
        }
        return;
      case Token::Kind::Text: {
        // tmpl.nodes.emplace_back(Node::Op::PrintText, tok.text, 0u, tok.text.data() - tmpl.content.c_str());
        current_block->nodes.emplace_back(std::make_shared<TextNode>(static_cast<std::string>(tok.text)));
        break;
      }
      case Token::Kind::StatementOpen:
        get_next_token();
        if (!parse_statement(tmpl, path)) {
          throw_parser_error("expected statement, got '" + tok.describe() + "'");
        }
        if (tok.kind != Token::Kind::StatementClose) {
          throw_parser_error("expected statement close, got '" + tok.describe() + "'");
        }
        break;
      case Token::Kind::LineStatementOpen:
        get_next_token();
        parse_statement(tmpl, path);
        if (tok.kind != Token::Kind::LineStatementClose && tok.kind != Token::Kind::Eof) {
          throw_parser_error("expected line statement close, got '" + tok.describe() + "'");
        }
        break;
      case Token::Kind::ExpressionOpen: {
        get_next_token();

        auto expression_list_node = std::make_shared<ExpressionListNode>();
        current_block->nodes.emplace_back(expression_list_node);
        current_expression_list = expression_list_node.get();

        if (!parse_expression(tmpl)) {
          throw_parser_error("expected expression, got '" + tok.describe() + "'");
        }
        // append_function(tmpl, Node::Op::PrintValue, 1);
        if (tok.kind != Token::Kind::ExpressionClose) {
          throw_parser_error("expected expression close, got '" + tok.describe() + "'");
        }
      } break;
      case Token::Kind::CommentOpen:
        get_next_token();
        if (tok.kind != Token::Kind::CommentClose) {
          throw_parser_error("expected comment close, got '" + tok.describe() + "'");
        }
        break;
      default:
        throw_parser_error("unexpected token '" + tok.describe() + "'");
        break;
      }
    }
  }

  Template parse(nonstd::string_view input, nonstd::string_view path) {
    auto result = Template(static_cast<std::string>(input));
    parse_into(result, path);
    return result;
  }

  Template parse(nonstd::string_view input) {
    return parse(input, "./");
  }

  void parse_into_template(Template& tmpl, nonstd::string_view filename) {
    nonstd::string_view path = filename.substr(0, filename.find_last_of("/\\") + 1);

    // StringRef path = sys::path::parent_path(filename);
    auto sub_parser = Parser(config, lexer.get_config(), template_storage);
    sub_parser.parse_into(tmpl, path);
  }

  std::string load_file(nonstd::string_view filename) {
    std::ifstream file = open_file_or_throw(static_cast<std::string>(filename));
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return text;
  }
};

} // namespace inja

#endif // INCLUDE_INJA_PARSER_HPP_
