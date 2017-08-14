#include "catch.hpp"
#include "json.hpp"
#include "inja.hpp"


using Environment = inja::Environment;
using json = nlohmann::json;


TEST_CASE("Renderer") {
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
		CHECK( env.render("Hello World!", data) == "Hello World!" );
		CHECK( env.render("", data, "../") == "" );
	}
	
	SECTION("Variables") {
		CHECK( env.render("Hello {{ name }}!", data) == "Hello Peter!" );
		CHECK( env.render("{{ name }}", data) == "Peter" );
		CHECK( env.render("{{name}}", data) == "Peter" );
		CHECK( env.render("{{ name }} is {{ age }} years old.", data) == "Peter is 29 years old." );
		CHECK( env.render("Hello {{ name }}! I come from {{ city }}.", data) == "Hello Peter! I come from Brunswick." );
		CHECK( env.render("Hello {{ names/1 }}!", data) == "Hello Seb!" );
		CHECK( env.render("Hello {{ brother/name }}!", data) == "Hello Chris!" );
		CHECK( env.render("Hello {{ brother/daughter0/name }}!", data) == "Hello Maria!" );
	}
	
	SECTION("Comments") {
		CHECK( env.render("Hello{# This is a comment #}!", data) == "Hello!" );
		CHECK( env.render("{# --- #Todo --- #}", data) == "" );
	}
	
	SECTION("Loops") {
		CHECK( env.render("Hello (% for name in names %){{ name }} (% endfor %)!", data) == "Hello Jeff Seb !" );
		CHECK( env.render("Hello (% for name in names %){{ index }}: {{ name }}, (% endfor %)!", data) == "Hello 0: Jeff, 1: Seb, !" );
	}
	
	SECTION("Conditionals") {
		CHECK( env.render("(% if is_happy %)Yeah!(% endif %)", data) == "Yeah!" );
		CHECK( env.render("(% if is_sad %)Yeah!(% endif %)", data) == "" );
		CHECK( env.render("(% if is_sad %)Yeah!(% else %)Nooo...(% endif %)", data) == "Nooo..." );
		CHECK( env.render("(% if age == 29 %)Right(% else %)Wrong(% endif %)", data) == "Right" );
		CHECK( env.render("(% if age > 29 %)Right(% else %)Wrong(% endif %)", data) == "Wrong" );
		CHECK( env.render("(% if age <= 29 %)Right(% else %)Wrong(% endif %)", data) == "Right" );
		CHECK( env.render("(% if age != 28 %)Right(% else %)Wrong(% endif %)", data) == "Right" );
		CHECK( env.render("(% if age >= 30 %)Right(% else %)Wrong(% endif %)", data) == "Wrong" );
		CHECK( env.render("(% if age in [28, 29, 30] %)True(% endif %)", data) == "True" );
		
		// Only works with gcc-5
		// CHECK( env.render("(% if name in [\"Simon\", \"Tom\"] %)Test1(% else if name in [\"Peter\"] %)Test2(% else %)Test3(% endif %)", data) == "Test2" );
	}
}