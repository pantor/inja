#include "catch.hpp"
#include "json.hpp"
#include "inja.hpp"


using Environment = inja::Environment;
using json = nlohmann::json;


TEST_CASE("string vector join function") {
	REQUIRE( inja::join_strings({"1", "2", "3"}, ",") == "1,2,3" );
	REQUIRE( inja::join_strings({"1", "2", "3", "4", "5"}, " ") == "1 2 3 4 5" );
	REQUIRE( inja::join_strings({}, " ") == "" );
	REQUIRE( inja::join_strings({"single"}, "---") == "single" );
}

TEST_CASE("basic search in string") {
	std::string input = "lorem ipsum dolor it";
	std::regex regex("i(.*)m");
	
	SECTION("basic search from start") {
		inja::SearchMatch match = inja::search(input, regex, 0);
		REQUIRE( match.found == true );
		REQUIRE( match.position == 6 );
		REQUIRE( match.length == 5 );
		REQUIRE( match.end_position == 11 );
		REQUIRE( match.outer == "ipsum" );
		REQUIRE( match.inner == "psu" );
	}
	
	SECTION("basic search from position") {
		inja::SearchMatch match = inja::search(input, regex, 8);
		REQUIRE( match.found == false );
		REQUIRE( match.length == 0 );
	}
}

TEST_CASE("search in string with multiple possible regexes") {
	std::string input = "lorem ipsum dolor amit estas tronum.";
	
	SECTION("basic 1") {
		std::vector<std::string> regex_patterns = { "tras", "do(\\w*)or", "es(\\w*)as", "ip(\\w*)um" };
		inja::SearchMatch match = inja::search(input, regex_patterns, 0);
		REQUIRE( match.regex_number == 3 );
		REQUIRE( match.outer == "ipsum" );
		REQUIRE( match.inner == "s" );
	}
	
	SECTION("basic 2") {
		std::vector<std::string> regex_patterns = { "tras", "ip(\\w*)um", "do(\\w*)or", "es(\\w*)as" };
		inja::SearchMatch match = inja::search(input, regex_patterns, 0);
		REQUIRE( match.regex_number == 1 );
		REQUIRE( match.outer == "ipsum" );
		REQUIRE( match.inner == "s" );
	}
}

TEST_CASE("search on level") {
	std::string input = "(% up %)(% up %)(% N1 %)(% down %)...(% up %)(% N2 %)(% up %)(% N3 %)(% down %)(% N4 %)(% down %)(% N5 %)(% down %)";
	
	std::regex regex_statement("\\(\\% (.*?) \\%\\)");
	std::regex regex_level_up("up");
	std::regex regex_level_down("down");
	std::regex regex_search("N(\\d+)");
	
	SECTION("basic 1") {
		inja::SearchMatch open_match = inja::search(input, regex_statement, 0);
		
		REQUIRE( open_match.position == 0 );
		REQUIRE( open_match.end_position == 8 );
		REQUIRE( open_match.inner == "up" );
		
		inja::SearchClosedMatch match = inja::search_on_level(input, regex_statement, regex_level_up, regex_level_down, regex_search, open_match);
		
		REQUIRE( match.position == 0 );
		REQUIRE( match.end_position == 105 );
	}
	
	SECTION("basic 1") {
		inja::SearchMatch open_match = inja::search(input, regex_statement, 4);
		
		REQUIRE( open_match.position == 8 );
		REQUIRE( open_match.end_position == 16 );
		REQUIRE( open_match.inner == "up" );
		
		inja::SearchClosedMatch match = inja::search_on_level(input, regex_statement, regex_level_up, regex_level_down, regex_search, open_match);
		
		REQUIRE( match.position == 8 );
		// REQUIRE( match.end_position == 24 );
		// REQUIRE( match.outer == "(% up %)(% N1 %)(% down %)" );
		// REQUIRE( match.inner == "(% N1 %)" );
	}
}