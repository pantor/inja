#ifndef PANTOR_INJA_PARSER_HPP
#define PANTOR_INJA_PARSER_HPP

#include <limits>

#include "bytecode.hpp"
#include "config.hpp"
#include "function_storage.hpp"
#include "lexer.hpp"
#include "template.hpp"
#include "token.hpp"
#include "utils.hpp"

#include <nlohmann/json.hpp>


namespace inja {

class ParserStatic {
  ParserStatic() {
    functions.add_builtin("at", 2, Bytecode::Op::At);
    functions.add_builtin("default", 2, Bytecode::Op::Default);
    functions.add_builtin("divisibleBy", 2, Bytecode::Op::DivisibleBy);
    functions.add_builtin("even", 1, Bytecode::Op::Even);
    functions.add_builtin("first", 1, Bytecode::Op::First);
    functions.add_builtin("float", 1, Bytecode::Op::Float);
    functions.add_builtin("int", 1, Bytecode::Op::Int);
    functions.add_builtin("last", 1, Bytecode::Op::Last);
    functions.add_builtin("length", 1, Bytecode::Op::Length);
    functions.add_builtin("lower", 1, Bytecode::Op::Lower);
    functions.add_builtin("max", 1, Bytecode::Op::Max);
    functions.add_builtin("min", 1, Bytecode::Op::Min);
    functions.add_builtin("odd", 1, Bytecode::Op::Odd);
    functions.add_builtin("range", 1, Bytecode::Op::Range);
    functions.add_builtin("round", 2, Bytecode::Op::Round);
    functions.add_builtin("sort", 1, Bytecode::Op::Sort);
    functions.add_builtin("upper", 1, Bytecode::Op::Upper);
    functions.add_builtin("exists", 1, Bytecode::Op::Exists);
    functions.add_builtin("existsIn", 2, Bytecode::Op::ExistsInObject);
    functions.add_builtin("isBoolean", 1, Bytecode::Op::IsBoolean);
    functions.add_builtin("isNumber", 1, Bytecode::Op::IsNumber);
    functions.add_builtin("isInteger", 1, Bytecode::Op::IsInteger);
    functions.add_builtin("isFloat", 1, Bytecode::Op::IsFloat);
    functions.add_builtin("isObject", 1, Bytecode::Op::IsObject);
    functions.add_builtin("isArray", 1, Bytecode::Op::IsArray);
    functions.add_builtin("isString", 1, Bytecode::Op::IsString);
  }

 public:
  ParserStatic(const ParserStatic&) = delete;
  ParserStatic& operator=(const ParserStatic&) = delete;

  static const ParserStatic& get_instance() {
    static ParserStatic inst;
    return inst;
  }

  FunctionStorage functions;
};

class Parser {
 public:
  explicit Parser(const ParserConfig& parser_config, const LexerConfig& lexer_config, TemplateStorage& included_templates): m_config(parser_config), m_lexer(lexer_config), m_included_templates(included_templates), m_static(ParserStatic::get_instance()) { }

  bool parse_expression(Template& tmpl) {
    if (!parse_expression_and(tmpl)) return false;
    if (m_tok.kind != Token::Kind::Id || m_tok.text != static_cast<decltype(m_tok.text)>("or")) return true;
    get_next_token();
    if (!parse_expression_and(tmpl)) return false;
    append_function(tmpl, Bytecode::Op::Or, 2);
    return true;
  }

  bool parse_expression_and(Template& tmpl) {
    if (!parse_expression_not(tmpl)) return false;
    if (m_tok.kind != Token::Kind::Id || m_tok.text != static_cast<decltype(m_tok.text)>("and")) return true;
    get_next_token();
    if (!parse_expression_not(tmpl)) return false;
    append_function(tmpl, Bytecode::Op::And, 2);
    return true;
  }

  bool parse_expression_not(Template& tmpl) {
    if (m_tok.kind == Token::Kind::Id && m_tok.text == static_cast<decltype(m_tok.text)>("not")) {
      get_next_token();
      if (!parse_expression_not(tmpl)) return false;
      append_function(tmpl, Bytecode::Op::Not, 1);
      return true;
    } else {
      return parse_expression_comparison(tmpl);
    }
  }

