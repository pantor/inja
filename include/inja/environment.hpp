#ifndef PANTOR_INJA_ENVIRONMENT_HPP
#define PANTOR_INJA_ENVIRONMENT_HPP

#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "config.hpp"
#include "function_storage.hpp"
#include "parser.hpp"
#include "polyfill.hpp"
#include "renderer.hpp"
#include "template.hpp"


namespace inja {

using namespace nlohmann;

class Environment {
  class Impl {
   public:
    std::string path;

    lexer_config lexer_config;
    parser_config parser_config;

    FunctionStorage callbacks;
    TemplateStorage included_templates;
  };
  std::unique_ptr<Impl> m_impl;

 public:
  Environment(): Environment("./") { }

  explicit Environment(const std::string& path): m_impl(std::make_unique<Impl>()) {
    m_impl->path = path;
  }

  void set_statement(const std::string& open, const std::string& close) {
    m_impl->lexer_config.statement_open = open;
    m_impl->lexer_config.statement_close = close;
    m_impl->lexer_config.update_open_chars();
  }

  void set_line_statement(const std::string& open) {
    m_impl->lexer_config.line_statement = open;
    m_impl->lexer_config.update_open_chars();
  }

  void set_expression(const std::string& open, const std::string& close) {
    m_impl->lexer_config.expression_open = open;
    m_impl->lexer_config.expression_close = close;
    m_impl->lexer_config.update_open_chars();
  }

  void set_comment(const std::string& open, const std::string& close) {
    m_impl->lexer_config.comment_open = open;
    m_impl->lexer_config.comment_close = close;
    m_impl->lexer_config.update_open_chars();
  }

  void set_element_notation(ElementNotation notation) {
    m_impl->parser_config.notation = notation;
  }

  void set_load_file(std::function<std::string(std::string_view)> loadFile) {
    m_impl->parser_config.loadFile = loadFile;
  }

  Template parse(std::string_view input) {
    Parser parser(m_impl->parser_config, m_impl->lexer_config, m_impl->included_templates);
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
    Renderer(m_impl->included_templates, m_impl->callbacks).render_to(os, tmpl, data);
    return os;
  }

  void add_callback(std::string_view name, unsigned int numArgs, const CallbackFunction& callback) {
    m_impl->callbacks.add_callback(name, numArgs, callback);
  }

  void include_template(const std::string& name, const Template& tmpl) {
    m_impl->included_templates[name] = tmpl;
  }
};

/*!
@brief render with default settings
*/
inline std::string render(std::string_view input, const json& data) {
  return Environment().render(input, data);
}

}

#endif // PANTOR_INJA_ENVIRONMENT_HPP
