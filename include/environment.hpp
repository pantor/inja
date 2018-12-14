#ifndef PANTOR_INJA_ENVIRONMENT_HPP
#define PANTOR_INJA_ENVIRONMENT_HPP

#include <fstream>
#include <iostream>
#include <string>

#include <regex.hpp>
#include <parser.hpp>
#include <renderer.hpp>
#include <template.hpp>


namespace inja {

using json = nlohmann::json;


/*!
@brief Environment class
*/
class Environment {
	const std::string input_path;
	const std::string output_path;

	Parser parser;
	Renderer renderer;

public:
	Environment(): Environment("./") { }
	explicit Environment(const std::string& global_path): input_path(global_path), output_path(global_path), parser() { }
	explicit Environment(const std::string& input_path, const std::string& output_path): input_path(input_path), output_path(output_path), parser() { }

	void set_statement(const std::string& open, const std::string& close) {
		parser.regex_map_delimiters[Parsed::Delimiter::Statement] = Regex{open + "\\s*(.+?)\\s*" + close};
	}

	void set_line_statement(const std::string& open) {
		parser.regex_map_delimiters[Parsed::Delimiter::LineStatement] = Regex{"(?:^|\\n)" + open + " *(.+?) *(?:\\n|$)"};
	}

	void set_expression(const std::string& open, const std::string& close) {
		parser.regex_map_delimiters[Parsed::Delimiter::Expression] = Regex{open + "\\s*(.+?)\\s*" + close};
	}

	void set_comment(const std::string& open, const std::string& close) {
		parser.regex_map_delimiters[Parsed::Delimiter::Comment] = Regex{open + "\\s*(.+?)\\s*" + close};
	}

	void set_element_notation(const ElementNotation element_notation_) {
		parser.element_notation = element_notation_;
	}

	Template parse(const std::string& input) {
		return parser.parse(input);
	}

	Template parse_template(const std::string& filename) {
		return parser.parse_template(input_path + filename);
	}

	std::string render(const std::string& input, const json& data) {
		return renderer.render(parse(input), data);
	}

	std::string render_template(const Template& temp, const json& data) {
		return renderer.render(temp, data);
	}

	std::string render_file(const std::string& filename, const json& data) {
		return renderer.render(parse_template(filename), data);
	}

	std::string render_file_with_json_file(const std::string& filename, const std::string& filename_data) {
		const json data = load_json(filename_data);
		return render_file(filename, data);
	}

	void write(const std::string& filename, const json& data, const std::string& filename_out) {
		std::ofstream file(output_path + filename_out);
		file << render_file(filename, data);
		file.close();
	}

	void write(const Template& temp, const json& data, const std::string& filename_out) {
		std::ofstream file(output_path + filename_out);
		file << render_template(temp, data);
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

	std::string load_global_file(const std::string& filename) {
		return parser.load_file(input_path + filename);
	}

	json load_json(const std::string& filename) {
		std::ifstream file(input_path + filename);
		json j;
		file >> j;
		return j;
	}

	void add_callback(std::string name, int number_arguments, const std::function<json(const Parsed::Arguments&, const json&)>& callback) {
		const Parsed::CallbackSignature signature = std::make_pair(name, number_arguments);
		parser.regex_map_callbacks[signature] = Parser::function_regex(name, number_arguments);
		renderer.map_callbacks[signature] = callback;
	}

	void include_template(std::string name, const Template& temp) {
		parser.included_templates[name] = temp;
	}

	template<typename T = json>
	T get_argument(const Parsed::Arguments& args, int index, const json& data) {
		return renderer.eval_expression<T>(args[index], data);
	}
};

}

#endif // PANTOR_INJA_ENVIRONMENT_HPP