  bool parse_expression_comparison(Template& tmpl) {
    if (!parse_expression_datum(tmpl)) return false;
    Bytecode::Op op;
    switch (m_tok.kind) {
      case Token::Kind::Id:
        if (m_tok.text == static_cast<decltype(m_tok.text)>("in"))
          op = Bytecode::Op::In;
        else
          return true;
        break;
      case Token::Kind::Equal:
        op = Bytecode::Op::Equal;
        break;
      case Token::Kind::GreaterThan:
        op = Bytecode::Op::Greater;
        break;
      case Token::Kind::LessThan:
        op = Bytecode::Op::Less;
        break;
      case Token::Kind::LessEqual:
        op = Bytecode::Op::LessEqual;
        break;
      case Token::Kind::GreaterEqual:
        op = Bytecode::Op::GreaterEqual;
        break;
      case Token::Kind::NotEqual:
        op = Bytecode::Op::Different;
        break;
      default:
        return true;
    }
    get_next_token();
    if (!parse_expression_datum(tmpl)) return false;
    append_function(tmpl, op, 2);
    return true;
  }

  bool parse_expression_datum(Template& tmpl) {
    nonstd::string_view json_first;
    size_t bracket_level = 0;
    size_t brace_level = 0;

    for (;;) {
      switch (m_tok.kind) {
        case Token::Kind::LeftParen: {
          get_next_token();
          if (!parse_expression(tmpl)) return false;
          if (m_tok.kind != Token::Kind::RightParen) {
            inja_throw("parser_error", "unmatched '('");
          }
          get_next_token();
          return true;
        }
        case Token::Kind::Id:
          get_peek_token();
          if (m_peek_tok.kind == Token::Kind::LeftParen) {
            // function call, parse arguments
            Token func_token = m_tok;
            get_next_token();  // id
            get_next_token();  // leftParen
            unsigned int num_args = 0;
            if (m_tok.kind == Token::Kind::RightParen) {
              // no args
              get_next_token();
            } else {
              for (;;) {
                if (!parse_expression(tmpl)) {
                  inja_throw("parser_error", "expected expression, got '" + m_tok.describe() + "'");
                }
                num_args += 1;
                if (m_tok.kind == Token::Kind::RightParen) {
                  get_next_token();
                  break;
                }
                if (m_tok.kind != Token::Kind::Comma) {
                  inja_throw("parser_error", "expected ')' or ',', got '" + m_tok.describe() + "'");
                }
                get_next_token();
              }
            }

            auto op = m_static.functions.find_builtin(func_token.text, num_args);

            if (op != Bytecode::Op::Nop) {
              // swap arguments for default(); see comment in RenderTo()
              if (op == Bytecode::Op::Default)
                std::swap(tmpl.bytecodes.back(), *(tmpl.bytecodes.rbegin() + 1));
              append_function(tmpl, op, num_args);
              return true;
            } else {
              append_callback(tmpl, func_token.text, num_args);
              return true;
            }
          } else if (m_tok.text == static_cast<decltype(m_tok.text)>("true") ||
              m_tok.text == static_cast<decltype(m_tok.text)>("false") ||
              m_tok.text == static_cast<decltype(m_tok.text)>("null")) {
            // true, false, null are json literals
            if (brace_level == 0 && bracket_level == 0) {
              json_first = m_tok.text;
              goto returnJson;
            }
            break;
          } else {
            // normal literal (json read)
            tmpl.bytecodes.emplace_back(
                Bytecode::Op::Push, m_tok.text,
                m_config.notation == ElementNotation::Pointer ? Bytecode::Flag::ValueLookupPointer : Bytecode::Flag::ValueLookupDot);
            get_next_token();
            return true;
          }
        // json passthrough
        case Token::Kind::Number:
        case Token::Kind::String:
          if (brace_level == 0 && bracket_level == 0) {
            json_first = m_tok.text;
            goto returnJson;
          }
          break;
        case Token::Kind::Comma:
        case Token::Kind::Colon:
          if (brace_level == 0 && bracket_level == 0) {
            inja_throw("parser_error", "unexpected token '" + m_tok.describe() + "'");
          }
          break;
        case Token::Kind::LeftBracket:
          if (brace_level == 0 && bracket_level == 0) {
            json_first = m_tok.text;
          }
          bracket_level += 1;
          break;
        case Token::Kind::LeftBrace:
          if (brace_level == 0 && bracket_level == 0) {
            json_first = m_tok.text;
          }
          brace_level += 1;
          break;
        case Token::Kind::RightBracket:
          if (bracket_level == 0) {
            inja_throw("parser_error", "unexpected ']'");
          }
          --bracket_level;
          if (brace_level == 0 && bracket_level == 0) goto returnJson;
          break;
        case Token::Kind::RightBrace:
          if (brace_level == 0) {
            inja_throw("parser_error", "unexpected '}'");
          }
          --brace_level;
          if (brace_level == 0 && bracket_level == 0) goto returnJson;
          break;
        default:
          if (brace_level != 0) {
            inja_throw("parser_error", "unmatched '{'");
          }
          if (bracket_level != 0) {
            inja_throw("parser_error", "unmatched '['");
          }
          return false;
      }

      get_next_token();
    }

  returnJson:
    // bridge across all intermediate tokens
    nonstd::string_view json_text(json_first.data(), m_tok.text.data() - json_first.data() + m_tok.text.size());
    tmpl.bytecodes.emplace_back(Bytecode::Op::Push, json::parse(json_text), Bytecode::Flag::ValueImmediate);
    get_next_token();
    return true;
  }

