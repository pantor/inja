#include "catch.hpp"
#include "json.hpp"
#include "inja.hpp"


TEST_CASE("String vector join function") {
	CHECK( inja::join_strings({"1", "2", "3"}, ",") == "1,2,3" );
	CHECK( inja::join_strings({"1", "2", "3", "4", "5"}, " ") == "1 2 3 4 5" );
	CHECK( inja::join_strings({}, " ") == "" );
	CHECK( inja::join_strings({"single"}, "---") == "single" );
}

TEST_CASE("Basic search in string") {
	std::string input = "lorem ipsum dolor it";
	std::regex regex("i(.*)m");

	SECTION("Basic search from start") {
		inja::SearchMatch match = inja::search(input, regex, 0);
		CHECK( match.found == true );
		CHECK( match.position == 6 );
		CHECK( match.length == 5 );
		CHECK( match.end_position == 11 );
		CHECK( match.outer == "ipsum" );
		CHECK( match.inner == "psu" );
	}

	SECTION("Basic search from position") {
		inja::SearchMatch match = inja::search(input, regex, 8);
		CHECK( match.found == false );
		CHECK( match.length == 0 );
	}
}

TEST_CASE("Search in string with multiple possible regexes") {
	std::string input = "lorem ipsum dolor amit estas tronum.";

	SECTION("Basic 1") {
		std::vector<std::string> regex_patterns = { "tras", "do(\\w*)or", "es(\\w*)as", "ip(\\w*)um" };
		inja::SearchMatchVector match = inja::search(input, regex_patterns, 0);
		CHECK( match.regex_number == 3 );
		CHECK( match.outer == "ipsum" );
		CHECK( match.inner == "s" );
	}

	SECTION("Basic 2") {
		std::vector<std::string> regex_patterns = { "tras", "ip(\\w*)um", "do(\\w*)or", "es(\\w*)as" };
		inja::SearchMatchVector match = inja::search(input, regex_patterns, 0);
		CHECK( match.regex_number == 1 );
		CHECK( match.outer == "ipsum" );
		CHECK( match.inner == "s" );
	}
}

TEST_CASE("Search on level") {
	std::string input = "(% up %)(% up %)Test(% N1 %)(% down %)...(% up %)(% N2 %)(% up %)(% N3 %)(% down %)(% N4 %)(% down %)(% N5 %)(% down %)";

	std::regex regex_statement("\\(\\% (.*?) \\%\\)");
	std::regex regex_level_up("up");
	std::regex regex_level_down("down");
	std::regex regex_search("N(\\d+)");

	SECTION("First instance") {
		inja::SearchMatch open_match = inja::search(input, regex_statement, 0);
		CHECK( open_match.position == 0 );
		CHECK( open_match.end_position == 8 );
		CHECK( open_match.inner == "up" );

		inja::SearchClosedMatch match = inja::search_closed_match_on_level(input, regex_statement, regex_level_up, regex_level_down, regex_search, open_match);
		CHECK( match.position == 0 );
		CHECK( match.end_position == 109 );
	}

	SECTION("Second instance") {
		inja::SearchMatch open_match = inja::search(input, regex_statement, 4);

		CHECK( open_match.position == 8 );
		CHECK( open_match.end_position == 16 );
		CHECK( open_match.inner == "up" );

		inja::SearchClosedMatch match = inja::search_closed_match_on_level(input, regex_statement, regex_level_up, regex_level_down, regex_search, open_match);

		CHECK( match.open_match.position == 8 );
		CHECK( match.open_match.end_position== 16 );
		CHECK( match.close_match.position == 20 );
		CHECK( match.close_match.end_position == 28 );
		CHECK( match.position == 8 );
		CHECK( match.end_position == 28 );
		CHECK( match.outer == "(% up %)Test(% N1 %)" );
		CHECK( match.inner == "Test" );
	}
}
