#ifndef PANTOR_INJA_ENVIRONMENT_HPP
#define PANTOR_INJA_ENVIRONMENT_HPP

#include <memory>
#include <fstream>
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
    std::string input_path;
    std::string output_path;

    LexerConfig lexer_config;
    ParserConfig parser_config;

    FunctionStorage callbacks;
    TemplateStorage included_templates;
  };

  std::unique_ptr<Impl> m_impl;

 public:
  Environment(): Environment("./") { }

  explicit Environment(const std::string& global_path): m_impl(std::make_unique<Impl>()) {
    m_impl->input_path = global_path;
    m_impl->output_path = global_path;
  }

  explicit Environment(const std::string& input_path, const std::string& output_path): m_impl(std::make_unique<Impl>()) {
    m_impl->input_path = input_path;
    m_impl->output_path = output_path;
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


  Template parse(std::string_view input) {
    Parser parser(m_impl->parser_config, m_impl->lexer_config, m_impl->included_templates);
    return parser.parse(input);
  }

  Template parse_template(std::string_view filename) {
    Parser parser(m_impl->parser_config, m_impl->lexer_config, m_impl->included_templates);
		return parser.parse_template(m_impl->input_path + static_cast<std::string>(filename));
	}

  std::string render(std::string_view input, const json& data) {
    return render(parse(input), data);
  }

  std::string render(const Template& tmpl, const json& data) {
    std::stringstream os;
    render_to(os, tmpl, data);
    return os.str();
  }

  std::string render_file(const std::string& filename, const json& data) {
		return render(parse_template(filename), data);
	}

  std::string render_file_with_json_file(const std::string& filename, const std::string& filename_data) {
		const json data = load_json(filename_data);
		return render_file(filename, data);
	}

  void write(const std::string& filename, const json& data, const std::string& filename_out) {
		std::ofstream file(m_impl->output_path + filename_out);
		file << render_file(filename, data);
		file.close();
	}

  void write(const Template& temp, const json& data, const std::string& filename_out) {
		std::ofstream file(m_impl->output_path + filename_out);
		file << render(temp, data);
		file.close();
	}

	void write_with_json_file(const std::string& filename, const std::string& filename_data, const std::string& filename_out) {
		const json data = load_json(filename_data);
		write(filename, data, filename_out);
	}

	void write_with_json_file(const Template& temp, const std::string& filename_data, const std::string& filename_out) {
		const json data = load_json(filename_data);
		write(temp, data, filename_out);
	}

  std::stringstream& render_to(std::stringstream& os, const Template& tmpl, const json& data) {
    Renderer(m_impl->included_templates, m_impl->callbacks).render_to(os, tmpl, data);
    return os;
  }

  std::string load_file(const std::string& filename) {
    Parser parser(m_impl->parser_config, m_impl->lexer_config, m_impl->included_templates);
		return parser.load_file(m_impl->input_path + filename);
	}

  json load_json(const std::string& filename) {
		std::ifstream file(m_impl->input_path + filename);
		json j;
		file >> j;
		return j;
	}

  void add_callback(const std::string& name, unsigned int numArgs, const CallbackFunction& callback) {
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
