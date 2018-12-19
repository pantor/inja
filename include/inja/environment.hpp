#ifndef PANTOR_INJA_ENVIRONMENT_HPP
#define PANTOR_INJA_ENVIRONMENT_HPP

#include <sstream>

#include <nlohmann/json.hpp>

#include "config.hpp"
#include "function_storage.hpp"
#include "parser.hpp"
#include "renderer.hpp"
#include "template.hpp"


namespace inja {

using namespace nlohmann;

class Environment {
  class Impl {
   public:
    std::string path;

    LexerConfig lexerConfig;
    ParserConfig parserConfig;

    FunctionStorage callbacks;

    TemplateStorage includedTemplates;
  };
  std::unique_ptr<Impl> m_impl;

 public:
  Environment(): Environment("./") { }

  explicit Environment(const std::string& path): m_impl(std::make_unique<Impl>()) {
    m_impl->path = path;
  }

  ~Environment() { }

  void set_statement(const std::string& open, const std::string& close) {
    m_impl->lexerConfig.statementOpen = open;
    m_impl->lexerConfig.statementClose = close;
    m_impl->lexerConfig.update_open_chars();
  }

  void set_line_statement(const std::string& open) {
    m_impl->lexerConfig.lineStatement = open;
    m_impl->lexerConfig.update_open_chars();
  }

  void set_expression(const std::string& open, const std::string& close) {
    m_impl->lexerConfig.expressionOpen = open;
    m_impl->lexerConfig.expressionClose = close;
    m_impl->lexerConfig.update_open_chars();
  }

  void set_comment(const std::string& open, const std::string& close) {
    m_impl->lexerConfig.commentOpen = open;
    m_impl->lexerConfig.commentClose = close;
    m_impl->lexerConfig.update_open_chars();
  }

  void set_element_notation(ElementNotation notation) {
    m_impl->parserConfig.notation = notation;
  }

  void set_load_file(std::function<std::string(std::string_view)> loadFile) {
    m_impl->parserConfig.loadFile = loadFile;
  }

  Template parse(std::string_view input) {
    Parser parser(m_impl->parserConfig, m_impl->lexerConfig, m_impl->includedTemplates);
    return parser.parse(input);
  }

  std::string render(std::string_view input, const json& data) {
    return render(parse(input), data);
  }

  std::string render(const Template& tmpl, const json& data) {
    std::stringstream os;
    render_to(os, tmpl, data);
    return os.str();
  }

  std::stringstream& render_to(std::stringstream& os, const Template& tmpl, const json& data) {
    Renderer(m_impl->includedTemplates, m_impl->callbacks).render_to(os, tmpl, data);
    return os;
  }

  void add_callback(std::string_view name, unsigned int numArgs, const CallbackFunction& callback) {
    m_impl->callbacks.add_callback(name, numArgs, callback);
  }

  void include_template(const std::string& name, const Template& tmpl) {
    m_impl->includedTemplates[name] = tmpl;
  }
};

/*!
@brief render with default settings
*/
inline std::string render(const std::string& input, const json& data) {
  return Environment().render(input, data);
}

}

#endif // PANTOR_INJA_ENVIRONMENT_HPP
