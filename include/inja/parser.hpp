// Copyright (c) 2020 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_PARSER_HPP_
#define INCLUDE_INJA_PARSER_HPP_

#include <limits>
#include <stack>
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
  using Op = FunctionStorage::Operation;

  ParserStatic() {
    function_storage.add_builtin("at", 2, Op::At);
    function_storage.add_builtin("default", 2, Op::Default);
    function_storage.add_builtin("divisibleBy", 2, Op::DivisibleBy);
    function_storage.add_builtin("even", 1, Op::Even);
    function_storage.add_builtin("exists", 1, Op::Exists);
    function_storage.add_builtin("existsIn", 2, Op::ExistsInObject);
    function_storage.add_builtin("first", 1, Op::First);
    function_storage.add_builtin("float", 1, Op::Float);
    function_storage.add_builtin("int", 1, Op::Int);
    function_storage.add_builtin("isArray", 1, Op::IsArray);
    function_storage.add_builtin("isBoolean", 1, Op::IsBoolean);
    function_storage.add_builtin("isFloat", 1, Op::IsFloat);
    function_storage.add_builtin("isInteger", 1, Op::IsInteger);
    function_storage.add_builtin("isNumber", 1, Op::IsNumber);
    function_storage.add_builtin("isObject", 1, Op::IsObject);
    function_storage.add_builtin("isString", 1, Op::IsString);
    function_storage.add_builtin("last", 1, Op::Last);
    function_storage.add_builtin("length", 1, Op::Length);
    function_storage.add_builtin("lower", 1, Op::Lower);
    function_storage.add_builtin("max", 1, Op::Max);
    function_storage.add_builtin("min", 1, Op::Min);
    function_storage.add_builtin("odd", 1, Op::Odd);
    function_storage.add_builtin("range", 1, Op::Range);
    function_storage.add_builtin("round", 2, Op::Round);
    function_storage.add_builtin("sort", 1, Op::Sort);
    function_storage.add_builtin("upper", 1, Op::Upper);
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

  Token tok, peek_tok;
  bool have_peek_tok {false};

  unsigned int current_paren_level {0};
  BlockNode *current_block {nullptr};
  ExpressionListNode *current_expression_list {nullptr};
  std::stack<FunctionNode*> function_stack;
  std::stack<unsigned int> function_paren_level;
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
      if (tok.kind == Token::Kind::String) {
        current_expression_list->rpn_output.emplace_back(std::make_shared<LiteralNode>(json::parse(tok.text), tok.text.data() - tmpl.content.c_str()));

      } else if (tok.kind == Token::Kind::Minus) {
        // TODO
        // get_prior_token()
        // if (prior_token ==

      } else if (tok.kind == Token::Kind::Number) {
        current_expression_list->rpn_output.emplace_back(std::make_shared<LiteralNode>(json::parse(tok.text), tok.text.data() - tmpl.content.c_str()));

      } else if (tok.kind == Token::Kind::Id) {
        get_peek_token();

        // Json Literal
        if (tok.text == static_cast<decltype(tok.text)>("true") || tok.text == static_cast<decltype(tok.text)>("false") || tok.text == static_cast<decltype(tok.text)>("null")) {
          current_expression_list->rpn_output.emplace_back(std::make_shared<LiteralNode>(json::parse(tok.text), tok.text.data() - tmpl.content.c_str()));

        // Functions
        } else if (peek_tok.kind == Token::Kind::LeftParen) {
          auto name = static_cast<std::string>(tok.text);
          operator_stack.emplace(std::make_shared<FunctionNode>(static_cast<std::string>(tok.text), tok.text.data() - tmpl.content.c_str()));
          function_stack.emplace(operator_stack.top().get());
          function_paren_level.emplace(current_paren_level);

        // Operator
        } else if (tok.text == "and" || tok.text == "or" || tok.text == "in" || tok.text == "not") {
          goto parse_operator;

        // Variables
        } else {
          current_expression_list->rpn_output.emplace_back(std::make_shared<JsonNode>(static_cast<std::string>(tok.text), tok.text.data() - tmpl.content.c_str()));
        }

      // Operators
      } else if (tok.kind == Token::Kind::Equal || tok.kind == Token::Kind::NotEqual || tok.kind == Token::Kind::GreaterThan || tok.kind == Token::Kind::GreaterEqual || tok.kind == Token::Kind::LessThan || tok.kind == Token::Kind::LessEqual || tok.kind == Token::Kind::Plus || tok.kind == Token::Kind::Times || tok.kind == Token::Kind::Slash || tok.kind == Token::Kind::Power || tok.kind == Token::Kind::Percent) {

  parse_operator:
        FunctionStorage::Operation operation;
        switch (tok.kind) {
        case Token::Kind::Id: {
          if (tok.text == "and") {
            operation = FunctionStorage::Operation::And;
          } else if (tok.text == "or") {
            operation = FunctionStorage::Operation::Or;
          } else if (tok.text == "in") {
            operation = FunctionStorage::Operation::In;
          } else if (tok.text == "not") {
            operation = FunctionStorage::Operation::Not;
          } else {
            throw_parser_error("unknown operator in parser.");
          }
        } break;
        case Token::Kind::Equal: {
          operation = FunctionStorage::Operation::Equal;
        } break;
        case Token::Kind::NotEqual: {
          operation = FunctionStorage::Operation::NotEqual;
        } break;
        case Token::Kind::GreaterThan: {
          operation = FunctionStorage::Operation::Greater;
        } break;
        case Token::Kind::GreaterEqual: {
          operation = FunctionStorage::Operation::GreaterEqual;
        } break;
        case Token::Kind::LessThan: {
          operation = FunctionStorage::Operation::Less;
        } break;
        case Token::Kind::LessEqual: {
          operation = FunctionStorage::Operation::LessEqual;
        } break;
        case Token::Kind::Plus: {
          operation = FunctionStorage::Operation::Add;
        } break;
        case Token::Kind::Minus: {
          operation = FunctionStorage::Operation::Subtract;
        } break;
        case Token::Kind::Times: {
          operation = FunctionStorage::Operation::Multiplication;
        } break;
        case Token::Kind::Slash: {
          operation = FunctionStorage::Operation::Division;
        } break;
        case Token::Kind::Power: {
          operation = FunctionStorage::Operation::Power;
        } break;
        case Token::Kind::Percent: {
          operation = FunctionStorage::Operation::Modulo;
        } break;
        default: {
          throw_parser_error("unknown operator in parser.");
        }
        }
        auto function_node = std::make_shared<FunctionNode>(operation, tok.text.data() - tmpl.content.c_str());

        while (!operator_stack.empty() && ((operator_stack.top()->precedence > function_node->precedence) || (operator_stack.top()->precedence == function_node->precedence && function_node->associativity == FunctionNode::Associativity::Left)) && (operator_stack.top()->operation != FunctionStorage::Operation::ParenLeft)) {
          current_expression_list->rpn_output.emplace_back(operator_stack.top());
          operator_stack.pop();
        }

        operator_stack.emplace(function_node);

      // Comma
      } else if (tok.kind == Token::Kind::Comma) {
        function_stack.top()->number_args += 1;

      // Parens
      } else if (tok.kind == Token::Kind::LeftParen) {
        current_paren_level += 1;
        operator_stack.emplace(std::make_shared<FunctionNode>(FunctionStorage::Operation::ParenLeft, tok.text.data() - tmpl.content.c_str()));

      } else if (tok.kind == Token::Kind::RightParen) {
        current_paren_level -= 1;
        while (operator_stack.top()->operation != FunctionStorage::Operation::ParenLeft) {
          current_expression_list->rpn_output.emplace_back(operator_stack.top());
          operator_stack.pop();
        }

        if (operator_stack.top()->operation == FunctionStorage::Operation::ParenLeft) {
          operator_stack.pop();
        }

        if (function_paren_level.top() == current_paren_level) {
          auto func = function_stack.top();
          auto function_data = parser_static.function_storage.find_function(func->name, func->number_args);
          if (function_data.operation == FunctionStorage::Operation::None) {
            throw_parser_error("unknown function " + func->name);
          }
          func->operation = function_data.operation;
          if (function_data.operation == FunctionStorage::Operation::Callback) {
            func->callback = function_data.callback;
          }

          function_paren_level.pop();
          function_stack.pop();
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

      auto if_statement_node = std::make_shared<IfStatementNode>(tok.text.data() - tmpl.content.c_str());
      current_block->nodes.emplace_back(if_statement_node);
      if_statement_node->parent = current_block;
      if_statement_stack.emplace(if_statement_node.get());
      current_block = &if_statement_node->true_statement;
      current_expression_list = &if_statement_node->condition;

      if (!parse_expression(tmpl)) {
        return false;
      }

    } else if (tok.text == static_cast<decltype(tok.text)>("else")) {
      if (if_statement_stack.empty()) {
        throw_parser_error("else without matching if");
      }
      auto &if_statement_data = if_statement_stack.top();
      get_next_token();

      if_statement_data->has_false_statement = true;
      current_block = &if_statement_data->false_statement;

      // Chained else if
      if (tok.kind == Token::Kind::Id && tok.text == static_cast<decltype(tok.text)>("if")) {
        get_next_token();

        auto if_statement_node = std::make_shared<IfStatementNode>(true, tok.text.data() - tmpl.content.c_str());
        current_block->nodes.emplace_back(if_statement_node);
        if_statement_node->parent = current_block;
        if_statement_stack.emplace(if_statement_node.get());
        current_block = &if_statement_node->true_statement;
        current_expression_list = &if_statement_node->condition;

        if (!parse_expression(tmpl)) {
          return false;
        }
      }

    } else if (tok.text == static_cast<decltype(tok.text)>("endif")) {
      if (if_statement_stack.empty()) {
        throw_parser_error("endif without matching if");
      }

      // Nested if statements
      while (if_statement_stack.top()->is_nested) {
        if_statement_stack.pop();
      }

      auto &if_statement_data = if_statement_stack.top();
      get_next_token();

      current_block = if_statement_data->parent;
      if_statement_stack.pop();

    } else if (tok.text == static_cast<decltype(tok.text)>("for")) {
      get_next_token();

      // options: for a in arr; for a, b in obj
      if (tok.kind != Token::Kind::Id) {
        throw_parser_error("expected id, got '" + tok.describe() + "'");
      }

      Token value_token = tok;
      get_next_token();

      // Object type
      std::shared_ptr<ForStatementNode> for_statement_node;
      if (tok.kind == Token::Kind::Comma) {
        get_next_token();
        if (tok.kind != Token::Kind::Id) {
          throw_parser_error("expected id, got '" + tok.describe() + "'");
        }

        Token key_token = std::move(value_token);
        value_token = tok;
        get_next_token();

        for_statement_node = std::make_shared<ForObjectStatementNode>(key_token.text, value_token.text, tok.text.data() - tmpl.content.c_str());

      // Array type
      } else {
        for_statement_node = std::make_shared<ForArrayStatementNode>(value_token.text, tok.text.data() - tmpl.content.c_str());
      }

      current_block->nodes.emplace_back(for_statement_node);
      for_statement_node->parent = current_block;
      for_statement_stack.emplace(for_statement_node.get());
      current_block = &for_statement_node->body;
      current_expression_list = &for_statement_node->condition;

      if (tok.kind != Token::Kind::Id || tok.text != static_cast<decltype(tok.text)>("in")) {
        throw_parser_error("expected 'in', got '" + tok.describe() + "'");
      }
      get_next_token();

      if (!parse_expression(tmpl)) {
        return false;
      }

    } else if (tok.text == static_cast<decltype(tok.text)>("endfor")) {
      if (for_statement_stack.empty()) {
        throw_parser_error("endfor without matching for");
      }

      auto &for_statement_data = for_statement_stack.top();
      get_next_token();

      current_block = for_statement_data->parent;
      for_statement_stack.pop();

    } else if (tok.text == static_cast<decltype(tok.text)>("include")) {
      get_next_token();

      if (tok.kind != Token::Kind::String) {
        throw_parser_error("expected string, got '" + tok.describe() + "'");
      }

      // Build the relative path
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

      current_block->nodes.emplace_back(std::make_shared<IncludeStatementNode>(pathname, tok.text.data() - tmpl.content.c_str()));

      get_next_token();

    } else {
      return false;
    }
    return true;
  }

  void parse_into(Template &tmpl, nonstd::string_view path) {
    lexer.start(tmpl.content);
    current_block = &tmpl.root;

    for (;;) {
      get_next_token();
      switch (tok.kind) {
      case Token::Kind::Eof: {
        if (!if_statement_stack.empty()) {
          throw_parser_error("unmatched if");
        }
        if (!for_statement_stack.empty()) {
          throw_parser_error("unmatched for");
        }
      } return;
      case Token::Kind::Text: {
        current_block->nodes.emplace_back(std::make_shared<TextNode>(static_cast<std::string>(tok.text), tok.text.data() - tmpl.content.c_str()));
        break;
      }
      case Token::Kind::StatementOpen: {
        get_next_token();
        if (!parse_statement(tmpl, path)) {
          throw_parser_error("expected statement, got '" + tok.describe() + "'");
        }
        if (tok.kind != Token::Kind::StatementClose) {
          throw_parser_error("expected statement close, got '" + tok.describe() + "'");
        }
      } break;
      case Token::Kind::LineStatementOpen: {
        get_next_token();
        parse_statement(tmpl, path);
        if (tok.kind != Token::Kind::LineStatementClose && tok.kind != Token::Kind::Eof) {
          throw_parser_error("expected line statement close, got '" + tok.describe() + "'");
        }
      } break;
      case Token::Kind::ExpressionOpen: {
        get_next_token();

        auto expression_list_node = std::make_shared<ExpressionListNode>(tok.text.data() - tmpl.content.c_str());
        current_block->nodes.emplace_back(expression_list_node);
        current_expression_list = expression_list_node.get();

        if (!parse_expression(tmpl)) {
          throw_parser_error("expected expression, got '" + tok.describe() + "'");
        }

        if (tok.kind != Token::Kind::ExpressionClose) {
          throw_parser_error("expected expression close, got '" + tok.describe() + "'");
        }
      } break;
      case Token::Kind::CommentOpen: {
        get_next_token();
        if (tok.kind != Token::Kind::CommentClose) {
          throw_parser_error("expected comment close, got '" + tok.describe() + "'");
        }
      } break;
      default: {
        throw_parser_error("unexpected token '" + tok.describe() + "'");
      } break;
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