  bool parse_statement(Template& tmpl, nonstd::string_view path) {
    if (m_tok.kind != Token::Kind::Id) return false;

    if (m_tok.text == static_cast<decltype(m_tok.text)>("if")) {
      get_next_token();

      // evaluate expression
      if (!parse_expression(tmpl)) return false;

      // start a new if block on if stack
      m_if_stack.emplace_back(tmpl.bytecodes.size());

      // conditional jump; destination will be filled in by else or endif
      tmpl.bytecodes.emplace_back(Bytecode::Op::ConditionalJump);
    } else if (m_tok.text == static_cast<decltype(m_tok.text)>("endif")) {
      if (m_if_stack.empty()) {
        inja_throw("parser_error", "endif without matching if");
      }
      auto& if_data = m_if_stack.back();
      get_next_token();

      // previous conditional jump jumps here
      if (if_data.prev_cond_jump != std::numeric_limits<unsigned int>::max()) {
        tmpl.bytecodes[if_data.prev_cond_jump].args = tmpl.bytecodes.size();
      }

      // update all previous unconditional jumps to here
      for (unsigned int i: if_data.uncond_jumps) {
        tmpl.bytecodes[i].args = tmpl.bytecodes.size();
      }

      // pop if stack
      m_if_stack.pop_back();
    } else if (m_tok.text == static_cast<decltype(m_tok.text)>("else")) {
      if (m_if_stack.empty())
        inja_throw("parser_error", "else without matching if");
      auto& if_data = m_if_stack.back();
      get_next_token();

      // end previous block with unconditional jump to endif; destination will be
      // filled in by endif
      if_data.uncond_jumps.push_back(tmpl.bytecodes.size());
      tmpl.bytecodes.emplace_back(Bytecode::Op::Jump);

      // previous conditional jump jumps here
      tmpl.bytecodes[if_data.prev_cond_jump].args = tmpl.bytecodes.size();
      if_data.prev_cond_jump = std::numeric_limits<unsigned int>::max();

      // chained else if
      if (m_tok.kind == Token::Kind::Id && m_tok.text == static_cast<decltype(m_tok.text)>("if")) {
        get_next_token();

        // evaluate expression
        if (!parse_expression(tmpl)) return false;

        // update "previous jump"
        if_data.prev_cond_jump = tmpl.bytecodes.size();

        // conditional jump; destination will be filled in by else or endif
        tmpl.bytecodes.emplace_back(Bytecode::Op::ConditionalJump);
      }
    } else if (m_tok.text == static_cast<decltype(m_tok.text)>("for")) {
      get_next_token();

      // options: for a in arr; for a, b in obj
      if (m_tok.kind != Token::Kind::Id)
        inja_throw("parser_error", "expected id, got '" + m_tok.describe() + "'");
      Token value_token = m_tok;
      get_next_token();

      Token key_token;
      if (m_tok.kind == Token::Kind::Comma) {
        get_next_token();
        if (m_tok.kind != Token::Kind::Id)
          inja_throw("parser_error", "expected id, got '" + m_tok.describe() + "'");
        key_token = std::move(value_token);
        value_token = m_tok;
        get_next_token();
      }

      if (m_tok.kind != Token::Kind::Id || m_tok.text != static_cast<decltype(m_tok.text)>("in"))
        inja_throw("parser_error",
                   "expected 'in', got '" + m_tok.describe() + "'");
      get_next_token();

      if (!parse_expression(tmpl)) return false;

      m_loop_stack.push_back(tmpl.bytecodes.size());

      tmpl.bytecodes.emplace_back(Bytecode::Op::StartLoop);
      if (!key_token.text.empty()) {
        tmpl.bytecodes.back().value = key_token.text;
      }
      tmpl.bytecodes.back().str = static_cast<std::string>(value_token.text);
    } else if (m_tok.text == static_cast<decltype(m_tok.text)>("endfor")) {
      get_next_token();
      if (m_loop_stack.empty()) {
        inja_throw("parser_error", "endfor without matching for");
      }

      // update loop with EndLoop index (for empty case)
      tmpl.bytecodes[m_loop_stack.back()].args = tmpl.bytecodes.size();

      tmpl.bytecodes.emplace_back(Bytecode::Op::EndLoop);
      tmpl.bytecodes.back().args = m_loop_stack.back() + 1;  // loop body
      m_loop_stack.pop_back();
    } else if (m_tok.text == static_cast<decltype(m_tok.text)>("include")) {
      get_next_token();

      if (m_tok.kind != Token::Kind::String) {
        inja_throw("parser_error", "expected string, got '" + m_tok.describe() + "'");
      }

      // build the relative path
      json json_name = json::parse(m_tok.text);
      std::string pathname = static_cast<std::string>(path);
      pathname += json_name.get_ref<const std::string&>();
      if (pathname.compare(0, 2, "./") == 0) {
        pathname.erase(0, 2);
      }
      // sys::path::remove_dots(pathname, true, sys::path::Style::posix);

      Template include_template = parse_template(pathname);
      m_included_templates.emplace(pathname, include_template);

      // generate a reference bytecode
      tmpl.bytecodes.emplace_back(Bytecode::Op::Include, json(pathname), Bytecode::Flag::ValueImmediate);

      get_next_token();
    } else {
      return false;
    }
    return true;
  }

