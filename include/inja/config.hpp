// Copyright (c) 2019 Pantor. All rights reserved.

#ifndef INCLUDE_INJA_CONFIG_HPP_
#define INCLUDE_INJA_CONFIG_HPP_

#include <functional>
#include <string>

#include "string_view.hpp"


namespace inja {

enum class ElementNotation {
  Dot,
  Pointer
};

/*!
 * \brief Class for lexer configuration.
 */
struct LexerConfig {
  std::string statement_open {"{%"};
  std::string statement_close {"%}"};
  std::string line_statement {"##"};
  std::string expression_open {"{{"};
  std::string expression_close {"}}"};
  std::string comment_open {"{#"};
  std::string comment_close {"#}"};
  std::string open_chars {"#{"};

  bool trim_blocks {false};
  bool lstrip_blocks {false};

  void update_open_chars() {
    open_chars = "";
    if (open_chars.find(line_statement[0]) == std::string::npos) {
      open_chars += line_statement[0];
    }
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

/*!
 * \brief Class for parser configuration.
 */
struct ParserConfig {
  ElementNotation notation {ElementNotation::Dot};
};

}  // namespace inja

#endif  // INCLUDE_INJA_CONFIG_HPP_
