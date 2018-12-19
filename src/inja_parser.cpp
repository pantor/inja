/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/

#include <inja/inja_parser.hpp>


using namespace wpi;
using namespace inja;

StringRef Token::Describe() const {
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

void LexerConfig::UpdateOpenChars() {
  openChars = "\n";
  if (openChars.find(statementOpen[0]) == std::string::npos)
    openChars += statementOpen[0];
  if (openChars.find(expressionOpen[0]) == std::string::npos)
    openChars += expressionOpen[0];
  if (openChars.find(commentOpen[0]) == std::string::npos)
    openChars += commentOpen[0];
}

void Lexer::Start(StringRef in) {
  m_in = in;
  m_tokStart = 0;
  m_pos = 0;
  m_state = State::Text;
}

Token Lexer::Scan() {
  m_tokStart = m_pos;

again:
  if (m_tokStart >= m_in.size()) return MakeToken(Token::kEof);

  switch (m_state) {
    default:
    case State::Text: {
      // fast-scan to first open character
      size_t openStart = m_in.substr(m_pos).find_first_of(m_config.openChars);
      if (openStart == StringRef::npos) {
        // didn't find open, return remaining text as text token
        m_pos = m_in.size();
        return MakeToken(Token::kText);
      }
      m_pos += openStart;

      // try to match one of the opening sequences, and get the close
      StringRef openStr = m_in.substr(m_pos);
      if (openStr.startswith(m_config.expressionOpen)) {
        m_state = State::ExpressionStart;
      } else if (openStr.startswith(m_config.statementOpen)) {
        m_state = State::StatementStart;
      } else if (openStr.startswith(m_config.commentOpen)) {
        m_state = State::CommentStart;
      } else if ((m_pos == 0 || m_in[m_pos - 1] == '\n') &&
                 openStr.startswith(m_config.lineStatement)) {
        m_state = State::LineStart;
      } else {
        ++m_pos; // wasn't actually an opening sequence
        goto again;
      }
      if (m_pos == m_tokStart) goto again;  // don't generate empty token
      return MakeToken(Token::kText);
    }
    case State::ExpressionStart: {
      m_state = State::ExpressionBody;
      m_pos += m_config.expressionOpen.size();
      return MakeToken(Token::kExpressionOpen);
    }
    case State::LineStart: {
      m_state = State::LineBody;
      m_pos += m_config.lineStatement.size();
      return MakeToken(Token::kLineStatementOpen);
    }
    case State::StatementStart: {
      m_state = State::StatementBody;
      m_pos += m_config.statementOpen.size();
      return MakeToken(Token::kStatementOpen);
    }
    case State::CommentStart: {
      m_state = State::CommentBody;
      m_pos += m_config.commentOpen.size();
      return MakeToken(Token::kCommentOpen);
    }
    case State::ExpressionBody:
      return ScanBody(m_config.expressionClose, Token::kExpressionClose);
    case State::LineBody:
      return ScanBody("\n", Token::kLineStatementClose);
    case State::StatementBody:
      return ScanBody(m_config.statementClose, Token::kStatementClose);
    case State::CommentBody: {
      // fast-scan to comment close
      size_t end = m_in.substr(m_pos).find(m_config.commentClose);
      if (end == StringRef::npos) {
        m_pos = m_in.size();
        return MakeToken(Token::kEof);
      }
      // return the entire comment in the close token
      m_state = State::Text;
      m_pos += end + m_config.commentClose.size();
      return MakeToken(Token::kCommentClose);
    }
  }
}

Token Lexer::ScanBody(StringRef close, Token::Kind closeKind) {
again:
  // skip whitespace (except for \n as it might be a close)
  if (m_tokStart >= m_in.size()) return MakeToken(Token::kEof);
  char ch = m_in[m_tokStart];
  if (ch == ' ' || ch == '\t' || ch == '\r') {
    ++m_tokStart;
    goto again;
  }

  // check for close
  if (m_in.substr(m_tokStart).startswith(close)) {
    m_state = State::Text;
    m_pos = m_tokStart + close.size();
    return MakeToken(closeKind);
  }

  // skip \n
  if (ch == '\n') {
    ++m_tokStart;
    goto again;
  }

  m_pos = m_tokStart + 1;
  if (std::isalpha(ch)) return ScanId();
  switch (ch) {
    case ',':
      return MakeToken(Token::kComma);
    case ':':
      return MakeToken(Token::kColon);
    case '(':
      return MakeToken(Token::kLeftParen);
    case ')':
      return MakeToken(Token::kRightParen);
    case '[':
      return MakeToken(Token::kLeftBracket);
    case ']':
      return MakeToken(Token::kRightBracket);
    case '{':
      return MakeToken(Token::kLeftBrace);
    case '}':
      return MakeToken(Token::kRightBrace);
    case '>':
      if (m_pos < m_in.size() && m_in[m_pos] == '=') {
        ++m_pos;
        return MakeToken(Token::kGreaterEqual);
      }
      return MakeToken(Token::kGreaterThan);
    case '<':
      if (m_pos < m_in.size() && m_in[m_pos] == '=') {
        ++m_pos;
        return MakeToken(Token::kLessEqual);
      }
      return MakeToken(Token::kLessThan);
    case '=':
      if (m_pos < m_in.size() && m_in[m_pos] == '=') {
        ++m_pos;
        return MakeToken(Token::kEqual);
      }
      return MakeToken(Token::kUnknown);
    case '!':
      if (m_pos < m_in.size() && m_in[m_pos] == '=') {
        ++m_pos;
        return MakeToken(Token::kNotEqual);
      }
      return MakeToken(Token::kUnknown);
    case '\"':
      return ScanString();
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
      return ScanNumber();
    case '_':
      return ScanId();
    default:
      return MakeToken(Token::kUnknown);
  }
}

Token Lexer::ScanId() {
  for (;;) {
    if (m_pos >= m_in.size()) break;
    char ch = m_in[m_pos];
    if (!isalnum(ch) && ch != '.' && ch != '/' && ch != '_' && ch != '-') break;
    ++m_pos;
  }
  return MakeToken(Token::kId);
}

Token Lexer::ScanNumber() {
  for (;;) {
    if (m_pos >= m_in.size()) break;
    char ch = m_in[m_pos];
    // be very permissive in lexer (we'll catch errors when conversion happens)
    if (!isdigit(ch) && ch != '.' && ch != 'e' && ch != 'E' && ch != '+' &&
        ch != '-')
      break;
    ++m_pos;
  }
  return MakeToken(Token::kNumber);
}

Token Lexer::ScanString() {
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
  return MakeToken(Token::kString);
}

const ParserStatic& ParserStatic::GetInstance() {
  static ParserStatic inst;
  return inst;
}

ParserStatic::ParserStatic() {
  functions.AddBuiltin("default", 2, Bytecode::Op::Default);
  functions.AddBuiltin("divisibleBy", 2, Bytecode::Op::DivisibleBy);
  functions.AddBuiltin("even", 1, Bytecode::Op::Even);
  functions.AddBuiltin("first", 1, Bytecode::Op::First);
  functions.AddBuiltin("float", 1, Bytecode::Op::Float);
  functions.AddBuiltin("int", 1, Bytecode::Op::Int);
  functions.AddBuiltin("last", 1, Bytecode::Op::Last);
  functions.AddBuiltin("length", 1, Bytecode::Op::Length);
  functions.AddBuiltin("lower", 1, Bytecode::Op::Lower);
  functions.AddBuiltin("max", 1, Bytecode::Op::Max);
  functions.AddBuiltin("min", 1, Bytecode::Op::Min);
  functions.AddBuiltin("odd", 1, Bytecode::Op::Odd);
  functions.AddBuiltin("range", 1, Bytecode::Op::Range);
  functions.AddBuiltin("round", 2, Bytecode::Op::Round);
  functions.AddBuiltin("sort", 1, Bytecode::Op::Sort);
  functions.AddBuiltin("upper", 1, Bytecode::Op::Upper);
  functions.AddBuiltin("exists", 1, Bytecode::Op::Exists);
  functions.AddBuiltin("existsIn", 2, Bytecode::Op::ExistsInObject);
  functions.AddBuiltin("isBoolean", 1, Bytecode::Op::IsBoolean);
  functions.AddBuiltin("isNumber", 1, Bytecode::Op::IsNumber);
  functions.AddBuiltin("isInteger", 1, Bytecode::Op::IsInteger);
  functions.AddBuiltin("isFloat", 1, Bytecode::Op::IsFloat);
  functions.AddBuiltin("isObject", 1, Bytecode::Op::IsObject);
  functions.AddBuiltin("isArray", 1, Bytecode::Op::IsArray);
  functions.AddBuiltin("isString", 1, Bytecode::Op::IsString);
}

Parser::Parser(const ParserConfig& parserConfig, const LexerConfig& lexerConfig,
               TemplateStorage& includedTemplates)
    : m_config(parserConfig),
      m_lexer(lexerConfig),
      m_includedTemplates(includedTemplates),
      m_static(ParserStatic::GetInstance()) {

  }

bool Parser::ParseExpression(Template& tmpl) {
  if (!ParseExpressionAnd(tmpl)) return false;
  if (m_tok.kind != Token::kId || m_tok.text != "or") return true;
  GetNextToken();
  if (!ParseExpressionAnd(tmpl)) return false;
  AppendFunction(tmpl, Bytecode::Op::Or, 2);
  return true;
}

bool Parser::ParseExpressionAnd(Template& tmpl) {
  if (!ParseExpressionNot(tmpl)) return false;
  if (m_tok.kind != Token::kId || m_tok.text != "and") return true;
  GetNextToken();
  if (!ParseExpressionNot(tmpl)) return false;
  AppendFunction(tmpl, Bytecode::Op::And, 2);
  return true;
}

bool Parser::ParseExpressionNot(Template& tmpl) {
  if (m_tok.kind == Token::kId && m_tok.text == "not") {
    GetNextToken();
    if (!ParseExpressionNot(tmpl)) return false;
    AppendFunction(tmpl, Bytecode::Op::Not, 1);
    return true;
  } else {
    return ParseExpressionComparison(tmpl);
  }
}

bool Parser::ParseExpressionComparison(Template& tmpl) {
  if (!ParseExpressionDatum(tmpl)) return false;
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
  GetNextToken();
  if (!ParseExpressionDatum(tmpl)) return false;
  AppendFunction(tmpl, op, 2);
  return true;
}

bool Parser::ParseExpressionDatum(Template& tmpl) {
  StringRef jsonFirst;
  size_t bracketLevel = 0;
  size_t braceLevel = 0;

  for (;;) {
    switch (m_tok.kind) {
      case Token::kLeftParen: {
        GetNextToken();
        if (!ParseExpression(tmpl)) return false;
        if (m_tok.kind != Token::kRightParen) {
          inja_throw("parser_error", "unmatched '('");
        }
        GetNextToken();
        return true;
      }
      case Token::kId:
        GetPeekToken();
        if (m_peekTok.kind == Token::kLeftParen) {
          // function call, parse arguments
          Token funcTok = m_tok;
          GetNextToken();  // id
          GetNextToken();  // leftParen
          unsigned int numArgs = 0;
          if (m_tok.kind == Token::kRightParen) {
            // no args
            GetNextToken();
          } else {
            for (;;) {
              if (!ParseExpression(tmpl)) {
                inja_throw("parser_error", "expected expression, got '" +
                                               m_tok.Describe().str() + "'");
              }
              ++numArgs;
              if (m_tok.kind == Token::kRightParen) {
                GetNextToken();
                break;
              }
              if (m_tok.kind != Token::kComma) {
                inja_throw("parser_error", "expected ')' or ',', got '" +
                                               m_tok.Describe().str() + "'");
              }
              GetNextToken();
            }
          }

          auto op = m_static.functions.FindBuiltin(funcTok.text, numArgs);
          if (op != Bytecode::Op::Nop) {
            // swap arguments for default(); see comment in RenderTo()
            if (op == Bytecode::Op::Default)
              std::swap(tmpl.bytecodes.back(), *(tmpl.bytecodes.rbegin() + 1));
            AppendFunction(tmpl, op, numArgs);
            return true;
          } else {
            AppendCallback(tmpl, funcTok.text, numArgs);
            return true;
          }
        } else if (m_tok.text == "true" || m_tok.text == "false" ||
                   m_tok.text == "null") {
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
              m_config.notation == ElementNotation::Pointer
                  ? Bytecode::kFlagValueLookupPointer
                  : Bytecode::kFlagValueLookupDot);
          GetNextToken();
          return true;
        }
      /* json passthrough */
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
          inja_throw("parser_error",
                     "unexpected token '" + m_tok.Describe().str() + "'");
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

    GetNextToken();
  }

returnJson:
  // bridge across all intermediate tokens
  StringRef jsonText(
      jsonFirst.data(),
      m_tok.text.data() - jsonFirst.data() + m_tok.text.size());

  tmpl.bytecodes.emplace_back(Bytecode::Op::Push, json::parse(jsonText),
                              Bytecode::kFlagValueImmediate);
  GetNextToken();
  return true;
}

bool Parser::ParseStatement(Template& tmpl, StringRef path) {
  if (m_tok.kind != Token::kId) return false;

  if (m_tok.text == "if") {
    GetNextToken();

    // evaluate expression
    if (!ParseExpression(tmpl)) return false;

    // start a new if block on if stack
    m_ifStack.emplace_back(tmpl.bytecodes.size());

    // conditional jump; destination will be filled in by else or endif
    tmpl.bytecodes.emplace_back(Bytecode::Op::ConditionalJump);
  } else if (m_tok.text == "endif") {
    if (m_ifStack.empty())
      inja_throw("parser_error", "endif without matching if");
    auto& ifData = m_ifStack.back();
    GetNextToken();

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
    GetNextToken();

    // end previous block with unconditional jump to endif; destination will be
    // filled in by endif
    ifData.uncondJumps.push_back(tmpl.bytecodes.size());
    tmpl.bytecodes.emplace_back(Bytecode::Op::Jump);

    // previous conditional jump jumps here
    tmpl.bytecodes[ifData.prevCondJump].args = tmpl.bytecodes.size();
    ifData.prevCondJump = std::numeric_limits<unsigned int>::max();

    // chained else if
    if (m_tok.kind == Token::kId && m_tok.text == "if") {
      GetNextToken();

      // evaluate expression
      if (!ParseExpression(tmpl)) return false;

      // update "previous jump"
      ifData.prevCondJump = tmpl.bytecodes.size();

      // conditional jump; destination will be filled in by else or endif
      tmpl.bytecodes.emplace_back(Bytecode::Op::ConditionalJump);
    }
  } else if (m_tok.text == "for") {
    GetNextToken();

    // options: for a in arr; for a, b in obj
    if (m_tok.kind != Token::kId)
      inja_throw("parser_error",
                 "expected id, got '" + m_tok.Describe().str() + "'");
    Token valueTok = m_tok;
    GetNextToken();

    Token keyTok;
    if (m_tok.kind == Token::kComma) {
      GetNextToken();
      if (m_tok.kind != Token::kId)
        inja_throw("parser_error",
                   "expected id, got '" + m_tok.Describe().str() + "'");
      keyTok = std::move(valueTok);
      valueTok = m_tok;
      GetNextToken();
    }

    if (m_tok.kind != Token::kId || m_tok.text != "in")
      inja_throw("parser_error",
                 "expected 'in', got '" + m_tok.Describe().str() + "'");
    GetNextToken();

    if (!ParseExpression(tmpl)) return false;

    m_loopStack.push_back(tmpl.bytecodes.size());

    tmpl.bytecodes.emplace_back(Bytecode::Op::StartLoop);
    if (!keyTok.text.empty()) tmpl.bytecodes.back().value = keyTok.text;
    tmpl.bytecodes.back().str = valueTok.text;
  } else if (m_tok.text == "endfor") {
    GetNextToken();
    if (m_loopStack.empty())
      inja_throw("parser_error", "endfor without matching for");

    // update loop with EndLoop index (for empty case)
    tmpl.bytecodes[m_loopStack.back()].args = tmpl.bytecodes.size();

    tmpl.bytecodes.emplace_back(Bytecode::Op::EndLoop);
    tmpl.bytecodes.back().args = m_loopStack.back() + 1;  // loop body
    m_loopStack.pop_back();
  } else if (m_tok.text == "include") {
    GetNextToken();

    if (m_tok.kind != Token::kString)
      inja_throw("parser_error", "expected string, got '" + m_tok.Describe().str() + "'");

    // build the relative path
    json jsonName = json::parse(m_tok.text);
    std::string pathname = path;
    pathname += jsonName.get_ref<const std::string&>();
    // sys::path::remove_dots(pathname, true, sys::path::Style::posix);

    // parse it only if it's new
    TemplateStorage::iterator included;
    bool isNew;
    // TODO: std::tie(included, isNew) = m_includedTemplates.emplace(pathname);
    if (isNew) included->second = ParseTemplate(pathname);

    // generate a reference bytecode
    tmpl.bytecodes.emplace_back(Bytecode::Op::Include, json(pathname), Bytecode::kFlagValueImmediate);

    GetNextToken();
  } else {
    return false;
  }
  return true;
}

void Parser::AppendFunction(Template& tmpl, Bytecode::Op op, unsigned int numArgs) {
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

void Parser::AppendCallback(Template& tmpl, StringRef name,
                            unsigned int numArgs) {
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

void Parser::ParseInto(Template& tmpl, StringRef path) {
  m_lexer.Start(tmpl.contents);

  for (;;) {
    GetNextToken();
    switch (m_tok.kind) {
      case Token::kEof:
        if (!m_ifStack.empty()) inja_throw("parser_error", "unmatched if");
        if (!m_loopStack.empty()) inja_throw("parser_error", "unmatched for");
        return;
      case Token::kText:
        tmpl.bytecodes.emplace_back(Bytecode::Op::PrintText, m_tok.text, 0u);
        break;
      case Token::kStatementOpen:
        GetNextToken();
        if (!ParseStatement(tmpl, path)) {
          inja_throw("parser_error", "expected statement, got '" +
                                         m_tok.Describe().str() + "'");
        }
        if (m_tok.kind != Token::kStatementClose) {
          inja_throw("parser_error", "expected statement close, got '" +
                                         m_tok.Describe().str() + "'");
        }
        break;
      case Token::kLineStatementOpen:
        GetNextToken();
        ParseStatement(tmpl, path);
        if (m_tok.kind != Token::kLineStatementClose &&
            m_tok.kind != Token::kEof) {
          inja_throw("parser_error",
                     "expected line statement close, got '" +
                         m_tok.Describe().str() + "'");
        }
        break;
      case Token::kExpressionOpen:
        GetNextToken();
        if (!ParseExpression(tmpl)) {
          inja_throw("parser_error", "expected expression, got '" +
                                         m_tok.Describe().str() + "'");
        }
        AppendFunction(tmpl, Bytecode::Op::PrintValue, 1);
        if (m_tok.kind != Token::kExpressionClose) {
          inja_throw("parser_error", "expected expression close, got '" +
                                         m_tok.Describe().str() + "'");
        }
        break;
      case Token::kCommentOpen:
        GetNextToken();
        if (m_tok.kind != Token::kCommentClose) {
          inja_throw("parser_error", "expected comment close, got '" +
                                         m_tok.Describe().str() + "'");
        }
        break;
      default:
        inja_throw("parser_error", "unexpected token '" + m_tok.Describe().str() + "'");
        break;
    }
  }
}

Template Parser::Parse(StringRef input, StringRef path) {
  Template result;
  result.contents = input;
  ParseInto(result, path);
  return result;
}

Template Parser::ParseTemplate(StringRef filename) {
  Template result;
  if (m_config.loadFile) {
    result.contents = m_config.loadFile(filename);
    StringRef path = filename.substr(0, filename.find_last_of("/\\") + 1);
    // StringRef path = sys::path::parent_path(filename);
    Parser(m_config, m_lexer.GetConfig(), m_includedTemplates).ParseInto(result, path);
  }
  return result;
}
