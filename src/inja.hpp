#ifndef PANTOR_INJA_HPP
#define PANTOR_INJA_HPP

#ifndef NLOHMANN_JSON_HPP
	static_assert(false, "nlohmann/json not found.");
#endif


#include <iostream>
#include <fstream>
#include <string>
#include <locale>
#include <regex>
#include <type_traits>


namespace inja {

using json = nlohmann::json;

/*!
@brief returns the values of a std key-value-map
*/
template<typename S, typename T>
inline std::vector<T> get_values(std::map<S, T> map) {
	std::vector<T> result;
	for (auto const element: map) { result.push_back(element.second); }
  return result;
}


/*!
@brief dot notation to json pointer notiation
*/
inline std::string dot_to_json_pointer_notation(std::string dot) {
	std::string result = dot;
	while (result.find(".") != std::string::npos) {
		result.replace(result.find("."), 1, "/");
	}
	result.insert(0, "/");
	return result;
}


enum class ElementNotation {
	Pointer,
	Dot
};


class Regex: public std::regex {
	std::string pattern_;

public:
	Regex(): std::regex() {}
	explicit Regex(const std::string& pattern): std::regex(pattern), pattern_(pattern) { }

	std::string pattern() { return pattern_; }
};


class Match: public std::smatch {
	size_t offset_ = 0;
	int group_offset_ = 0;
	Regex regex_;
	unsigned int regex_number_ = 0;

public:
	Match(): std::smatch() { }
	explicit Match(size_t offset): std::smatch(), offset_(offset) { }
	Match(size_t offset, Regex& regex): std::smatch(), offset_(offset), regex_(regex) { }

	void setGroupOffset(int group_offset) { group_offset_ = group_offset; }
	void setRegex(Regex regex) { regex_ = regex; }
	void setRegexNumber(unsigned int regex_number) { regex_number_ = regex_number; }

	size_t position() const { return offset_ + std::smatch::position(); }
	size_t end_position() const { return position() + length(); }
	bool found() const { return not empty(); }
	const std::string str() const { return str(0); }
	const std::string str(int i) const { return std::smatch::str(i + group_offset_); }
	Regex regex() const { return regex_; }
	unsigned int regex_number() const { return regex_number_; }
};


class MatchClosed {
public:
	Match open_match, close_match;

	MatchClosed() { }
	MatchClosed(Match& open_match, Match& close_match): open_match(open_match), close_match(close_match) { }

	size_t position() const { return open_match.position(); }
	size_t end_position() const { return close_match.end_position(); }
	int length() const { return close_match.end_position() - open_match.position(); }
	bool found() const { return open_match.found() and close_match.found(); }
	std::string prefix() const { return open_match.prefix(); }
	std::string suffix() const { return close_match.suffix(); }
	std::string outer() const { return open_match.str() + static_cast<std::string>(open_match.suffix()).substr(0, close_match.end_position() - open_match.end_position()); }
	std::string inner() const { return static_cast<std::string>(open_match.suffix()).substr(0, close_match.position() - open_match.end_position()); }
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
	std::vector<int> regex_mark_counts;
	for (int i = 0; i < regexes.size(); i++) {
		for (int j = 0; j < regexes[i].mark_count() + 1; j++) {
			regex_mark_counts.push_back(i);
		}
	}

	int number_regex = -1, number_inner = 1;
	for (int i = 1; i < search_match.size(); i++) {
		if (search_match.length(i) > 0) {
			number_inner = i;
			number_regex = regex_mark_counts[i];
			break;
		}
	}

	search_match.setGroupOffset(number_inner);
	search_match.setRegexNumber(number_regex);
	if (number_regex >= 0) { search_match.setRegex(regexes[number_regex]); }
	return search_match;
}

inline MatchClosed search_closed_on_level(const std::string& input, const Regex regex_statement, const Regex regex_level_up, const Regex regex_level_down, const Regex regex_search, Match open_match) {

	int level = 0;
	size_t current_position = open_match.end_position();
	Match match_delimiter = search(input, regex_statement, current_position);
	while (match_delimiter.found()) {
		current_position = match_delimiter.end_position();

		const std::string inner = match_delimiter.str(1);
		if (std::regex_match(inner, regex_search) and level == 0) { break; }
		if (std::regex_match(inner, regex_level_up)) { level += 1; }
		else if (std::regex_match(inner, regex_level_down)) { level -= 1; }

		match_delimiter = search(input, regex_statement, current_position);
	}

	return MatchClosed(open_match, match_delimiter);
}

inline MatchClosed search_closed(const std::string& input, const Regex regex_statement, const Regex regex_open, const Regex regex_close, Match open_match) {
	return search_closed_on_level(input, regex_statement, regex_open, regex_close, regex_close, open_match);
}

inline Match match(const std::string& input, std::vector<Regex> regexes) {
	Match match;
	match.setRegexNumber(-1);
	for (int i = 0; i < regexes.size(); i++) {
		if (std::regex_match(input, match, regexes[i])) {
			match.setRegexNumber(i);
			match.setRegex(regexes[i]);
			break;
		}
	}
	return match;
}



struct Parsed {
	enum class Type {
		String,
		Loop,
		Condition,
		ConditionBranch,
		Comment,
		Expression
	};

