#ifndef PANTOR_INJA_PARSER_HPP
#define PANTOR_INJA_PARSER_HPP

#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include <regex.hpp>
#include <template.hpp>


namespace inja {

using json = nlohmann::json;


class Parser {
public:
	ElementNotation element_notation = ElementNotation::Pointer;

	std::map<Parsed::CallbackSignature, Regex, std::greater<Parsed::CallbackSignature>> regex_map_callbacks;

	std::map<const std::string, Template> included_templates;

	/*!
	@brief create a corresponding regex for a function name with a number of arguments separated by ,
	*/
	static Regex function_regex(const std::string& name, int number_arguments) {
		std::string pattern = name;
		pattern.append("(?:\\(");
		for (int i = 0; i < number_arguments; i++) {
			if (i != 0) pattern.append(",");
			pattern.append("(.*)");
		}
		pattern.append("\\))");
		if (number_arguments == 0) { // Without arguments, allow to use the callback without parenthesis
			pattern.append("?");
		}
		return Regex{"\\s*" + pattern + "\\s*"};
	}

	/*!
	@brief dot notation to json pointer notation
	*/
	static std::string dot_to_json_pointer_notation(const std::string& dot) {
		std::string result = dot;
		while (result.find(".") != std::string::npos) {
			result.replace(result.find("."), 1, "/");
		}
		result.insert(0, "/");
		return result;
	}

	std::map<Parsed::Delimiter, Regex> regex_map_delimiters = {
		{Parsed::Delimiter::Statement, Regex{"\\{\\%\\s*(.+?)\\s*\\%\\}"}},
		{Parsed::Delimiter::LineStatement, Regex{"(?:^|\\n)## *(.+?) *(?:\\n|$)"}},
		{Parsed::Delimiter::Expression, Regex{"\\{\\{\\s*(.+?)\\s*\\}\\}"}},
		{Parsed::Delimiter::Comment, Regex{"\\{#\\s*(.*?)\\s*#\\}"}}
	};

	const std::map<Parsed::Statement, Regex> regex_map_statement_openers = {
		{Parsed::Statement::Loop, Regex{"for\\s+(.+)"}},
		{Parsed::Statement::Condition, Regex{"if\\s+(.+)"}},
		{Parsed::Statement::Include, Regex{"include\\s+\"(.+)\""}}
	};

	const std::map<Parsed::Statement, Regex> regex_map_statement_closers = {
		{Parsed::Statement::Loop, Regex{"endfor"}},
		{Parsed::Statement::Condition, Regex{"endif"}}
	};

	const std::map<Parsed::Loop, Regex> regex_map_loop = {
		{Parsed::Loop::ForListIn, Regex{"for\\s+(\\w+)\\s+in\\s+(.+)"}},
		{Parsed::Loop::ForMapIn, Regex{"for\\s+(\\w+),\\s+(\\w+)\\s+in\\s+(.+)"}},
	};

	const std::map<Parsed::Condition, Regex> regex_map_condition = {
		{Parsed::Condition::If, Regex{"if\\s+(.+)"}},
		{Parsed::Condition::ElseIf, Regex{"else\\s+if\\s+(.+)"}},
		{Parsed::Condition::Else, Regex{"else"}}
	};

	const std::map<Parsed::Function, Regex> regex_map_functions = {
		{Parsed::Function::Not, Regex{"not (.+)"}},
		{Parsed::Function::And, Regex{"(.+) and (.+)"}},
		{Parsed::Function::Or, Regex{"(.+) or (.+)"}},
		{Parsed::Function::In, Regex{"(.+) in (.+)"}},
		{Parsed::Function::Equal, Regex{"(.+) == (.+)"}},
		{Parsed::Function::Greater, Regex{"(.+) > (.+)"}},
		{Parsed::Function::Less, Regex{"(.+) < (.+)"}},
		{Parsed::Function::GreaterEqual, Regex{"(.+) >= (.+)"}},
		{Parsed::Function::LessEqual, Regex{"(.+) <= (.+)"}},
		{Parsed::Function::Different, Regex{"(.+) != (.+)"}},
		{Parsed::Function::Default, function_regex("default", 2)},
		{Parsed::Function::DivisibleBy, function_regex("divisibleBy", 2)},
		{Parsed::Function::Even, function_regex("even", 1)},
		{Parsed::Function::First, function_regex("first", 1)},
		{Parsed::Function::Float, function_regex("float", 1)},
		{Parsed::Function::Int, function_regex("int", 1)},
		{Parsed::Function::Last, function_regex("last", 1)},
		{Parsed::Function::Length, function_regex("length", 1)},
		{Parsed::Function::Lower, function_regex("lower", 1)},
		{Parsed::Function::Max, function_regex("max", 1)},
		{Parsed::Function::Min, function_regex("min", 1)},
		{Parsed::Function::Odd, function_regex("odd", 1)},
		{Parsed::Function::Range, function_regex("range", 1)},
		{Parsed::Function::Round, function_regex("round", 2)},
		{Parsed::Function::Sort, function_regex("sort", 1)},
		{Parsed::Function::Upper, function_regex("upper", 1)},
		{Parsed::Function::Exists, function_regex("exists", 1)},
		{Parsed::Function::ExistsInObject, function_regex("existsIn", 2)},
		{Parsed::Function::IsBoolean, function_regex("isBoolean", 1)},
		{Parsed::Function::IsNumber, function_regex("isNumber", 1)},
		{Parsed::Function::IsInteger, function_regex("isInteger", 1)},
		{Parsed::Function::IsFloat, function_regex("isFloat", 1)},
		{Parsed::Function::IsObject, function_regex("isObject", 1)},
		{Parsed::Function::IsArray, function_regex("isArray", 1)},
		{Parsed::Function::IsString, function_regex("isString", 1)},
		{Parsed::Function::ReadJson, Regex{"\\s*([^\\(\\)]*\\S)\\s*"}}
	};

