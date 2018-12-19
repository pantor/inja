/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/


#include <inja/inja.hpp>
#include <inja/internal.hpp>
#include <inja/parser.hpp>
#include <inja/renderer.hpp>


using namespace inja;


void inja::inja_throw(const std::string& type, const std::string& message) {
  throw std::runtime_error("[inja.exception." + type + "] " + message);
}

class Environment::Impl {
 public:
  std::string path;

  FunctionStorage callbacks;

  LexerConfig lexerConfig;
  ParserConfig parserConfig;

  TemplateStorage includedTemplates;
};

Template::Template() {}

Template::~Template() {}

Template::Template(const Template& oth)
    : bytecodes(oth.bytecodes), contents(oth.contents) {}

Template::Template(Template&& oth)
    : bytecodes(std::move(oth.bytecodes)), contents(std::move(oth.contents)) {}

Template& Template::operator=(const Template& oth) {
  bytecodes = oth.bytecodes;
  contents = oth.contents;
  return *this;
}

Template& Template::operator=(Template&& oth) {
  bytecodes = std::move(oth.bytecodes);
  contents = std::move(oth.contents);
  return *this;
}

Environment::Environment() : Environment("./") {}

Environment::Environment(const std::string& path) : m_impl(std::make_unique<Impl>()) {
  m_impl->path = path;
}

Environment::~Environment() {}

void Environment::SetStatement(const std::string& open, const std::string& close) {
  m_impl->lexerConfig.statementOpen = open;
  m_impl->lexerConfig.statementClose = close;
  m_impl->lexerConfig.UpdateOpenChars();
}

void Environment::SetLineStatement(const std::string& open) {
  m_impl->lexerConfig.lineStatement = open;
  m_impl->lexerConfig.UpdateOpenChars();
}

void Environment::SetExpression(const std::string& open, const std::string& close) {
  m_impl->lexerConfig.expressionOpen = open;
  m_impl->lexerConfig.expressionClose = close;
  m_impl->lexerConfig.UpdateOpenChars();
}

void Environment::SetComment(const std::string& open, const std::string& close) {
  m_impl->lexerConfig.commentOpen = open;
  m_impl->lexerConfig.commentClose = close;
  m_impl->lexerConfig.UpdateOpenChars();
}

void Environment::SetElementNotation(ElementNotation notation) {
  m_impl->parserConfig.notation = notation;
}

void Environment::SetLoadFile(std::function<std::string(std::string_view)> loadFile) {
  m_impl->parserConfig.loadFile = loadFile;
}

Template Environment::Parse(std::string_view input) {
  Parser parser(m_impl->parserConfig, m_impl->lexerConfig, m_impl->includedTemplates);
  return parser.Parse(input);
}

std::string Environment::Render(std::string_view input, const json& data) {
  return Render(Parse(input), data);
}

std::string Environment::Render(const Template& tmpl, const json& data) {
  std::stringstream os;
  RenderTo(os, tmpl, data);
  return os.str();
}

std::stringstream& Environment::RenderTo(std::stringstream& os, const Template& tmpl, const json& data) {
  Renderer(m_impl->includedTemplates, m_impl->callbacks).RenderTo(os, tmpl, data);
  return os;
}

void Environment::AddCallback(std::string_view name, unsigned int numArgs, const CallbackFunction& callback) {
  m_impl->callbacks.AddCallback(name, numArgs, callback);
}

void Environment::IncludeTemplate(const std::string& name, const Template& tmpl) {
  std::string buf;
  m_impl->includedTemplates[name] = tmpl;
}

FunctionStorage::FunctionData& FunctionStorage::GetOrNew(std::string_view name, unsigned int numArgs) {
  auto& vec = m_map[name.data()];
  for (auto& i : vec) {
    if (i.numArgs == numArgs) return i;
  }
  vec.emplace_back();
  vec.back().numArgs = numArgs;
  return vec.back();
}

const FunctionStorage::FunctionData* FunctionStorage::Get(std::string_view name, unsigned int numArgs) const {
  auto it = m_map.find(static_cast<std::string>(name));
  if (it == m_map.end()) return nullptr;
  for (auto&& i : it->second) {
    if (i.numArgs == numArgs) return &i;
  }
  return nullptr;
}
