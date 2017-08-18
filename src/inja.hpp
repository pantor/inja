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


template<typename S, typename T>
inline std::vector<T> get_values(std::map<S, T> map) {
	std::vector<T> result;
	for (auto const element: map) { result.push_back(element.second); }
  return result;
}


class Regex: public std::regex {
	std::string pattern_;

public:
	Regex(): std::regex() {}
	Regex(std::string pattern): std::regex(pattern), pattern_(pattern) { }

	std::string pattern() { return pattern_; }
};


class Match: public std::smatch {
	size_t offset_ = 0;
	int group_offset_ = 0;
	Regex regex_;
	unsigned int regex_number_ = 0;

public:
	Match(): std::smatch() { }
	Match(size_t offset): std::smatch(), offset_(offset) { }
	Match(size_t offset, Regex regex): std::smatch(), offset_(offset), regex_(regex) { }

	void setGroupOffset(int group_offset) { group_offset_ = group_offset; }
	void setRegex(Regex regex) { regex_ = regex; }
	void setRegexNumber(unsigned int regex_number) { regex_number_ = regex_number; }

	size_t position() { return offset_ + std::smatch::position(); }
	size_t end_position() { return position() + length(); }
	bool found() { return not empty(); }
	std::string str() { return str(0); }
	std::string str(int i) { return std::smatch::str(i + group_offset_); }
	Regex regex() { return regex_; }
	unsigned int regex_number() { return regex_number_; }
};


class MatchClosed {
public:
	Match open_match, close_match;
	std::string inner, outer;

	MatchClosed() { }
	MatchClosed(std::string input, Match open_match, Match close_match): open_match(open_match), close_match(close_match) {
		outer = input.substr(position(), length());
		inner = input.substr(open_match.end_position(), close_match.position() - open_match.end_position());
	}

	size_t position() { return open_match.position(); }
	size_t end_position() { return close_match.end_position(); }
	int length() { return close_match.end_position() - open_match.position(); }
	bool found() { return open_match.found() and close_match.found(); }
	std::string prefix() { return open_match.prefix(); }
	std::string suffix() { return close_match.suffix(); }
};


inline Match search(const std::string& input, Regex regex, size_t position) {
	if (position >= input.length()) { return Match(); }

	Match match{position, regex};
	std::regex_search(input.cbegin() + position, input.cend(), match, regex);
	return match;
}

inline Match search(const std::string& input, std::vector<Regex> regexes, size_t position) {
	// Regex or join
	std::stringstream ss;
	for (size_t i = 0; i < regexes.size(); ++i)
	{
		if (i != 0) { ss << ")|("; }
		ss << regexes[i].pattern();
	}
	Regex regex{"(" + ss.str() + ")"};

	Match search_match = search(input, regex, position);
	if (not search_match.found()) { return Match(); }

	// Vector of id vs groups
	// 0: 1, 1: 2, 2: 3, 3: 2
	// 0 1 1 2 2 2 3 3
	std::vector<int> regex_mark_counts;
	for (int i = 0; i < regexes.size(); i++) {
		for (int j = 0; j < regexes[i].mark_count() + 1; j++) {
			regex_mark_counts.push_back(i);
		}
	}

	int number_regex = 0, number_inner = 1;
	for (int i = 1; i < search_match.size(); i++) {
		if (search_match.length(i) > 0) {
			number_inner = i;
			number_regex = regex_mark_counts[i];
			break;
		}
	}

	search_match.setGroupOffset(number_inner);
	search_match.setRegexNumber(number_regex);
	search_match.setRegex(regexes[number_regex]);
	return search_match;
}

inline MatchClosed search_closed_match_on_level(const std::string& input, Regex regex_statement, Regex regex_level_up, Regex regex_level_down, Regex regex_search, Match open_match) {

	int level = 0;
	size_t current_position = open_match.end_position();
	Match match_delimiter = search(input, regex_statement, current_position);
	while (match_delimiter.found()) {
		current_position = match_delimiter.end_position();

		std::string inner = match_delimiter.str(1);
		if (std::regex_match(inner, regex_search) and level == 0) { break; }
		if (std::regex_match(inner, regex_level_up)) { level += 1; }
		else if (std::regex_match(inner, regex_level_down)) { level -= 1; }

		match_delimiter = search(input, regex_statement, current_position);
	}

	return MatchClosed(input, open_match, match_delimiter);
}

