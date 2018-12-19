#ifndef PANTOR_INJA_ENVIRONMENT_HPP
#define PANTOR_INJA_ENVIRONMENT_HPP

#include <sstream>

#include <nlohmann/json.hpp>

#include <inja/internal.hpp>
#include <inja/template.hpp>


namespace inja {

using namespace nlohmann;

class Environment {
  class Impl;
  std::unique_ptr<Impl> m_impl;

 public:
  Environment();
  explicit Environment(const std::string& path);
  ~Environment();

  void set_statement(const std::string& open, const std::string& close);

  void set_line_statement(const std::string& open);

  void set_expression(const std::string& open, const std::string& close);

  void set_comment(const std::string& open, const std::string& close);

  void set_element_notation(ElementNotation notation);

  void set_load_file(std::function<std::string(std::string_view)> loadFile);

  Template parse(std::string_view input);

  std::string render(std::string_view input, const json& data);

  std::string render(const Template& tmpl, const json& data);

  std::stringstream& render_to(std::stringstream& os, const Template& tmpl, const json& data);

  void add_callback(std::string_view name, unsigned int numArgs, const CallbackFunction& callback);

  void include_template(const std::string& name, const Template& tmpl);
};

/*!
@brief render with default settings
*/
inline std::string render(const std::string& input, const json& data) {
  return Environment().render(input, data);
}

}

#endif // PANTOR_INJA_ENVIRONMENT_HPP