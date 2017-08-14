#define CATCH_CONFIG_MAIN
#include "../thirdparty/catch.hpp"

#include "../../src/inja.hpp"


using Environment = inja::Environment;
using json = nlohmann::json;


TEST_CASE("String functions") {
	SECTION("Vector join") {
		REQUIRE( inja::join_strings({"1", "2", "3"}, ",") == "1,2,3" );
		REQUIRE( inja::join_strings({"1", "2", "3", "4", "5"}, " ") == "1 2 3 4 5" );
		REQUIRE( inja::join_strings({}, " ") == "" );
		REQUIRE( inja::join_strings({"single"}, "---") == "single" );
	}
	
	SECTION("Basic search") {
		inja::SearchMatch match = inja::search("lorem ipsum dolor it", std::regex("i(.*)m"), 0);
		inja::SearchMatch match_position = inja::search("lorem ipsum dolor it", std::regex("i(.*)m"), 8);
		
		REQUIRE( match.found == true );
		REQUIRE( match.position == 6 );
		REQUIRE( match.length == 5 );
		REQUIRE( match.end_position == 11 );
		REQUIRE( match.outer == "ipsum" );
		REQUIRE( match.inner == "psu" );
		
		REQUIRE( match_position.found == false );
	}
	
	SECTION("Vector search") {
		std::vector<std::string> regex_patterns = { "tras", "do(\\w*)or", "es(\\w*)as", "ip(\\w*)um" };
		inja::SearchMatch match = inja::search("lorem ipsum dolor amit estas tronum.", regex_patterns, 0);
		
		std::vector<std::string> regex_patterns_rearranged = { "tras", "ip(\\w*)um", "do(\\w*)or", "es(\\w*)as" };
		inja::SearchMatch match_rearranged = inja::search("lorem ipsum dolor amit estas tronum.", regex_patterns_rearranged, 0);
		
		REQUIRE( match.regex_number == 3 );
		REQUIRE( match.outer == "ipsum" );
		REQUIRE( match.inner == "s" );
		
		REQUIRE( match_rearranged.regex_number == 1 );
		REQUIRE( match_rearranged.outer == "ipsum" );
		REQUIRE( match_rearranged.inner == "s" );
	}
}


TEST_CASE("Parser") {
	Environment env = Environment();
	
	SECTION("Basic") {
		std::string test = "asdf";
		json result = {{{"type", "string"}, {"text", "asdf"}}};
				
		REQUIRE( env.parse(test) == result );
	}
	
	SECTION("Variables") {
		std::string test = "{{ name }}";
		json result = {{{"type", "variable"}, {"command", "name"}}};
		REQUIRE( env.parse(test) == result );
		
		std::string test_combined = "Hello {{ name }}!";
		json result_combined = {
			{{"type", "string"}, {"text", "Hello "}},
			{{"type", "variable"}, {"command", "name"}},
			{{"type", "string"}, {"text", "!"}}
		};
		REQUIRE( env.parse(test_combined) == result_combined );
		
		std::string test_multiple = "Hello {{ name }}! I come from {{ city }}.";
		json result_multiple = {
			{{"type", "string"}, {"text", "Hello "}},
			{{"type", "variable"}, {"command", "name"}},
			{{"type", "string"}, {"text", "! I come from "}},
			{{"type", "variable"}, {"command", "city"}},
			{{"type", "string"}, {"text", "."}}
		};
		REQUIRE( env.parse(test_multiple) == result_multiple );
	}
	
	SECTION("Loops") {
		std::string test = "open (% for e in list %)lorem(% endfor %) closing";
		json result = {
			{{"type", "string"}, {"text", "open "}},
			{{"type", "loop"}, {"command", "for e in list"}, {"children", {
				{{"type", "string"}, {"text", "lorem"}}
			}}},
			{{"type", "string"}, {"text", " closing"}}
		};
		
		std::string test_nested = "(% for e in list %)(% for b in list2 %)lorem(% endfor %)(% endfor %)";
		json result_nested = {
			{{"type", "loop"}, {"command", "for e in list"}, {"children", {
				{{"type", "loop"}, {"command", "for b in list2"}, {"children", {
					{{"type", "string"}, {"text", "lorem"}}
				}}}
			}}}
		};
			
		REQUIRE( env.parse(test) == result );
		REQUIRE( env.parse(test_nested) == result_nested );
	}
	
	SECTION("Conditionals") {
		std::string test = "(% if true %)dfgh(% endif %)";
		json result = {
			{{"type", "condition"}, {"children", {
				{{"type", "condition_branch"}, {"command", "if true"}, {"children", {
					{{"type", "string"}, {"text", "dfgh"}}
				}}}
			}}}
		};
		
		std::string test2 = "if: (% if maybe %)first if(% else if perhaps %)first else if(% else if sometimes %)second else if(% else %)test else(% endif %)";
		json result2 = {
			{{"type", "string"}, {"text", "if: "}},
			{{"type", "condition"}, {"children", {
				{{"type", "condition_branch"}, {"command", "if maybe"}, {"children", {
					{{"type", "string"}, {"text", "first if"}}
				}}},
				{{"type", "condition_branch"}, {"command", "else if perhaps"}, {"children", {
					{{"type", "string"}, {"text", "first else if"}}
				}}},
				{{"type", "condition_branch"}, {"command", "else if sometimes"}, {"children", {
					{{"type", "string"}, {"text", "second else if"}}
				}}}, 
				{{"type", "condition_branch"}, {"command", "else"}, {"children", {
					{{"type", "string"}, {"text", "test else"}}
				}}}, 
			}}}
		};
				
		REQUIRE( env.parse(test) == result );
		REQUIRE( env.parse(test2) == result2 );
	}
	
	
	json data;
	data["name"] = "Peter";
	data["city"] = "Washington D.C.";
	data["age"] = 29;
	data["names"] = {"Jeff", "Seb"};
	data["brother"]["name"] = "Chris";
	data["brother"]["daughters"] = {"Maria", "Helen"};
	data["brother"]["daughter0"] = { { "name", "Maria" } };
		
	SECTION("Variables from values") {
		REQUIRE( env.parse_variable("42", data) == 42 );
		REQUIRE( env.parse_variable("3.1415", data) == 3.1415 );
		REQUIRE( env.parse_variable("\"hello\"", data) == "hello" );
	}
		
	SECTION("Variables from JSON data") {
		REQUIRE( env.parse_variable("name", data) == "Peter" );
		REQUIRE( env.parse_variable("age", data) == 29 );
		REQUIRE( env.parse_variable("names/1", data) == "Seb" );
		REQUIRE( env.parse_variable("brother/name", data) == "Chris" );
		REQUIRE( env.parse_variable("brother/daughters/0", data) == "Maria" );
		REQUIRE_THROWS_WITH( env.parse_variable("noelement", data), "JSON pointer found no element." );
	}
}