	enum class Delimiter {
		Statement,
		LineStatement,
		Expression,
		Comment
	};

	enum class Statement {
		Loop,
		Condition,
		Include
	};

	enum class ConditionOperators {
		Not,
		And,
		Or,
		In,
		Equal,
		Greater,
		Less,
		GreaterEqual,
		LessEqual,
		Different
	};

	enum class Function {
		Upper,
		Lower,
		Range,
		Length,
		Round,
		DivisibleBy,
		Odd,
		Even
	};
};



class Template {
public:
	const json parsed_template;
	ElementNotation elementNotation;

	explicit Template(const json parsed_template): parsed_template(parsed_template) { }
	explicit Template(const json parsed_template, ElementNotation elementNotation): parsed_template(parsed_template), elementNotation(elementNotation) { }

	template<typename T = json>
	T eval_variable(json element, json data) {
		const json var = eval_variable(element, data, true);
		if (std::is_same<T, json>::value) { return var; }
		return var.get<T>();
	}

	json eval_variable(json element, json data, bool throw_error) {
		if (element.find("function") != element.end()) {
			switch ( static_cast<Parsed::Function>(element["function"]) ) {
				case Parsed::Function::Upper: {
					std::string str = eval_variable<std::string>(element["arg1"], data);
					std::transform(str.begin(), str.end(), str.begin(), toupper);
					return str;
				}
				case Parsed::Function::Lower: {
					std::string str = eval_variable<std::string>(element["arg1"], data);
					std::transform(str.begin(), str.end(), str.begin(), tolower);
					return str;
				}
				case Parsed::Function::Range: {
					const int number = eval_variable<int>(element["arg1"], data);
					std::vector<int> result(number);
					std::iota(std::begin(result), std::end(result), 0);
					return result;
				}
				case Parsed::Function::Length: {
					const std::vector<json> list = eval_variable<std::vector<json>>(element["arg1"], data);
					return list.size();
				}
				case Parsed::Function::Round: {
					const double number = eval_variable<double>(element["arg1"], data);
					const int precision = eval_variable<int>(element["arg2"], data);
					return std::round(number * std::pow(10.0, precision)) / std::pow(10.0, precision);
				}
				case Parsed::Function::DivisibleBy: {
					const int number = eval_variable<int>(element["arg1"], data);
					const int divisor = eval_variable<int>(element["arg2"], data);
					return (number % divisor == 0);
				}
				case Parsed::Function::Odd: {
					const int number = eval_variable<int>(element["arg1"], data);
					return (number % 2 != 0);
				}
				case Parsed::Function::Even: {
					const int number = eval_variable<int>(element["arg1"], data);
					return (number % 2 == 0);
				}
			}
		}

		const std::string input = element["command"];

		// Json Raw Data
		if ( json::accept(input) ) { return json::parse(input); }

		std::string input_copy = input;
		switch (elementNotation) {
			case ElementNotation::Pointer: {
				if (input_copy[0] != '/') { input_copy.insert(0, "/"); }
				break;
			}
			case ElementNotation::Dot: {
				input_copy = dot_to_json_pointer_notation(input_copy);
				break;
			}
		}

		json::json_pointer ptr(input_copy);
		json result = data[ptr];

		if (throw_error && result.is_null()) { throw std::runtime_error("JSON pointer found no element."); }
		return result;
	}

