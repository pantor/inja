#include <iostream>
#include <fstream>
#include <string>
#include <regex>

#include "json/json.hpp"


namespace inja {
	
using json = nlohmann::json;
using string = std::string;



template<class BidirIt, class Traits, class CharT, class UnaryFunction>
std::basic_string<CharT> regex_replace(BidirIt first, BidirIt last,
	const std::basic_regex<CharT,Traits>& re, UnaryFunction f)
{
	std::basic_string<CharT> s;

	typename std::match_results<BidirIt>::difference_type positionOfLastMatch = 0;
	auto endOfLastMatch = first;

	auto callback = [&](const std::match_results<BidirIt>& match)
	{
		auto positionOfThisMatch = match.position(0);
		auto diff = positionOfThisMatch - positionOfLastMatch;

		auto startOfThisMatch = endOfLastMatch;
		std::advance(startOfThisMatch, diff);

		s.append(endOfLastMatch, startOfThisMatch);
		s.append(f(match));

		auto lengthOfMatch = match.length(0);

		positionOfLastMatch = positionOfThisMatch + lengthOfMatch;

		endOfLastMatch = startOfThisMatch;
		std::advance(endOfLastMatch, lengthOfMatch);
	};

	std::sregex_iterator begin(first, last, re), end;
	std::for_each(begin, end, callback);

	s.append(endOfLastMatch, last);

	return s;
}

template<class Traits, class CharT, class UnaryFunction>
std::string regex_replace(const std::string& s,
	const std::basic_regex<CharT,Traits>& re, UnaryFunction f)
{
	return regex_replace(s.cbegin(), s.cend(), re, f);
}



class Environment {
	
	
	
		
public:
	Environment() {
			
	}
	
	json get_variable_data(string variable_name, json data) {
		// Json Raw Data
		if ( json::accept(variable_name) ) {
			return json::parse(variable_name);
		}
		
		// Implement range function
		std::regex range_regex("^range\\((\\d+)\\)$");
		std::smatch range_match;
		if (std::regex_match(variable_name, range_match, range_regex)) {
			int counter = std::stoi(range_match[1].str());
			std::vector<int> range(counter);
			std::iota(range.begin(), range.end(), 0);
			return range;
		}

		if (variable_name[0] != '/') {
			variable_name = "/" + variable_name;
		}
		
		json::json_pointer ptr(variable_name);
		json result = data[ptr];
		if (result.is_null()) {
			throw std::runtime_error("JSON pointer found no element.");
		}
		return result;
	}
	
	
	string render(string template_input, json data) {
		return render(template_input, data, "./");
	}
		
	string render(string template_input, json data, string template_path) {
		string result = template_input;
		
		const std::regex include_regex("\\(\\% include \"(.*)\" \\%\\)");
		result = inja::regex_replace(result, include_regex,
			[this, template_path](const std::smatch& match) {
				string filename = template_path + match[1].str();
				return load_template(filename);
			}
		);
		
		const std::regex loop_regex("\\(\\% for (\\w+) in (.+) \\%\\)(.*)\\(\\% endfor \\%\\)");
		result = inja::regex_replace(result, loop_regex,
			[this, data](const std::smatch& match) {
				string result = "";
				string entry_name = match[1].str();
				string list_name = match[2].str();
				string inner_string = match[3].str();
				json list = get_variable_data(list_name, data);
				if (!list.is_array()) throw std::runtime_error("JSON variable is not a list.");
				
				for (int i = 0; i < list.size(); i++) {
					const std::regex entry_regex("\\{\\{.*" + entry_name + ".*\\}\\}");
					result += inja::regex_replace(inner_string, entry_regex,
						[list_name, i](const std::smatch& match) {
							return "{{ " + list_name + "/" + std::to_string(i) + " }}";
						}
					);					
				}
				return result;
			}
		);
		
		const std::regex condition_regex("\\(\\% if (\\w+) \\%\\)(.*)\\(\\% endif \\%\\)");
		result = inja::regex_replace(result, condition_regex,
			[this, data](const std::smatch& match) {
				string condition_variable_name = match[1].str();
				string inner_string = match[2].str();
				if (get_variable_data(condition_variable_name, data)) {
					return inner_string;
				}
				return string("");
			}
		);
		
		const std::regex variable_regex("\\{\\{\\s*([^\\}]*[^\\s])\\s*\\}\\}");
		result = inja::regex_replace(result, variable_regex,
			[this, data](const std::smatch& match) {
				string variable_name = match[1].str(); 
				return get_variable_data(variable_name, data);
			}
		);
		
		return result;
	}
	
	string render_template(string filename, json data) {
		string text = load_template(filename);
		string path = filename.substr(0, filename.find_last_of("/\\") + 1); // Include / itself
		return render(text, data, path);
	}
	
	string load_template(string filename) {
		std::ifstream file(filename);
		string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		return text;
	}
	
	json load_json(string filename) {
		std::ifstream file(filename);
		json j;
		file >> j;
		return j;
	}
};

} // namespace inja