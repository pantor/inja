#ifndef PANTOR_INJA_HPP
#define PANTOR_INJA_HPP

#ifndef NLOHMANN_JSON_HPP
	static_assert(false, "nlohmann/json not found.");
#endif


#include <iostream>
#include <fstream>
#include <string>
#include <regex>


namespace inja {

using json = nlohmann::json;
using string = std::string;


inline string join_strings(std::vector<string> vector, string delimiter) {
	std::stringstream ss;
	for (size_t i = 0; i < vector.size(); ++i)
	{
		if (i != 0) ss << delimiter;
		ss << vector[i];
	}
	return ss.str();
}


class smatch: public std::smatch {
	size_t offset;

	smatch() {}
	smatch(std::smatch match, size_t offset): std::smatch(match), offset(offset) { }

public:
	size_t pos() { return offset + position(); }
	size_t end_pos() { return pos() + length(); }
	size_t found() { return !empty(); }
	string outer() { return str(0); }
	string inner() { return str(1); }
};


struct SearchMatch {
	SearchMatch() { }

	SearchMatch(std::smatch match, size_t offset): match(match), offset(offset) {
		position = offset + match.position();
		length = match.length();
		end_position = position + length;
		found = !match.empty();
		outer = match.str(0);
		inner = match.str(1);
		prefix = match.prefix();
		suffix = match.suffix();
	}

	std::smatch match;
	size_t offset;

	string outer, inner, prefix, suffix;
	size_t position, end_position;
	int length;
	bool found = false;
};

struct SearchMatchVector: public SearchMatch {
	SearchMatchVector(): SearchMatch() { }

	SearchMatchVector(std::smatch match, size_t offset, int inner_group, int regex_number, std::vector<string> regex_patterns): SearchMatch(match, offset), regex_number(regex_number) {
		inner = match.str(inner_group);
		if (regex_number >= 0) {
			regex_delimiter = std::regex( regex_patterns.at(regex_number) );
		}
	}

	int regex_number = -1;
	std::regex regex_delimiter;
};

struct SearchClosedMatch: public SearchMatch {
	SearchClosedMatch(): SearchMatch() { }

	SearchClosedMatch(string input, SearchMatch open_match, SearchMatch close_match): SearchMatch(), open_match(open_match), close_match(close_match) {
		position = open_match.position;
		length = close_match.end_position - open_match.position;
		end_position = close_match.end_position;
		found = open_match.found && close_match.found;
		outer = input.substr(position, length);
		inner = input.substr(open_match.end_position, close_match.position - open_match.end_position);
		prefix = open_match.prefix;
		suffix = close_match.suffix;
	}

	SearchMatch open_match, close_match;
};

inline SearchMatch search(string input, std::regex regex, size_t position) {
	auto first = input.cbegin();
	auto last = input.cend();

	if (position >= input.length()) {
		return SearchMatch();
	}

	std::smatch match;
	std::regex_search(first + position, last, match, regex);
	return SearchMatch(match, position);
}

inline SearchMatchVector search(string input, std::vector<string> regex_patterns, size_t position) {
	// Vector of id vs groups
	std::vector<int> regex_mark_counts;
	for (int i = 0; i < regex_patterns.size(); i++) {
		for (int j = 0; j < std::regex(regex_patterns[i]).mark_count() + 1; j++) {
			regex_mark_counts.push_back(i);
		}
	}

	string regex_pattern = "(" + join_strings(regex_patterns, ")|(") + ")";
	std::regex regex(regex_pattern);

	auto first = input.cbegin();
	auto last = input.cend();

	if (position >= input.length()) {
		return SearchMatchVector();
	}

	std::smatch match;
	std::regex_search(first + position, last, match, regex);

	int number_regex = -1;
	int number_inner = 1;
	for (int i = 1; i < match.size(); i++) {
		if (match.length(i) > 0) {
			number_inner = i + 1;
			number_regex = regex_mark_counts[i];
			break;
		}
	}

	return SearchMatchVector(match, position, number_inner, number_regex, regex_patterns);
}

inline SearchClosedMatch search_closed_match_on_level(string input, std::regex regex_statement, std::regex regex_level_up, std::regex regex_level_down, std::regex regex_search, SearchMatch open_match) {

	int level = 0;
	size_t current_position = open_match.end_position;
	SearchMatch statement_match = search(input, regex_statement, current_position);
	while (statement_match.found) {
		current_position = statement_match.end_position;

		if (level == 0 && std::regex_match(statement_match.inner, regex_search)) break;
		if (std::regex_match(statement_match.inner, regex_level_up)) level += 1;
		else if (std::regex_match(statement_match.inner, regex_level_down)) level -= 1;

		statement_match = search(input, regex_statement, current_position);
	}

	return SearchClosedMatch(input, open_match, statement_match);
}

inline SearchClosedMatch search_closed_match(string input, std::regex regex_statement, std::regex regex_open, std::regex regex_close, SearchMatch open_match) {
	return search_closed_match_on_level(input, regex_statement, regex_open, regex_close, regex_close, open_match);
}


class Parser {
public:
	enum class Delimiter {
		Statement,
		LineStatement,
		Expression,
		Comment
	};