inline MatchClosed search_closed_match(std::string input, Regex regex_statement, Regex regex_open, Regex regex_close, Match open_match) {
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

	enum class Type {
		String,
		Loop,
		Condition,
		ConditionBranch,
		Include,
		Comment,
		Variable
	};

	std::map<Delimiter, Regex> regex_map_delimiters = {
		{Delimiter::Statement, Regex{"\\{\\%\\s*(.+?)\\s*\\%\\}"}},
		{Delimiter::LineStatement, Regex{"(?:^|\\n)##\\s*(.+)\\s*"}},
		{Delimiter::Expression, Regex{"\\{\\{\\s*(.+?)\\s*\\}\\}"}},
		{Delimiter::Comment, Regex{"\\{#\\s*(.*?)\\s*#\\}"}}
	};

	const Regex regex_loop_open{"for (.*)"};
	const Regex regex_loop_close{"endfor"};

	const Regex regex_include{"include \"(.*)\""};

	const Regex regex_condition_open{"if (.*)"};
	const Regex regex_condition_else_if{"else if (.*)"};
	const Regex regex_condition_else{"else"};
	const Regex regex_condition_close{"endif"};

	const Regex regex_condition_not{"not (.+)"};
	const Regex regex_condition_and{"(.+) and (.+)"};
	const Regex regex_condition_or{"(.+) or (.+)"};
	const Regex regex_condition_in{"(.+) in (.+)"};
	const Regex regex_condition_equal{"(.+) == (.+)"};
	const Regex regex_condition_greater{"(.+) > (.+)"};
	const Regex regex_condition_less{"(.+) < (.+)"};
	const Regex regex_condition_greater_equal{"(.+) >= (.+)"};
	const Regex regex_condition_less_equal{"(.+) <= (.+)"};
	const Regex regex_condition_different{"(.+) != (.+)"};

	const Regex regex_function_range{"range\\(\\s*(.*?)\\s*\\)"};
	const Regex regex_function_upper{"upper\\(\\s*(.*?)\\s*\\)"};
	const Regex regex_function_lower{"lower\\(\\s*(.*?)\\s*\\)"};

	Parser() { }

	json element(Type type, json element_data) {
		element_data["type"] = type;
		return element_data;
	}

	json parse_level(std::string input) {
		json result;

		std::vector<Regex> regex_delimiters = get_values(regex_map_delimiters);

		size_t current_position = 0;
		Match match_delimiter = search(input, regex_delimiters, current_position);
		while (match_delimiter.found()) {
			current_position = match_delimiter.end_position();
			std::string string_prefix = match_delimiter.prefix();
			if (not string_prefix.empty()) {
				result += element(Type::String, {{"text", string_prefix}});
			}

			std::string delimiter_inner = match_delimiter.str(1);
			switch ( static_cast<Delimiter>(match_delimiter.regex_number()) ) {
				case Delimiter::Statement:
				case Delimiter::LineStatement: {

					Match inner_match_delimiter;
					// Loop
					if (std::regex_match(delimiter_inner, inner_match_delimiter, regex_loop_open)) {
						MatchClosed loop_match = search_closed_match(input, match_delimiter.regex(), regex_loop_open, regex_loop_close, match_delimiter);

						current_position = loop_match.end_position();
						std::string loop_command = inner_match_delimiter.str(0);
						result += element(Type::Loop, {{"command", loop_command}, {"inner", loop_match.inner}});
					}
					// Include
					else if (std::regex_match(delimiter_inner, inner_match_delimiter, regex_include)) {
						std::string filename = inner_match_delimiter.str(1);
						result += element(Type::Include, {{"filename", filename}});
					}
					// Condition
					else if (std::regex_match(delimiter_inner, inner_match_delimiter, regex_condition_open)) {
						json condition_result = element(Parser::Type::Condition, {{"children", json::array()}});

						Match condition_match = match_delimiter;

						MatchClosed else_if_match = search_closed_match_on_level(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, regex_condition_else_if, condition_match);
						while (else_if_match.found()) {
							condition_match = else_if_match.close_match;

							condition_result["children"] += element(Type::ConditionBranch, {{"command", else_if_match.open_match.str(1)}, {"inner", else_if_match.inner}});

							else_if_match = search_closed_match_on_level(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, regex_condition_else_if, condition_match);
						}

						MatchClosed else_match = search_closed_match_on_level(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, regex_condition_else, condition_match);
						if (else_match.found()) {
							condition_match = else_match.close_match;

							condition_result["children"] += element(Type::ConditionBranch, {{"command", else_match.open_match.str(1)}, {"inner", else_match.inner}});
						}

						MatchClosed last_if_match = search_closed_match(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, condition_match);

						condition_result["children"] += element(Type::ConditionBranch, {{"command", last_if_match.open_match.str(1)}, {"inner", last_if_match.inner}});

						current_position = last_if_match.end_position();
						result += condition_result;
					}

					break;
				}
				case Delimiter::Expression: {
					result += element(Type::Variable, {{"command", delimiter_inner}});
					break;
				}
				case Delimiter::Comment: {
					result += element(Type::Comment, {{"text", delimiter_inner}});
					break;
				}
				default: {
					throw std::runtime_error("Parser error: Unknown delimiter.");
				}
			}

			match_delimiter = search(input, regex_delimiters, current_position);
		}
		if (current_position < input.length()) {
			result += element(Parser::Type::String, {{"text", input.substr(current_position)}});
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

	json parse(std::string input) {
		return parse_tree({{"inner", input}})["children"];
	}
};



class Environment {
	std::string global_path;

	Parser parser;


public:
	Environment(): Environment("./") { }
	Environment(std::string global_path): global_path(global_path), parser() { }

	void setExpression(std::string open, std::string close) {
		parser.regex_map_delimiters[Parser::Delimiter::Expression] = Regex{open + close};
	}

	json parse_variable(std::string input, json data) {
		return parse_variable(input, data, true);
	}

	json parse_variable(std::string input, json data, bool throw_error) {
		// Json Raw Data
		if ( json::accept(input) ) { return json::parse(input); }

		// TODO Implement functions


		if (input[0] != '/') { input.insert(0, "/"); }
		json::json_pointer ptr(input);
		json result = data[ptr];

		if (throw_error && result.is_null()) { throw std::runtime_error("JSON pointer found no element."); }
		return result;
	}

	bool parse_condition(std::string condition, json data) {
		Match match_condition;
		if (std::regex_match(condition, match_condition, parser.regex_condition_not)) {
			return not parse_condition(match_condition.str(1), data);
		}
		else if (std::regex_match(condition, match_condition, parser.regex_condition_and)) {
			return (parse_condition(match_condition.str(1), data) and parse_condition(match_condition.str(2), data));
		}
		else if (std::regex_match(condition, match_condition, parser.regex_condition_or)) {
			return (parse_condition(match_condition.str(1), data) or parse_condition(match_condition.str(2), data));
		}
		else if (std::regex_match(condition, match_condition, parser.regex_condition_in)) {
			json item = parse_variable(match_condition.str(1), data);
			json list = parse_variable(match_condition.str(2), data);
			return (std::find(list.begin(), list.end(), item) != list.end());
		}
		else if (std::regex_match(condition, match_condition, parser.regex_condition_equal)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 == comp2;
		}
		else if (std::regex_match(condition, match_condition, parser.regex_condition_greater)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 > comp2;
		}
		else if (std::regex_match(condition, match_condition, parser.regex_condition_less)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 < comp2;
		}
		else if (std::regex_match(condition, match_condition, parser.regex_condition_greater_equal)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 >= comp2;
		}
		else if (std::regex_match(condition, match_condition, parser.regex_condition_less_equal)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 <= comp2;
		}
		else if (std::regex_match(condition, match_condition, parser.regex_condition_different)) {
			json comp1 = parse_variable(match_condition.str(1), data);
			json comp2 = parse_variable(match_condition.str(2), data);
			return comp1 != comp2;
		}

		json var = parse_variable(condition, data, false);
		if (var.empty()) { return false; }
		else if (var.is_boolean()) { return var; }
		else if (var.is_number()) { return (var != 0); }
		else if (var.is_string()) { return not var.empty(); }
		return false;
	}

	std::string render_json(json data) {
		if (data.is_string()) { return data; }

		std::stringstream ss;
		ss << data;
		return ss.str();
	}

	std::string render_tree(json input, json data, std::string path) {
		std::string result = "";
		for (auto element: input) {
			switch ( static_cast<Parser::Type>(element["type"]) ) {
    		case Parser::Type::String: {
					result += element["text"].get<std::string>();
          break;
				}
    		case Parser::Type::Variable: {
					json variable = parse_variable(element["command"], data);
					result += render_json(variable);
          break;
				}
				case Parser::Type::Include: {
					result += render_template(path + element["filename"].get<std::string>(), data);
					break;
				}
				case Parser::Type::Loop: {
					const Regex regex_loop_list{"for (\\w+) in (.+)"};

					std::string command = element["command"].get<std::string>();
					Match match_command;
					if (std::regex_match(command, match_command, regex_loop_list)) {
						std::string item_name = match_command.str(1);
						std::string list_name = match_command.str(2);

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
					const Regex regex_condition{"(if|else if|else) ?(.*)"};

					for (auto branch: element["children"]) {
						std::string command = branch["command"].get<std::string>();
						Match match_command;
						if (std::regex_match(command, match_command, regex_condition)) {
							std::string condition_type = match_command.str(1);
							std::string condition = match_command.str(2);

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

	std::string render(std::string input, json data, std::string path) {
		json parsed = parser.parse(input);
		return render_tree(parsed, data, path);
	}


	std::string render(std::string input, json data) {
		return render(input, data, "./");
	}

	std::string render_template(std::string filename, json data) {
		std::string text = load_file(filename);
		std::string path = filename.substr(0, filename.find_last_of("/\\") + 1);
		return render(text, data, path);
	}

	std::string render_template_with_json_file(std::string filename, std::string filename_data) {
		json data = load_json(filename_data);
		return render_template(filename, data);
	}

	std::string load_file(std::string filename) {
		std::ifstream file(global_path + filename);
		std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return text;
	}

	json load_json(std::string filename) {
		std::ifstream file(global_path + filename);
		json j;
		file >> j;
		return j;
	}
};

inline std::string render(std::string input, json data) {
	return Environment().render(input, data);
}

} // namespace inja

#endif // PANTOR_INJA_HPP