	Parser() { }

	Parsed::ElementExpression parse_expression(const std::string& input) {
		const MatchType<Parsed::CallbackSignature> match_callback = match(input, regex_map_callbacks);
		if (!match_callback.type().first.empty()) {
			std::vector<Parsed::ElementExpression> args {};
			for (unsigned int i = 1; i < match_callback.size(); i++) { // str(0) is whole group
				args.push_back( parse_expression(match_callback.str(i)) );
			}

			Parsed::ElementExpression result = Parsed::ElementExpression(Parsed::Function::Callback);
			result.args = args;
			result.command = match_callback.type().first;
			return result;
		}

		const MatchType<Parsed::Function> match_function = match(input, regex_map_functions);
		switch ( match_function.type() ) {
			case Parsed::Function::ReadJson: {
				std::string command = match_function.str(1);
				if ( json::accept(command) ) { // JSON Result
					Parsed::ElementExpression result = Parsed::ElementExpression(Parsed::Function::Result);
					result.result = json::parse(command);
					return result;
				}

				Parsed::ElementExpression result = Parsed::ElementExpression(Parsed::Function::ReadJson);
				switch (element_notation) {
					case ElementNotation::Pointer: {
						if (command[0] != '/') { command.insert(0, "/"); }
						result.command = command;
						break;
					}
					case ElementNotation::Dot: {
						result.command = dot_to_json_pointer_notation(command);
						break;
					}
				}
				return result;
			}
			default: {
				std::vector<Parsed::ElementExpression> args = {};
				for (unsigned int i = 1; i < match_function.size(); i++) { // str(0) is whole group
					args.push_back( parse_expression(match_function.str(i)) );
				}

				Parsed::ElementExpression result = Parsed::ElementExpression(match_function.type());
				result.args = args;
				return result;
			}
		}
	}