	// std::map<Delimiter, string> regex_pattern_map;

	enum class Type {
		String,
		Loop,
		Condition,
		ConditionBranch,
		Include,
		Comment,
		Variable
	};
};


class Environment {
	std::regex regex_statement;
	std::regex regex_line_statement;
	std::regex regex_expression;
	std::regex regex_comment;
	std::vector<string> regex_pattern_delimiters;

	string global_path;


public:
	Environment(): Environment("./") { }

	Environment(string global_path): global_path(global_path) {
		const string regex_pattern_statement = "\\(\\%\\s*(.+?)\\s*\\%\\)";
		const string regex_pattern_line_statement = "(?:^|\\n)##\\s*(.+)\\s*";
		const string regex_pattern_expression = "\\{\\{\\s*(.+?)\\s*\\}\\}";
		const string regex_pattern_comment = "\\{#\\s*(.*?)\\s*#\\}";

		// Parser parser;
		// parser.regex_pattern_map[Parser::Delimiter::Statement] = "\\(\\%\\s*(.+?)\\s*\\%\\)";

		regex_statement = std::regex(regex_pattern_statement);
		regex_line_statement = std::regex(regex_pattern_line_statement);
		regex_expression = std::regex(regex_pattern_expression);
		regex_comment = std::regex(regex_pattern_comment);
		regex_pattern_delimiters = { regex_pattern_statement, regex_pattern_line_statement, regex_pattern_expression, regex_pattern_comment }; // Order of Parser::Delimiter enum
	}