TEST_CASE("Render") {
	Environment env = Environment();
	json data;
	data["name"] = "Peter";
	data["city"] = "Brunswick";
	data["age"] = 29;
	data["names"] = {"Jeff", "Seb"};
	data["brother"]["name"] = "Chris";
	data["brother"]["daughters"] = {"Maria", "Helen"};
	data["brother"]["daughter0"] = { { "name", "Maria" } };
	data["is_happy"] = true;
		
	SECTION("Basic") {
		REQUIRE( env.render("Hello World!", data) == "Hello World!" );
		REQUIRE( env.render("", data, "../") == "" );
	}
	
	SECTION("Variables") {
		REQUIRE( env.render("Hello {{ name }}!", data) == "Hello Peter!" );
		REQUIRE( env.render("{{ name }}", data) == "Peter" );
		REQUIRE( env.render("{{name}}", data) == "Peter" );
		REQUIRE( env.render("{{ name }} is {{ age }} years old.", data) == "Peter is 29 years old." );
		REQUIRE( env.render("Hello {{ name }}! I come from {{ city }}.", data) == "Hello Peter! I come from Brunswick." );
		REQUIRE( env.render("Hello {{ names/1 }}!", data) == "Hello Seb!" );
		REQUIRE( env.render("Hello {{ brother/name }}!", data) == "Hello Chris!" );
		REQUIRE( env.render("Hello {{ brother/daughter0/name }}!", data) == "Hello Maria!" );
	}
	
	SECTION("Comments") {
		REQUIRE( env.render("Hello{# This is a comment #}!", data) == "Hello!" );
		REQUIRE( env.render("{# --- #Todo --- #}", data) == "" );
	}
	
	SECTION("Loops") {
		REQUIRE( env.render("Hello (% for name in names %){{ name }} (% endfor %)!", data) == "Hello Jeff Seb !" );
		REQUIRE( env.render("Hello (% for name in names %){{ index }}: {{ name }}, (% endfor %)!", data) == "Hello 0: Jeff, 1: Seb, !" );
	}
	
	SECTION("Conditionals") {
		REQUIRE( env.render("(% if is_happy %)Yeah!(% endif %)", data) == "Yeah!" );
		REQUIRE( env.render("(% if is_sad %)Yeah!(% endif %)", data) == "" );
		REQUIRE( env.render("(% if is_sad %)Yeah!(% else %)Nooo...(% endif %)", data) == "Nooo..." );
		REQUIRE( env.render("(% if age == 29 %)Right(% else %)Wrong(% endif %)", data) == "Right" );
		REQUIRE( env.render("(% if age > 29 %)Right(% else %)Wrong(% endif %)", data) == "Wrong" );
		REQUIRE( env.render("(% if age <= 29 %)Right(% else %)Wrong(% endif %)", data) == "Right" );
		REQUIRE( env.render("(% if age != 28 %)Right(% else %)Wrong(% endif %)", data) == "Right" );
		REQUIRE( env.render("(% if age >= 30 %)Right(% else %)Wrong(% endif %)", data) == "Wrong" );
		REQUIRE( env.render("(% if age in [28, 29, 30] %)True(% endif %)", data) == "True" );
		// REQUIRE( env.render("(% if name in [\"Simon\", \"Tom\"] %)Test1(% else if name in [\"Peter\"] %)Test2(% else %)Test3(% endif %)", data) == "Test2" );
	}
}

TEST_CASE("Files") {
	Environment env = Environment();
	json data;
	data["name"] = "Jeff";
	
	SECTION("Files should be loaded") {
		REQUIRE( env.load_template("data/simple.txt") == "Hello {{ name }}." );
	}
	
	SECTION("Files should be rendered") {
		REQUIRE( env.render_template("data/simple.txt", data) == "Hello Jeff." );
	}
	
	SECTION("File includes should be rendered") {
		REQUIRE( env.render_template("data/include.txt", data) == "Answer: Hello Jeff." );
	}
}