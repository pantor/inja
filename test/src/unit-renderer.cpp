#include "catch.hpp"
#include "nlohmann/json.hpp"
#include "inja.hpp"


using json = nlohmann::json;


TEST_CASE("Renderer") {
	inja::Environment env = inja::Environment();
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
		CHECK( env.render("", data) == "" );
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
		CHECK( env.render("Hello {% for name in names %}{{ name }} {% endfor %}!", data) == "Hello Jeff Seb !" );
		CHECK( env.render("Hello {% for name in names %}{{ index }}: {{ name }}, {% endfor %}!", data) == "Hello 0: Jeff, 1: Seb, !" );
	}

	SECTION("Conditionals") {
		CHECK( env.render("{% if is_happy %}Yeah!{% endif %}", data) == "Yeah!" );
		CHECK( env.render("{% if is_sad %}Yeah!{% endif %}", data) == "" );
		CHECK( env.render("{% if is_sad %}Yeah!{% else %}Nooo...{% endif %}", data) == "Nooo..." );
		CHECK( env.render("{% if age == 29 %}Right{% else %}Wrong{% endif %}", data) == "Right" );
		CHECK( env.render("{% if age > 29 %}Right{% else %}Wrong{% endif %}", data) == "Wrong" );
		CHECK( env.render("{% if age <= 29 %}Right{% else %}Wrong{% endif %}", data) == "Right" );
		CHECK( env.render("{% if age != 28 %}Right{% else %}Wrong{% endif %}", data) == "Right" );
		CHECK( env.render("{% if age >= 30 %}Right{% else %}Wrong{% endif %}", data) == "Wrong" );
		CHECK( env.render("{% if age in [28, 29, 30] %}True{% endif %}", data) == "True" );
	}
}

TEST_CASE("Render functions") {
	inja::Environment env = inja::Environment();

	json data;
	data["name"] = "Peter";
	data["city"] = "New York";
	data["names"] = {"Jeff", "Seb", "Peter", "Tom"};
	data["temperature"] = 25.6789;

	SECTION("Upper") {
		CHECK( env.render("{{ upper(name) }}", data) == "PETER" );
		CHECK( env.render("{{ upper(city) }}", data) == "NEW YORK" );
		CHECK_THROWS_WITH( env.render("{{ upper(5) }}", data), "[json.exception.type_error.302] type must be string, but is number" );
	}

	SECTION("Lower") {
		CHECK( env.render("{{ lower(name) }}", data) == "peter" );
		CHECK( env.render("{{ lower(city) }}", data) == "new york" );
		CHECK_THROWS_WITH( env.render("{{ lower(5.45) }}", data), "[json.exception.type_error.302] type must be string, but is number" );
	}

	SECTION("Range") {
		// CHECK( env.render("range(4)", data) == std::vector<int>({0, 1, 2, 3}) );
		CHECK_THROWS_WITH( env.render("{{ range(name) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("Length") {
		CHECK( env.render("{{ length(names) }}", data) == "4" );
		CHECK_THROWS_WITH( env.render("{{ length(5) }}", data), "[json.exception.type_error.302] type must be array, but is number" );
	}

	SECTION("Round") {
		CHECK( env.render("{{ round(4, 0) }}", data) == "4.0" );
		CHECK( env.render("{{ round(temperature, 2) }}", data) == "25.68" );
		CHECK_THROWS_WITH( env.render("{{ round(name, 2) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("DivisibleBy") {
		CHECK( env.render("{{ divisibleBy(50, 5) }}", data) == "true" );
		CHECK( env.render("{{ divisibleBy(12, 3) }}", data) == "true" );
		CHECK( env.render("{{ divisibleBy(11, 3) }}", data) == "false" );
		CHECK_THROWS_WITH( env.render("{{ divisibleBy(name, 2) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("Odd") {
		CHECK( env.render("{{ odd(11) }}", data) == "true" );
		CHECK( env.render("{{ odd(12) }}", data) == "false" );
		CHECK_THROWS_WITH( env.render("{{ odd(name) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("Even") {
		CHECK( env.render("{{ even(11) }}", data) == "false" );
		CHECK( env.render("{{ even(12) }}", data) == "true" );
		CHECK_THROWS_WITH( env.render("{{ even(name) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}
}

TEST_CASE("Renderer other syntax") {
	json data;
	data["name"] = "Peter";
	data["names"] = {"Jeff", "Seb"};

	SECTION("Other expression syntax") {
		inja::Environment env = inja::Environment();

		CHECK( env.render("Hello {{ name }}!", data) == "Hello Peter!" );

		env.setExpression("\\(&", "&\\)");

		CHECK( env.render("Hello {{ name }}!", data) == "Hello {{ name }}!" );
		CHECK( env.render("Hello (& name &)!", data) == "Hello Peter!" );
	}

	SECTION("Other comment syntax") {
		inja::Environment env = inja::Environment();
		env.setComment("\\(&", "&\\)");

		CHECK( env.render("Hello {# Test #}", data) == "Hello {# Test #}" );
		CHECK( env.render("Hello (& Test &)", data) == "Hello " );
	}
}
