// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_TOKEN_HPP_
#define INCLUDE_INJA_TOKEN_HPP_

#include <string>

#include "string_view.hpp"


namespace inja {

/*!
 * \brief Helper-class for the inja Parser.
 */
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

  nonstd::string_view text;

  constexpr Token() = default;
  constexpr Token(Kind kind, nonstd::string_view text): kind(kind), text(text) {}

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

#endif  // INCLUDE_INJA_TOKEN_HPP_
