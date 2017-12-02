#include "catch.hpp"
#include "nlohmann/json.hpp"
#include "inja.hpp"



TEST_CASE("dot to pointer") {
	CHECK( inja::dot_to_json_pointer_notation("person.names.surname") == "/person/names/surname" );
	CHECK( inja::dot_to_json_pointer_notation("guests.2") == "/guests/2" );
}

TEST_CASE("basic-search") {
	std::string input = "lorem ipsum dolor it";
	inja::Regex regex("i(.*)m");

	SECTION("from start") {
		inja::Match match = inja::search(input, regex, 0);
		CHECK( match.found() == true );
		CHECK( match.position() == 6 );
		CHECK( match.length() == 5 );
		CHECK( match.end_position() == 11 );
		CHECK( match.str() == "ipsum" );
		CHECK( match.str(1) == "psu" );
	}

	SECTION("from position") {
		inja::Match match = inja::search(input, regex, 8);
		CHECK( match.found() == false );
		CHECK( match.length() == 0 );
	}
}

TEST_CASE("search-multiple-regexes") {
	std::string input = "lorem ipsum dolor amit estas tronum.";

	SECTION("basic 1") {
		std::map<int, inja::Regex> regex_patterns = {
			{0, inja::Regex("tras")},
			{1, inja::Regex("do(\\w*)or")},
			{2, inja::Regex("es(\\w*)as")},
			{3, inja::Regex("ip(\\w*)um")}
		};
		inja::MatchType<int> match = inja::search(input, regex_patterns, 0);
		CHECK( match.type() == 3 );
		CHECK( match.str() == "ipsum" );
		CHECK( match.str(1) == "s" );
	}

	SECTION("basic 2") {
		std::map<int, inja::Regex> regex_patterns = {
			{11, inja::Regex("tras")},
			{21, inja::Regex("ip(\\w*)um")},
			{31, inja::Regex("do(\\w*)or")},
			{41, inja::Regex("es(\\w*)as")}
		};
		inja::MatchType<int> match = inja::search(input, regex_patterns, 0);
		CHECK( match.type() == 21 );
		CHECK( match.str() == "ipsum" );
		CHECK( match.str(1) == "s" );
	}

	SECTION("basic 3") {
		std::map<int, inja::Regex> regex_patterns = {
			{0, inja::Regex("upper\\((.*)\\)")},
			{1, inja::Regex("lower\\((.*)\\)")},
			{2, inja::Regex("[^()]*?")}
		};
		inja::MatchType<int> match = inja::search("upper(lower(name))", regex_patterns, 0);
		CHECK( match.type() == 0 );
		CHECK( match.str(1) == "lower(name)" );
	}
}

TEST_CASE("match-multiple-regexes") {
	std::string input = "ipsum";

	SECTION("basic 1") {
		std::map<int, inja::Regex> regex_patterns = {
			{1, inja::Regex("tras")},
			{2, inja::Regex("ip(\\w*)um")},
			{3, inja::Regex("do(\\w*)or")},
			{4, inja::Regex("es(\\w*)as")}
		};
		inja::MatchType<int> match = inja::match(input, regex_patterns);
		CHECK( match.type() == 2 );
		CHECK( match.str() == "ipsum" );
		CHECK( match.str(1) == "s" );
	}
}

TEST_CASE("search-on-level") {
	std::string input = "(% up %)(% up %)Test(% N1 %)(% down %)...(% up %)(% N2 %)(% up %)(% N3 %)(% down %)(% N4 %)(% down %)(% N5 %)(% down %)";

	inja::Regex regex_statement("\\(\\% (.*?) \\%\\)");
	inja::Regex regex_level_up("up");
	inja::Regex regex_level_down("down");
	inja::Regex regex_search("N(\\d+)");

	SECTION("first instance") {
		inja::Match open_match = inja::search(input, regex_statement, 0);
		CHECK( open_match.position() == 0 );
		CHECK( open_match.end_position() == 8 );
		CHECK( open_match.str(1) == "up" );

		inja::MatchClosed match = inja::search_closed_on_level(input, regex_statement, regex_level_up, regex_level_down, regex_search, open_match);
		CHECK( match.position() == 0 );
		CHECK( match.end_position() == 109 );
	}

	SECTION("second instance") {
		inja::Match open_match = inja::search(input, regex_statement, 4);

		CHECK( open_match.position() == 8 );
		CHECK( open_match.end_position() == 16 );
		CHECK( open_match.str(1) == "up" );

		inja::MatchClosed match = inja::search_closed_on_level(input, regex_statement, regex_level_up, regex_level_down, regex_search, open_match);

		CHECK( match.open_match.position() == 8 );
		CHECK( match.open_match.end_position() == 16 );
		CHECK( match.close_match.position() == 20 );
		CHECK( match.close_match.end_position() == 28 );
		CHECK( match.position() == 8 );
		CHECK( match.end_position() == 28 );
		CHECK( match.outer() == "(% up %)Test(% N1 %)" );
		CHECK( match.inner() == "Test" );
	}
}

TEST_CASE("match-functions") {
	auto map_regex = inja::Parser().regex_map_functions;

	CHECK( inja::match("not test", map_regex).type() == inja::Parsed::Function::Not );
	CHECK( inja::match("not test", map_regex).type() != inja::Parsed::Function::And );
	CHECK( inja::match("2 == 3", map_regex).type() == inja::Parsed::Function::Equal );
	CHECK( inja::match("test and test", map_regex).type() == inja::Parsed::Function::And );
	CHECK( inja::match("test and test", map_regex).type() != inja::Parsed::Function::ReadJson );
	CHECK( inja::match("test", map_regex).type() == inja::Parsed::Function::ReadJson );
	CHECK( inja::match("upper", map_regex).type() == inja::Parsed::Function::ReadJson );
	CHECK( inja::match("upper()", map_regex).type() == inja::Parsed::Function::Upper );
	CHECK( inja::match("upper(var)", map_regex).type() == inja::Parsed::Function::Upper );
	CHECK( inja::match("upper(  var	)", map_regex).type() == inja::Parsed::Function::Upper );
	CHECK( inja::match("upper(lower())", map_regex).type() == inja::Parsed::Function::Upper );
	CHECK( inja::match("upper(  lower()  )", map_regex).type() == inja::Parsed::Function::Upper );
	CHECK( inja::match(" upper(lower()) ", map_regex).type() == inja::Parsed::Function::Upper );
	CHECK( inja::match("lower(upper(test))", map_regex).type() == inja::Parsed::Function::Lower );
	CHECK( inja::match("round(2, 3)", map_regex).type() == inja::Parsed::Function::Round );

	CHECK_THROWS_WITH( inja::match("test(var)", map_regex), "Could not match input: test(var)" );
	CHECK_THROWS_WITH( inja::match("round(var)", map_regex), "Could not match input: round(var)" );
}