	bool eval_condition(json element, json data) {
		if (element.find("condition") != element.end()) {
			switch ( static_cast<Parsed::ConditionOperators>(element["condition"]) ) {
				case Parsed::ConditionOperators::Not: {
					return not eval_condition(element["arg1"], data);
				}
				case Parsed::ConditionOperators::And: {
					return (eval_condition(element["arg1"], data) and eval_condition(element["arg2"], data));
				}
				case Parsed::ConditionOperators::Or: {
					return (eval_condition(element["arg1"], data) or eval_condition(element["arg2"], data));
				}
				case Parsed::ConditionOperators::In: {
					const json item = eval_variable(element["arg1"], data);
					const json list = eval_variable(element["arg2"], data);
					return (std::find(list.begin(), list.end(), item) != list.end());
				}
				case Parsed::ConditionOperators::Equal: {
					return eval_variable(element["arg1"], data) == eval_variable(element["arg2"], data);
				}
				case Parsed::ConditionOperators::Greater: {
					return eval_variable(element["arg1"], data) > eval_variable(element["arg2"], data);
				}
				case Parsed::ConditionOperators::Less: {
					return eval_variable(element["arg1"], data) < eval_variable(element["arg2"], data);
				}
				case Parsed::ConditionOperators::GreaterEqual: {
					return eval_variable(element["arg1"], data) >= eval_variable(element["arg2"], data);
				}
				case Parsed::ConditionOperators::LessEqual: {
					return eval_variable(element["arg1"], data) <= eval_variable(element["arg2"], data);
				}
				case Parsed::ConditionOperators::Different: {
					return eval_variable(element["arg1"], data) != eval_variable(element["arg2"], data);
				}
			}
		}

		const json var = eval_variable(element, data, false);
		if (var.empty()) { return false; }
		else if (var.is_boolean()) { return var; }
		else if (var.is_number()) { return (var != 0); }
		else if (var.is_string()) { return not var.empty(); }
		return true;
	}

	std::string render(json data) {
		std::string result = "";
		for (auto element: parsed_template) {
			switch ( static_cast<Parsed::Type>(element["type"]) ) {
    		case Parsed::Type::String: {
					result += element["text"].get<std::string>();
          break;
				}
    		case Parsed::Type::Expression: {
					json variable = eval_variable(element, data);
					if (variable.is_string()) {
						result += variable.get<std::string>();
					} else {
						std::stringstream ss;
						ss << variable;
						result += ss.str();
					}
          break;
				}
				case Parsed::Type::Loop: {
					const std::string item_name = element["item"].get<std::string>();

					std::vector<json> list = eval_variable<std::vector<json>>(element["list"], data);
					for (int i = 0; i < list.size(); i++) {
						json data_loop = data;
						data_loop[item_name] = list[i];
						data_loop["index"] = i;
						data_loop["index1"] = i + 1;
						data_loop["is_first"] = (i == 0);
						data_loop["is_last"] = (i == list.size() - 1);
						result += Template(element["children"], elementNotation).render(data_loop);
					}
					break;
				}
				case Parsed::Type::Condition: {
					for (auto branch: element["children"]) {
						if (eval_condition(branch["condition"], data) || branch["condition_type"] == "else") {
							result += Template(branch["children"], elementNotation).render(data);
							break;
						}
					}
					break;
				}
				case Parsed::Type::Comment: { break; }
				default: { throw std::runtime_error("Unknown type in renderer."); }
		 	}
		}
		return result;
	}
};


class Parser {
public:
	std::map<Parsed::Delimiter, Regex> regex_map_delimiters = {
		{Parsed::Delimiter::Statement, Regex{"\\{\\%\\s*(.+?)\\s*\\%\\}"}},
		{Parsed::Delimiter::LineStatement, Regex{"(?:^|\\n)##\\s*(.+)\\s*"}},
		{Parsed::Delimiter::Expression, Regex{"\\{\\{\\s*(.+?)\\s*\\}\\}"}},
		{Parsed::Delimiter::Comment, Regex{"\\{#\\s*(.*?)\\s*#\\}"}}
	};

	const std::map<Parsed::Statement, Regex> regex_map_statement_openers = {
		{Parsed::Statement::Loop, Regex{"for (.*)"}},
		{Parsed::Statement::Condition, Regex{"if (.*)"}},
		{Parsed::Statement::Include, Regex{"include \"(.*)\""}}
	};

	const Regex regex_loop_open = regex_map_statement_openers.at(Parsed::Statement::Loop);
	const Regex regex_loop_in_list{"for (\\w+) in (.+)"};
	const Regex regex_loop_close{"endfor"};

	const Regex regex_condition_open = regex_map_statement_openers.at(Parsed::Statement::Condition);
	const Regex regex_condition_else_if{"else if (.*)"};
	const Regex regex_condition_else{"else"};
	const Regex regex_condition_close{"endif"};

