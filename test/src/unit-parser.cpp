#include "catch.hpp"
#include "json.hpp"
#include "inja.hpp"


using Environment = inja::Environment;
using json = nlohmann::json;


TEST_CASE("parser") {
	Environment env = Environment();
	
	SECTION("basic") {
		std::string test = "asdf";
		json result = {{{"type", "string"}, {"text", "asdf"}}};
				
		REQUIRE( env.parse(test) == result );
	}
	
	SECTION("variables") {
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
	
	SECTION("loops") {
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
	
	SECTION("conditionals") {
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
		
	SECTION("variables from values") {
		REQUIRE( env.parse_variable("42", data) == 42 );
		REQUIRE( env.parse_variable("3.1415", data) == 3.1415 );
		REQUIRE( env.parse_variable("\"hello\"", data) == "hello" );
	}
		
	SECTION("variables from JSON data") {
		REQUIRE( env.parse_variable("name", data) == "Peter" );
		REQUIRE( env.parse_variable("age", data) == 29 );
		REQUIRE( env.parse_variable("names/1", data) == "Seb" );
		REQUIRE( env.parse_variable("brother/name", data) == "Chris" );
		REQUIRE( env.parse_variable("brother/daughters/0", data) == "Maria" );
		REQUIRE_THROWS_WITH( env.parse_variable("noelement", data), "JSON pointer found no element." );
	}
}