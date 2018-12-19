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