	const std::map<Parsed::ConditionOperators, Regex> regex_map_condition_operators = {
		{Parsed::ConditionOperators::Not, Regex{"not (.+)"}},
		{Parsed::ConditionOperators::And, Regex{"(.+) and (.+)"}},
		{Parsed::ConditionOperators::Or, Regex{"(.+) or (.+)"}},
		{Parsed::ConditionOperators::In, Regex{"(.+) in (.+)"}},
		{Parsed::ConditionOperators::Equal, Regex{"(.+) == (.+)"}},
		{Parsed::ConditionOperators::Greater, Regex{"(.+) > (.+)"}},
		{Parsed::ConditionOperators::Less, Regex{"(.+) < (.+)"}},
		{Parsed::ConditionOperators::GreaterEqual, Regex{"(.+) >= (.+)"}},
		{Parsed::ConditionOperators::LessEqual, Regex{"(.+) <= (.+)"}},
		{Parsed::ConditionOperators::Different, Regex{"(.+) != (.+)"}}
	};

	const std::map<Parsed::Function, Regex> regex_map_functions = {
		{Parsed::Function::Upper, Regex{"upper\\(\\s*(.*?)\\s*\\)"}},
		{Parsed::Function::Lower, Regex{"lower\\(\\s*(.*?)\\s*\\)"}},
		{Parsed::Function::Range, Regex{"range\\(\\s*(.*?)\\s*\\)"}},
		{Parsed::Function::Length, Regex{"length\\(\\s*(.*?)\\s*\\)"}},
		{Parsed::Function::Round, Regex{"round\\(\\s*(.*?)\\s*,\\s*(.*?)\\s*\\)"}},
		{Parsed::Function::DivisibleBy, Regex{"divisibleBy\\(\\s*(.*?)\\s*,\\s*(.*?)\\s*\\)"}},
		{Parsed::Function::Odd, Regex{"odd\\(\\s*(.*?)\\s*\\)"}},
		{Parsed::Function::Even, Regex{"even\\(\\s*(.*?)\\s*\\)"}}
	};

	Parser() { }

	json element(Parsed::Type type, json element_data) {
		element_data["type"] = type;
		return element_data;
	}

	json element_function(Parsed::Function func, int number_args, Match match) {
		json result = element(Parsed::Type::Expression, {{"function", func}});
		for (int i = 1; i < number_args + 1; i++) {
			result["arg" + std::to_string(i)] = parse_expression(match.str(i));
		}
		return result;
	}

	json element_condition(Parsed::ConditionOperators op, int number_args, Match match) {
		json result = element(Parsed::Type::Expression, {{"condition", op}});
		for (int i = 1; i < number_args + 1; i++) {
			result["arg" + std::to_string(i)] = parse_expression(match.str(i));
		}
		return result;
	}

	json parse_expression(const std::string& input) {
		Match match_function = match(input, get_values(regex_map_functions));
		switch ( static_cast<Parsed::Function>(match_function.regex_number()) ) {
			case Parsed::Function::Upper: {
				return element_function(Parsed::Function::Upper, 1, match_function);
			}
			case Parsed::Function::Lower: {
				return element_function(Parsed::Function::Lower, 1, match_function);
			}
			case Parsed::Function::Range: {
				return element_function(Parsed::Function::Range, 1, match_function);
			}
			case Parsed::Function::Length: {
				return element_function(Parsed::Function::Length, 1, match_function);
			}
			case Parsed::Function::Round: {
				return element_function(Parsed::Function::Round, 2, match_function);
			}
			case Parsed::Function::DivisibleBy: {
				return element_function(Parsed::Function::DivisibleBy, 2, match_function);
			}
			case Parsed::Function::Odd: {
				return element_function(Parsed::Function::Odd, 1, match_function);
			}
			case Parsed::Function::Even: {
				return element_function(Parsed::Function::Even, 1, match_function);
			}
		}

		return element(Parsed::Type::Expression, {{"command", input}});
	}

