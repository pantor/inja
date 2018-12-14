#ifndef PANTOR_INJA_REGEX_HPP
#define PANTOR_INJA_REGEX_HPP

#include <regex>
#include <string>
#include <sstream>


namespace inja {

/*!
@brief inja regex class, saves string pattern in addition to std::regex
*/
class Regex: public std::regex {
	std::string pattern_;

public:
	Regex(): std::regex() {}
	explicit Regex(const std::string& pattern): std::regex(pattern, std::regex_constants::ECMAScript), pattern_(pattern) { }

	std::string pattern() const { return pattern_; }
};


class Match: public std::match_results<std::string::const_iterator> {
	size_t offset_ {0};
	unsigned int group_offset_ {0};
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
	const std::string str() const { return str(0); }
	const std::string str(int i) const { return std::match_results<std::string::const_iterator>::str(i + group_offset_); }
	Regex regex() const { return regex_; }
};


template<typename T>
class MatchType: public Match {
	T type_;

public:
	MatchType(): Match() { }
	explicit MatchType(const Match& obj): Match(obj) { }
	MatchType(Match&& obj): Match(std::move(obj)) { }

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


inline Match search(const std::string& input, const Regex& regex, size_t position) {
	if (position >= input.length()) { return Match(); }

	Match match{position, regex};
	std::regex_search(input.cbegin() + position, input.cend(), match, regex);
	return match;
}


template<typename T>
inline MatchType<T> search(const std::string& input, const std::map<T, Regex>& regexes, size_t position) {
	// Map to vectors
	std::vector<T> class_vector;
	std::vector<Regex> regexes_vector;
	for (const auto& element: regexes) {
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

	inja_throw("regex_search_error", "error while searching in input: " + input);
	return search_match;
}

inline MatchClosed search_closed_on_level(const std::string& input, const Regex& regex_statement, const Regex& regex_level_up, const Regex& regex_level_down, const Regex& regex_search, Match open_match) {

	int level {0};
	size_t current_position = open_match.end_position();
	Match match_delimiter = search(input, regex_statement, current_position);
	while (match_delimiter.found()) {
		current_position = match_delimiter.end_position();

		const std::string inner = match_delimiter.str(1);
		if (std::regex_match(inner.cbegin(), inner.cend(), regex_search) and level == 0) { break; }
		if (std::regex_match(inner.cbegin(), inner.cend(), regex_level_up)) { level += 1; }
		else if (std::regex_match(inner.cbegin(), inner.cend(), regex_level_down)) { level -= 1; }

		if (level < 0) { return MatchClosed(); }
		match_delimiter = search(input, regex_statement, current_position);
	}

	return MatchClosed(open_match, match_delimiter);
}

inline MatchClosed search_closed(const std::string& input, const Regex& regex_statement, const Regex& regex_open, const Regex& regex_close, Match& open_match) {
	return search_closed_on_level(input, regex_statement, regex_open, regex_close, regex_close, open_match);
}

template<typename T, typename S>
inline MatchType<T> match(const std::string& input, const std::map<T, Regex, S>& regexes) {
	MatchType<T> match;
	for (const auto& e: regexes) {
		if (std::regex_match(input.cbegin(), input.cend(), match, e.second)) {
			match.set_type(e.first);
			match.set_regex(e.second);
			return match;
		}
	}
	return match;
}

}

#endif // PANTOR_INJA_REGEX_HPP
