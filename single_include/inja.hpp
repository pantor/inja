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

#include <sstream>

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
  std::string statementOpen{"{%"};
  std::string statementClose{"%}"};
  std::string lineStatement{"##"};
  std::string expressionOpen{"{{"};
  std::string expressionClose{"}}"};
  std::string commentOpen{"{#"};
  std::string commentClose{"#}"};
  std::string openChars{"#{"};

  void update_open_chars() {
    openChars = "\n";
    if (openChars.find(statementOpen[0]) == std::string::npos)
      openChars += statementOpen[0];
    if (openChars.find(expressionOpen[0]) == std::string::npos)
      openChars += expressionOpen[0];
    if (openChars.find(commentOpen[0]) == std::string::npos)
      openChars += commentOpen[0];
  }
};

struct ParserConfig {
  ElementNotation notation = ElementNotation::Pointer;
  std::function<std::string(std::string_view filename)> loadFile;
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

#include <utility>

#include <nlohmann/json.hpp>


namespace inja {

using namespace nlohmann;


class Bytecode {
 public:
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
    EndLoop
  };

  enum Flag {
    // location of value for value-taking ops (mask)
    kFlagValueMask = 0x03,
    // pop value off stack
    kFlagValuePop = 0x00,
    // value is immediate rather than on stack
    kFlagValueImmediate = 0x01,
    // lookup immediate str (dot notation)
    kFlagValueLookupDot = 0x02,
    // lookup immediate str (json pointer notation)
    kFlagValueLookupPointer = 0x03
  };

  Op op = Op::Nop;
  uint32_t args : 30;
  uint32_t flags : 2;

  json value;
  std::string_view str;

  Bytecode() : args(0), flags(0) {}
  explicit Bytecode(Op op, unsigned int args = 0): op(op), args(args), flags(0) {}
  Bytecode(Op op, std::string_view str, unsigned int flags): op(op), args(0), flags(flags), str(str) {}
  Bytecode(Op op, json&& value, unsigned int flags): op(op), args(0), flags(flags), value(std::move(value)) {}
};

}  // namespace inja

#endif  // PANTOR_INJA_BYTECODE_HPP



namespace inja {

using namespace nlohmann;

using CallbackFunction = std::function<json(std::vector<const json*>& args, const json& data)>;

class FunctionStorage {
 public:
  void add_builtin(std::string_view name, unsigned int numArgs, Bytecode::Op op) {
    auto& data = get_or_new(name, numArgs);
    data.op = op;
  }

  void add_callback(std::string_view name, unsigned int numArgs, const CallbackFunction& function) {
    auto& data = get_or_new(name, numArgs);
    data.function = function;
  }

  Bytecode::Op find_builtin(std::string_view name, unsigned int numArgs) const {
    if (auto ptr = get(name, numArgs))
      return ptr->op;
    else
      return Bytecode::Op::Nop;
  }

  CallbackFunction find_callback(std::string_view name, unsigned int numArgs) const {
    if (auto ptr = get(name, numArgs))
      return ptr->function;
    else
      return nullptr;
  }

 private:
  struct FunctionData {
    unsigned int numArgs = 0;
    Bytecode::Op op = Bytecode::Op::Nop; // for builtins
    CallbackFunction function; // for callbacks
  };

  FunctionData& get_or_new(std::string_view name, unsigned int numArgs) {
    auto &vec = m_map[name.data()];
    for (auto &i : vec) {
      if (i.numArgs == numArgs) return i;
    }
    vec.emplace_back();
    vec.back().numArgs = numArgs;
    return vec.back();
  }