	json parse_condition(const std::string& input) {
		Match match_condition = match(input, get_values(regex_map_condition_operators));

		switch ( static_cast<Parsed::ConditionOperators>(match_condition.regex_number()) ) {
			case Parsed::ConditionOperators::Not: {
				return element_condition(Parsed::ConditionOperators::Not, 1, match_condition);
			}
			case Parsed::ConditionOperators::And: {
				return element_condition(Parsed::ConditionOperators::And, 2, match_condition);
			}
			case Parsed::ConditionOperators::Or: {
				return element_condition(Parsed::ConditionOperators::Or, 2, match_condition);
			}
			case Parsed::ConditionOperators::In: {
				return element_condition(Parsed::ConditionOperators::In, 2, match_condition);
			}
			case Parsed::ConditionOperators::Equal: {
				return element_condition(Parsed::ConditionOperators::Equal, 2, match_condition);
			}
			case Parsed::ConditionOperators::Greater: {
				return element_condition(Parsed::ConditionOperators::Greater, 2, match_condition);
			}
			case Parsed::ConditionOperators::Less: {
				return element_condition(Parsed::ConditionOperators::Less, 2, match_condition);
			}
			case Parsed::ConditionOperators::GreaterEqual: {
				return element_condition(Parsed::ConditionOperators::GreaterEqual, 2, match_condition);
			}
			case Parsed::ConditionOperators::LessEqual: {
				return element_condition(Parsed::ConditionOperators::LessEqual, 2, match_condition);
			}
			case Parsed::ConditionOperators::Different: {
				return element_condition(Parsed::ConditionOperators::Different, 2, match_condition);
			}
		}

		return element(Parsed::Type::Expression, {{"command", input}});
	}

	json parse_level(const std::string& input, const std::string& path) {
		json result;

		std::vector<Regex> regex_delimiters = get_values(regex_map_delimiters);

		size_t current_position = 0;
		Match match_delimiter = search(input, regex_delimiters, current_position);
		while (match_delimiter.found()) {
			current_position = match_delimiter.end_position();
			std::string string_prefix = match_delimiter.prefix();
			if (not string_prefix.empty()) {
				result += element(Parsed::Type::String, {{"text", string_prefix}});
			}

			std::string delimiter_inner = match_delimiter.str(1);

			switch ( static_cast<Parsed::Delimiter>(match_delimiter.regex_number()) ) {
				case Parsed::Delimiter::Statement:
				case Parsed::Delimiter::LineStatement: {

					Match match_statement = match(delimiter_inner, get_values(regex_map_statement_openers));
					switch ( static_cast<Parsed::Statement>(match_statement.regex_number()) ) {
						case Parsed::Statement::Loop: {
							MatchClosed loop_match = search_closed(input, match_delimiter.regex(), regex_loop_open, regex_loop_close, match_delimiter);

							current_position = loop_match.end_position();
							const std::string loop_command = match_statement.str(0);

							Match match_command;
							if (std::regex_match(loop_command, match_command, regex_loop_in_list)) {
								const std::string item_name = match_command.str(1);
								const std::string list_name = match_command.str(2);

								result += element(Parsed::Type::Loop, {{"item", item_name}, {"list", parse_expression(list_name)}, {"inner", loop_match.inner()}});
							} else {
								throw std::runtime_error("Parser error: Unknown loop command.");
							}

							break;
						}
						case Parsed::Statement::Condition: {
							json condition_result = element(Parsed::Type::Condition, {{"children", json::array()}});

							const Regex regex_condition{"(if|else if|else) ?(.*)"};

							Match condition_match = match_delimiter;

							MatchClosed else_if_match = search_closed_on_level(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, regex_condition_else_if, condition_match);
							while (else_if_match.found()) {
								condition_match = else_if_match.close_match;

								Match match_command;
								if (std::regex_match(else_if_match.open_match.str(1), match_command, regex_condition)) {
									condition_result["children"] += element(Parsed::Type::ConditionBranch, {{"inner", else_if_match.inner()}, {"condition_type", match_command.str(1)}, {"condition", parse_condition(match_command.str(2))}});
								}

								else_if_match = search_closed_on_level(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, regex_condition_else_if, condition_match);
							}

							MatchClosed else_match = search_closed_on_level(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, regex_condition_else, condition_match);
							if (else_match.found()) {
								condition_match = else_match.close_match;

								Match match_command;
								if (std::regex_match(else_match.open_match.str(1), match_command, regex_condition)) {
									condition_result["children"] += element(Parsed::Type::ConditionBranch, {{"inner", else_match.inner()}, {"condition_type", match_command.str(1)}, {"condition", parse_condition(match_command.str(2))}});
								}
							}

							MatchClosed last_if_match = search_closed(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, condition_match);

							Match match_command;
							if (std::regex_match(last_if_match.open_match.str(1), match_command, regex_condition)) {
								condition_result["children"] += element(Parsed::Type::ConditionBranch, {{"inner", last_if_match.inner()}, {"condition_type", match_command.str(1)}, {"condition", parse_condition(match_command.str(2))}});
							}

							current_position = last_if_match.end_position();
							result += condition_result;
							break;
						}
						case Parsed::Statement::Include: {
							std::string included_filename = path + match_statement.str(1);
							Template included_template = parse_template(included_filename);
							for (json element : included_template.parsed_template) {
								result += element;
							}
							break;
						}
						default: { throw std::runtime_error("Parser error: Unknown statement."); }
					}

					break;
				}
				case Parsed::Delimiter::Expression: {
					result += parse_expression(delimiter_inner);
					break;
				}
				case Parsed::Delimiter::Comment: {
					result += element(Parsed::Type::Comment, {{"text", delimiter_inner}});
					break;
				}
				default: { throw std::runtime_error("Parser error: Unknown delimiter."); }
			}

			match_delimiter = search(input, regex_delimiters, current_position);
		}
		if (current_position < input.length()) {
			result += element(Parsed::Type::String, {{"text", input.substr(current_position)}});
		}

		return result;
	}