  void append_function(Template& tmpl, Bytecode::Op op, unsigned int num_args) {
    // we can merge with back-to-back push
    if (!tmpl.bytecodes.empty()) {
      Bytecode& last = tmpl.bytecodes.back();
      if (last.op == Bytecode::Op::Push) {
        last.op = op;
        last.args = num_args;
        return;
      }
    }

    // otherwise just add it to the end
    tmpl.bytecodes.emplace_back(op, num_args);
  }

  void append_callback(Template& tmpl, nonstd::string_view name, unsigned int num_args) {
    // we can merge with back-to-back push value (not lookup)
    if (!tmpl.bytecodes.empty()) {
      Bytecode& last = tmpl.bytecodes.back();
      if (last.op == Bytecode::Op::Push &&
          (last.flags & Bytecode::Flag::ValueMask) == Bytecode::Flag::ValueImmediate) {
        last.op = Bytecode::Op::Callback;
        last.args = num_args;
        last.str = static_cast<std::string>(name);
        return;
      }
    }

    // otherwise just add it to the end
    tmpl.bytecodes.emplace_back(Bytecode::Op::Callback, num_args);
    tmpl.bytecodes.back().str = static_cast<std::string>(name);
  }

  void parse_into(Template& tmpl, nonstd::string_view path) {
    m_lexer.start(tmpl.content);

    for (;;) {
      get_next_token();
      switch (m_tok.kind) {
        case Token::Kind::Eof:
          if (!m_if_stack.empty()) inja_throw("parser_error", "unmatched if");
          if (!m_loop_stack.empty()) inja_throw("parser_error", "unmatched for");
          return;
        case Token::Kind::Text:
          tmpl.bytecodes.emplace_back(Bytecode::Op::PrintText, m_tok.text, 0u);
          break;
        case Token::Kind::StatementOpen:
          get_next_token();
          if (!parse_statement(tmpl, path)) {
            inja_throw("parser_error", "expected statement, got '" + m_tok.describe() + "'");
          }
          if (m_tok.kind != Token::Kind::StatementClose) {
            inja_throw("parser_error", "expected statement close, got '" + m_tok.describe() + "'");
          }
          break;
        case Token::Kind::LineStatementOpen:
          get_next_token();
          parse_statement(tmpl, path);
          if (m_tok.kind != Token::Kind::LineStatementClose &&
              m_tok.kind != Token::Kind::Eof) {
            inja_throw("parser_error", "expected line statement close, got '" + m_tok.describe() + "'");
          }
          break;
        case Token::Kind::ExpressionOpen:
          get_next_token();
          if (!parse_expression(tmpl)) {
            inja_throw("parser_error", "expected expression, got '" + m_tok.describe() + "'");
          }
          append_function(tmpl, Bytecode::Op::PrintValue, 1);
          if (m_tok.kind != Token::Kind::ExpressionClose) {
            inja_throw("parser_error", "expected expression close, got '" + m_tok.describe() + "'");
          }
          break;
        case Token::Kind::CommentOpen:
          get_next_token();
          if (m_tok.kind != Token::Kind::CommentClose) {
            inja_throw("parser_error", "expected comment close, got '" + m_tok.describe() + "'");
          }
          break;
        default:
          inja_throw("parser_error", "unexpected token '" + m_tok.describe() + "'");
          break;
      }
    }
  }

