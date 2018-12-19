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
