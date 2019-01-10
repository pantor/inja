#ifndef PANTOR_INJA_HPP
#define PANTOR_INJA_HPP

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

// #include "environment.hpp"
#ifndef PANTOR_INJA_ENVIRONMENT_HPP
#define PANTOR_INJA_ENVIRONMENT_HPP

#include <memory>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

// #include "config.hpp"
#ifndef PANTOR_INJA_CONFIG_HPP
#define PANTOR_INJA_CONFIG_HPP

#include <functional>
#include <string>
#include <string_view>


namespace inja {

enum class ElementNotation {
  Dot,
  Pointer
};

struct LexerConfig {
  std::string statement_open {"{%"};
  std::string statement_close {"%}"};
  std::string line_statement {"##"};
  std::string expression_open {"{{"};
  std::string expression_close {"}}"};
  std::string comment_open {"{#"};
  std::string comment_close {"#}"};
  std::string open_chars {"#{"};

  void update_open_chars() {
    open_chars = "\n";
    if (open_chars.find(statement_open[0]) == std::string::npos) {
      open_chars += statement_open[0];
    }
    if (open_chars.find(expression_open[0]) == std::string::npos) {
      open_chars += expression_open[0];
    }
    if (open_chars.find(comment_open[0]) == std::string::npos) {
      open_chars += comment_open[0];
    }
  }
};

struct ParserConfig {
  ElementNotation notation {ElementNotation::Dot};
};

}

#endif // PANTOR_INJA_CONFIG_HPP

// #include "function_storage.hpp"
#ifndef PANTOR_INJA_FUNCTION_STORAGE_HPP
#define PANTOR_INJA_FUNCTION_STORAGE_HPP

#include <string_view>

// #include "bytecode.hpp"
#ifndef PANTOR_INJA_BYTECODE_HPP
#define PANTOR_INJA_BYTECODE_HPP

#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>


namespace inja {

using namespace nlohmann;


struct Bytecode {
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
    // args is the index of the bytecode to jump to.
    Jump,

    // conditional jump
    // value popped off stack is checked for truthyness
    // if false, args is the index of the bytecode to jump to.
    // if true, no action is taken (falls through)
    ConditionalJump,

    // start loop
    // value popped off stack is what is iterated over
    // args is index of bytecode after end loop (jumped to if iterable is
    // empty)
    // immediate value is key name (for maps)
    // str is value name
    StartLoop,

    // end a loop
    // args is index of the first bytecode in the loop body
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
  uint32_t args: 30;
  uint32_t flags: 2;

  json value;
  std::string str;

  Bytecode(): args(0), flags(0) {}
  explicit Bytecode(Op op, unsigned int args = 0): op(op), args(args), flags(0) {}
  explicit Bytecode(Op op, std::string_view str, unsigned int flags): op(op), args(0), flags(flags), str(str) {}
  explicit Bytecode(Op op, json&& value, unsigned int flags): op(op), args(0), flags(flags), value(std::move(value)) {}
};

}  // namespace inja

#endif  // PANTOR_INJA_BYTECODE_HPP



namespace inja {

using namespace nlohmann;

using Arguments = std::vector<const json*>;
using CallbackFunction = std::function<json(Arguments& args)>;

class FunctionStorage {
 public:
  void add_builtin(std::string_view name, unsigned int num_args, Bytecode::Op op) {
    auto& data = get_or_new(name, num_args);
    data.op = op;
  }

  void add_callback(std::string_view name, unsigned int num_args, const CallbackFunction& function) {
    auto& data = get_or_new(name, num_args);
    data.function = function;
  }

  Bytecode::Op find_builtin(std::string_view name, unsigned int num_args) const {
    if (auto ptr = get(name, num_args)) {
      return ptr->op;
    }
    return Bytecode::Op::Nop;
  }

  CallbackFunction find_callback(std::string_view name, unsigned int num_args) const {
    if (auto ptr = get(name, num_args)) {
      return ptr->function;
    }
    return nullptr;
  }

 private:
  struct FunctionData {
    unsigned int num_args {0};
    Bytecode::Op op {Bytecode::Op::Nop}; // for builtins
    CallbackFunction function; // for callbacks
  };

  FunctionData& get_or_new(std::string_view name, unsigned int num_args) {
    auto &vec = m_map[static_cast<std::string>(name)];
    for (auto &i: vec) {
      if (i.num_args == num_args) return i;
    }
    vec.emplace_back();
    vec.back().num_args = num_args;
    return vec.back();
  }

  const FunctionData* get(std::string_view name, unsigned int num_args) const {
    auto it = m_map.find(static_cast<std::string>(name));
    if (it == m_map.end()) return nullptr;
    for (auto &&i: it->second) {
      if (i.num_args == num_args) return &i;
    }
    return nullptr;
  }

  std::map<std::string, std::vector<FunctionData>> m_map;
};

}

#endif // PANTOR_INJA_FUNCTION_STORAGE_HPP

// #include "parser.hpp"
#ifndef PANTOR_INJA_PARSER_HPP
#define PANTOR_INJA_PARSER_HPP

#include <limits>

// #include "bytecode.hpp"