	json parse_level(string input) {
		json result;

		size_t current_position = 0;
		SearchMatchVector statement_match = search(input, regex_pattern_delimiters, current_position);
		while (statement_match.found) {
			current_position = statement_match.end_position;
			if (!statement_match.prefix.empty()) {
				result += {{"type", Parser::Type::String}, {"text", statement_match.prefix}};
			}

			switch ( static_cast<Parser::Delimiter>(statement_match.regex_number) ) {
				case Parser::Delimiter::Statement:
				case Parser::Delimiter::LineStatement: {
					const std::regex regex_loop_open("for (.*)");
					const std::regex regex_loop_close("endfor");

					const std::regex regex_include("include \"(.*)\"");

					const std::regex regex_condition_open("if (.*)");
					const std::regex regex_condition_else_if("else if (.*)");
					const std::regex regex_condition_else("else");
					const std::regex regex_condition_close("endif");

					std::smatch inner_statement_match;
					// Loop
					if (std::regex_match(statement_match.inner, inner_statement_match, regex_loop_open)) {
						SearchClosedMatch loop_match = search_closed_match(input, statement_match.regex_delimiter, regex_loop_open, regex_loop_close, statement_match);

						current_position = loop_match.end_position;
						string loop_command = inner_statement_match.str(0);
						result += {{"type", Parser::Type::Loop}, {"command", loop_command}, {"inner", loop_match.inner}};
					}
					// Include
					else if (std::regex_match(statement_match.inner, inner_statement_match, regex_include)) {
						string include_command = inner_statement_match.str(0);
						string filename = inner_statement_match.str(1);
						result += {{"type", Parser::Type::Include}, {"filename", filename}};
					}
					// Condition
					else if (std::regex_match(statement_match.inner, inner_statement_match, regex_condition_open)) {
						string if_command = inner_statement_match.str(0);
						json condition_result = {{"type", Parser::Type::Condition}, {"children", json::array()}};


						SearchMatch condition_match = statement_match;

						SearchClosedMatch else_if_match = search_closed_match_on_level(input, statement_match.regex_delimiter, regex_condition_open, regex_condition_close, regex_condition_else_if, condition_match);
						while (else_if_match.found) {
							condition_match = else_if_match.close_match;

							condition_result["children"] += {{"type", Parser::Type::ConditionBranch}, {"command", else_if_match.open_match.inner}, {"inner", else_if_match.inner}};

							else_if_match = search_closed_match_on_level(input, regex_statement, regex_condition_open, regex_condition_close, regex_condition_else_if, condition_match);
						}

						SearchClosedMatch else_match = search_closed_match_on_level(input, statement_match.regex_delimiter, regex_condition_open, regex_condition_close, regex_condition_else, condition_match);
						if (else_match.found) {
							condition_match = else_match.close_match;

							condition_result["children"] += {{"type", Parser::Type::ConditionBranch}, {"command", else_match.open_match.inner}, {"inner", else_match.inner}};
						}

						SearchClosedMatch last_if_match = search_closed_match(input, statement_match.regex_delimiter, regex_condition_open, regex_condition_close, condition_match);

						condition_result["children"] += {{"type", Parser::Type::ConditionBranch}, {"command", last_if_match.open_match.inner}, {"inner", last_if_match.inner}};

						current_position = last_if_match.end_position;
						result += condition_result;
					}

					break;
				}
				case Parser::Delimiter::Expression: {
					result += {{"type", Parser::Type::Variable}, {"command", statement_match.inner}};
					break;
				}
				case Parser::Delimiter::Comment: {
					result += {{"type", Parser::Type::Comment}, {"text", statement_match.inner}};
					break;
				}
				default: {
					throw std::runtime_error("Parser error: Unknown delimiter.");
					break;
				}
			}

			statement_match = search(input, regex_pattern_delimiters, current_position);
		}
		if (current_position < input.length()) {
			result += {{"type", Parser::Type::String}, {"text", input.substr(current_position)}};
		}

		return result;
	}

	json parse_tree(json current_element) {
		if (current_element.find("inner") != current_element.end()) {
			current_element["children"] = parse_level(current_element["inner"]);
			current_element.erase("inner");
		}
		if (current_element.find("children") != current_element.end()) {
			for (auto& child: current_element["children"]) {
				child = parse_tree(child);
			}
		}
		return current_element;
	}

	json parse(string input) {
		return parse_tree({{"inner", input}})["children"];
	}


	json parse_variable(string input, json data) {
		return parse_variable(input, data, true);
	}

	json parse_variable(string input, json data, bool throw_error) {
		// Json Raw Data
		if ( json::accept(input) ) {
			return json::parse(input);
		}

		// TODO Implement filter and functions

		if (input[0] != '/') input.insert(0, "/");

		json::json_pointer ptr(input);
		json result = data[ptr];

		if (throw_error && result.is_null()) throw std::runtime_error("JSON pointer found no element.");
		return result;
	}

