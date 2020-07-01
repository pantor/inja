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
  ParserStatic() {
    function_storage.add_builtin("at", 2, Node::Op::At);
    function_storage.add_builtin("default", 2, Node::Op::Default);
    function_storage.add_builtin("divisibleBy", 2, Node::Op::DivisibleBy);
    function_storage.add_builtin("even", 1, Node::Op::Even);
    function_storage.add_builtin("first", 1, Node::Op::First);
    function_storage.add_builtin("float", 1, Node::Op::Float);
    function_storage.add_builtin("int", 1, Node::Op::Int);
    function_storage.add_builtin("last", 1, Node::Op::Last);
    function_storage.add_builtin("length", 1, Node::Op::Length);
    function_storage.add_builtin("lower", 1, Node::Op::Lower);
    function_storage.add_builtin("max", 1, Node::Op::Max);
    function_storage.add_builtin("min", 1, Node::Op::Min);
    function_storage.add_builtin("odd", 1, Node::Op::Odd);
    function_storage.add_builtin("range", 1, Node::Op::Range);
    function_storage.add_builtin("round", 2, Node::Op::Round);
    function_storage.add_builtin("sort", 1, Node::Op::Sort);
    function_storage.add_builtin("upper", 1, Node::Op::Upper);
    function_storage.add_builtin("exists", 1, Node::Op::Exists);
    function_storage.add_builtin("existsIn", 2, Node::Op::ExistsInObject);
    function_storage.add_builtin("isBoolean", 1, Node::Op::IsBoolean);
    function_storage.add_builtin("isNumber", 1, Node::Op::IsNumber);
    function_storage.add_builtin("isInteger", 1, Node::Op::IsInteger);
    function_storage.add_builtin("isFloat", 1, Node::Op::IsFloat);
    function_storage.add_builtin("isObject", 1, Node::Op::IsObject);
    function_storage.add_builtin("isArray", 1, Node::Op::IsArray);
    function_storage.add_builtin("isString", 1, Node::Op::IsString);
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
  struct IfData {
    using jump_t = size_t;
    jump_t prev_cond_jump;
    std::vector<jump_t> uncond_jumps;

    explicit IfData(jump_t condJump) : prev_cond_jump(condJump) {}
  };


  const ParserStatic &parser_static;
  const ParserConfig &config;
  Lexer lexer;
  TemplateStorage &template_storage;

  Token tok;
  Token peek_tok;
  bool have_peek_tok {false};

  std::vector<IfData> if_stack;
  std::vector<size_t> loop_stack;

  BlockNode *current_block {nullptr};
  std::queue<ExpressionNode*> expression_stack;
  std::queue<IfStatementNode*> if_statement_stack;
  std::queue<ForStatementNode*> for_statement_stack;

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
    if (!parse_expression_and(tmpl)) {
      return false;
    }
    if (tok.kind != Token::Kind::Id || tok.text != static_cast<decltype(tok.text)>("or")) {
      return true;
    }
    get_next_token();
    if (!parse_expression_and(tmpl)) {
      return false;
    }
    append_function(tmpl, Node::Op::Or, 2);
    return true;
  }

  bool parse_expression_and(Template &tmpl) {
    if (!parse_expression_not(tmpl)) {
      return false;
    } 
    if (tok.kind != Token::Kind::Id || tok.text != static_cast<decltype(tok.text)>("and")) {
      return true;
    }
    get_next_token();
    if (!parse_expression_not(tmpl)) {
      return false;
    }
    append_function(tmpl, Node::Op::And, 2);
    return true;
  }

  bool parse_expression_not(Template &tmpl) {
    if (tok.kind == Token::Kind::Id && tok.text == static_cast<decltype(tok.text)>("not")) {
      get_next_token();
      if (!parse_expression_not(tmpl)) {
        return false;
      }
      append_function(tmpl, Node::Op::Not, 1);
      return true;
    } else {
      return parse_expression_comparison(tmpl);
    }
  }

  bool parse_expression_comparison(Template &tmpl) {
    if (!parse_expression_datum(tmpl)) {
      return false;
    }
    Node::Op op;
    switch (tok.kind) {
    case Token::Kind::Id:
      if (tok.text == static_cast<decltype(tok.text)>("in")) {
        op = Node::Op::In;
      } else {
        return true;
      }
      break;
    case Token::Kind::Equal:
      op = Node::Op::Equal;
      break;
    case Token::Kind::GreaterThan:
      op = Node::Op::Greater;
      break;
    case Token::Kind::LessThan:
      op = Node::Op::Less;
      break;
    case Token::Kind::LessEqual:
      op = Node::Op::LessEqual;
      break;
    case Token::Kind::GreaterEqual:
      op = Node::Op::GreaterEqual;
      break;
    case Token::Kind::NotEqual:
      op = Node::Op::Different;
      break;
    default:
      return true;
    }
    get_next_token();
    if (!parse_expression_datum(tmpl)) {
      return false;
    }
    append_function(tmpl, op, 2);
    return true;
  }

  bool parse_expression_datum(Template &tmpl) {
    nonstd::string_view json_first;
    size_t bracket_level = 0;
    size_t brace_level = 0;

    for (;;) {
      switch (tok.kind) {
      case Token::Kind::LeftParen: {
        get_next_token();
        if (!parse_expression(tmpl)) {
          return false;
        }
        if (tok.kind != Token::Kind::RightParen) {
          throw_parser_error("unmatched '('");
        }
        get_next_token();
        return true;
      }
      case Token::Kind::Id:
        get_peek_token();
        if (peek_tok.kind == Token::Kind::LeftParen) {
          // function call, parse arguments
          Token func_token = tok;
          get_next_token(); // id
          get_next_token(); // leftParen
          unsigned int num_args = 0;
          if (tok.kind == Token::Kind::RightParen) {
            // no args
            get_next_token();
          } else {
            for (;;) {
              if (!parse_expression(tmpl)) {
                throw_parser_error("expected expression, got '" + tok.describe() + "'");
              }
              num_args += 1;
              if (tok.kind == Token::Kind::RightParen) {
                get_next_token();
                break;
              }
              if (tok.kind != Token::Kind::Comma) {
                throw_parser_error("expected ')' or ',', got '" + tok.describe() + "'");
              }
              get_next_token();
            }
          }

          auto op = parser_static.function_storage.find_builtin(func_token.text, num_args);

          if (op != Node::Op::Nop) {
            // swap arguments for default(); see comment in RenderTo()
            if (op == Node::Op::Default) {
              std::swap(tmpl.nodes.back(), *(tmpl.nodes.rbegin() + 1));
            }
            append_function(tmpl, op, num_args);
            return true;
          } else {
            append_callback(tmpl, func_token.text, num_args);
            return true;
          }
        } else if (tok.text == static_cast<decltype(tok.text)>("true") ||
                   tok.text == static_cast<decltype(tok.text)>("false") ||
                   tok.text == static_cast<decltype(tok.text)>("null")) {
          // true, false, null are json literals
          if (brace_level == 0 && bracket_level == 0) {
            json_first = tok.text;
            goto returnJson;
          }
          break;
        } else {
          // normal literal (json read)

          auto flag = config.notation == ElementNotation::Pointer ? Node::Flag::ValueLookupPointer : Node::Flag::ValueLookupDot;
          tmpl.nodes.emplace_back(Node::Op::Push, tok.text, flag, tok.text.data() - tmpl.content.c_str());

          if (expression_stack.empty()) {
            current_block->nodes.emplace_back(std::make_shared<JsonNode>(static_cast<std::string>(tok.text)));
          } else {

          }

          get_next_token();
          return true;
        }
      // json passthrough
      case Token::Kind::Number:
      case Token::Kind::String:
        if (brace_level == 0 && bracket_level == 0) {
          json_first = tok.text;
          goto returnJson;
        }
        break;
      case Token::Kind::Comma:
      case Token::Kind::Colon:
        if (brace_level == 0 && bracket_level == 0) {
          throw_parser_error("unexpected token '" + tok.describe() + "'");
        }
        break;
      case Token::Kind::LeftBracket:
        if (brace_level == 0 && bracket_level == 0) {
          json_first = tok.text;
        }
        bracket_level += 1;
        break;
      case Token::Kind::LeftBrace:
        if (brace_level == 0 && bracket_level == 0) {
          json_first = tok.text;
        }
        brace_level += 1;
        break;
      case Token::Kind::RightBracket:
        if (bracket_level == 0) {
          throw_parser_error("unexpected ']'");
        }
        bracket_level -= 1;
        if (brace_level == 0 && bracket_level == 0) {
          goto returnJson;
        }
        break;
      case Token::Kind::RightBrace:
        if (brace_level == 0) {
          throw_parser_error("unexpected '}'");
        }
        brace_level -= 1;
        if (brace_level == 0 && bracket_level == 0) {
          goto returnJson;
        }
        break;
      default:
        if (brace_level != 0) {
          throw_parser_error("unmatched '{'");
        }
        if (bracket_level != 0) {
          throw_parser_error("unmatched '['");
        }
        return false;
      }

      get_next_token();
    }

  returnJson:
    // bridge across all intermediate tokens
    nonstd::string_view json_text(json_first.data(), tok.text.data() - json_first.data() + tok.text.size());
    tmpl.nodes.emplace_back(Node::Op::Push, json::parse(json_text), Node::Flag::ValueImmediate, tok.text.data() - tmpl.content.c_str());
    current_block->nodes.emplace_back(std::make_shared<LiteralNode>(json::parse(json_text)));
    get_next_token();
    return true;
  }

  bool parse_statement(Template &tmpl, nonstd::string_view path) {
    if (tok.kind != Token::Kind::Id) {
      return false;
    }

    if (tok.text == static_cast<decltype(tok.text)>("if")) {
      get_next_token();

      // evaluate expression
      if (!parse_expression(tmpl)) {
        return false;
      }

      // start a new if block on if stack
      if_stack.emplace_back(static_cast<decltype(if_stack)::value_type::jump_t>(tmpl.nodes.size()));

      // conditional jump; destination will be filled in by else or endif
      tmpl.nodes.emplace_back(Node::Op::ConditionalJump, 0, tok.text.data() - tmpl.content.c_str());
      
      auto if_statement_node = std::make_shared<IfStatementNode>();
      current_block->nodes.emplace_back(if_statement_node);
      if_statement_node->parent = current_block;
      if_statement_stack.emplace(if_statement_node.get());
      current_block = &if_statement_node->true_statement;
    } else if (tok.text == static_cast<decltype(tok.text)>("endif")) {
      if (if_stack.empty()) {
        throw_parser_error("endif without matching if");
      }
      auto &if_data = if_stack.back();
      auto &if_statement_data = if_statement_stack.back();
      get_next_token();

      // previous conditional jump jumps here
      if (if_data.prev_cond_jump != std::numeric_limits<unsigned int>::max()) {
        tmpl.nodes[if_data.prev_cond_jump].args = tmpl.nodes.size();
      }

      // update all previous unconditional jumps to here
      for (size_t i : if_data.uncond_jumps) {
        tmpl.nodes[i].args = tmpl.nodes.size();
      }

      // pop if stack
      if_stack.pop_back();

      current_block = if_statement_data->parent;
      if_statement_stack.pop();
    } else if (tok.text == static_cast<decltype(tok.text)>("else")) {
      if (if_stack.empty()) {
        throw_parser_error("else without matching if");
      }
      auto &if_data = if_stack.back();
      auto &if_statement_data = if_statement_stack.back();
      get_next_token();

      // end previous block with unconditional jump to endif; destination will be
      // filled in by endif
      if_data.uncond_jumps.push_back(tmpl.nodes.size());
      tmpl.nodes.emplace_back(Node::Op::Jump, 0, tok.text.data() - tmpl.content.c_str());

      // previous conditional jump jumps here
      tmpl.nodes[if_data.prev_cond_jump].args = tmpl.nodes.size();
      if_data.prev_cond_jump = std::numeric_limits<unsigned int>::max();

      if_statement_data->has_false_statement = true;
      current_block = &if_statement_data->false_statement;

      // chained else if
      if (tok.kind == Token::Kind::Id && tok.text == static_cast<decltype(tok.text)>("if")) {
        get_next_token();

        // evaluate expression
        if (!parse_expression(tmpl)) {
          return false;
        }

        // update "previous jump"
        if_data.prev_cond_jump = tmpl.nodes.size();

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

  void append_function(Template &tmpl, Node::Op op, unsigned int num_args) {
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
  }

  void parse_into(Template &tmpl, nonstd::string_view path) {
    lexer.start(tmpl.content);
    current_block = &tmpl.root;

    for (;;) {
      get_next_token();
      switch (tok.kind) {
      case Token::Kind::Eof:
        if (!if_stack.empty()) {
          throw_parser_error("unmatched if");
        }
        if (!loop_stack.empty()) {
          throw_parser_error("unmatched for");
        }
        return;
      case Token::Kind::Text: {
        tmpl.nodes.emplace_back(Node::Op::PrintText, tok.text, 0u, tok.text.data() - tmpl.content.c_str());
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
      case Token::Kind::ExpressionOpen:
        get_next_token();
        if (!parse_expression(tmpl)) {
          throw_parser_error("expected expression, got '" + tok.describe() + "'");
        }
        append_function(tmpl, Node::Op::PrintValue, 1);
        if (tok.kind != Token::Kind::ExpressionClose) {
          throw_parser_error("expected expression close, got '" + tok.describe() + "'");
        }
        break;
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
