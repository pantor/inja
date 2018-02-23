/*
Inja - A Template Engine for Modern C++
version 1.0.0
https://github.com/pantor/inja

Licensed under the MIT License <https://opensource.org/licenses/MIT>.
Copyright (c) 2017-2018 Pantor <https://github.com/pantor>.

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef PANTOR_INJA_HPP
#define PANTOR_INJA_HPP

#define PANTOR_INJA_VERSION_MAJOR 1
#define PANTOR_INJA_VERSION_MINOR 0
#define PANTOR_INJA_VERSION_PATCH 0


#ifndef NLOHMANN_JSON_HPP
	static_assert(false, "nlohmann/json not found.");
#endif


#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <locale>
#include <regex>
#include <vector>
#include <algorithm>
#include <type_traits>


namespace inja {

using json = nlohmann::json;



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



/*!
@brief inja regex class, saves string pattern in addition to std::regex
*/
class Regex: public std::regex {
	std::string pattern_;

public:
	Regex(): std::regex() {}
	explicit Regex(const std::string& pattern): std::regex(pattern, std::regex_constants::ECMAScript), pattern_(pattern) { }

	std::string pattern() { return pattern_; }
};



class Match: public std::match_results<std::string::const_iterator> {
	size_t offset_ = 0;
	unsigned int group_offset_ = 0;
	Regex regex_;

public:
	Match(): std::match_results<std::string::const_iterator>() { }
	explicit Match(size_t offset): std::match_results<std::string::const_iterator>(), offset_(offset) { }
	explicit Match(size_t offset, const Regex& regex): std::match_results<std::string::const_iterator>(), offset_(offset), regex_(regex) { }

	void set_group_offset(unsigned int group_offset) { group_offset_ = group_offset; }
	void set_regex(Regex regex) { regex_ = regex; }

	size_t position() const { return offset_ + std::match_results<std::string::const_iterator>::position(); }
	size_t end_position() const { return position() + length(); }
	bool found() const { return not empty(); }
	bool valid(int i) const { return std::match_results<std::string::const_iterator>::length(i + group_offset_) > 0; }
	const std::string str() const { return str(0); }
	const std::string str(int i) const { return std::match_results<std::string::const_iterator>::str(i + group_offset_); }
	Regex regex() const { return regex_; }
};



template<typename T>
class MatchType: public Match {
	T type_;

public:
	MatchType() : Match() { }
	explicit MatchType(const Match& obj) : Match(obj) { }
	MatchType(Match&& obj) : Match(std::move(obj)) { }

	void set_type(T type) { type_ = type; }

	T type() const { return type_; }
};



class MatchClosed {
public:
	Match open_match, close_match;

	MatchClosed() { }
	MatchClosed(Match& open_match, Match& close_match): open_match(open_match), close_match(close_match) { }

	size_t position() const { return open_match.position(); }
	size_t end_position() const { return close_match.end_position(); }
	size_t length() const { return close_match.end_position() - open_match.position(); }
	bool found() const { return open_match.found() and close_match.found(); }
	std::string prefix() const { return open_match.prefix().str(); }
	std::string suffix() const { return close_match.suffix().str(); }
	std::string outer() const { return open_match.str() + static_cast<std::string>(open_match.suffix()).substr(0, close_match.end_position() - open_match.end_position()); }
	std::string inner() const { return static_cast<std::string>(open_match.suffix()).substr(0, close_match.position() - open_match.end_position()); }
};


inline Match search(const std::string& input, Regex regex, size_t position) {
	if (position >= input.length()) { return Match(); }

	Match match{position, regex};
	std::regex_search(input.cbegin() + position, input.cend(), match, regex);
	return match;
}