	bool parse_condition(string condition, json data) {
		const std::regex regex_condition_not("not (.+)");
		const std::regex regex_condition_equal("(.+) == (.+)");
		const std::regex regex_condition_greater("(.+) > (.+)");
		const std::regex regex_condition_less("(.+) < (.+)");
		const std::regex regex_condition_greater_equal("(.+) >= (.+)");
		const std::regex regex_condition_less_equal("(.+) <= (.+)");
		const std::regex regex_condition_different("(.+) != (.+)");
		const std::regex regex_condition_in("(.+) in (.+)");

		std::smatch match_condition;
		if (std::regex_match(condition, match_condition, regex_condition_not)) {
			return !parse_condition(match_condition.str(1), data);
		}
		else if (std::regex_match(condition, match_condition, regex_condition_equal)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 == comp2;
		}
		else if (std::regex_match(condition, match_condition, regex_condition_greater)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 > comp2;
		}
		else if (std::regex_match(condition, match_condition, regex_condition_less)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 < comp2;
		}
		else if (std::regex_match(condition, match_condition, regex_condition_greater_equal)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 >= comp2;
		}
		else if (std::regex_match(condition, match_condition, regex_condition_less_equal)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 <= comp2;
		}
		else if (std::regex_match(condition, match_condition, regex_condition_different)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 != comp2;
		}
		else if (std::regex_match(condition, match_condition, regex_condition_in)) {
			json item = parse_variable(match_condition.str(1), data);
			json list = parse_variable(match_condition.str(2), data);
			return (std::find(list.begin(), list.end(), item) != list.end());
		}

		json var = parse_variable(condition, data, false);
		if (var.empty()) return false;
		else if (var.is_boolean()) return var;
		else if (var.is_number()) return (var != 0);
		else if (var.is_string()) return (var != "");
		return false;
	}

	string render_json(json data) {
		if (data.is_string()) {
			return data;
		}
		else {
			std::stringstream ss;
			ss << data;
			return ss.str();
		}
	}

	string render_tree(json input, json data, string path) {
		string result = "";
		for (auto element: input) {
			switch ( (Parser::Type) element["type"] ) {
    		case Parser::Type::String: {
					result += element["text"];
          break;
				}
    		case Parser::Type::Variable: {
					json variable = parse_variable(element["command"], data);
					result += render_json(variable);
          break;
				}
				case Parser::Type::Include: {
					result += render_template(path + element["filename"].get<string>(), data);
					break;
				}
				case Parser::Type::Loop: {
					const std::regex regex_loop_list("for (\\w+) in (.+)");

					string command = element["command"].get<string>();
					std::smatch match_command;
					if (std::regex_match(command, match_command, regex_loop_list)) {
						string item_name = match_command.str(1);
						string list_name = match_command.str(2);

						json list = parse_variable(list_name, data);
						for (int i = 0; i < list.size(); i++) {
							json data_loop = data;
							data_loop[item_name] = list[i];
							data_loop["index"] = i;
							data_loop["index1"] = i + 1;
							data_loop["is_first"] = (i == 0);
							data_loop["is_last"] = (i == list.size() - 1);
							result += render_tree(element["children"], data_loop, path);
						}
					}
					else {
						throw std::runtime_error("Unknown loop in renderer.");
					}
					break;
				}
				case Parser::Type::Condition: {
					const std::regex regex_condition("(if|else if|else) ?(.*)");

					for (auto branch: element["children"]) {
						string command = branch["command"].get<string>();
						std::smatch match_command;
						if (std::regex_match(command, match_command, regex_condition)) {
							string condition_type = match_command.str(1);
							string condition = match_command.str(2);

							if (parse_condition(condition, data) || condition_type == "else") {
								result += render_tree(branch["children"], data, path);
								break;
							}
						}
						else {
							throw std::runtime_error("Unknown condition in renderer.");
						}
					}
					break;
				}
				case Parser::Type::Comment: {
					break;
				}
				default: {
					throw std::runtime_error("Unknown type in renderer.");
				}
		 	}
		}
		return result;
	}

	string render(string input, json data, string path) {
		json parsed = parse(input);
		return render_tree(parsed, data, path);
	}


	string render(string input, json data) {
		return render(input, data, "./");
	}

	string render_template(string filename, json data) {
		string text = load_file(filename);
		string path = filename.substr(0, filename.find_last_of("/\\") + 1);
		return render(text, data, path);
	}

	string render_template_with_json_file(string filename, string filename_data) {
		json data = load_json(filename_data);
		return render_template(filename, data);
	}

	string load_file(string filename) {
		std::ifstream file(global_path + filename);
		string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return text;
	}

	json load_json(string filename) {
		std::ifstream file(global_path + filename);
		json j;
		file >> j;
		return j;
	}
};

inline string render(string input, json data) {
	return Environment().render(input, data);
}

} // namespace inja

#endif // PANTOR_INJA_HPP
