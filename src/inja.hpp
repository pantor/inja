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



/*!
@brief inja regex class, saves string pattern in addition to std::regex
*/
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
	for (unsigned int i = 0; i < regexes.size(); i++) {
		for (unsigned int j = 0; j < regexes[i].mark_count() + 1; j++) {
			regex_mark_counts.push_back(i);
		}
	}

	int number_regex = -1, number_inner = 1;
	for (unsigned int i = 1; i < search_match.size(); i++) {
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
	for (unsigned int i = 0; i < regexes.size(); i++) {
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
		Main,
		String,
		Comment,
		Expression,
		Loop,
		Condition,
		ConditionBranch
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

	enum class Function {
		Not,
		And,
		Or,
		In,
		Equal,
		Greater,
		Less,
		GreaterEqual,
		LessEqual,
		Different,
		Upper,
		Lower,
		Range,
		Length,
		Round,
		DivisibleBy,
		Odd,
		Even,
		ReadJson
	};

	struct Element {
		Type type;
		std::string inner;
		std::vector<std::shared_ptr<Element>> children;

		explicit Element(Type type): Element(type, "") { }
		explicit Element(Type type, const std::string& inner): type(type), inner(inner), children({}) { }
	};

	struct ElementString: public Element {
		const std::string text;

		explicit ElementString(const std::string& text): Element(Type::String), text(text) { }
	};

	struct ElementComment: public Element {
		const std::string text;

		explicit ElementComment(const std::string& text): Element(Type::Comment), text(text) { }
	};

	struct ElementExpression: public Element {
		Function function;
		std::vector<ElementExpression> args;
		std::string command;

		explicit ElementExpression(): ElementExpression(Function::ReadJson) { }
		ElementExpression(const Function function): Element(Type::Expression), function(function), args({}), command("") { }
	};

	struct ElementLoop: public Element {
		const std::string item;
		const ElementExpression list;

		explicit ElementLoop(const std::string& item, const ElementExpression& list, const std::string& inner): Element(Type::Loop, inner), item(item), list(list) { }
	};

	struct ElementConditionContainer: public Element {
		explicit ElementConditionContainer(): Element(Type::Condition) { }
	};

	struct ElementConditionBranch: public Element {
		const std::string condition_type;
		const ElementExpression condition;

		explicit ElementConditionBranch(const std::string& inner, const std::string& condition_type, const ElementExpression& condition): Element(Type::ConditionBranch, inner), condition_type(condition_type), condition(condition) { }
	};
};



class Template {
public:
	const Parsed::Element parsed_template;
	ElementNotation elementNotation;

	explicit Template(const Parsed::Element& parsed_template): Template(parsed_template, ElementNotation::Pointer) { }
	explicit Template(const Parsed::Element& parsed_template, ElementNotation elementNotation): parsed_template(parsed_template), elementNotation(elementNotation) { }

	template<typename T = json>
	T eval_variable(const Parsed::ElementExpression& element, json data) {
		const json var = eval_variable(element, data, true);
		if (std::is_same<T, json>::value) { return var; }
		return var.get<T>();
	}

	json eval_variable(const Parsed::ElementExpression& element, json data, bool throw_error) {
		switch (element.function) {
			case Parsed::Function::Upper: {
				std::string str = eval_variable<std::string>(element.args[0], data);
				std::transform(str.begin(), str.end(), str.begin(), toupper);
				return str;
			}
			case Parsed::Function::Lower: {
				std::string str = eval_variable<std::string>(element.args[0], data);
				std::transform(str.begin(), str.end(), str.begin(), tolower);
				return str;
			}
			case Parsed::Function::Range: {
				const int number = eval_variable<int>(element.args[0], data);
				std::vector<int> result(number);
				std::iota(std::begin(result), std::end(result), 0);
				return result;
			}
			case Parsed::Function::Length: {
				const std::vector<json> list = eval_variable<std::vector<json>>(element.args[0], data);
				return list.size();
			}
			case Parsed::Function::Round: {
				const double number = eval_variable<double>(element.args[0], data);
				const int precision = eval_variable<int>(element.args[1], data);
				return std::round(number * std::pow(10.0, precision)) / std::pow(10.0, precision);
			}
			case Parsed::Function::DivisibleBy: {
				const int number = eval_variable<int>(element.args[0], data);
				const int divisor = eval_variable<int>(element.args[1], data);
				return (number % divisor == 0);
			}
			case Parsed::Function::Odd: {
				const int number = eval_variable<int>(element.args[0], data);
				return (number % 2 != 0);
			}
			case Parsed::Function::Even: {
				const int number = eval_variable<int>(element.args[0], data);
				return (number % 2 == 0);
			}
			case Parsed::Function::Not: {
				return not eval_condition(element.args[0], data);
			}
			case Parsed::Function::And: {
				return (eval_condition(element.args[0], data) and eval_condition(element.args[1], data));
			}
			case Parsed::Function::Or: {
				return (eval_condition(element.args[0], data) or eval_condition(element.args[1], data));
			}
			case Parsed::Function::In: {
				const json item = eval_variable(element.args[0], data);
				const json list = eval_variable(element.args[1], data);
				return (std::find(list.begin(), list.end(), item) != list.end());
			}
			case Parsed::Function::Equal: {
				return eval_variable(element.args[0], data) == eval_variable(element.args[1], data);
			}
			case Parsed::Function::Greater: {
				return eval_variable(element.args[0], data) > eval_variable(element.args[1], data);
			}
			case Parsed::Function::Less: {
				return eval_variable(element.args[0], data) < eval_variable(element.args[1], data);
			}
			case Parsed::Function::GreaterEqual: {
				return eval_variable(element.args[0], data) >= eval_variable(element.args[1], data);
			}
			case Parsed::Function::LessEqual: {
				return eval_variable(element.args[0], data) <= eval_variable(element.args[1], data);
			}
			case Parsed::Function::Different: {
				return eval_variable(element.args[0], data) != eval_variable(element.args[1], data);
			}
			case Parsed::Function::ReadJson: {
				// Json Raw Data
				if ( json::accept(element.command) ) { return json::parse(element.command); }

				std::string input = element.command;
				switch (elementNotation) {
					case ElementNotation::Pointer: {
						if (input[0] != '/') { input.insert(0, "/"); }
						break;
					}
					case ElementNotation::Dot: {
						input = dot_to_json_pointer_notation(input);
						break;
					}
				}

				const json::json_pointer ptr(input);
				const json result = data[ptr];

				if (throw_error && result.is_null()) { throw std::runtime_error("Did not found json element: " + element.command); }
				return result;
			}
		}
	}

	bool eval_condition(const Parsed::ElementExpression& element, json data) {
		const json var = eval_variable(element, data, false);
		if (var.empty()) { return false; }
		else if (var.is_number()) { return (var != 0); }
		else if (var.is_string()) { return not var.empty(); }
		return var;
	}

	std::string render(json data) {
		std::string result = "";
		for (auto element: parsed_template.children) {
			switch (element->type) {
    		case Parsed::Type::String: {
					auto elementString = std::static_pointer_cast<Parsed::ElementString>(element);
					result += elementString->text;
          break;
				}
    		case Parsed::Type::Expression: {
					auto elementExpression = std::static_pointer_cast<Parsed::ElementExpression>(element);
					json variable = eval_variable(*elementExpression, data);
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
					auto elementLoop = std::static_pointer_cast<Parsed::ElementLoop>(element);
					const std::string item_name = elementLoop->item;

					const std::vector<json> list = eval_variable<std::vector<json>>(elementLoop->list, data);
					for (unsigned int i = 0; i < list.size(); i++) {
						json data_loop = data;
						data_loop[item_name] = list[i];
						data_loop["index"] = i;
						data_loop["index1"] = i + 1;
						data_loop["is_first"] = (i == 0);
						data_loop["is_last"] = (i == list.size() - 1);
						result += Template(*elementLoop, elementNotation).render(data_loop);
					}
					break;
				}
				case Parsed::Type::Condition: {
					auto elementCondition = std::static_pointer_cast<Parsed::ElementConditionContainer>(element);
					for (auto branch: elementCondition->children) {
						auto elementBranch = std::static_pointer_cast<Parsed::ElementConditionBranch>(branch);
						if (eval_condition(elementBranch->condition, data) || elementBranch->condition_type == "else") {
							result += Template(*elementBranch, elementNotation).render(data);
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
		{Parsed::Function::Upper, Regex{"upper\\(\\s*(.*)\\s*\\)"}},
		{Parsed::Function::Lower, Regex{"lower\\(\\s*(.*)\\s*\\)"}},
		{Parsed::Function::Range, Regex{"range\\(\\s*(.*)\\s*\\)"}},
		{Parsed::Function::Length, Regex{"length\\(\\s*(.*)\\s*\\)"}},
		{Parsed::Function::Round, Regex{"round\\(\\s*(.*)\\s*,\\s*(.*)\\s*\\)"}},
		{Parsed::Function::DivisibleBy, Regex{"divisibleBy\\(\\s*(.*)\\s*,\\s*(.*)\\s*\\)"}},
		{Parsed::Function::Odd, Regex{"odd\\(\\s*(.*)\\s*\\)"}},
		{Parsed::Function::Even, Regex{"even\\(\\s*(.*)\\s*\\)"}},
		{Parsed::Function::ReadJson, Regex{"\\s*([^()]*?)\\s*"}}
	};

	Parser() { }

	Parsed::ElementExpression element_function(Parsed::Function func, unsigned int number_args, const Match& match) {
		std::vector<Parsed::ElementExpression> args = {};
		for (unsigned int i = 0; i < number_args; i++) {
			args.push_back( parse_expression(match.str(i + 1)) ); // str(0) is whole group
		}

		Parsed::ElementExpression result = Parsed::ElementExpression(func);
		result.args = args;
		return result;
	}

	Parsed::ElementExpression parse_expression(const std::string& input) {
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
			case Parsed::Function::Not: {
				return element_function(Parsed::Function::Not, 1, match_function);
			}
			case Parsed::Function::And: {
				return element_function(Parsed::Function::And, 2, match_function);
			}
			case Parsed::Function::Or: {
				return element_function(Parsed::Function::Or, 2, match_function);
			}
			case Parsed::Function::In: {
				return element_function(Parsed::Function::In, 2, match_function);
			}
			case Parsed::Function::Equal: {
				return element_function(Parsed::Function::Equal, 2, match_function);
			}
			case Parsed::Function::Greater: {
				return element_function(Parsed::Function::Greater, 2, match_function);
			}
			case Parsed::Function::Less: {
				return element_function(Parsed::Function::Less, 2, match_function);
			}
			case Parsed::Function::GreaterEqual: {
				return element_function(Parsed::Function::GreaterEqual, 2, match_function);
			}
			case Parsed::Function::LessEqual: {
				return element_function(Parsed::Function::LessEqual, 2, match_function);
			}
			case Parsed::Function::Different: {
				return element_function(Parsed::Function::Different, 2, match_function);
			}
			case Parsed::Function::ReadJson:
			{
				Parsed::ElementExpression result = Parsed::ElementExpression(Parsed::Function::ReadJson);
				result.command = match_function.str(1);
				return result;
			}
			default: {
				throw std::runtime_error("Parser error: Could not parse input: " + input);
			}
		}
	}

	std::vector<std::shared_ptr<Parsed::Element>> parse_level(const std::string& input, const std::string& path) {
		std::vector<std::shared_ptr<Parsed::Element>> result;

		std::vector<Regex> regex_delimiters = get_values(regex_map_delimiters);

		size_t current_position = 0;
		Match match_delimiter = search(input, regex_delimiters, current_position);
		while (match_delimiter.found()) {
			current_position = match_delimiter.end_position();
			std::string string_prefix = match_delimiter.prefix();
			if (not string_prefix.empty()) {
				result.emplace_back( std::make_shared<Parsed::ElementString>(string_prefix) );
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

								result.emplace_back( std::make_shared<Parsed::ElementLoop>(item_name, parse_expression(list_name), loop_match.inner()));
							} else {
								throw std::runtime_error("Parser error: Unknown loop command.");
							}
							break;
						}
						case Parsed::Statement::Condition: {
							auto condition_container = std::make_shared<Parsed::ElementConditionContainer>();

							const Regex regex_condition{"(if|else if|else(?! if)) ?(.*)"};
							Match condition_match = match_delimiter;
							MatchClosed else_if_match = search_closed_on_level(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, regex_condition_else_if, condition_match);
							while (else_if_match.found()) {
								condition_match = else_if_match.close_match;

								Match match_command;
								std::string else_if_open_str = else_if_match.open_match.str(1); // Regex match no r-value
								if (std::regex_match(else_if_open_str, match_command, regex_condition)) {
									condition_container->children.push_back( std::make_shared<Parsed::ElementConditionBranch>(else_if_match.inner(), match_command.str(1), parse_expression(match_command.str(2))) );
								}

								else_if_match = search_closed_on_level(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, regex_condition_else_if, condition_match);
							}

							MatchClosed else_match = search_closed_on_level(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, regex_condition_else, condition_match);
							if (else_match.found()) {
								condition_match = else_match.close_match;

								Match match_command;
								std::string else_open_str = else_match.open_match.str(1);
								if (std::regex_match(else_open_str, match_command, regex_condition)) {
									condition_container->children.push_back( std::make_shared<Parsed::ElementConditionBranch>(else_match.inner(), match_command.str(1), parse_expression(match_command.str(2))) );
								}
							}

							MatchClosed last_if_match = search_closed(input, match_delimiter.regex(), regex_condition_open, regex_condition_close, condition_match);

							Match match_command;
							std::string last_if_open_str = last_if_match.open_match.str(1);
							if (std::regex_match(last_if_open_str, match_command, regex_condition)) {
								condition_container->children.push_back( std::make_shared<Parsed::ElementConditionBranch>(last_if_match.inner(), match_command.str(1), parse_expression(match_command.str(2))) );
							}

							current_position = last_if_match.end_position();
							result.emplace_back(condition_container);
							break;
						}
						case Parsed::Statement::Include: {
							std::string included_filename = path + match_statement.str(1);
							Template included_template = parse_template(included_filename);
							for (auto element : included_template.parsed_template.children) {
								result.emplace_back(element);
							}
							break;
						}
						default: { throw std::runtime_error("Parser error: Unknown statement."); }
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
				default: { throw std::runtime_error("Parser error: Unknown delimiter."); }
			}

			match_delimiter = search(input, regex_delimiters, current_position);
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
		auto parsed = parse_tree(std::make_shared<Parsed::Element>(Parsed::Element(Parsed::Type::String, input)), "./");
		return Template(*parsed);
	}

	Template parse_template(const std::string& filename) {
		std::string input = load_file(filename);
		std::string path = filename.substr(0, filename.find_last_of("/\\") + 1);
		auto parsed = parse_tree(std::make_shared<Parsed::Element>(Parsed::Element(Parsed::Type::String, input)), path);
		return Template(*parsed);
	}

	std::string load_file(const std::string& filename) {
		std::ifstream file(filename);
		std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return text;
	}
};



/*!
@brief Environment class
*/
class Environment {
	const std::string global_path;

	ElementNotation elementNotation = ElementNotation::Pointer;

	Parser parser = Parser();

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


	Template parse(const std::string& input) {
		Template parsed = parser.parse(input);
		parsed.elementNotation = elementNotation;
		return parsed;
	}

	Template parse_template(const std::string& filename) {
		Template parsed = parser.parse_template(global_path + filename);
		parsed.elementNotation = elementNotation;
		return parsed;
	}

	std::string render(const std::string& input, json data) {
		return parse(input).render(data);
	}

	std::string render_template(const std::string& filename, json data) {
		return parse_template(filename).render(data);
	}

	std::string render_template_with_json_file(const std::string& filename, const std::string& filename_data) {
		json data = load_json(filename_data);
		return render_template(filename, data);
	}

	void write(const std::string& filename, json data, const std::string& filename_out) {
		std::ofstream file(global_path + filename_out);
		file << render_template(filename, data);
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
