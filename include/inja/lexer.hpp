#ifndef PANTOR_INJA_LEXER_HPP
#define PANTOR_INJA_LEXER_HPP

#include <inja/internal.hpp>
#include <inja/token.hpp>


namespace inja {

struct LexerConfig {
  std::string statementOpen{"{%"};
  std::string statementClose{"%}"};
  std::string lineStatement{"##"};
  std::string expressionOpen{"{{"};
  std::string expressionClose{"}}"};
  std::string commentOpen{"{#"};
  std::string commentClose{"#}"};
  std::string openChars{"#{"};

  void UpdateOpenChars() {
    openChars = "\n";
    if (openChars.find(statementOpen[0]) == std::string::npos)
      openChars += statementOpen[0];
    if (openChars.find(expressionOpen[0]) == std::string::npos)
      openChars += expressionOpen[0];
    if (openChars.find(commentOpen[0]) == std::string::npos)
      openChars += commentOpen[0];
  }
};

class Lexer {
 public:
  explicit Lexer(const LexerConfig& config) : m_config(config) {}

  void Start(std::string_view in) {
    m_in = in;
    m_tokStart = 0;
    m_pos = 0;
    m_state = State::Text;
  }

  Token Scan();

  const LexerConfig& GetConfig() const { return m_config; }

 private:
  Token ScanBody(std::string_view close, Token::Kind closeKind);
  Token ScanId();
  Token ScanNumber();
  Token ScanString();
  Token MakeToken(Token::Kind kind) const {
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