  Template parse(nonstd::string_view input, nonstd::string_view path) {
    Template result;
    result.content = static_cast<std::string>(input);
    parse_into(result, path);
    return result;
  }

  Template parse(nonstd::string_view input) {
    return parse(input, "./");
  }

  Template parse_template(nonstd::string_view filename) {
    Template result;
    result.content = load_file(filename);

    nonstd::string_view path = filename.substr(0, filename.find_last_of("/\\") + 1);
      // StringRef path = sys::path::parent_path(filename);
    Parser(m_config, m_lexer.get_config(), m_included_templates).parse_into(result, path);
    return result;
  }

  std::string load_file(nonstd::string_view filename) {
		std::ifstream file(static_cast<std::string>(filename));
		std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return text;
	}

 private:
  const ParserConfig& m_config;
  Lexer m_lexer;
  Token m_tok;
  Token m_peek_tok;
  bool m_have_peek_tok {false};
  TemplateStorage& m_included_templates;
  const ParserStatic& m_static;

  struct IfData {
    unsigned int prev_cond_jump;
    std::vector<unsigned int> uncond_jumps;

    explicit IfData(unsigned int condJump): prev_cond_jump(condJump) {}
  };

  std::vector<IfData> m_if_stack;
  std::vector<unsigned int> m_loop_stack;

  void get_next_token() {
    if (m_have_peek_tok) {
      m_tok = m_peek_tok;
      m_have_peek_tok = false;
    } else {
      m_tok = m_lexer.scan();
    }
  }

  void get_peek_token() {
    if (!m_have_peek_tok) {
      m_peek_tok = m_lexer.scan();
      m_have_peek_tok = true;
    }
  }
};

}  // namespace inja

#endif  // PANTOR_INJA_PARSER_HPP