template<typename T>
inline MatchType<T> search(const std::string& input, std::map<T, Regex>& regexes, size_t position) {
	// Map to vectors
	std::vector<T> class_vector;
	std::vector<Regex> regexes_vector;
	for (const auto element: regexes) {
		class_vector.push_back(element.first);
		regexes_vector.push_back(element.second);
	}

	// Regex join
	std::stringstream ss;
	for (size_t i = 0; i < regexes_vector.size(); ++i)
	{
		if (i != 0) { ss << ")|("; }
		ss << regexes_vector[i].pattern();
	}
	Regex regex{"(" + ss.str() + ")"};

	MatchType<T> search_match = search(input, regex, position);
	if (not search_match.found()) { return MatchType<T>(); }

	// Vector of id vs groups
	std::vector<unsigned int> regex_mark_counts = {};
	for (unsigned int i = 0; i < regexes_vector.size(); i++) {
		for (unsigned int j = 0; j < regexes_vector[i].mark_count() + 1; j++) {
			regex_mark_counts.push_back(i);
		}
	}

	for (unsigned int i = 1; i < search_match.size(); i++) {
		if (search_match.length(i) > 0) {
			search_match.set_group_offset(i);
			search_match.set_type(class_vector[regex_mark_counts[i]]);
			search_match.set_regex(regexes_vector[regex_mark_counts[i]]);
			return search_match;
		}
	}

	throw std::runtime_error("Error while searching in input: " + input);
	return search_match;
}

inline MatchClosed search_closed_on_level(const std::string& input, const Regex& regex_statement, const Regex& regex_level_up, const Regex& regex_level_down, const Regex& regex_search, Match open_match) {

	int level = 0;
	size_t current_position = open_match.end_position();
	Match match_delimiter = search(input, regex_statement, current_position);
	while (match_delimiter.found()) {
		current_position = match_delimiter.end_position();

		const std::string inner = match_delimiter.str(1);
		if (std::regex_match(inner.cbegin(), inner.cend(), regex_search) and level == 0) { break; }
		if (std::regex_match(inner.cbegin(), inner.cend(), regex_level_up)) { level += 1; }
		else if (std::regex_match(inner.cbegin(), inner.cend(), regex_level_down)) { level -= 1; }

		match_delimiter = search(input, regex_statement, current_position);
	}

	return MatchClosed(open_match, match_delimiter);
}

inline MatchClosed search_closed(const std::string& input, const Regex& regex_statement, const Regex& regex_open, const Regex& regex_close, Match& open_match) {
	return search_closed_on_level(input, regex_statement, regex_open, regex_close, regex_close, open_match);
}

template<typename T>
inline MatchType<T> match(const std::string& input, std::map<T, Regex> regexes) {
	MatchType<T> match;
	for (const auto e : regexes) {
		if (std::regex_match(input.cbegin(), input.cend(), match, e.second)) {
			match.set_type(e.first);
			match.set_regex(e.second);
			return match;
		}
	}
	return match;
}



enum class ElementNotation {
	Pointer,
	Dot
};

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
		Sort,
		First,
		Last,
		Round,
		DivisibleBy,
		Odd,
		Even,
		Max,
		Min,
		ReadJson,
		Default,
		Callback
	};

	enum class Condition {
		If,
		ElseIf,
		Else
	};

	enum class Loop {
		ForListIn,
		ForMapIn
	};

	struct Element {
		Type type;
		std::string inner;
		std::vector<std::shared_ptr<Element>> children;

		explicit Element(const Type type): Element(type, "") { }
		explicit Element(const Type type, const std::string& inner): type(type), inner(inner), children({}) { }
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
		explicit ElementExpression(const Function function_): Element(Type::Expression), function(function_), args({}), command("") { }
	};
	typedef std::vector<ElementExpression> Arguments;

	struct ElementLoop: public Element {
		Loop loop;
		const std::string key;
		const std::string value;
		const ElementExpression list;

		explicit ElementLoop(const Loop loop_, const std::string& value, const ElementExpression& list, const std::string& inner): Element(Type::Loop, inner), loop(loop_), value(value), list(list) { }
		explicit ElementLoop(const Loop loop_, const std::string& key, const std::string& value, const ElementExpression& list, const std::string& inner): Element(Type::Loop, inner), loop(loop_), key(key), value(value), list(list) { }
	};

	struct ElementConditionContainer: public Element {
		explicit ElementConditionContainer(): Element(Type::Condition) { }
	};

	struct ElementConditionBranch: public Element {
		const Condition condition_type;
		const ElementExpression condition;

		explicit ElementConditionBranch(const std::string& inner, const Condition condition_type): Element(Type::ConditionBranch, inner), condition_type(condition_type) { }
		explicit ElementConditionBranch(const std::string& inner, const Condition condition_type, const ElementExpression& condition): Element(Type::ConditionBranch, inner), condition_type(condition_type), condition(condition) { }
	};
};



