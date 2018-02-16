#include "catch.hpp"
#include "nlohmann/json.hpp"
#include "inja.hpp"



using json = nlohmann::json;


TEST_CASE("types") {
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
	data["is_sad"] = false;

	SECTION("basic") {
		CHECK( env.render("", data) == "" );
		CHECK( env.render("Hello World!", data) == "Hello World!" );
	}

	SECTION("variables") {
		CHECK( env.render("Hello {{ name }}!", data) == "Hello Peter!" );
		CHECK( env.render("{{ name }}", data) == "Peter" );
		CHECK( env.render("{{name}}", data) == "Peter" );
		CHECK( env.render("{{ name }} is {{ age }} years old.", data) == "Peter is 29 years old." );
		CHECK( env.render("Hello {{ name }}! I come from {{ city }}.", data) == "Hello Peter! I come from Brunswick." );
		CHECK( env.render("Hello {{ names/1 }}!", data) == "Hello Seb!" );
		CHECK( env.render("Hello {{ brother/name }}!", data) == "Hello Chris!" );
		CHECK( env.render("Hello {{ brother/daughter0/name }}!", data) == "Hello Maria!" );

		CHECK_THROWS_WITH( env.render("{{unknown}}", data), "Did not found json element: unknown" );
	}

	SECTION("comments") {
		CHECK( env.render("Hello{# This is a comment #}!", data) == "Hello!" );
		CHECK( env.render("{# --- #Todo --- #}", data) == "" );
	}

	SECTION("loops") {
		CHECK( env.render("{% for name in names %}a{% endfor %}", data) == "aa" );
		CHECK( env.render("Hello {% for name in names %}{{ name }} {% endfor %}!", data) == "Hello Jeff Seb !" );
		CHECK( env.render("Hello {% for name in names %}{{ index }}: {{ name }}, {% endfor %}!", data) == "Hello 0: Jeff, 1: Seb, !" );
	}

	SECTION("conditionals") {
		CHECK( env.render("{% if is_happy %}Yeah!{% endif %}", data) == "Yeah!" );
		CHECK( env.render("{% if is_sad %}Yeah!{% endif %}", data) == "" );
		CHECK( env.render("{% if is_sad %}Yeah!{% else %}Nooo...{% endif %}", data) == "Nooo..." );
		CHECK( env.render("{% if age == 29 %}Right{% else %}Wrong{% endif %}", data) == "Right" );
		CHECK( env.render("{% if age > 29 %}Right{% else %}Wrong{% endif %}", data) == "Wrong" );
		CHECK( env.render("{% if age <= 29 %}Right{% else %}Wrong{% endif %}", data) == "Right" );
		CHECK( env.render("{% if age != 28 %}Right{% else %}Wrong{% endif %}", data) == "Right" );
		CHECK( env.render("{% if age >= 30 %}Right{% else %}Wrong{% endif %}", data) == "Wrong" );
		CHECK( env.render("{% if age in [28, 29, 30] %}True{% endif %}", data) == "True" );
		CHECK( env.render("{% if age == 28 %}28{% else if age == 29 %}29{% endif %}", data) == "29" );
		CHECK( env.render("{% if age == 26 %}26{% else if age == 27 %}27{% else if age == 28 %}28{% else %}29{% endif %}", data) == "29" );
	}
}