// #include "config.hpp"

// #include "function_storage.hpp"

// #include "lexer.hpp"
#ifndef PANTOR_INJA_LEXER_HPP
#define PANTOR_INJA_LEXER_HPP

#include <cctype>
#include <locale>

// #include "config.hpp"

// #include "token.hpp"
#ifndef PANTOR_INJA_TOKEN_HPP
#define PANTOR_INJA_TOKEN_HPP

#include <string_view>


namespace inja {

struct Token {
  enum class Kind {
    Text,
    ExpressionOpen,      // {{
    ExpressionClose,     // }}
    LineStatementOpen,   // ##
    LineStatementClose,  // \n
    StatementOpen,       // {%
    StatementClose,      // %}
    CommentOpen,         // {#
    CommentClose,        // #}
    Id,                  // this, this.foo
    Number,              // 1, 2, -1, 5.2, -5.3
    String,              // "this"
    Comma,               // ,
    Colon,               // :
    LeftParen,           // (
    RightParen,          // )
    LeftBracket,         // [
    RightBracket,        // ]
    LeftBrace,           // {
    RightBrace,          // }
    Equal,               // ==
    GreaterThan,         // >
    GreaterEqual,        // >=
    LessThan,            // <
    LessEqual,           // <=
    NotEqual,            // !=
    Unknown,
    Eof
  } kind {Kind::Unknown};

  std::string_view text;

  constexpr Token() = default;
  constexpr Token(Kind kind, std::string_view text): kind(kind), text(text) {}

  std::string describe() const {
    switch (kind) {
      case Kind::Text:
        return "<text>";
      case Kind::LineStatementClose:
        return "<eol>";
      case Kind::Eof:
        return "<eof>";
      default:
        return static_cast<std::string>(text);
    }
  }
};

}

#endif // PANTOR_INJA_TOKEN_HPP

// #include "utils.hpp"
#ifndef PANTOR_INJA_UTILS_HPP
#define PANTOR_INJA_UTILS_HPP

#include <stdexcept>
#include <string_view>


namespace inja {

inline void inja_throw(const std::string& type, const std::string& message) {
  throw std::runtime_error("[inja.exception." + type + "] " + message);
}

namespace string_view {
  inline std::string_view slice(std::string_view view, size_t start, size_t end) {
    start = std::min(start, view.size());
    end = std::min(std::max(start, end), view.size());
    return view.substr(start, end - start); // StringRef(Data + Start, End - Start);
  }

  inline std::pair<std::string_view, std::string_view> split(std::string_view view, char Separator) {
    size_t idx = view.find(Separator);
    if (idx == std::string_view::npos) {
      return std::make_pair(view, std::string_view());
    }
    return std::make_pair(slice(view, 0, idx), slice(view, idx + 1, std::string_view::npos));
  }

  inline bool starts_with(std::string_view view, std::string_view prefix) {
    return (view.size() >= prefix.size() && view.compare(0, prefix.size(), prefix) == 0);
  }
}  // namespace string

}  // namespace inja

#endif  // PANTOR_INJA_UTILS_HPP



namespace inja {

class Lexer {
  enum class State {
    Text,
    ExpressionStart,
    ExpressionBody,
    LineStart,
    LineBody,
    StatementStart,
    StatementBody,
    CommentStart,
    CommentBody
  } m_state;

  const LexerConfig& m_config;
  std::string_view m_in;
  size_t m_tok_start;
  size_t m_pos;

 public:
  explicit Lexer(const LexerConfig& config) : m_config(config) {}

  void start(std::string_view in) {
    m_in = in;
    m_tok_start = 0;
    m_pos = 0;
    m_state = State::Text;
  }

  Token scan() {
    m_tok_start = m_pos;

  again:
    if (m_tok_start >= m_in.size()) return make_token(Token::Kind::Eof);

    switch (m_state) {
      default:
      case State::Text: {
        // fast-scan to first open character
        size_t open_start = m_in.substr(m_pos).find_first_of(m_config.open_chars);
        if (open_start == std::string_view::npos) {
          // didn't find open, return remaining text as text token
          m_pos = m_in.size();
          return make_token(Token::Kind::Text);
        }
        m_pos += open_start;

        // try to match one of the opening sequences, and get the close
        std::string_view open_str = m_in.substr(m_pos);
        if (string_view::starts_with(open_str, m_config.expression_open)) {
          m_state = State::ExpressionStart;
        } else if (string_view::starts_with(open_str, m_config.statement_open)) {
          m_state = State::StatementStart;
        } else if (string_view::starts_with(open_str, m_config.comment_open)) {
          m_state = State::CommentStart;
        } else if ((m_pos == 0 || m_in[m_pos - 1] == '\n') &&
                   string_view::starts_with(open_str, m_config.line_statement)) {
          m_state = State::LineStart;
        } else {
          m_pos += 1; // wasn't actually an opening sequence
          goto again;
        }
        if (m_pos == m_tok_start) goto again;  // don't generate empty token
        return make_token(Token::Kind::Text);
      }
      case State::ExpressionStart: {
        m_state = State::ExpressionBody;
        m_pos += m_config.expression_open.size();
        return make_token(Token::Kind::ExpressionOpen);
      }
      case State::LineStart: {
        m_state = State::LineBody;
        m_pos += m_config.line_statement.size();
        return make_token(Token::Kind::LineStatementOpen);
      }
      case State::StatementStart: {
        m_state = State::StatementBody;
        m_pos += m_config.statement_open.size();
        return make_token(Token::Kind::StatementOpen);
      }
      case State::CommentStart: {
        m_state = State::CommentBody;
        m_pos += m_config.comment_open.size();
        return make_token(Token::Kind::CommentOpen);
      }
      case State::ExpressionBody:
        return scan_body(m_config.expression_close, Token::Kind::ExpressionClose);
      case State::LineBody:
        return scan_body("\n", Token::Kind::LineStatementClose);
      case State::StatementBody:
        return scan_body(m_config.statement_close, Token::Kind::StatementClose);
      case State::CommentBody: {
        // fast-scan to comment close
        size_t end = m_in.substr(m_pos).find(m_config.comment_close);
        if (end == std::string_view::npos) {
          m_pos = m_in.size();
          return make_token(Token::Kind::Eof);
        }
        // return the entire comment in the close token
        m_state = State::Text;
        m_pos += end + m_config.comment_close.size();
        return make_token(Token::Kind::CommentClose);
      }
    }
  }

