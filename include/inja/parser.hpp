#ifndef PANTOR_INJA_PARSER_HPP
#define PANTOR_INJA_PARSER_HPP

#include <limits>

#include <inja/bytecode.hpp>
#include <inja/config.hpp>
#include <inja/function_storage.hpp>
#include <inja/lexer.hpp>
#include <inja/template.hpp>
#include <inja/token.hpp>
#include <inja/utils.hpp>

#include <nlohmann/json.hpp>


namespace inja {

class ParserStatic {
 public:
  ParserStatic(const ParserStatic&) = delete;
  ParserStatic& operator=(const ParserStatic&) = delete;

  static const ParserStatic& get_instance() {
    static ParserStatic inst;
    return inst;
  }

  FunctionStorage functions;

 private:
  ParserStatic() {
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
};

class Parser {
 public:
  Parser(const ParserConfig& parserConfig, const LexerConfig& lexerConfig,
         TemplateStorage& includedTemplates): m_config(parserConfig),
           m_lexer(lexerConfig),
           m_includedTemplates(includedTemplates),
           m_static(ParserStatic::get_instance()) { }

  bool parse_expression(Template& tmpl) {
    if (!parse_expression_and(tmpl)) return false;
    if (m_tok.kind != Token::kId || m_tok.text != "or") return true;
    get_next_token();
    if (!parse_expression_and(tmpl)) return false;
    append_function(tmpl, Bytecode::Op::Or, 2);
    return true;
  }

  bool parse_expression_and(Template& tmpl) {
    if (!parse_expression_not(tmpl)) return false;
    if (m_tok.kind != Token::kId || m_tok.text != "and") return true;
    get_next_token();
    if (!parse_expression_not(tmpl)) return false;
    append_function(tmpl, Bytecode::Op::And, 2);
    return true;
  }