class Template {
public:
	const Parsed::Element parsed_template;

	explicit Template(const Parsed::Element& parsed_template): parsed_template(parsed_template) { }
};


class Renderer {
public:
	ElementNotation element_notation;

	std::map<std::string, std::function<json(const Parsed::Arguments&, const json&)>> map_callbacks;

	template<bool>
	bool eval_expression(const Parsed::ElementExpression& element, const json &data) {
		const json var = eval_function(element, data);
		if (var.empty()) { return false; }
		else if (var.is_number()) { return (var != 0); }
		else if (var.is_string()) { return not var.empty(); }
		return var.get<bool>();
	}

	template<typename T = json>
  T eval_expression(const Parsed::ElementExpression& element, const json &data) {
		return eval_function(element, data).get<T>();
	}

	json eval_function(const Parsed::ElementExpression& element, const json& data) {
		switch (element.function) {
			case Parsed::Function::Upper: {
				std::string str = eval_expression<std::string>(element.args[0], data);
				std::transform(str.begin(), str.end(), str.begin(), toupper);
				return str;
			}
			case Parsed::Function::Lower: {
				std::string str = eval_expression<std::string>(element.args[0], data);
				std::transform(str.begin(), str.end(), str.begin(), tolower);
				return str;
			}
			case Parsed::Function::Range: {
				const int number = eval_expression<int>(element.args[0], data);
				std::vector<int> result(number);
				std::iota(std::begin(result), std::end(result), 0);
				return result;
			}
			case Parsed::Function::Length: {
				const std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				return list.size();
			}
			case Parsed::Function::Sort: {
				std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				std::sort(list.begin(), list.end());
				return list;
			}
			case Parsed::Function::First: {
				const std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				return list.front();
			}
			case Parsed::Function::Last: {
				const std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				return list.back();
			}
			case Parsed::Function::Round: {
				const double number = eval_expression<double>(element.args[0], data);
				const int precision = eval_expression<int>(element.args[1], data);
				return std::round(number * std::pow(10.0, precision)) / std::pow(10.0, precision);
			}
			case Parsed::Function::DivisibleBy: {
				const int number = eval_expression<int>(element.args[0], data);
				const int divisor = eval_expression<int>(element.args[1], data);
				return (number % divisor == 0);
			}
			case Parsed::Function::Odd: {
				const int number = eval_expression<int>(element.args[0], data);
				return (number % 2 != 0);
			}
			case Parsed::Function::Even: {
				const int number = eval_expression<int>(element.args[0], data);
				return (number % 2 == 0);
			}
			case Parsed::Function::Max: {
				const std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				return *std::max_element(list.begin(), list.end());
			}
			case Parsed::Function::Min: {
				const std::vector<json> list = eval_expression<std::vector<json>>(element.args[0], data);
				return *std::min_element(list.begin(), list.end());
			}
			case Parsed::Function::Not: {
				return not eval_expression<bool>(element.args[0], data);
			}
			case Parsed::Function::And: {
				return (eval_expression<bool>(element.args[0], data) and eval_expression<bool>(element.args[1], data));
			}
			case Parsed::Function::Or: {
				return (eval_expression<bool>(element.args[0], data) or eval_expression<bool>(element.args[1], data));
			}
			case Parsed::Function::In: {
				const json value = eval_expression(element.args[0], data);
				const json list = eval_expression(element.args[1], data);
				return (std::find(list.begin(), list.end(), value) != list.end());
			}
			case Parsed::Function::Equal: {
				return eval_expression(element.args[0], data) == eval_expression(element.args[1], data);
			}
			case Parsed::Function::Greater: {
				return eval_expression(element.args[0], data) > eval_expression(element.args[1], data);
			}
			case Parsed::Function::Less: {
				return eval_expression(element.args[0], data) < eval_expression(element.args[1], data);
			}
			case Parsed::Function::GreaterEqual: {
				return eval_expression(element.args[0], data) >= eval_expression(element.args[1], data);
			}
			case Parsed::Function::LessEqual: {
				return eval_expression(element.args[0], data) <= eval_expression(element.args[1], data);
			}
			case Parsed::Function::Different: {
				return eval_expression(element.args[0], data) != eval_expression(element.args[1], data);
			}
			case Parsed::Function::ReadJson: {
				if ( json::accept(element.command) ) { return json::parse(element.command); } // Json Raw Data

				std::string input = element.command;
				switch (element_notation) {
					case ElementNotation::Pointer: {
						if (input[0] != '/') { input.insert(0, "/"); }
						break;
					}
					case ElementNotation::Dot: {
						input = dot_to_json_pointer_notation(input);
						break;
					}
				}

				return data.at(json::json_pointer(input));
			}
			case Parsed::Function::Default: {
				try {
					return eval_expression(element.args[0], data);
				} catch (std::exception&) {
					return eval_expression(element.args[1], data);
				}
			}
			case Parsed::Function::Callback: {
				return map_callbacks[element.command](element.args, data);
			}
		}

		throw std::runtime_error("Unknown function in renderer.");
		return json();
	}

