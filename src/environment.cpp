/*----------------------------------------------------------------------------*/
/* Copyright (c) 2018 FIRST. All Rights Reserved.                             */
/* Open Source Software - may be modified and shared by FRC teams. The code   */
/* must be accompanied by the FIRST BSD license file in the root directory of */
/* the project.                                                               */
/*----------------------------------------------------------------------------*/


#include <inja/environment.hpp>
#include <inja/parser.hpp>
#include <inja/renderer.hpp>


using namespace inja;


class Environment::Impl {
 public:
  std::string path;
  
  LexerConfig lexerConfig;
  ParserConfig parserConfig;

  FunctionStorage callbacks;

  TemplateStorage includedTemplates;
};

Environment::Environment() : Environment("./") {}

Environment::Environment(const std::string& path) : m_impl(std::make_unique<Impl>()) {
  m_impl->path = path;
}

Environment::~Environment() {}

void Environment::set_statement(const std::string& open, const std::string& close) {
  m_impl->lexerConfig.statementOpen = open;
  m_impl->lexerConfig.statementClose = close;
  m_impl->lexerConfig.UpdateOpenChars();
}

void Environment::set_line_statement(const std::string& open) {
  m_impl->lexerConfig.lineStatement = open;
  m_impl->lexerConfig.UpdateOpenChars();
}

void Environment::set_expression(const std::string& open, const std::string& close) {
  m_impl->lexerConfig.expressionOpen = open;
  m_impl->lexerConfig.expressionClose = close;
  m_impl->lexerConfig.UpdateOpenChars();
}

void Environment::set_comment(const std::string& open, const std::string& close) {
  m_impl->lexerConfig.commentOpen = open;
  m_impl->lexerConfig.commentClose = close;
  m_impl->lexerConfig.UpdateOpenChars();
}

void Environment::set_element_notation(ElementNotation notation) {
  m_impl->parserConfig.notation = notation;
}

void Environment::set_load_file(std::function<std::string(std::string_view)> loadFile) {
  m_impl->parserConfig.loadFile = loadFile;
}

Template Environment::parse(std::string_view input) {
  Parser parser(m_impl->parserConfig, m_impl->lexerConfig, m_impl->includedTemplates);
  return parser.parse(input);
}

std::string Environment::render(std::string_view input, const json& data) {
  return render(parse(input), data);
}

std::string Environment::render(const Template& tmpl, const json& data) {
  std::stringstream os;
  render_to(os, tmpl, data);
  return os.str();
}

std::stringstream& Environment::render_to(std::stringstream& os, const Template& tmpl, const json& data) {
  Renderer(m_impl->includedTemplates, m_impl->callbacks).render_to(os, tmpl, data);
  return os;
}

void Environment::add_callback(std::string_view name, unsigned int numArgs, const CallbackFunction& callback) {
  m_impl->callbacks.AddCallback(name, numArgs, callback);
}

void Environment::include_template(const std::string& name, const Template& tmpl) {
  m_impl->includedTemplates[name] = tmpl;
}
