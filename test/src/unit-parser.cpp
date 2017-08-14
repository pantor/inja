#include "catch.hpp"
#include "json.hpp"
#include "inja.hpp"


using Environment = inja::Environment;
using json = nlohmann::json;
using Type = inja::Parser::Type;


TEST_CASE("parse structure") {
	Environment env = Environment();
	
	SECTION("basic") {
		std::string test = "asdf";
		json result = {{{"type", Type::String}, {"text", "asdf"}}};
				
		CHECK( env.parse(test) == result );
	}
	
	SECTION("variables") {
		std::string test = "{{ name }}";
		json result = {{{"type", Type::Variable}, {"command", "name"}}};
		CHECK( env.parse(test) == result );
		
		std::string test_combined = "Hello {{ name }}!";
		json result_combined = {
			{{"type", Type::String}, {"text", "Hello "}},
			{{"type", Type::Variable}, {"command", "name"}},
			{{"type", Type::String}, {"text", "!"}}
		};
		CHECK( env.parse(test_combined) == result_combined );
		
		std::string test_multiple = "Hello {{ name }}! I come from {{ city }}.";
		json result_multiple = {
			{{"type", Type::String}, {"text", "Hello "}},
			{{"type", Type::Variable}, {"command", "name"}},
			{{"type", Type::String}, {"text", "! I come from "}},
			{{"type", Type::Variable}, {"command", "city"}},
			{{"type", Type::String}, {"text", "."}}
		};
		CHECK( env.parse(test_multiple) == result_multiple );
	}
	
	SECTION("loops") {
		std::string test = "open (% for e in list %)lorem(% endfor %) closing";
		json result = {
			{{"type", Type::String}, {"text", "open "}},
			{{"type", Type::Loop}, {"command", "for e in list"}, {"children", {
				{{"type", Type::String}, {"text", "lorem"}}
			}}},
			{{"type", Type::String}, {"text", " closing"}}
		};
		
		std::string test_nested = "(% for e in list %)(% for b in list2 %)lorem(% endfor %)(% endfor %)";
		json result_nested = {
			{{"type", Type::Loop}, {"command", "for e in list"}, {"children", {
				{{"type", Type::Loop}, {"command", "for b in list2"}, {"children", {
					{{"type", Type::String}, {"text", "lorem"}}
				}}}
			}}}
		};
			
		CHECK( env.parse(test) == result );
		CHECK( env.parse(test_nested) == result_nested );
	}
	
	SECTION("conditionals") {
		std::string test = "(% if true %)dfgh(% endif %)";
		json result = {
			{{"type", Type::Condition}, {"children", {
				{{"type", Type::ConditionBranch}, {"command", "if true"}, {"children", {
					{{"type", Type::String}, {"text", "dfgh"}}
				}}}
			}}}
		};
		
		std::string test2 = "if: (% if maybe %)first if(% else if perhaps %)first else if(% else if sometimes %)second else if(% else %)test else(% endif %)";
		json result2 = {
			{{"type", Type::String}, {"text", "if: "}},
			{{"type", Type::Condition}, {"children", {
				{{"type", Type::ConditionBranch}, {"command", "if maybe"}, {"children", {
					{{"type", Type::String}, {"text", "first if"}}
				}}},
				{{"type", Type::ConditionBranch}, {"command", "else if perhaps"}, {"children", {
					{{"type", Type::String}, {"text", "first else if"}}
				}}},
				{{"type", Type::ConditionBranch}, {"command", "else if sometimes"}, {"children", {
					{{"type", Type::String}, {"text", "second else if"}}
				}}}, 
				{{"type", Type::ConditionBranch}, {"command", "else"}, {"children", {
					{{"type", Type::String}, {"text", "test else"}}
				}}}, 
			}}}
		};
				
		CHECK( env.parse(test) == result );
		CHECK( env.parse(test2) == result2 );
	}
}
	
TEST_CASE("parse json") {
	Environment env = Environment();
	
	json data;
	data["name"] = "Peter";
	data["city"] = "Washington D.C.";
	data["age"] = 29;
	data["names"] = {"Jeff", "Seb"};
	data["brother"]["name"] = "Chris";
	data["brother"]["daughters"] = {"Maria", "Helen"};
	data["brother"]["daughter0"] = { { "name", "Maria" } };
		
	SECTION("variables from values") {
		CHECK( env.parse_variable("42", data) == 42 );
		CHECK( env.parse_variable("3.1415", data) == 3.1415 );
		CHECK( env.parse_variable("\"hello\"", data) == "hello" );
	}
		
	SECTION("variables from JSON data") {
		CHECK( env.parse_variable("name", data) == "Peter" );
		CHECK( env.parse_variable("age", data) == 29 );
		CHECK( env.parse_variable("names/1", data) == "Seb" );
		CHECK( env.parse_variable("brother/name", data) == "Chris" );
		CHECK( env.parse_variable("brother/daughters/0", data) == "Maria" );
		CHECK_THROWS_WITH( env.parse_variable("noelement", data), "JSON pointer found no element." );
	}
}

TEST_CASE("parse conditions") {
	Environment env = Environment();
	
	json data;
	data["age"] = 29;
	data["brother"] = "Peter";
	data["father"] = "Peter";
	
	SECTION("elements") {
		// CHECK( env.parse_condition("age", data) );
		CHECK_FALSE( env.parse_condition("size", data) );
	}
	
	SECTION("numbers") {
		CHECK( env.parse_condition("age == 29", data) );
		CHECK( env.parse_condition("age >= 29", data) );
		CHECK( env.parse_condition("age <= 29", data) );
		CHECK( env.parse_condition("age < 100", data) );
		CHECK_FALSE( env.parse_condition("age > 29", data) );
		CHECK_FALSE( env.parse_condition("age != 29", data) );
		CHECK_FALSE( env.parse_condition("age < 28", data) );
		CHECK_FALSE( env.parse_condition("age < -100.0", data) );
	}
	
	SECTION("strings") {
		CHECK( env.parse_condition("brother == father", data) );
		CHECK( env.parse_condition("brother == \"Peter\"", data) );
	}
}