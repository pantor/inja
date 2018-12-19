/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#ifndef WPIUTIL_INJA_PARSER_H_
#define WPIUTIL_INJA_PARSER_H_

#include <limits>

#include <inja/function_storage.hpp>
#include <inja/internal.hpp>
#include <inja/lexer.hpp>
#include <inja/template.hpp>
#include <inja/token.hpp>

#include <nlohmann/json.hpp>


namespace inja {

class ParserStatic {
 public:
  ParserStatic(const ParserStatic&) = delete;
  ParserStatic& operator=(const ParserStatic&) = delete;

  static const ParserStatic& GetInstance();

  FunctionStorage functions;

 private:
  ParserStatic();
};

struct ParserConfig {
  ElementNotation notation = ElementNotation::Pointer;
  std::function<std::string(std::string_view filename)> loadFile;
};

class Parser {
 public:
  Parser(const ParserConfig& parserConfig, const LexerConfig& lexerConfig,
         TemplateStorage& includedTemplates);

  bool ParseExpression(Template& tmpl);
  bool ParseExpressionAnd(Template& tmpl);
  bool ParseExpressionNot(Template& tmpl);
  bool ParseExpressionComparison(Template& tmpl);
  bool ParseExpressionDatum(Template& tmpl);
  bool ParseStatement(Template& tmpl, std::string_view path);
  void AppendFunction(Template& tmpl, Bytecode::Op op, unsigned int numArgs);
  void AppendCallback(Template& tmpl, std::string_view name, unsigned int numArgs);
  void ParseInto(Template& tmpl, std::string_view path);

  Template parse(std::string_view input, std::string_view path = "./");
  Template parse_template(std::string_view filename);

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

  void GetNextToken() {
    if (m_havePeekTok) {
      m_tok = m_peekTok;
      m_havePeekTok = false;
    } else {
      m_tok = m_lexer.Scan();
    }
  }

  void GetPeekToken() {
    if (!m_havePeekTok) {
      m_peekTok = m_lexer.Scan();
      m_havePeekTok = true;
    }
  }
};

}  // namespace inja

#endif  // WPIUTIL_INJA_PARSER_H_