  const LexerConfig& get_config() const { return m_config; }

 private:
  Token scan_body(std::string_view close, Token::Kind closeKind) {
  again:
    // skip whitespace (except for \n as it might be a close)
    if (m_tok_start >= m_in.size()) return make_token(Token::Kind::Eof);
    char ch = m_in[m_tok_start];
    if (ch == ' ' || ch == '\t' || ch == '\r') {
      m_tok_start += 1;
      goto again;
    }

    // check for close
    if (string_view::starts_with(m_in.substr(m_tok_start), close)) {
      m_state = State::Text;
      m_pos = m_tok_start + close.size();
      return make_token(closeKind);
    }

    // skip \n
    if (ch == '\n') {
      m_tok_start += 1;
      goto again;
    }

    m_pos = m_tok_start + 1;
    if (std::isalpha(ch)) return scan_id();
    switch (ch) {
      case ',':
        return make_token(Token::Kind::Comma);
      case ':':
        return make_token(Token::Kind::Colon);
      case '(':
        return make_token(Token::Kind::LeftParen);
      case ')':
        return make_token(Token::Kind::RightParen);
      case '[':
        return make_token(Token::Kind::LeftBracket);
      case ']':
        return make_token(Token::Kind::RightBracket);
      case '{':
        return make_token(Token::Kind::LeftBrace);
      case '}':
        return make_token(Token::Kind::RightBrace);
      case '>':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          m_pos += 1;
          return make_token(Token::Kind::GreaterEqual);
        }
        return make_token(Token::Kind::GreaterThan);
      case '<':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          m_pos += 1;
          return make_token(Token::Kind::LessEqual);
        }
        return make_token(Token::Kind::LessThan);
      case '=':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          m_pos += 1;
          return make_token(Token::Kind::Equal);
        }
        return make_token(Token::Kind::Unknown);
      case '!':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          m_pos += 1;
          return make_token(Token::Kind::NotEqual);
        }
        return make_token(Token::Kind::Unknown);
      case '\"':
        return scan_string();
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-':
        return scan_number();
      case '_':
        return scan_id();
      default:
        return make_token(Token::Kind::Unknown);
    }
  }

  Token scan_id() {
    for (;;) {
      if (m_pos >= m_in.size()) {
        break;
      }
      char ch = m_in[m_pos];
      if (!std::isalnum(ch) && ch != '.' && ch != '/' && ch != '_' && ch != '-') {
        break;
      }
      m_pos += 1;
    }
    return make_token(Token::Kind::Id);
  }

  Token scan_number() {
    for (;;) {
      if (m_pos >= m_in.size()) {
        break;
      }
      char ch = m_in[m_pos];
      // be very permissive in lexer (we'll catch errors when conversion happens)
      if (!std::isdigit(ch) && ch != '.' && ch != 'e' && ch != 'E' && ch != '+' && ch != '-') {
        break;
      }
      m_pos += 1;
    }
    return make_token(Token::Kind::Number);
  }

  Token scan_string() {
    bool escape {false};
    for (;;) {
      if (m_pos >= m_in.size()) break;
      char ch = m_in[m_pos++];
      if (ch == '\\') {
        escape = true;
      } else if (!escape && ch == m_in[m_tok_start]) {
        break;
      } else {
        escape = false;
      }
    }
    return make_token(Token::Kind::String);
  }

  Token make_token(Token::Kind kind) const {
    return Token(kind, string_view::slice(m_in, m_tok_start, m_pos));
  }
};

}

#endif // PANTOR_INJA_LEXER_HPP

// #include "template.hpp"
#ifndef PANTOR_INJA_TEMPLATE_HPP
#define PANTOR_INJA_TEMPLATE_HPP

#include <string>
#include <vector>

// #include "bytecode.hpp"



