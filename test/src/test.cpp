#define CATCH_CONFIG_MAIN
#include "../thirdparty/catch.hpp"

#include "../../src/inja.hpp"


using Environment = inja::Environment;
using json = nlohmann::json;


TEST_CASE("Variables") {	
	Environment env = Environment();
	json data;
	data["name"] = "Peter";
	data["city"] = "Washington D.C.";
	data["names"] = {"Jeff", "Seb"};
	data["brother"]["name"] = "Chris";
	data["brother"]["daughters"] = {"Maria", "Helen"};
	data["brother"]["daughter0"] = { { "name", "Maria" } };
	
	SECTION("Variables from values") {
		REQUIRE( env.get_variable_data("42", data) == 42 );
		REQUIRE( env.get_variable_data("3.1415", data) == 3.1415 );
		REQUIRE( env.get_variable_data("\"hello\"", data) == "hello" );
	}
	
	SECTION("Variables from functions") {
		REQUIRE( env.get_variable_data("range(3)", data) == std::vector<int>({0, 1, 2}) );
	}
	
	SECTION("Variables from JSON data") {
		REQUIRE( env.get_variable_data("name", data) == "Peter" );
		REQUIRE( env.get_variable_data("names/1", data) == "Seb" );
		REQUIRE( env.get_variable_data("brother/name", data) == "Chris" );
		REQUIRE( env.get_variable_data("brother/daughters/0", data) == "Maria" );
		REQUIRE_THROWS_WITH( env.get_variable_data("noelement", data), "JSON pointer found no element." );
	}
	
	SECTION("Variables should be rendered") {
		REQUIRE( env.render("My name is...", data) == "My name is..." );
		REQUIRE( env.render("My name is {{ name }}.", data) == "My name is Peter." );
		REQUIRE( env.render("My name is {{name}}.", data) == "My name is Peter." );
		REQUIRE( env.render("My name is {{ name }}. I come from {{ city }}", data) == "My name is Peter. I come from Washington D.C." );
		REQUIRE( env.render("My name is {{ names/1 }}.", data) == "My name is Seb." );
		REQUIRE( env.render("My name is {{ brother/name }}.", data) == "My name is Chris." );
		REQUIRE( env.render("My name is {{ brother/daughter0/name }}.", data) == "My name is Maria." );
	}
}

TEST_CASE("Loops should be rendered") {
	Environment env = Environment();
	json data;
	data["list"] = {"v1", "v2", "v3", "v4"};
	
	REQUIRE( env.render("List: (% for entry in list %)a(% endfor %)", data) == "List: aaaa" );
	REQUIRE( env.render("List: (% for entry in list %){{ entry }}, (% endfor %)", data) == "List: v1, v2, v3, v4, " );
	REQUIRE( env.render("List: (% for i in range(4) %)a(% endfor %)", data) == "List: aaaa" );
}

TEST_CASE("Conditionals should be rendered") {
	Environment env = Environment();
	json data;
	data["good"] = true;
	data["bad"] = false;
	data["a"] = 2;
	
	REQUIRE( env.render("(% if good %)a(% endif %)", data) == "a" );
	// REQUIRE( env.render("(% if good %)a(% endif %) (% if bad %)b(% endif %)", data) == "a " );
	// REQUIRE( env.render("(% if good %)one(% else %)two(% endif %)", data) == "one" );
	// REQUIRE( env.render("(% if bad %)one(% else %)two(% endif %)", data) == "two" );
	// REQUIRE( env.render("(% if a == 2 %)one(% else %)two(% endif %)", data) == "one" );
	// REQUIRE( env.render("(% if "b" in {"a", "b", "c"} %)one(% endif %)", data) == "one" );
}

TEST_CASE("Files should handled") {
	Environment env = Environment();
	json data;
	data["name"] = "Jeff";
	
	SECTION("Files should be loaded") {
		REQUIRE( env.load_template("../data/simple.txt") == "Hello {{ name }}." );
	}
	
	SECTION("Files should be rendered") {
		REQUIRE( env.render_template("../data/simple.txt", data) == "Hello Jeff." );
	}
	
	SECTION("File includes should be rendered") {
		REQUIRE( env.render_template("../data/include.txt", data) == "Answer: Hello Jeff." );
	}
}