	std::vector<std::shared_ptr<Parsed::Element>> parse_level(const std::string& input, const std::string& path) {
		std::vector<std::shared_ptr<Parsed::Element>> result;

		size_t current_position = 0;
		MatchType<Parsed::Delimiter> match_delimiter = search(input, regex_map_delimiters, current_position);
		while (match_delimiter.found()) {
			current_position = match_delimiter.end_position();
			const std::string string_prefix = match_delimiter.prefix();
			if (not string_prefix.empty()) {
				result.emplace_back( std::make_shared<Parsed::ElementString>(string_prefix) );
			}

			const std::string delimiter_inner = match_delimiter.str(1);

			switch ( match_delimiter.type() ) {
				case Parsed::Delimiter::Statement:
				case Parsed::Delimiter::LineStatement: {

					const MatchType<Parsed::Statement> match_statement = match(delimiter_inner, regex_map_statement_openers);
					switch ( match_statement.type() ) {
						case Parsed::Statement::Loop: {
							const MatchClosed loop_match = search_closed(input, match_delimiter.regex(), regex_map_statement_openers.at(Parsed::Statement::Loop), regex_map_statement_closers.at(Parsed::Statement::Loop), match_delimiter);

							current_position = loop_match.end_position();

							const std::string loop_inner = match_statement.str(0);
							const MatchType<Parsed::Loop> match_command = match(loop_inner, regex_map_loop);
							if (not match_command.found()) {
								inja_throw("parser_error", "unknown loop statement: " + loop_inner);
							}
							switch (match_command.type()) {
								case Parsed::Loop::ForListIn: {
									const std::string value_name = match_command.str(1);
									const std::string list_name = match_command.str(2);

									result.emplace_back( std::make_shared<Parsed::ElementLoop>(match_command.type(), value_name, parse_expression(list_name), loop_match.inner()));
									break;
								}
								case Parsed::Loop::ForMapIn: {
									const std::string key_name = match_command.str(1);
									const std::string value_name = match_command.str(2);
									const std::string list_name = match_command.str(3);

									result.emplace_back( std::make_shared<Parsed::ElementLoop>(match_command.type(), key_name, value_name, parse_expression(list_name), loop_match.inner()));
									break;
								}
							}
							break;
						}
						case Parsed::Statement::Condition: {
							auto condition_container = std::make_shared<Parsed::ElementConditionContainer>();

							Match condition_match = match_delimiter;
							MatchClosed else_if_match = search_closed_on_level(input, match_delimiter.regex(), regex_map_statement_openers.at(Parsed::Statement::Condition), regex_map_statement_closers.at(Parsed::Statement::Condition), regex_map_condition.at(Parsed::Condition::ElseIf), condition_match);
							while (else_if_match.found()) {
								condition_match = else_if_match.close_match;

								const std::string else_if_match_inner = else_if_match.open_match.str(1);
								const MatchType<Parsed::Condition> match_command = match(else_if_match_inner, regex_map_condition);
								if (not match_command.found()) {
									inja_throw("parser_error", "unknown if statement: " + else_if_match.open_match.str());
								}
								condition_container->children.push_back( std::make_shared<Parsed::ElementConditionBranch>(else_if_match.inner(), match_command.type(), parse_expression(match_command.str(1))) );

								else_if_match = search_closed_on_level(input, match_delimiter.regex(), regex_map_statement_openers.at(Parsed::Statement::Condition), regex_map_statement_closers.at(Parsed::Statement::Condition), regex_map_condition.at(Parsed::Condition::ElseIf), condition_match);
							}

							MatchClosed else_match = search_closed_on_level(input, match_delimiter.regex(), regex_map_statement_openers.at(Parsed::Statement::Condition), regex_map_statement_closers.at(Parsed::Statement::Condition), regex_map_condition.at(Parsed::Condition::Else), condition_match);
							if (else_match.found()) {
								condition_match = else_match.close_match;

								const std::string else_match_inner = else_match.open_match.str(1);
								const MatchType<Parsed::Condition> match_command = match(else_match_inner, regex_map_condition);
								if (not match_command.found()) {
									inja_throw("parser_error", "unknown if statement: " + else_match.open_match.str());
								}
								condition_container->children.push_back( std::make_shared<Parsed::ElementConditionBranch>(else_match.inner(), match_command.type(), parse_expression(match_command.str(1))) );
							}

							const MatchClosed last_if_match = search_closed(input, match_delimiter.regex(), regex_map_statement_openers.at(Parsed::Statement::Condition), regex_map_statement_closers.at(Parsed::Statement::Condition), condition_match);
							if (not last_if_match.found()) {
								inja_throw("parser_error", "misordered if statement");
							}

							const std::string last_if_match_inner = last_if_match.open_match.str(1);
							const MatchType<Parsed::Condition> match_command = match(last_if_match_inner, regex_map_condition);
							if (not match_command.found()) {
								inja_throw("parser_error", "unknown if statement: " + last_if_match.open_match.str());
							}
							if (match_command.type() == Parsed::Condition::Else) {
								condition_container->children.push_back( std::make_shared<Parsed::ElementConditionBranch>(last_if_match.inner(), match_command.type()) );
							} else {
								condition_container->children.push_back( std::make_shared<Parsed::ElementConditionBranch>(last_if_match.inner(), match_command.type(), parse_expression(match_command.str(1))) );
							}

							current_position = last_if_match.end_position();
							result.emplace_back(condition_container);
							break;
						}
						case Parsed::Statement::Include: {
							const std::string template_name = match_statement.str(1);
							Template included_template;
							if (included_templates.find( template_name ) != included_templates.end()) {
								included_template = included_templates[template_name];
							} else {
								included_template = parse_template(path + template_name);
							}

							auto children = included_template.parsed_template().children;
							result.insert(result.end(), children.begin(), children.end());
							break;
						}
					}
					break;
				}
				case Parsed::Delimiter::Expression: {
					result.emplace_back( std::make_shared<Parsed::ElementExpression>(parse_expression(delimiter_inner)) );
					break;
				}
				case Parsed::Delimiter::Comment: {
					result.emplace_back( std::make_shared<Parsed::ElementComment>(delimiter_inner) );
					break;
				}
			}

			match_delimiter = search(input, regex_map_delimiters, current_position);
		}
		if (current_position < input.length()) {
			result.emplace_back( std::make_shared<Parsed::ElementString>(input.substr(current_position)) );
		}

		return result;
	}

	std::shared_ptr<Parsed::Element> parse_tree(std::shared_ptr<Parsed::Element> current_element, const std::string& path) {
		if (not current_element->inner.empty()) {
			current_element->children = parse_level(current_element->inner, path);
			current_element->inner.clear();
		}

		if (not current_element->children.empty()) {
			for (auto& child: current_element->children) {
				child = parse_tree(child, path);
			}
		}
		return current_element;
	}

	Template parse(const std::string& input) {
		auto parsed = parse_tree(std::make_shared<Parsed::Element>(Parsed::Element(Parsed::Type::Main, input)), "./");
		return Template(*parsed);
	}

	Template parse_template(const std::string& filename) {
		const std::string input = load_file(filename);
		const std::string path = filename.substr(0, filename.find_last_of("/\\") + 1);
		auto parsed = parse_tree(std::make_shared<Parsed::Element>(Parsed::Element(Parsed::Type::Main, input)), path);
		return Template(*parsed);
	}

	std::string load_file(const std::string& filename) {
		std::ifstream file(filename);
		std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return text;
	}
};

}

#endif // PANTOR_INJA_PARSER_HPP