	json parse_tree(json current_element, const std::string& path) {
		if (current_element.find("inner") != current_element.end()) {
			current_element["children"] = parse_level(current_element["inner"], path);
			current_element.erase("inner");
		}
		if (current_element.find("children") != current_element.end()) {
			for (auto& child: current_element["children"]) {
				child = parse_tree(child, path);
			}
		}
		return current_element;
	}

	Template parse(const std::string& input) {
		json parsed = parse_tree({{"inner", input}}, "./")["children"];
		return Template(parsed);
	}

	Template parse_template(const std::string& filename) {
		std::string text = load_file(filename);
		std::string path = filename.substr(0, filename.find_last_of("/\\") + 1);
		json parsed = parse_tree({{"inner", text}}, path)["children"];
		return Template(parsed);
	}

	std::string load_file(const std::string& filename) {
		std::ifstream file(filename);
		std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return text;
	}
};



class Environment {
	const std::string global_path;

	ElementNotation elementNotation = ElementNotation::Pointer;

	Parser parser;

public:
	Environment(): Environment("./") { }
	explicit Environment(const std::string& global_path): global_path(global_path), parser() { }

	void setStatement(const std::string& open, const std::string& close) {
		parser.regex_map_delimiters[Parsed::Delimiter::Statement] = Regex{open + "\\s*(.+?)\\s*" + close};
	}

	void setLineStatement(const std::string& open) {
		parser.regex_map_delimiters[Parsed::Delimiter::LineStatement] = Regex{"(?:^|\\n)" + open + "\\s*(.+)\\s*"};
	}

	void setExpression(const std::string& open, const std::string& close) {
		parser.regex_map_delimiters[Parsed::Delimiter::Expression] = Regex{open + "\\s*(.+?)\\s*" + close};
	}

	void setComment(const std::string& open, const std::string& close) {
		parser.regex_map_delimiters[Parsed::Delimiter::Comment] = Regex{open + "\\s*(.+?)\\s*" + close};
	}

	void setElementNotation(const ElementNotation elementNotation_) {
		elementNotation = elementNotation_;
	}

	std::string render(const std::string& input, json data) {
		Template parsed = parser.parse(input);
		parsed.elementNotation = elementNotation;
		return parsed.render(data);
	}

	std::string render_template(const std::string& filename, json data) {
		Template parsed = parser.parse_template(global_path + filename);
		parsed.elementNotation = elementNotation;
		return parsed.render(data);
	}

	std::string render_template_with_json_file(const std::string& filename, const std::string& filename_data) {
		json data = load_json(filename_data);
		return render_template(filename, data);
	}

	void write(const std::string& filename, json data, const std::string& filename_out) {
		std::ofstream file(global_path + filename_out);
		file << render_template_with_json_file(filename, data);
		file.close();
	}

	void write(const std::string& filename, const std::string& filename_data, const std::string& filename_out) {
		json data = load_json(filename_data);
		write(filename, data, filename_out);
	}

	std::string load_global_file(const std::string& filename) {
		return parser.load_file(global_path + filename);
	}

	json load_json(const std::string& filename) {
		std::ifstream file(global_path + filename);
		json j;
		file >> j;
		return j;
	}
};



/*!
@brief render with default settings
*/
inline std::string render(const std::string& input, json data) {
	return Environment().render(input, data);
}

} // namespace inja

#endif // PANTOR_INJA_HPP