namespace inja {

struct Template {
  std::vector<Bytecode> bytecodes;
  std::string content;
};

using TemplateStorage = std::map<std::string, Template>;

}

#endif // PANTOR_INJA_TEMPLATE_HPP

// #include "token.hpp"

// #include "utils.hpp"


#include <nlohmann/json.hpp>


namespace inja {

class ParserStatic {
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
    if (m_tok.kind != Token::Kind::Id || m_tok.text != "or") return true;
    get_next_token();
    if (!parse_expression_and(tmpl)) return false;
    append_function(tmpl, Bytecode::Op::Or, 2);
    return true;
  }

  bool parse_expression_and(Template& tmpl) {
    if (!parse_expression_not(tmpl)) return false;
    if (m_tok.kind != Token::Kind::Id || m_tok.text != "and") return true;
    get_next_token();
    if (!parse_expression_not(tmpl)) return false;
    append_function(tmpl, Bytecode::Op::And, 2);
    return true;
  }

  bool parse_expression_not(Template& tmpl) {
    if (m_tok.kind == Token::Kind::Id && m_tok.text == "not") {
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
        if (m_tok.text == "in")
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
    std::string_view json_first;
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
          } else if (m_tok.text == "true" || m_tok.text == "false" || m_tok.text == "null") {
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
    std::string_view json_text(json_first.data(), m_tok.text.data() - json_first.data() + m_tok.text.size());
    tmpl.bytecodes.emplace_back(Bytecode::Op::Push, json::parse(json_text), Bytecode::Flag::ValueImmediate);
    get_next_token();
    return true;
  }

  bool parse_statement(Template& tmpl, std::string_view path) {
    if (m_tok.kind != Token::Kind::Id) return false;

    if (m_tok.text == "if") {
      get_next_token();

      // evaluate expression
      if (!parse_expression(tmpl)) return false;

      // start a new if block on if stack
      m_if_stack.emplace_back(tmpl.bytecodes.size());

      // conditional jump; destination will be filled in by else or endif
      tmpl.bytecodes.emplace_back(Bytecode::Op::ConditionalJump);
    } else if (m_tok.text == "endif") {
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
    } else if (m_tok.text == "else") {
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
      if (m_tok.kind == Token::Kind::Id && m_tok.text == "if") {
        get_next_token();

        // evaluate expression
        if (!parse_expression(tmpl)) return false;

        // update "previous jump"
        if_data.prev_cond_jump = tmpl.bytecodes.size();

        // conditional jump; destination will be filled in by else or endif
        tmpl.bytecodes.emplace_back(Bytecode::Op::ConditionalJump);
      }
    } else if (m_tok.text == "for") {
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

      if (m_tok.kind != Token::Kind::Id || m_tok.text != "in")
        inja_throw("parser_error",
                   "expected 'in', got '" + m_tok.describe() + "'");
      get_next_token();

      if (!parse_expression(tmpl)) return false;

      m_loop_stack.push_back(tmpl.bytecodes.size());

      tmpl.bytecodes.emplace_back(Bytecode::Op::StartLoop);
      if (!key_token.text.empty()) {
        tmpl.bytecodes.back().value = key_token.text;
      }
      tmpl.bytecodes.back().str = value_token.text;
    } else if (m_tok.text == "endfor") {
      get_next_token();
      if (m_loop_stack.empty()) {
        inja_throw("parser_error", "endfor without matching for");
      }

      // update loop with EndLoop index (for empty case)
      tmpl.bytecodes[m_loop_stack.back()].args = tmpl.bytecodes.size();

      tmpl.bytecodes.emplace_back(Bytecode::Op::EndLoop);
      tmpl.bytecodes.back().args = m_loop_stack.back() + 1;  // loop body
      m_loop_stack.pop_back();
    } else if (m_tok.text == "include") {
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

  void append_callback(Template& tmpl, std::string_view name, unsigned int num_args) {
    // we can merge with back-to-back push value (not lookup)
    if (!tmpl.bytecodes.empty()) {
      Bytecode& last = tmpl.bytecodes.back();
      if (last.op == Bytecode::Op::Push &&
          (last.flags & Bytecode::Flag::ValueMask) == Bytecode::Flag::ValueImmediate) {
        last.op = Bytecode::Op::Callback;
        last.args = num_args;
        last.str = name;
        return;
      }
    }

    // otherwise just add it to the end
    tmpl.bytecodes.emplace_back(Bytecode::Op::Callback, num_args);
    tmpl.bytecodes.back().str = name;
  }

  void parse_into(Template& tmpl, std::string_view path) {
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

  Template parse(std::string_view input, std::string_view path) {
    Template result;
    result.content = input;
    parse_into(result, path);
    return result;
  }

  Template parse(std::string_view input) {
    return parse(input, "./");
  }

  Template parse_template(std::string_view filename) {
    Template result;
    result.content = load_file(filename);

    std::string_view path = filename.substr(0, filename.find_last_of("/\\") + 1);
      // StringRef path = sys::path::parent_path(filename);
    Parser(m_config, m_lexer.get_config(), m_included_templates).parse_into(result, path);
    return result;
  }

  std::string load_file(std::string_view filename) {
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

// #include "polyfill.hpp"
#ifndef PANTOR_INJA_POLYFILL_HPP
#define PANTOR_INJA_POLYFILL_HPP


#if __cplusplus < 201402L

#include <cstddef>
#include <type_traits>
#include <utility>


namespace stdinja {
  template<class T> struct _Unique_if {
    typedef std::unique_ptr<T> _Single_object;
  };

  template<class T> struct _Unique_if<T[]> {
    typedef std::unique_ptr<T[]> _Unknown_bound;
  };

  template<class T, size_t N> struct _Unique_if<T[N]> {
    typedef void _Known_bound;
  };

  template<class T, class... Args>
  typename _Unique_if<T>::_Single_object
  make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }

  template<class T>
  typename _Unique_if<T>::_Unknown_bound
  make_unique(size_t n) {
    typedef typename std::remove_extent<T>::type U;
    return std::unique_ptr<T>(new U[n]());
  }

  template<class T, class... Args>
  typename _Unique_if<T>::_Known_bound
  make_unique(Args&&...) = delete;
}

#else

namespace stdinja = std;

#endif // memory */


#endif // PANTOR_INJA_POLYFILL_HPP

// #include "renderer.hpp"
#ifndef PANTOR_INJA_RENDERER_HPP
#define PANTOR_INJA_RENDERER_HPP

#include <algorithm>
#include <numeric>

#include <nlohmann/json.hpp>

// #include "bytecode.hpp"

// #include "template.hpp"

// #include "utils.hpp"



namespace inja {

inline std::string_view convert_dot_to_json_pointer(std::string_view dot, std::string& out) {
  out.clear();
  do {
    std::string_view part;
    std::tie(part, dot) = string_view::split(dot, '.');
    out.push_back('/');
    out.append(part.begin(), part.end());
  } while (!dot.empty());
  return std::string_view(out.data(), out.size());
}

class Renderer {
  std::vector<const json*>& get_args(const Bytecode& bc) {
    m_tmp_args.clear();

    bool has_imm = ((bc.flags & Bytecode::Flag::ValueMask) != Bytecode::Flag::ValuePop);

    // get args from stack
    unsigned int pop_args = bc.args;
    if (has_imm) {
      pop_args -= 1;
    }

    for (auto i = std::prev(m_stack.end(), pop_args); i != m_stack.end(); i++) {
      m_tmp_args.push_back(&(*i));
    }

    // get immediate arg
    if (has_imm) {
      m_tmp_args.push_back(get_imm(bc));
    }

    return m_tmp_args;
  }

  void pop_args(const Bytecode& bc) {
    unsigned int popArgs = bc.args;
    if ((bc.flags & Bytecode::Flag::ValueMask) != Bytecode::Flag::ValuePop) {
      popArgs -= 1;
    }
    for (unsigned int i = 0; i < popArgs; ++i) {
      m_stack.pop_back();
    }
  }

  const json* get_imm(const Bytecode& bc) {
    std::string ptr_buffer;
    std::string_view ptr;
    switch (bc.flags & Bytecode::Flag::ValueMask) {
      case Bytecode::Flag::ValuePop:
        return nullptr;
      case Bytecode::Flag::ValueImmediate:
        return &bc.value;
      case Bytecode::Flag::ValueLookupDot:
        ptr = convert_dot_to_json_pointer(bc.str, ptr_buffer);
        break;
      case Bytecode::Flag::ValueLookupPointer:
        ptr_buffer += '/';
        ptr_buffer += bc.str;
        ptr = ptr_buffer;
        break;
    }
    try {
      return &m_data->at(json::json_pointer(ptr.data()));
    } catch (std::exception&) {
      // try to evaluate as a no-argument callback
      if (auto callback = m_callbacks.find_callback(bc.str, 0)) {
        std::vector<const json*> arguments {};
        m_tmp_val = callback(arguments);
        return &m_tmp_val;
      }
      inja_throw("render_error", "variable '" + static_cast<std::string>(bc.str) + "' not found");
      return nullptr;
    }
  }

  bool truthy(const json& var) const {
    if (var.empty()) {
      return false;
    } else if (var.is_number()) {
      return (var != 0);
    } else if (var.is_string()) {
      return !var.empty();
    }

    try {
      return var.get<bool>();
    } catch (json::type_error& e) {
      inja_throw("json_error", e.what());
      throw;
    }
  }

  void update_loop_data()  {
    LoopLevel& level = m_loop_stack.back();

    if (m_loop_stack.size() > 1) {
      for (int i = m_loop_stack.size() - 2; i >= 0; i--) {
        auto& level_it = m_loop_stack.at(i);

        level.data[static_cast<std::string>(level_it.value_name)] = level_it.values.at(level_it.index);
      }
    }

    if (level.key_name.empty()) {
      level.data[static_cast<std::string>(level.value_name)] = level.values.at(level.index); // *level.it;
      auto& loopData = level.data["loop"];
      loopData["index"] = level.index;
      loopData["index1"] = level.index + 1;
      loopData["is_first"] = (level.index == 0);
      loopData["is_last"] = (level.index == level.size - 1);
    } else {
      level.data[static_cast<std::string>(level.key_name)] = level.map_it->first;
      level.data[static_cast<std::string>(level.value_name)] = *level.map_it->second;
    }
  }

  const TemplateStorage& m_included_templates;
  const FunctionStorage& m_callbacks;

  std::vector<json> m_stack;

  struct LoopLevel {
    std::string_view key_name;       // variable name for keys
    std::string_view value_name;     // variable name for values
    json data;                      // data with loop info added

    json values;                    // values to iterate over

    // loop over list
    json::iterator it;              // iterator over values
    size_t index;                   // current list index
    size_t size;                    // length of list

    // loop over map
    using KeyValue = std::pair<std::string_view, json*>;
    using MapValues = std::vector<KeyValue>;
    MapValues map_values;            // values to iterate over
    MapValues::iterator map_it;      // iterator over values
  };

  std::vector<LoopLevel> m_loop_stack;
  const json* m_data;

  std::vector<const json*> m_tmp_args;
  json m_tmp_val;


 public:
  Renderer(const TemplateStorage& included_templates, const FunctionStorage& callbacks): m_included_templates(included_templates), m_callbacks(callbacks) {
    m_stack.reserve(16);
    m_tmp_args.reserve(4);
  }

  void render_to(std::ostream& os, const Template& tmpl, const json& data) {
    m_data = &data;

    for (size_t i = 0; i < tmpl.bytecodes.size(); ++i) {
      const auto& bc = tmpl.bytecodes[i];

      switch (bc.op) {
        case Bytecode::Op::Nop: {
          break;
        }
        case Bytecode::Op::PrintText: {
          os << bc.str;
          break;
        }
        case Bytecode::Op::PrintValue: {
          const json& val = *get_args(bc)[0];
          if (val.is_string())
            os << val.get_ref<const std::string&>();
          else
            os << val.dump();
            // val.dump(os);
          pop_args(bc);
          break;
        }
        case Bytecode::Op::Push: {
          m_stack.emplace_back(*get_imm(bc));
          break;
        }
        case Bytecode::Op::Upper: {
          auto result = get_args(bc)[0]->get<std::string>();
          std::transform(result.begin(), result.end(), result.begin(), ::toupper);
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Lower: {
          auto result = get_args(bc)[0]->get<std::string>();
          std::transform(result.begin(), result.end(), result.begin(), ::tolower);
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Range: {
          int number = get_args(bc)[0]->get<int>();
          std::vector<int> result(number);
          std::iota(std::begin(result), std::end(result), 0);
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Length: {
          auto result = get_args(bc)[0]->size();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Sort: {
          auto result = get_args(bc)[0]->get<std::vector<json>>();
          std::sort(result.begin(), result.end());
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::First: {
          auto result = get_args(bc)[0]->front();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Last: {
          auto result = get_args(bc)[0]->back();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Round: {
          auto args = get_args(bc);
          double number = args[0]->get<double>();
          int precision = args[1]->get<int>();
          pop_args(bc);
          m_stack.emplace_back(std::round(number * std::pow(10.0, precision)) / std::pow(10.0, precision));
          break;
        }
        case Bytecode::Op::DivisibleBy: {
          auto args = get_args(bc);
          int number = args[0]->get<int>();
          int divisor = args[1]->get<int>();
          pop_args(bc);
          m_stack.emplace_back((divisor != 0) && (number % divisor == 0));
          break;
        }
        case Bytecode::Op::Odd: {
          int number = get_args(bc)[0]->get<int>();
          pop_args(bc);
          m_stack.emplace_back(number % 2 != 0);
          break;
        }
        case Bytecode::Op::Even: {
          int number = get_args(bc)[0]->get<int>();
          pop_args(bc);
          m_stack.emplace_back(number % 2 == 0);
          break;
        }
        case Bytecode::Op::Max: {
          auto args = get_args(bc);
          auto result = *std::max_element(args[0]->begin(), args[0]->end());
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Min: {
          auto args = get_args(bc);
          auto result = *std::min_element(args[0]->begin(), args[0]->end());
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Not: {
          bool result = !truthy(*get_args(bc)[0]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::And: {
          auto args = get_args(bc);
          bool result = truthy(*args[0]) && truthy(*args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Or: {
          auto args = get_args(bc);
          bool result = truthy(*args[0]) || truthy(*args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::In: {
          auto args = get_args(bc);
          bool result = std::find(args[1]->begin(), args[1]->end(), *args[0]) !=
                        args[1]->end();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Equal: {
          auto args = get_args(bc);
          bool result = (*args[0] == *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Greater: {
          auto args = get_args(bc);
          bool result = (*args[0] > *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Less: {
          auto args = get_args(bc);
          bool result = (*args[0] < *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::GreaterEqual: {
          auto args = get_args(bc);
          bool result = (*args[0] >= *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::LessEqual: {
          auto args = get_args(bc);
          bool result = (*args[0] <= *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Different: {
          auto args = get_args(bc);
          bool result = (*args[0] != *args[1]);
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Float: {
          double result =
              std::stod(get_args(bc)[0]->get_ref<const std::string&>());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Int: {
          int result = std::stoi(get_args(bc)[0]->get_ref<const std::string&>());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Exists: {
          auto&& name = get_args(bc)[0]->get_ref<const std::string&>();
          bool result = (data.find(name) != data.end());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::ExistsInObject: {
          auto args = get_args(bc);
          auto&& name = args[1]->get_ref<const std::string&>();
          bool result = (args[0]->find(name) != args[0]->end());
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsBoolean: {
          bool result = get_args(bc)[0]->is_boolean();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsNumber: {
          bool result = get_args(bc)[0]->is_number();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsInteger: {
          bool result = get_args(bc)[0]->is_number_integer();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsFloat: {
          bool result = get_args(bc)[0]->is_number_float();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsObject: {
          bool result = get_args(bc)[0]->is_object();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsArray: {
          bool result = get_args(bc)[0]->is_array();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::IsString: {
          bool result = get_args(bc)[0]->is_string();
          pop_args(bc);
          m_stack.emplace_back(result);
          break;
        }
        case Bytecode::Op::Default: {
          // default needs to be a bit "magic"; we can't evaluate the first
          // argument during the push operation, so we swap the arguments during
          // the parse phase so the second argument is pushed on the stack and
          // the first argument is in the immediate
          try {
            const json* imm = get_imm(bc);
            // if no exception was raised, replace the stack value with it
            m_stack.back() = *imm;
          } catch (std::exception&) {
            // couldn't read immediate, just leave the stack as is
          }
          break;
        }
        case Bytecode::Op::Include:
          Renderer(m_included_templates, m_callbacks).render_to(os, m_included_templates.find(get_imm(bc)->get_ref<const std::string&>())->second, data);
          break;
        case Bytecode::Op::Callback: {
          auto callback = m_callbacks.find_callback(bc.str, bc.args);
          if (!callback) {
            inja_throw("render_error", "function '" + static_cast<std::string>(bc.str) + "' (" + std::to_string(static_cast<unsigned int>(bc.args)) + ") not found");
          }
          json result = callback(get_args(bc));
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Jump: {
          i = bc.args - 1;  // -1 due to ++i in loop
          break;
        }
        case Bytecode::Op::ConditionalJump: {
          if (!truthy(m_stack.back())) {
            i = bc.args - 1;  // -1 due to ++i in loop
          }
          m_stack.pop_back();
          break;
        }
        case Bytecode::Op::StartLoop: {
          // jump past loop body if empty
          if (m_stack.back().empty()) {
            m_stack.pop_back();
            i = bc.args;  // ++i in loop will take it past EndLoop
            break;
          }

          m_loop_stack.emplace_back();
          LoopLevel& level = m_loop_stack.back();
          level.value_name = bc.str;
          level.values = std::move(m_stack.back());
          level.data = data;
          m_stack.pop_back();

          if (bc.value.is_string()) {
            // map iterator
            if (!level.values.is_object()) {
              m_loop_stack.pop_back();
              inja_throw("render_error", "for key, value requires object");
            }
            level.key_name = bc.value.get_ref<const std::string&>();

            // sort by key
            for (auto it = level.values.begin(), end = level.values.end(); it != end; ++it) {
              level.map_values.emplace_back(it.key(), &it.value());
            }
            std::sort(level.map_values.begin(), level.map_values.end(), [](const LoopLevel::KeyValue& a, const LoopLevel::KeyValue& b) { return a.first < b.first; });
            level.map_it = level.map_values.begin();
          } else {
            if (!level.values.is_array()) {
              m_loop_stack.pop_back();
              inja_throw("render_error", "type must be array");
            }

            // list iterator
            level.it = level.values.begin();
            level.index = 0;
            level.size = level.values.size();
          }

          // provide parent access in nested loop
          auto parent_loop_it = level.data.find("loop");
          if (parent_loop_it != level.data.end()) {
            json loop_copy = *parent_loop_it;
            (*parent_loop_it)["parent"] = std::move(loop_copy);
          }

          // set "current" data to loop data
          m_data = &level.data;
          update_loop_data();
          break;
        }
        case Bytecode::Op::EndLoop: {
          if (m_loop_stack.empty()) {
            inja_throw("render_error", "unexpected state in renderer");
          }
          LoopLevel& level = m_loop_stack.back();

          bool done;
          if (level.key_name.empty()) {
            level.it += 1;
            level.index += 1;
            // done = (level.it == level.values.end());
            done = (level.index == level.values.size());
          } else {
            level.map_it += 1;
            done = (level.map_it == level.map_values.end());
          }

          if (done) {
            m_loop_stack.pop_back();
            // set "current" data to outer loop data or main data as appropriate
            if (!m_loop_stack.empty()) {
              m_data = &m_loop_stack.back().data;
            } else {
              m_data = &data;
            }
            break;
          }

          update_loop_data();

          // jump back to start of loop
          i = bc.args - 1;  // -1 due to ++i in loop
          break;
        }
        default: {
          inja_throw("render_error", "unknown op in renderer: " + std::to_string(static_cast<unsigned int>(bc.op)));
        }
      }
    }
  }
};

}  // namespace inja

#endif  // PANTOR_INJA_RENDERER_HPP

// #include "template.hpp"



namespace inja {

using namespace nlohmann;

class Environment {
  class Impl {
   public:
    std::string input_path;
    std::string output_path;

    LexerConfig lexer_config;
    ParserConfig parser_config;

    FunctionStorage callbacks;
    TemplateStorage included_templates;
  };

  std::unique_ptr<Impl> m_impl;

 public:
  Environment(): Environment("./") { }

  explicit Environment(const std::string& global_path): m_impl(stdinja::make_unique<Impl>()) {
    m_impl->input_path = global_path;
    m_impl->output_path = global_path;
  }

  explicit Environment(const std::string& input_path, const std::string& output_path): m_impl(stdinja::make_unique<Impl>()) {
    m_impl->input_path = input_path;
    m_impl->output_path = output_path;
  }

  /// Sets the opener and closer for template statements
  void set_statement(const std::string& open, const std::string& close) {
    m_impl->lexer_config.statement_open = open;
    m_impl->lexer_config.statement_close = close;
    m_impl->lexer_config.update_open_chars();
  }

  /// Sets the opener for template line statements
  void set_line_statement(const std::string& open) {
    m_impl->lexer_config.line_statement = open;
    m_impl->lexer_config.update_open_chars();
  }

  /// Sets the opener and closer for template expressions
  void set_expression(const std::string& open, const std::string& close) {
    m_impl->lexer_config.expression_open = open;
    m_impl->lexer_config.expression_close = close;
    m_impl->lexer_config.update_open_chars();
  }

  /// Sets the opener and closer for template comments
  void set_comment(const std::string& open, const std::string& close) {
    m_impl->lexer_config.comment_open = open;
    m_impl->lexer_config.comment_close = close;
    m_impl->lexer_config.update_open_chars();
  }

  /// Sets the element notation syntax
  void set_element_notation(ElementNotation notation) {
    m_impl->parser_config.notation = notation;
  }


  Template parse(std::string_view input) {
    Parser parser(m_impl->parser_config, m_impl->lexer_config, m_impl->included_templates);
    return parser.parse(input);
  }

  Template parse_template(const std::string& filename) {
    Parser parser(m_impl->parser_config, m_impl->lexer_config, m_impl->included_templates);
		return parser.parse_template(m_impl->input_path + static_cast<std::string>(filename));
	}

  std::string render(std::string_view input, const json& data) {
    return render(parse(input), data);
  }

  std::string render(const Template& tmpl, const json& data) {
    std::stringstream os;
    render_to(os, tmpl, data);
    return os.str();
  }

  std::string render_file(const std::string& filename, const json& data) {
		return render(parse_template(filename), data);
	}

  std::string render_file_with_json_file(const std::string& filename, const std::string& filename_data) {
		const json data = load_json(filename_data);
		return render_file(filename, data);
	}

  void write(const std::string& filename, const json& data, const std::string& filename_out) {
		std::ofstream file(m_impl->output_path + filename_out);
		file << render_file(filename, data);
		file.close();
	}

  void write(const Template& temp, const json& data, const std::string& filename_out) {
		std::ofstream file(m_impl->output_path + filename_out);
		file << render(temp, data);
		file.close();
	}

	void write_with_json_file(const std::string& filename, const std::string& filename_data, const std::string& filename_out) {
		const json data = load_json(filename_data);
		write(filename, data, filename_out);
	}

	void write_with_json_file(const Template& temp, const std::string& filename_data, const std::string& filename_out) {
		const json data = load_json(filename_data);
		write(temp, data, filename_out);
	}

  std::ostream& render_to(std::ostream& os, const Template& tmpl, const json& data) {
    Renderer(m_impl->included_templates, m_impl->callbacks).render_to(os, tmpl, data);
    return os;
  }

  std::string load_file(const std::string& filename) {
    Parser parser(m_impl->parser_config, m_impl->lexer_config, m_impl->included_templates);
		return parser.load_file(m_impl->input_path + filename);
	}

  json load_json(const std::string& filename) {
		std::ifstream file(m_impl->input_path + filename);
		json j;
		file >> j;
		return j;
	}

  void add_callback(const std::string& name, unsigned int numArgs, const CallbackFunction& callback) {
    m_impl->callbacks.add_callback(name, numArgs, callback);
  }

  /** Includes a template with a given name into the environment.
   * Then, a template can be rendered in another template using the
   * include "<name>" syntax.
   */
  void include_template(const std::string& name, const Template& tmpl) {
    m_impl->included_templates[name] = tmpl;
  }
};

/*!
@brief render with default settings to a string
*/
inline std::string render(std::string_view input, const json& data) {
  return Environment().render(input, data);
}

/*!
@brief render with default settings to the given output stream
*/
inline void render_to(std::ostream& os, std::string_view input, const json& data) {
  Environment env;
  env.render_to(os, env.parse(input), data);
}

}

#endif // PANTOR_INJA_ENVIRONMENT_HPP

// #include "template.hpp"

// #include "parser.hpp"

// #include "renderer.hpp"



#endif  // PANTOR_INJA_HPP