  bool parse_expression_not(Template& tmpl) {
    if (m_tok.kind == Token::kId && m_tok.text == "not") {
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
      case Token::kId:
        if (m_tok.text == "in")
          op = Bytecode::Op::In;
        else
          return true;
        break;
      case Token::kEqual:
        op = Bytecode::Op::Equal;
        break;
      case Token::kGreaterThan:
        op = Bytecode::Op::Greater;
        break;
      case Token::kLessThan:
        op = Bytecode::Op::Less;
        break;
      case Token::kLessEqual:
        op = Bytecode::Op::LessEqual;
        break;
      case Token::kGreaterEqual:
        op = Bytecode::Op::GreaterEqual;
        break;
      case Token::kNotEqual:
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
    std::string_view jsonFirst;
    size_t bracketLevel = 0;
    size_t braceLevel = 0;

    for (;;) {
      switch (m_tok.kind) {
        case Token::kLeftParen: {
          get_next_token();
          if (!parse_expression(tmpl)) return false;
          if (m_tok.kind != Token::kRightParen) {
            inja_throw("parser_error", "unmatched '('");
          }
          get_next_token();
          return true;
        }
        case Token::kId:
          get_peek_token();
          if (m_peekTok.kind == Token::kLeftParen) {
            // function call, parse arguments
            Token funcTok = m_tok;
            get_next_token();  // id
            get_next_token();  // leftParen
            unsigned int numArgs = 0;
            if (m_tok.kind == Token::kRightParen) {
              // no args
              get_next_token();
            } else {
              for (;;) {
                if (!parse_expression(tmpl)) {
                  inja_throw("parser_error", "expected expression, got '" + static_cast<std::string>(m_tok.describe()) + "'");
                }
                ++numArgs;
                if (m_tok.kind == Token::kRightParen) {
                  get_next_token();
                  break;
                }
                if (m_tok.kind != Token::kComma) {
                  inja_throw("parser_error", "expected ')' or ',', got '" + static_cast<std::string>(m_tok.describe()) + "'");
                }
                get_next_token();
              }
            }

            auto op = m_static.functions.find_builtin(funcTok.text, numArgs);

            if (op != Bytecode::Op::Nop) {
              // swap arguments for default(); see comment in RenderTo()
              if (op == Bytecode::Op::Default)
                std::swap(tmpl.bytecodes.back(), *(tmpl.bytecodes.rbegin() + 1));
              append_function(tmpl, op, numArgs);
              return true;
            } else {
              append_callback(tmpl, funcTok.text, numArgs);
              return true;
            }
          } else if (m_tok.text == "true" || m_tok.text == "false" || m_tok.text == "null") {
            // true, false, null are json literals
            if (braceLevel == 0 && bracketLevel == 0) {
              jsonFirst = m_tok.text;
              goto returnJson;
            }
            break;
          } else {
            // normal literal (json read)
            tmpl.bytecodes.emplace_back(
                Bytecode::Op::Push, m_tok.text,
                m_config.notation == ElementNotation::Pointer ? Bytecode::kFlagValueLookupPointer : Bytecode::kFlagValueLookupDot);
            get_next_token();
            return true;
          }
        // json passthrough
        case Token::kNumber:
        case Token::kString:
          if (braceLevel == 0 && bracketLevel == 0) {
            jsonFirst = m_tok.text;
            goto returnJson;
          }
          break;
        case Token::kComma:
        case Token::kColon:
          if (braceLevel == 0 && bracketLevel == 0) {
            inja_throw("parser_error", "unexpected token '" + static_cast<std::string>(m_tok.describe()) + "'");
          }
          break;
        case Token::kLeftBracket:
          if (braceLevel == 0 && bracketLevel == 0) jsonFirst = m_tok.text;
          ++bracketLevel;
          break;
        case Token::kLeftBrace:
          if (braceLevel == 0 && bracketLevel == 0) jsonFirst = m_tok.text;
          ++braceLevel;
          break;
        case Token::kRightBracket:
          if (bracketLevel == 0) {
            inja_throw("parser_error", "unexpected ']'");
          }
          --bracketLevel;
          if (braceLevel == 0 && bracketLevel == 0) goto returnJson;
          break;
        case Token::kRightBrace:
          if (braceLevel == 0) {
            inja_throw("parser_error", "unexpected '}'");
          }
          --braceLevel;
          if (braceLevel == 0 && bracketLevel == 0) goto returnJson;
          break;
        default:
          if (braceLevel != 0) {
            inja_throw("parser_error", "unmatched '{'");
          }
          if (bracketLevel != 0) {
            inja_throw("parser_error", "unmatched '['");
          }
          return false;
      }

      get_next_token();
    }

  returnJson:
    // bridge across all intermediate tokens
    std::string_view jsonText(jsonFirst.data(), m_tok.text.data() - jsonFirst.data() + m_tok.text.size());
    tmpl.bytecodes.emplace_back(Bytecode::Op::Push, json::parse(jsonText), Bytecode::kFlagValueImmediate);
    get_next_token();
    return true;
  }

  bool parse_statement(Template& tmpl, std::string_view path) {
    if (m_tok.kind != Token::kId) return false;

    if (m_tok.text == "if") {
      get_next_token();

      // evaluate expression
      if (!parse_expression(tmpl)) return false;

      // start a new if block on if stack
      m_ifStack.emplace_back(tmpl.bytecodes.size());

      // conditional jump; destination will be filled in by else or endif
      tmpl.bytecodes.emplace_back(Bytecode::Op::ConditionalJump);
    } else if (m_tok.text == "endif") {
      if (m_ifStack.empty())
        inja_throw("parser_error", "endif without matching if");
      auto& ifData = m_ifStack.back();
      get_next_token();

      // previous conditional jump jumps here
      if (ifData.prevCondJump != std::numeric_limits<unsigned int>::max())
        tmpl.bytecodes[ifData.prevCondJump].args = tmpl.bytecodes.size();

      // update all previous unconditional jumps to here
      for (unsigned int i : ifData.uncondJumps)
        tmpl.bytecodes[i].args = tmpl.bytecodes.size();

      // pop if stack
      m_ifStack.pop_back();
    } else if (m_tok.text == "else") {
      if (m_ifStack.empty())
        inja_throw("parser_error", "else without matching if");
      auto& ifData = m_ifStack.back();
      get_next_token();

      // end previous block with unconditional jump to endif; destination will be
      // filled in by endif
      ifData.uncondJumps.push_back(tmpl.bytecodes.size());
      tmpl.bytecodes.emplace_back(Bytecode::Op::Jump);

      // previous conditional jump jumps here
      tmpl.bytecodes[ifData.prevCondJump].args = tmpl.bytecodes.size();
      ifData.prevCondJump = std::numeric_limits<unsigned int>::max();

      // chained else if
      if (m_tok.kind == Token::kId && m_tok.text == "if") {
        get_next_token();

        // evaluate expression
        if (!parse_expression(tmpl)) return false;

        // update "previous jump"
        ifData.prevCondJump = tmpl.bytecodes.size();

        // conditional jump; destination will be filled in by else or endif
        tmpl.bytecodes.emplace_back(Bytecode::Op::ConditionalJump);
      }
    } else if (m_tok.text == "for") {
      get_next_token();

      // options: for a in arr; for a, b in obj
      if (m_tok.kind != Token::kId)
        inja_throw("parser_error",
                   "expected id, got '" + static_cast<std::string>(m_tok.describe()) + "'");
      Token valueTok = m_tok;
      get_next_token();

      Token keyTok;
      if (m_tok.kind == Token::kComma) {
        get_next_token();
        if (m_tok.kind != Token::kId)
          inja_throw("parser_error",
                     "expected id, got '" + static_cast<std::string>(m_tok.describe()) + "'");
        keyTok = std::move(valueTok);
        valueTok = m_tok;
        get_next_token();
      }

      if (m_tok.kind != Token::kId || m_tok.text != "in")
        inja_throw("parser_error",
                   "expected 'in', got '" + static_cast<std::string>(m_tok.describe()) + "'");
      get_next_token();

      if (!parse_expression(tmpl)) return false;

      m_loopStack.push_back(tmpl.bytecodes.size());

      tmpl.bytecodes.emplace_back(Bytecode::Op::StartLoop);
      if (!keyTok.text.empty()) tmpl.bytecodes.back().value = keyTok.text;
      tmpl.bytecodes.back().str = valueTok.text;
    } else if (m_tok.text == "endfor") {
      get_next_token();
      if (m_loopStack.empty())
        inja_throw("parser_error", "endfor without matching for");

      // update loop with EndLoop index (for empty case)
      tmpl.bytecodes[m_loopStack.back()].args = tmpl.bytecodes.size();

      tmpl.bytecodes.emplace_back(Bytecode::Op::EndLoop);
      tmpl.bytecodes.back().args = m_loopStack.back() + 1;  // loop body
      m_loopStack.pop_back();
    } else if (m_tok.text == "include") {
      get_next_token();

      if (m_tok.kind != Token::kString)
        inja_throw("parser_error", "expected string, got '" + static_cast<std::string>(m_tok.describe()) + "'");

      // build the relative path
      json jsonName = json::parse(m_tok.text);
      std::string pathname = static_cast<std::string>(path);
      pathname += jsonName.get_ref<const std::string&>();
      // sys::path::remove_dots(pathname, true, sys::path::Style::posix);

      // parse it only if it's new
      TemplateStorage::iterator included;
      bool is_new {true};
      // TODO: std::tie(included, isNew) = m_includedTemplates.emplace(pathname);
      if (is_new) included->second = parse_template(pathname);

      // generate a reference bytecode
      tmpl.bytecodes.emplace_back(Bytecode::Op::Include, json(pathname), Bytecode::kFlagValueImmediate);

      get_next_token();
    } else {
      return false;
    }
    return true;
  }

  void append_function(Template& tmpl, Bytecode::Op op, unsigned int numArgs) {
    // we can merge with back-to-back push
    if (!tmpl.bytecodes.empty()) {
      Bytecode& last = tmpl.bytecodes.back();
      if (last.op == Bytecode::Op::Push) {
        last.op = op;
        last.args = numArgs;
        return;
      }
    }

    // otherwise just add it to the end
    tmpl.bytecodes.emplace_back(op, numArgs);
  }

  void append_callback(Template& tmpl, std::string_view name, unsigned int numArgs) {
    // we can merge with back-to-back push value (not lookup)
    if (!tmpl.bytecodes.empty()) {
      Bytecode& last = tmpl.bytecodes.back();
      if (last.op == Bytecode::Op::Push &&
          (last.flags & Bytecode::kFlagValueMask) ==
              Bytecode::kFlagValueImmediate) {
        last.op = Bytecode::Op::Callback;
        last.args = numArgs;
        last.str = name;
        return;
      }
    }

    // otherwise just add it to the end
    tmpl.bytecodes.emplace_back(Bytecode::Op::Callback, numArgs);
    tmpl.bytecodes.back().str = name;
  }

  void parse_into(Template& tmpl, std::string_view path) {
    m_lexer.start(tmpl.contents);

    for (;;) {
      get_next_token();
      switch (m_tok.kind) {
        case Token::kEof:
          if (!m_ifStack.empty()) inja_throw("parser_error", "unmatched if");
          if (!m_loopStack.empty()) inja_throw("parser_error", "unmatched for");
          return;
        case Token::kText:
          tmpl.bytecodes.emplace_back(Bytecode::Op::PrintText, m_tok.text, 0u);
          break;
        case Token::kStatementOpen:
          get_next_token();
          if (!parse_statement(tmpl, path)) {
            inja_throw("parser_error", "expected statement, got '" + static_cast<std::string>(m_tok.describe()) + "'");
          }
          if (m_tok.kind != Token::kStatementClose) {
            inja_throw("parser_error", "expected statement close, got '" + static_cast<std::string>(m_tok.describe()) + "'");
          }
          break;
        case Token::kLineStatementOpen:
          get_next_token();
          parse_statement(tmpl, path);
          if (m_tok.kind != Token::kLineStatementClose &&
              m_tok.kind != Token::kEof) {
            inja_throw("parser_error", "expected line statement close, got '" + static_cast<std::string>(m_tok.describe()) + "'");
          }
          break;
        case Token::kExpressionOpen:
          get_next_token();
          if (!parse_expression(tmpl)) {
            inja_throw("parser_error", "expected expression, got '" + static_cast<std::string>(m_tok.describe()) + "'");
          }
          append_function(tmpl, Bytecode::Op::PrintValue, 1);
          if (m_tok.kind != Token::kExpressionClose) {
            inja_throw("parser_error", "expected expression close, got '" + static_cast<std::string>(m_tok.describe()) + "'");
          }
          break;
        case Token::kCommentOpen:
          get_next_token();
          if (m_tok.kind != Token::kCommentClose) {
            inja_throw("parser_error", "expected comment close, got '" + static_cast<std::string>(m_tok.describe()) + "'");
          }
          break;
        default:
          inja_throw("parser_error", "unexpected token '" + static_cast<std::string>(m_tok.describe()) + "'");
          break;
      }
    }
  }

  Template parse(std::string_view input, std::string_view path) {
    Template result;
    result.contents = input;
    parse_into(result, path);
    return result;
  }

  Template parse(std::string_view input) {
    return parse(input, "./");
  }

  Template parse_template(std::string_view filename) {
    Template result;
    if (m_config.loadFile) {
      result.contents = m_config.loadFile(filename);
      std::string_view path = filename.substr(0, filename.find_last_of("/\\") + 1);
      // StringRef path = sys::path::parent_path(filename);
      Parser(m_config, m_lexer.get_config(), m_includedTemplates).parse_into(result, path);
    }
    return result;
  }

 private:
  const ParserConfig& m_config;
  Lexer m_lexer;
  Token m_tok;
  Token m_peekTok;
  bool m_havePeekTok = false;
  TemplateStorage& m_includedTemplates;
  const ParserStatic& m_static;
  struct IfData {
    unsigned int prevCondJump;
    std::vector<unsigned int> uncondJumps;

    explicit IfData(unsigned int condJump): prevCondJump(condJump) {}
  };
  std::vector<IfData> m_ifStack;
  std::vector<unsigned int> m_loopStack;

  void get_next_token() {
    if (m_havePeekTok) {
      m_tok = m_peekTok;
      m_havePeekTok = false;
    } else {
      m_tok = m_lexer.scan();
    }
  }

  void get_peek_token() {
    if (!m_havePeekTok) {
      m_peekTok = m_lexer.scan();
      m_havePeekTok = true;
    }
  }
};

}  // namespace inja

#endif  // PANTOR_INJA_PARSER_HPP