TEST_CASE("functions") {
	inja::Environment env = inja::Environment();

	json data;
	data["name"] = "Peter";
	data["city"] = "New York";
	data["names"] = {"Jeff", "Seb", "Peter", "Tom"};
	data["temperature"] = 25.6789;

	SECTION("upper") {
		CHECK( env.render("{{ upper(name) }}", data) == "PETER" );
		CHECK( env.render("{{ upper(  name  ) }}", data) == "PETER" );
		CHECK( env.render("{{ upper(city) }}", data) == "NEW YORK" );
		CHECK( env.render("{{ upper(upper(name)) }}", data) == "PETER" );
		// CHECK_THROWS_WITH( env.render("{{ upper(5) }}", data), "[json.exception.type_error.302] type must be string, but is number" );
		// CHECK_THROWS_WITH( env.render("{{ upper(true) }}", data), "[json.exception.type_error.302] type must be string, but is boolean" );
	}

	SECTION("lower") {
		CHECK( env.render("{{ lower(name) }}", data) == "peter" );
		CHECK( env.render("{{ lower(city) }}", data) == "new york" );
		// CHECK_THROWS_WITH( env.render("{{ lower(5.45) }}", data), "[json.exception.type_error.302] type must be string, but is number" );
	}

	SECTION("range") {
		CHECK( env.render("{{ range(2) }}", data) == "[0,1]" );
		CHECK( env.render("{{ range(4) }}", data) == "[0,1,2,3]" );
		// CHECK_THROWS_WITH( env.render("{{ range(name) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("length") {
		CHECK( env.render("{{ length(names) }}", data) == "4" );
		// CHECK_THROWS_WITH( env.render("{{ length(5) }}", data), "[json.exception.type_error.302] type must be array, but is number" );
	}

	SECTION("round") {
		CHECK( env.render("{{ round(4, 0) }}", data) == "4.0" );
		CHECK( env.render("{{ round(temperature, 2) }}", data) == "25.68" );
		// CHECK_THROWS_WITH( env.render("{{ round(name, 2) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("divisibleBy") {
		CHECK( env.render("{{ divisibleBy(50, 5) }}", data) == "true" );
		CHECK( env.render("{{ divisibleBy(12, 3) }}", data) == "true" );
		CHECK( env.render("{{ divisibleBy(11, 3) }}", data) == "false" );
		// CHECK_THROWS_WITH( env.render("{{ divisibleBy(name, 2) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("odd") {
		CHECK( env.render("{{ odd(11) }}", data) == "true" );
		CHECK( env.render("{{ odd(12) }}", data) == "false" );
		// CHECK_THROWS_WITH( env.render("{{ odd(name) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("even") {
		CHECK( env.render("{{ even(11) }}", data) == "false" );
		CHECK( env.render("{{ even(12) }}", data) == "true" );
		// CHECK_THROWS_WITH( env.render("{{ even(name) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}
}

TEST_CASE("combinations") {
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

	CHECK( env.render("{% if upper(\"Peter\") == \"PETER\" %}TRUE{% endif %}", data) == "TRUE" );
	CHECK( env.render("{% if lower(upper(name)) == \"peter\" %}TRUE{% endif %}", data) == "TRUE" );
	CHECK( env.render("{% for i in range(4) %}{{ index1 }}{% endfor %}", data) == "1234" );
}

TEST_CASE("templates") {
	inja::Environment env = inja::Environment();
	inja::Template temp = env.parse("{% if is_happy %}{{ name }}{% else %}{{ city }}{% endif %}");

	json data;
	data["name"] = "Peter";
	data["city"] = "Brunswick";
	data["is_happy"] = true;

	CHECK( temp.render(data) == "Peter" );

	data["is_happy"] = false;

	CHECK( temp.render(data) == "Brunswick" );
}


TEST_CASE("callback")
{
	class CTestData : public inja::IStatementCallback
	{
	public:
		virtual std::string onCallback(std::string name) const override { return name + name; }
	} statementCallback;

	inja::Environment env = inja::Environment();
	env.setStatementCallback(statementCallback);

	json data;
	std::string res = env.render("test {% callback \"hi\" %} test", data);
	CHECK(res == "test hihi test");
}

TEST_CASE("other-syntax") {
	json data;
	data["name"] = "Peter";
	data["city"] = "Brunswick";
	data["age"] = 29;
	data["names"] = {"Jeff", "Seb"};
	data["brother"]["name"] = "Chris";
	data["brother"]["daughters"] = {"Maria", "Helen"};
	data["brother"]["daughter0"] = { { "name", "Maria" } };
	data["is_happy"] = true;

	SECTION("variables") {
		inja::Environment env = inja::Environment();
		env.setElementNotation(inja::ElementNotation::Dot);

		CHECK( env.render("{{ name }}", data) == "Peter" );
		CHECK( env.render("Hello {{ names.1 }}!", data) == "Hello Seb!" );
		CHECK( env.render("Hello {{ brother.name }}!", data) == "Hello Chris!" );
		CHECK( env.render("Hello {{ brother.daughter0.name }}!", data) == "Hello Maria!" );

		CHECK_THROWS_WITH( env.render("{{unknown}}", data), "Did not found json element: unknown" );
	}

	SECTION("other expression syntax") {
		inja::Environment env = inja::Environment();

		CHECK( env.render("Hello {{ name }}!", data) == "Hello Peter!" );

		env.setExpression("\\(&", "&\\)");

		CHECK( env.render("Hello {{ name }}!", data) == "Hello {{ name }}!" );
		CHECK( env.render("Hello (& name &)!", data) == "Hello Peter!" );
	}

	SECTION("other comment syntax") {
		inja::Environment env = inja::Environment();
		env.setComment("\\(&", "&\\)");

		CHECK( env.render("Hello {# Test #}", data) == "Hello {# Test #}" );
		CHECK( env.render("Hello (& Test &)", data) == "Hello " );
	}
}
