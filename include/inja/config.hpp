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
  std::string pre_open{ "#{" };
  std::string pre_close{ "}#" };
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
	if (open_chars.find(pre_open[0]) == std::string::npos) {
		open_chars += pre_open[0];
	}
  }
};

struct ParserConfig {
  ElementNotation notation {ElementNotation::Dot};
};

}

#endif // PANTOR_INJA_CONFIG_HPP
