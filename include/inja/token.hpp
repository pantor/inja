#ifndef PANTOR_INJA_TOKEN_HPP
#define PANTOR_INJA_TOKEN_HPP

#include <string_view>


namespace inja {

struct Token {
  enum class Kind {
    Text,
    ExpressionOpen,      // {{
    ExpressionClose,     // }}
    LinestatementOpen,   // ##
    LineStatementClose,  // \n
    statementOpen,       // {%
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

  std::string_view describe() const {
    switch (kind) {
      case Kind::Text:
        return "<text>";
      case Kind::ExpressionOpen:
        return "{{";
      case Kind::ExpressionClose:
        return "}}";
      case Kind::LinestatementOpen:
        return "##";
      case Kind::LineStatementClose:
        return "<eol>";
      case Kind::statementOpen:
        return "{%";
      case Kind::StatementClose:
        return "%}";
      case Kind::CommentOpen:
        return "{#";
      case Kind::CommentClose:
        return "#}";
      case Kind::Eof:
        return "<eof>";
      default:
        return text;
    }
  }
};

}

#endif // PANTOR_INJA_TOKEN_HPP