  const FunctionData* get(std::string_view name, unsigned int numArgs) const {
    auto it = m_map.find(static_cast<std::string>(name));
    if (it == m_map.end()) return nullptr;
    for (auto &&i : it->second) {
      if (i.numArgs == numArgs) return &i;
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

// #include "config.hpp"

// #include "token.hpp"
#ifndef PANTOR_INJA_TOKEN_HPP
#define PANTOR_INJA_TOKEN_HPP

#include <string_view>


namespace inja {

struct Token {
  enum Kind {
    kText,
    kExpressionOpen,      // {{
    kExpressionClose,     // }}
    kLineStatementOpen,   // ##
    kLineStatementClose,  // \n
    kStatementOpen,       // {%
    kStatementClose,      // %}
    kCommentOpen,         // {#
    kCommentClose,        // #}
    kId,                  // this, this.foo
    kNumber,              // 1, 2, -1, 5.2, -5.3
    kString,              // "this"
    kComma,               // ,
    kColon,               // :
    kLeftParen,           // (
    kRightParen,          // )
    kLeftBracket,         // [
    kRightBracket,        // ]
    kLeftBrace,           // {
    kRightBrace,          // }
    kEqual,               // ==
    kGreaterThan,         // >
    kLessThan,            // <
    kLessEqual,           // <=
    kGreaterEqual,        // >=
    kNotEqual,            // !=
    kUnknown,
    kEof
  };
  Kind kind = kUnknown;
  std::string_view text;

  Token() = default;

  constexpr Token(Kind kind, std::string_view text) : kind(kind), text(text) {}

  std::string_view describe() const {
    switch (kind) {
      case kText:
        return "<text>";
      case kExpressionOpen:
        return "{{";
      case kExpressionClose:
        return "}}";
      case kLineStatementOpen:
        return "##";
      case kLineStatementClose:
        return "<eol>";
      case kStatementOpen:
        return "{%";
      case kStatementClose:
        return "%}";
      case kCommentOpen:
        return "{#";
      case kCommentClose:
        return "#}";
      case kEof:
        return "<eof>";
      default:
        return text;
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

inline std::string_view string_view_slice(std::string_view view, size_t start, size_t end) {
  start = std::min(start, view.size());
  end = std::min(std::max(start, end), view.size());
  return view.substr(start, end - start); // StringRef(Data + Start, End - Start);
}

inline std::pair<std::string_view, std::string_view> string_view_split(std::string_view view, char Separator) {
  size_t idx = view.find(Separator);
  if (idx == std::string_view::npos) {
    return std::make_pair(view, std::string_view());
  }
  return std::make_pair(string_view_slice(view, 0, idx), string_view_slice(view, idx + 1, std::string_view::npos));
}

inline bool string_view_starts_with(std::string_view view, std::string_view prefix) {
  return (view.size() >= prefix.size() && view.compare(0, prefix.size(), prefix) == 0);
}

}  // namespace inja

#endif  // PANTOR_INJA_UTILS_HPP



namespace inja {

class Lexer {
 public:
  explicit Lexer(const LexerConfig& config) : m_config(config) {}

  void start(std::string_view in) {
    m_in = in;
    m_tokStart = 0;
    m_pos = 0;
    m_state = State::Text;
  }

  Token scan() {
    m_tokStart = m_pos;

  again:
    if (m_tokStart >= m_in.size()) return make_token(Token::kEof);

    switch (m_state) {
      default:
      case State::Text: {
        // fast-scan to first open character
        size_t openStart = m_in.substr(m_pos).find_first_of(m_config.openChars);
        if (openStart == std::string_view::npos) {
          // didn't find open, return remaining text as text token
          m_pos = m_in.size();
          return make_token(Token::kText);
        }
        m_pos += openStart;

        // try to match one of the opening sequences, and get the close
        std::string_view openStr = m_in.substr(m_pos);
        if (string_view_starts_with(openStr, m_config.expressionOpen)) {
          m_state = State::ExpressionStart;
        } else if (string_view_starts_with(openStr, m_config.statementOpen)) {
          m_state = State::StatementStart;
        } else if (string_view_starts_with(openStr, m_config.commentOpen)) {
          m_state = State::CommentStart;
        } else if ((m_pos == 0 || m_in[m_pos - 1] == '\n') &&
                   string_view_starts_with(openStr, m_config.lineStatement)) {
          m_state = State::LineStart;
        } else {
          ++m_pos; // wasn't actually an opening sequence
          goto again;
        }
        if (m_pos == m_tokStart) goto again;  // don't generate empty token
        return make_token(Token::kText);
      }
      case State::ExpressionStart: {
        m_state = State::ExpressionBody;
        m_pos += m_config.expressionOpen.size();
        return make_token(Token::kExpressionOpen);
      }
      case State::LineStart: {
        m_state = State::LineBody;
        m_pos += m_config.lineStatement.size();
        return make_token(Token::kLineStatementOpen);
      }
      case State::StatementStart: {
        m_state = State::StatementBody;
        m_pos += m_config.statementOpen.size();
        return make_token(Token::kStatementOpen);
      }
      case State::CommentStart: {
        m_state = State::CommentBody;
        m_pos += m_config.commentOpen.size();
        return make_token(Token::kCommentOpen);
      }
      case State::ExpressionBody:
        return scan_body(m_config.expressionClose, Token::kExpressionClose);
      case State::LineBody:
        return scan_body("\n", Token::kLineStatementClose);
      case State::StatementBody:
        return scan_body(m_config.statementClose, Token::kStatementClose);
      case State::CommentBody: {
        // fast-scan to comment close
        size_t end = m_in.substr(m_pos).find(m_config.commentClose);
        if (end == std::string_view::npos) {
          m_pos = m_in.size();
          return make_token(Token::kEof);
        }
        // return the entire comment in the close token
        m_state = State::Text;
        m_pos += end + m_config.commentClose.size();
        return make_token(Token::kCommentClose);
      }
    }
  }

  const LexerConfig& get_config() const { return m_config; }

 private:
  Token scan_body(std::string_view close, Token::Kind closeKind) {
  again:
    // skip whitespace (except for \n as it might be a close)
    if (m_tokStart >= m_in.size()) return make_token(Token::kEof);
    char ch = m_in[m_tokStart];
    if (ch == ' ' || ch == '\t' || ch == '\r') {
      ++m_tokStart;
      goto again;
    }

    // check for close
    if (string_view_starts_with(m_in.substr(m_tokStart), close)) {
      m_state = State::Text;
      m_pos = m_tokStart + close.size();
      return make_token(closeKind);
    }

    // skip \n
    if (ch == '\n') {
      ++m_tokStart;
      goto again;
    }

    m_pos = m_tokStart + 1;
    if (std::isalpha(ch)) return scan_id();
    switch (ch) {
      case ',':
        return make_token(Token::kComma);
      case ':':
        return make_token(Token::kColon);
      case '(':
        return make_token(Token::kLeftParen);
      case ')':
        return make_token(Token::kRightParen);
      case '[':
        return make_token(Token::kLeftBracket);
      case ']':
        return make_token(Token::kRightBracket);
      case '{':
        return make_token(Token::kLeftBrace);
      case '}':
        return make_token(Token::kRightBrace);
      case '>':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          ++m_pos;
          return make_token(Token::kGreaterEqual);
        }
        return make_token(Token::kGreaterThan);
      case '<':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          ++m_pos;
          return make_token(Token::kLessEqual);
        }
        return make_token(Token::kLessThan);
      case '=':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          ++m_pos;
          return make_token(Token::kEqual);
        }
        return make_token(Token::kUnknown);
      case '!':
        if (m_pos < m_in.size() && m_in[m_pos] == '=') {
          ++m_pos;
          return make_token(Token::kNotEqual);
        }
        return make_token(Token::kUnknown);
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
        return make_token(Token::kUnknown);
    }
  }

  Token scan_id() {
    for (;;) {
      if (m_pos >= m_in.size()) break;
      char ch = m_in[m_pos];
      if (!isalnum(ch) && ch != '.' && ch != '/' && ch != '_' && ch != '-') break;
      ++m_pos;
    }
    return make_token(Token::kId);
  }

  Token scan_number() {
    for (;;) {
      if (m_pos >= m_in.size()) break;
      char ch = m_in[m_pos];
      // be very permissive in lexer (we'll catch errors when conversion happens)
      if (!isdigit(ch) && ch != '.' && ch != 'e' && ch != 'E' && ch != '+' && ch != '-')
        break;
      ++m_pos;
    }
    return make_token(Token::kNumber);
  }

  Token scan_string() {
    bool escape = false;
    for (;;) {
      if (m_pos >= m_in.size()) break;
      char ch = m_in[m_pos++];
      if (ch == '\\')
        escape = true;
      else if (!escape && ch == m_in[m_tokStart])
        break;
      else
        escape = false;
    }
    return make_token(Token::kString);
  }

  Token make_token(Token::Kind kind) const {
    return Token(kind, string_view_slice(m_in, m_tokStart, m_pos));
  }

  const LexerConfig& m_config;
  std::string_view m_in;
  size_t m_tokStart;
  size_t m_pos;
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
  };
  State m_state;
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

class Template {
  friend class Parser;
  friend class Renderer;

 public:
  Template() {}

  ~Template() {}

  Template(const Template& oth): bytecodes(oth.bytecodes), contents(oth.contents) {}

  Template(Template&& oth): bytecodes(std::move(oth.bytecodes)), contents(std::move(oth.contents)) {}

  Template& operator=(const Template& oth) {
    bytecodes = oth.bytecodes;
    contents = oth.contents;
    return *this;
  }

  Template& operator=(Template&& oth) {
    bytecodes = std::move(oth.bytecodes);
    contents = std::move(oth.contents);
    return *this;
  }

 private:
  std::vector<Bytecode> bytecodes;
  std::string contents;
};

using TemplateStorage = std::map<std::string, Template>;

}

#endif // PANTOR_INJA_TEMPLATE_HPP

// #include "token.hpp"

// #include "utils.hpp"


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

inline bool truthy(const json& var) {
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

inline std::string_view convert_dot_to_json_pointer(std::string_view dot, std::string& out) {
  out.clear();
  do {
    std::string_view part;
    std::tie(part, dot) = string_view_split(dot, '.');
    out.push_back('/');
    out.append(part.begin(), part.end());
  } while (!dot.empty());
  return std::string_view(out.data(), out.size());
}

class Renderer {
 public:
  Renderer(const TemplateStorage& includedTemplates, const FunctionStorage& callbacks): m_includedTemplates(includedTemplates), m_callbacks(callbacks) {
    m_stack.reserve(16);
    m_tmpArgs.reserve(4);
  }

  void render_to(std::stringstream& os, const Template& tmpl, const json& data) {
    m_data = &data;

    for (size_t i = 0; i < tmpl.bytecodes.size(); ++i) {
      const auto& bc = tmpl.bytecodes[i];

      switch (bc.op) {
        case Bytecode::Op::Nop:
          break;
        case Bytecode::Op::PrintText:
          os << bc.str;
          break;
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
        case Bytecode::Op::Push:
          m_stack.emplace_back(*get_imm(bc));
          break;
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
          m_stack.emplace_back(std::round(number * std::pow(10.0, precision)) /
                               std::pow(10.0, precision));
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
          Renderer(m_includedTemplates, m_callbacks).render_to(os, m_includedTemplates.find(get_imm(bc)->get_ref<const std::string&>())->second, data);
          break;
        case Bytecode::Op::Callback: {
          auto cb = m_callbacks.find_callback(bc.str, bc.args);
          if (!cb)
            inja_throw("render_error", "function '" + static_cast<std::string>(bc.str) + "' (" + std::to_string(static_cast<unsigned int>(bc.args)) + ") not found");
          json result = cb(get_args(bc), data);
          pop_args(bc);
          m_stack.emplace_back(std::move(result));
          break;
        }
        case Bytecode::Op::Jump:
          i = bc.args - 1;  // -1 due to ++i in loop
          break;
        case Bytecode::Op::ConditionalJump: {
          if (!truthy(m_stack.back()))
            i = bc.args - 1;  // -1 due to ++i in loop
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

          m_loopStack.emplace_back();
          LoopLevel& level = m_loopStack.back();
          level.valueName = bc.str;
          level.values = std::move(m_stack.back());
          level.data = data;
          m_stack.pop_back();

          if (bc.value.is_string()) {
            // map iterator
            if (!level.values.is_object()) {
              m_loopStack.pop_back();
              inja_throw("render_error", "for key, value requires object");
            }
            level.keyName = bc.value.get_ref<const std::string&>();

            // sort by key
            for (auto it = level.values.begin(), end = level.values.end();
                 it != end; ++it)
              level.mapValues.emplace_back(it.key(), &it.value());
            std::sort(
                level.mapValues.begin(), level.mapValues.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
            level.mapIt = level.mapValues.begin();
          } else {
            // list iterator
            level.it = level.values.begin();
            level.index = 0;
            level.size = level.values.size();
          }

          // provide parent access in nested loop
          auto parentLoopIt = level.data.find("loop");
          if (parentLoopIt != level.data.end()) {
            json loopCopy = *parentLoopIt;
            (*parentLoopIt)["parent"] = std::move(loopCopy);
          }

          // set "current" data to loop data
          m_data = &level.data;
          update_loop_data();
          break;
        }
        case Bytecode::Op::EndLoop: {
          if (m_loopStack.empty())
            inja_throw("render_error", "unexpected state in renderer");
          LoopLevel& level = m_loopStack.back();

          bool done;
          if (level.keyName.empty()) {
            ++level.it;
            ++level.index;
            done = (level.it == level.values.end());
          } else {
            ++level.mapIt;
            done = (level.mapIt == level.mapValues.end());
          }

          if (done) {
            m_loopStack.pop_back();
            // set "current" data to outer loop data or main data as appropriate
            if (!m_loopStack.empty())
              m_data = &m_loopStack.back().data;
            else
              m_data = &data;
            break;
          }

          update_loop_data();

          // jump back to start of loop
          i = bc.args - 1;  // -1 due to ++i in loop
          break;
        }
        default:
          inja_throw("render_error", "unknown op in renderer: " + std::to_string(static_cast<unsigned int>(bc.op)));
      }
    }
  }

 private:
  std::vector<const json*>& get_args(const Bytecode& bc) {
    m_tmpArgs.clear();

    bool hasImm = ((bc.flags & Bytecode::kFlagValueMask) != Bytecode::kFlagValuePop);

    // get args from stack
    unsigned int popArgs = bc.args;
    if (hasImm) --popArgs;


    for (auto i = std::prev(m_stack.end(), popArgs); i != m_stack.end(); i++) {
      m_tmpArgs.push_back(&(*i));
    }

    // get immediate arg
    if (hasImm) {
      m_tmpArgs.push_back(get_imm(bc));
    }

    return m_tmpArgs;
  }

  void pop_args(const Bytecode& bc) {
    unsigned int popArgs = bc.args;
    if ((bc.flags & Bytecode::kFlagValueMask) != Bytecode::kFlagValuePop)
      --popArgs;
    for (unsigned int i = 0; i < popArgs; ++i) m_stack.pop_back();
  }

  const json* get_imm(const Bytecode& bc) {
    std::string ptrBuf;
    std::string_view ptr;
    switch (bc.flags & Bytecode::kFlagValueMask) {
      case Bytecode::kFlagValuePop:
        return nullptr;
      case Bytecode::kFlagValueImmediate:
        return &bc.value;
      case Bytecode::kFlagValueLookupDot:
        ptr = convert_dot_to_json_pointer(bc.str, ptrBuf);
        break;
      case Bytecode::kFlagValueLookupPointer:
        ptrBuf += '/';
        ptrBuf += bc.str;
        ptr = ptrBuf;
        break;
    }
    try {
      return &m_data->at(json::json_pointer(ptr.data()));
    } catch (std::exception&) {
      // try to evaluate as a no-argument callback
      if (auto cb = m_callbacks.find_callback(bc.str, 0)) {
        std::vector<const json*> asdf{};
        m_tmpVal = cb(asdf, *m_data);
        return &m_tmpVal;
      }
      inja_throw("render_error", "variable '" + static_cast<std::string>(bc.str) + "' not found");
      return nullptr;
    }
  }

  void update_loop_data()  {
    LoopLevel& level = m_loopStack.back();
    if (level.keyName.empty()) {
      level.data[level.valueName.data()] = *level.it;
      auto& loopData = level.data["loop"];
      loopData["index"] = level.index;
      loopData["index1"] = level.index + 1;
      loopData["is_first"] = (level.index == 0);
      loopData["is_last"] = (level.index == level.size - 1);
    } else {
      level.data[level.keyName.data()] = level.mapIt->first;
      level.data[level.valueName.data()] = *level.mapIt->second;
    }
  }

  const TemplateStorage& m_includedTemplates;
  const FunctionStorage& m_callbacks;

  std::vector<json> m_stack;

  struct LoopLevel {
    std::string_view keyName;       // variable name for keys
    std::string_view valueName;     // variable name for values
    json data;                      // data with loop info added

    json values;                    // values to iterate over

    // loop over list
    json::iterator it;              // iterator over values
    size_t index;                   // current list index
    size_t size;                    // length of list

    // loop over map
    using KeyValue = std::pair<std::string_view, json*>;
    using MapValues = std::vector<KeyValue>;
    MapValues mapValues;            // values to iterate over
    MapValues::iterator mapIt;      // iterator over values
  };

  std::vector<LoopLevel> m_loopStack;
  const json* m_data;

  std::vector<const json*> m_tmpArgs;
  json m_tmpVal;
};

}  // namespace inja

#endif  // PANTOR_INJA_RENDERER_HPP

// #include "template.hpp"



namespace inja {

using namespace nlohmann;

class Environment {
  class Impl {
   public:
    std::string path;

    LexerConfig lexerConfig;
    ParserConfig parserConfig;

    FunctionStorage callbacks;

    TemplateStorage includedTemplates;
  };
  std::unique_ptr<Impl> m_impl;

 public:
  Environment(): Environment("./") { }

  explicit Environment(const std::string& path): m_impl(std::make_unique<Impl>()) {
    m_impl->path = path;
  }

  ~Environment() { }

  void set_statement(const std::string& open, const std::string& close) {
    m_impl->lexerConfig.statementOpen = open;
    m_impl->lexerConfig.statementClose = close;
    m_impl->lexerConfig.update_open_chars();
  }

  void set_line_statement(const std::string& open) {
    m_impl->lexerConfig.lineStatement = open;
    m_impl->lexerConfig.update_open_chars();
  }

  void set_expression(const std::string& open, const std::string& close) {
    m_impl->lexerConfig.expressionOpen = open;
    m_impl->lexerConfig.expressionClose = close;
    m_impl->lexerConfig.update_open_chars();
  }

  void set_comment(const std::string& open, const std::string& close) {
    m_impl->lexerConfig.commentOpen = open;
    m_impl->lexerConfig.commentClose = close;
    m_impl->lexerConfig.update_open_chars();
  }

  void set_element_notation(ElementNotation notation) {
    m_impl->parserConfig.notation = notation;
  }

  void set_load_file(std::function<std::string(std::string_view)> loadFile) {
    m_impl->parserConfig.loadFile = loadFile;
  }

  Template parse(std::string_view input) {
    Parser parser(m_impl->parserConfig, m_impl->lexerConfig, m_impl->includedTemplates);
    return parser.parse(input);
  }

  std::string render(std::string_view input, const json& data) {
    return render(parse(input), data);
  }

  std::string render(const Template& tmpl, const json& data) {
    std::stringstream os;
    render_to(os, tmpl, data);
    return os.str();
  }

  std::stringstream& render_to(std::stringstream& os, const Template& tmpl, const json& data) {
    Renderer(m_impl->includedTemplates, m_impl->callbacks).render_to(os, tmpl, data);
    return os;
  }

  void add_callback(std::string_view name, unsigned int numArgs, const CallbackFunction& callback) {
    m_impl->callbacks.add_callback(name, numArgs, callback);
  }

  void include_template(const std::string& name, const Template& tmpl) {
    m_impl->includedTemplates[name] = tmpl;
  }
};

/*!
@brief render with default settings
*/
inline std::string render(const std::string& input, const json& data) {
  return Environment().render(input, data);
}

}

#endif // PANTOR_INJA_ENVIRONMENT_HPP

// #include "template.hpp"

// #include "parser.hpp"

// #include "renderer.hpp"



#endif  // PANTOR_INJA_HPP