	std::string render(Template temp, json data) {
		std::string result = "";
		for (auto element: temp.parsed_template.children) {
			switch (element->type) {
				case Parsed::Type::Main: { throw std::runtime_error("Main type in renderer."); }
				case Parsed::Type::String: {
					auto element_string = std::static_pointer_cast<Parsed::ElementString>(element);
					result.append(element_string->text);
					break;
				}
				case Parsed::Type::Expression: {
					auto element_expression = std::static_pointer_cast<Parsed::ElementExpression>(element);
					json variable = eval_expression(*element_expression, data);

					if (variable.is_string()) {
						result.append( variable.get<std::string>() );
					} else {
						std::stringstream ss;
						ss << variable;
						result.append( ss.str() );
					}
					break;
				}
				case Parsed::Type::Loop: {
					auto element_loop = std::static_pointer_cast<Parsed::ElementLoop>(element);
					switch (element_loop->loop) {
						case Parsed::Loop::ForListIn: {
							const std::vector<json> list = eval_expression<std::vector<json>>(element_loop->list, data);
							for (unsigned int i = 0; i < list.size(); i++) {
								json data_loop = data;
								data_loop[element_loop->value] = list[i];
								data_loop["index"] = i;
								data_loop["index1"] = i + 1;
								data_loop["is_first"] = (i == 0);
								data_loop["is_last"] = (i == list.size() - 1);
								result.append( render(Template(*element_loop), data_loop) );
							}
							break;
						}
						case Parsed::Loop::ForMapIn: {
							const std::map<std::string, json> map = eval_expression<std::map<std::string, json>>(element_loop->list, data);
							for (auto const& item : map) {
								json data_loop = data;
								data_loop[element_loop->key] = item.first;
								data_loop[element_loop->value] = item.second;
								result.append( render(Template(*element_loop), data_loop) );
							}
							break;
						}
					}

					break;
				}
				case Parsed::Type::Condition: {
					auto element_condition = std::static_pointer_cast<Parsed::ElementConditionContainer>(element);
					for (auto branch: element_condition->children) {
						auto element_branch = std::static_pointer_cast<Parsed::ElementConditionBranch>(branch);
						if (element_branch->condition_type == Parsed::Condition::Else || eval_expression<bool>(element_branch->condition, data)) {
							result.append( render(Template(*element_branch), data) );
							break;
						}
					}
					break;
				}
				case Parsed::Type::ConditionBranch: { throw std::runtime_error("ConditionBranch type in renderer."); }
				case Parsed::Type::Comment: { break; }
			}
		}
		return result;
	}
};


class Parser {
public:
	static Regex function_regex(std::string name, int minimum_number_arguments, int maximum_number_argumets = -1) {
		if (maximum_number_argumets < minimum_number_arguments )
			maximum_number_argumets = minimum_number_arguments;

		std::string pattern = name;
		if (maximum_number_argumets > 0) {
			pattern.append("\\(");

			if ( maximum_number_argumets == 1 )
				pattern.append("(.*)");
			else
				for (int i = 0; i < maximum_number_argumets; i++) {
					pattern.append(R"((?:((?:"[^"]*")|(?:[^,]*)|(?:\[[^"]*\])))");
					pattern.append(i != maximum_number_argumets - 1 ? ",)" : ")");
					pattern.append(i >= maximum_number_argumets - minimum_number_arguments ? "{1}" : "{0,1}");
				}
			pattern.append("\\)");
		}
		return Regex{"\\s*" + pattern + "\\s*"};
	}

	std::map<Parsed::Delimiter, Regex> regex_map_delimiters = {
		{Parsed::Delimiter::Statement, Regex{"\\{\\%\\s*(.+?)\\s*\\%\\}"}},
		{Parsed::Delimiter::LineStatement, Regex{"(?:^|\\n)##\\s*(.+)\\s*"}},
		{Parsed::Delimiter::Expression, Regex{"\\{\\{\\s*(.+?)\\s*\\}\\}"}},
		{Parsed::Delimiter::Comment, Regex{"\\{#\\s*(.*?)\\s*#\\}"}}
	};

	const std::map<Parsed::Statement, Regex> regex_map_statement_openers = {
		{Parsed::Statement::Loop, Regex{"for (.+)"}},
		{Parsed::Statement::Condition, Regex{"if (.+)"}},
		{Parsed::Statement::Include, Regex{"include \"(.+)\""}}
	};

	const std::map<Parsed::Statement, Regex> regex_map_statement_closers = {
		{Parsed::Statement::Loop, Regex{"endfor"}},
		{Parsed::Statement::Condition, Regex{"endif"}}
	};

	const std::map<Parsed::Loop, Regex> regex_map_loop = {
		{Parsed::Loop::ForListIn, Regex{"for (\\w+) in (.+)"}},
		{Parsed::Loop::ForMapIn, Regex{"for (\\w+), (\\w+) in (.+)"}},
	};

	const std::map<Parsed::Condition, Regex> regex_map_condition = {
		{Parsed::Condition::If, Regex{"if (.+)"}},
		{Parsed::Condition::ElseIf, Regex{"else if (.+)"}},
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
		{Parsed::Function::Upper, function_regex("upper", 1)},
		{Parsed::Function::Lower, function_regex("lower", 1)},
		{Parsed::Function::Range, function_regex("range", 1)},
		{Parsed::Function::Length, function_regex("length", 1)},
		{Parsed::Function::Sort, function_regex("sort", 1)},
		{Parsed::Function::First, function_regex("first", 1)},
		{Parsed::Function::Last, function_regex("last", 1)},
		{Parsed::Function::Round, function_regex("round", 2)},
		{Parsed::Function::DivisibleBy, function_regex("divisibleBy", 2)},
		{Parsed::Function::Odd, function_regex("odd", 1)},
		{Parsed::Function::Even, function_regex("even", 1)},
		{Parsed::Function::Max, function_regex("max", 1)},
		{Parsed::Function::Min, function_regex("min", 1)},
		{Parsed::Function::ReadJson, Regex{"\\s*([^\\(\\)]*\\S)\\s*"}},
		{Parsed::Function::Default, function_regex("default", 2)}
	};

	std::map<std::string, Regex> regex_map_callbacks;

	Parser() { }

	Parsed::ElementExpression parse_expression(const std::string& input) {
		MatchType<std::string> match_callback = match(input, regex_map_callbacks);
		if (!match_callback.type().empty()) {
			std::vector<Parsed::ElementExpression> args = {};
			for (unsigned int i = 1; i < match_callback.size(); i++) { // str(0) is whole group
				if ( match_callback.valid(i) )
					args.push_back( parse_expression(match_callback.str(i)) );
			}

			Parsed::ElementExpression result = Parsed::ElementExpression(Parsed::Function::Callback);
			result.args = args;
			result.command = match_callback.type();
			return result;
		}

		MatchType<Parsed::Function> match_function = match(input, regex_map_functions);
		switch ( match_function.type() ) {
			case Parsed::Function::ReadJson: {
				Parsed::ElementExpression result = Parsed::ElementExpression(Parsed::Function::ReadJson);
				result.command = match_function.str(1);
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
			std::string string_prefix = match_delimiter.prefix();
			if (not string_prefix.empty()) {
				result.emplace_back( std::make_shared<Parsed::ElementString>(string_prefix) );
			}

			std::string delimiter_inner = match_delimiter.str(1);

			switch ( match_delimiter.type() ) {
				case Parsed::Delimiter::Statement:
				case Parsed::Delimiter::LineStatement: {

					MatchType<Parsed::Statement> match_statement = match(delimiter_inner, regex_map_statement_openers);
					switch ( match_statement.type() ) {
						case Parsed::Statement::Loop: {
							MatchClosed loop_match = search_closed(input, match_delimiter.regex(), regex_map_statement_openers.at(Parsed::Statement::Loop), regex_map_statement_closers.at(Parsed::Statement::Loop), match_delimiter);

							current_position = loop_match.end_position();

							const std::string loop_inner = match_statement.str(0);
							MatchType<Parsed::Loop> match_command = match(loop_inner, regex_map_loop);
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
								default: {
									throw std::runtime_error("Unknown loop statement.");
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
								MatchType<Parsed::Condition> match_command = match(else_if_match_inner, regex_map_condition);
								condition_container->children.push_back( std::make_shared<Parsed::ElementConditionBranch>(else_if_match.inner(), match_command.type(), parse_expression(match_command.str(1))) );

								else_if_match = search_closed_on_level(input, match_delimiter.regex(), regex_map_statement_openers.at(Parsed::Statement::Condition), regex_map_statement_closers.at(Parsed::Statement::Condition), regex_map_condition.at(Parsed::Condition::ElseIf), condition_match);
							}

							MatchClosed else_match = search_closed_on_level(input, match_delimiter.regex(), regex_map_statement_openers.at(Parsed::Statement::Condition), regex_map_statement_closers.at(Parsed::Statement::Condition), regex_map_condition.at(Parsed::Condition::Else), condition_match);
							if (else_match.found()) {
								condition_match = else_match.close_match;

								const std::string else_match_inner = else_match.open_match.str(1);
								MatchType<Parsed::Condition> match_command = match(else_match_inner, regex_map_condition);
								condition_container->children.push_back( std::make_shared<Parsed::ElementConditionBranch>(else_match.inner(), match_command.type(), parse_expression(match_command.str(1))) );
							}

							MatchClosed last_if_match = search_closed(input, match_delimiter.regex(), regex_map_statement_openers.at(Parsed::Statement::Condition), regex_map_statement_closers.at(Parsed::Statement::Condition), condition_match);

							const std::string last_if_match_inner = last_if_match.open_match.str(1);
							MatchType<Parsed::Condition> match_command = match(last_if_match_inner, regex_map_condition);
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
							std::string included_filename = path + match_statement.str(1);
							Template included_template = parse_template(included_filename);
							for (auto element : included_template.parsed_template.children) {
								result.emplace_back(element);
							}
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
		std::string input = load_file(filename);
		std::string path = filename.substr(0, filename.find_last_of("/\\") + 1);
		auto parsed = parse_tree(std::make_shared<Parsed::Element>(Parsed::Element(Parsed::Type::Main, input)), path);
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
	const std::string input_path;
	const std::string output_path;

	ElementNotation element_notation = ElementNotation::Pointer;

	Parser parser = Parser();

public:
	Renderer renderer = Renderer();


	Environment(): Environment("./") { }
	explicit Environment(const std::string& global_path): input_path(global_path), output_path(global_path), parser() { }
	explicit Environment(const std::string& input_path, const std::string& output_path): input_path(input_path), output_path(output_path), parser() { }

	void set_statement(const std::string& open, const std::string& close) {
		parser.regex_map_delimiters[Parsed::Delimiter::Statement] = Regex{open + "\\s*(.+?)\\s*" + close};
	}

	void set_line_statement(const std::string& open) {
		parser.regex_map_delimiters[Parsed::Delimiter::LineStatement] = Regex{"(?:^|\\n)" + open + "\\s*(.+)\\s*"};
	}

	void set_expression(const std::string& open, const std::string& close) {
		parser.regex_map_delimiters[Parsed::Delimiter::Expression] = Regex{open + "\\s*(.+?)\\s*" + close};
	}

	void set_comment(const std::string& open, const std::string& close) {
		parser.regex_map_delimiters[Parsed::Delimiter::Comment] = Regex{open + "\\s*(.+?)\\s*" + close};
	}

	void set_element_notation(const ElementNotation element_notation_) {
		element_notation = element_notation_;
	}


	Template parse(const std::string& input) {
		Template parsed = parser.parse(input);
		return parsed;
	}

	Template parse_template(const std::string& filename) {
		Template parsed = parser.parse_template(input_path + filename);
		return parsed;
	}

	std::string render(const std::string& input, json data) {
		const std::string text = input;
		renderer.element_notation = element_notation;
		return renderer.render(parse(text), data);
	}

	std::string render_template(const Template& temp, json data) {
		renderer.element_notation = element_notation;
		return renderer.render(temp, data);
	}

	std::string render_file(const std::string& filename, json data) {
		renderer.element_notation = element_notation;
		return renderer.render(parse_template(filename), data);
	}

	std::string render_file_with_json_file(const std::string& filename, const std::string& filename_data) {
		json data = load_json(filename_data);
		return render_file(filename, data);
	}

	void write(const std::string& filename, json data, const std::string& filename_out) {
		std::ofstream file(output_path + filename_out);
		file << render_file(filename, data);
		file.close();
	}

	void write(const Template& temp, json data, const std::string& filename_out) {
		std::ofstream file(output_path + filename_out);
		file << render_template(temp, data);
		file.close();
	}

	void write_with_json_file(const std::string& filename, const std::string& filename_data, const std::string& filename_out) {
		json data = load_json(filename_data);
		write(filename, data, filename_out);
	}

	void write_with_json_file(const Template& temp, const std::string& filename_data, const std::string& filename_out) {
		json data = load_json(filename_data);
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

	void add_callback(std::string name, int number_arguments, std::function<json(const Parsed::Arguments&, const json&)> callback) {
		parser.regex_map_callbacks[name] = Parser::function_regex(name, number_arguments);
		renderer.map_callbacks[name] = callback;
	}
	void add_callback(std::string name, int minimum_number_arguments,int maximum_number_arguments, std::function<json(const Parsed::Arguments&, const json&)> callback) {
		parser.regex_map_callbacks[name] = Parser::function_regex(name, minimum_number_arguments, maximum_number_arguments);
		renderer.map_callbacks[name] = callback;
	}
	template<typename T = json>
	T get_argument(const Parsed::Arguments &args, int index, const json& data) {
		return renderer.eval_expression<T>(args[index], data);
	}

	size_t get_arguments_count(const Parsed::Arguments &args) {
		return args.size();
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
