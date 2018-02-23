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
	data["relatives"]["mother"] = "Maria";
	data["relatives"]["brother"] = "Chris";
	data["relatives"]["sister"] = "Jenny";


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

		// CHECK_THROWS_WITH( env.render("{{unknown}}", data), "[json.exception.out_of_range.403] key 'unknown' not found" );
	}

	SECTION("comments") {
		CHECK( env.render("Hello{# This is a comment #}!", data) == "Hello!" );
		CHECK( env.render("{# --- #Todo --- #}", data) == "" );
	}

	SECTION("loops") {
		CHECK( env.render("{% for name in names %}a{% endfor %}", data) == "aa" );
		CHECK( env.render("Hello {% for name in names %}{{ name }} {% endfor %}!", data) == "Hello Jeff Seb !" );
		CHECK( env.render("Hello {% for name in names %}{{ index }}: {{ name }}, {% endfor %}!", data) == "Hello 0: Jeff, 1: Seb, !" );
		CHECK( env.render("{% for type, name in relatives %}{{ type }}: {{ name }}, {% endfor %}", data) == "brother: Chris, mother: Maria, sister: Jenny, " );

		CHECK_THROWS_WITH( env.render("{% for name ins names %}a{% endfor %}", data), "Unknown loop statement."  );
		// CHECK_THROWS_WITH( env.render("{% for name in relatives %}{{ name }}{% endfor %}", data), "[json.exception.type_error.302] type must be array, but is object"  );
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

	SECTION("sort") {
		CHECK( env.render("{{ sort([3, 2, 1]) }}", data) == "[1,2,3]" );
		CHECK( env.render("{{ sort([\"bob\", \"charlie\", \"alice\"]) }}", data) == "[\"alice\",\"bob\",\"charlie\"]" );
		// CHECK_THROWS_WITH( env.render("{{ sort(5) }}", data), "[json.exception.type_error.302] type must be array, but is number" );
	}

	SECTION("first") {
		CHECK( env.render("{{ first(names) }}", data) == "Jeff" );
		// CHECK_THROWS_WITH( env.render("{{ length(5) }}", data), "[json.exception.type_error.302] type must be array, but is number" );
	}

	SECTION("last") {
		CHECK( env.render("{{ last(names) }}", data) == "Tom" );
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

	SECTION("max") {
		CHECK( env.render("{{ max([1, 2, 3]) }}", data) == "3" );
		CHECK( env.render("{{ max([-5.2, 100.2, 2.4]) }}", data) == "100.2" );
		// CHECK_THROWS_WITH( env.render("{{ even(name) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("min") {
		CHECK( env.render("{{ min([1, 2, 3]) }}", data) == "1" );
		CHECK( env.render("{{ min([-5.2, 100.2, 2.4]) }}", data) == "-5.2" );
		// CHECK_THROWS_WITH( env.render("{{ even(name) }}", data), "[json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("default") {
		CHECK( env.render("{{ default(11, 0) }}", data) == "11" );
		CHECK( env.render("{{ default(nothing, 0) }}", data) == "0" );
		CHECK( env.render("{{ default(name, \"nobody\") }}", data) == "Peter" );
		CHECK( env.render("{{ default(surname, \"nobody\") }}", data) == "nobody" );

		// CHECK_THROWS_WITH( env.render("{{ default(surname, lastname) }}", data), "[json.exception.out_of_range.403] key 'lastname' not found" );
	}
}

TEST_CASE("callbacks") {
	inja::Environment env = inja::Environment();
	json data;
	data["age"] = 28;

	env.add_callback("double", 1, [&env](inja::Parsed::Arguments args, json data) {
		int number = env.get_argument<double>(args, 0, data);
		return 2 * number;
	});

	env.add_callback("half", 1, [&env](inja::Parsed::Arguments args, json data) {
		int number = env.get_argument<double>(args, 0, data);
		return number / 2;
	});

	std::string greet = "Hello";
	env.add_callback("double-greetings", 0, [greet](inja::Parsed::Arguments args, json data) {
		return greet + " " + greet + "!";
	});

	env.add_callback("multiply", 2, [&env](inja::Parsed::Arguments args, json data) {
		double number1 = env.get_argument(args, 0, data);
		auto number2 = env.get_argument<double>(args, 1, data);
		return number1 * number2;
	});

	env.add_callback("multiply", 3, [&env](inja::Parsed::Arguments args, json data) {
		double number1 = env.get_argument(args, 0, data);
		double number2 = env.get_argument(args, 1, data);
		double number3 = env.get_argument(args, 2, data);
		return number1 * number2 * number3;
	});

	env.add_callback("multiply", 0, [&env](inja::Parsed::Arguments args, json data) {
		return 1.0;
	});

	CHECK( env.render("{{ double(age) }}", data) == "56" );
	CHECK( env.render("{{ half(age) }}", data) == "14" );
	CHECK( env.render("{{ double-greetings }}", data) == "Hello Hello!" );
	CHECK( env.render("{{ multiply(4, 5) }}", data) == "20.0" );
	CHECK( env.render("{{ multiply(3, 4, 5) }}", data) == "60.0" );
	CHECK( env.render("{{ multiply }}", data) == "1.0" );
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

	CHECK( env.renderer.render(temp, data) == "Peter" );

	data["is_happy"] = false;

	CHECK( env.renderer.render(temp, data) == "Brunswick" );
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
		env.set_element_notation(inja::ElementNotation::Dot);

		CHECK( env.render("{{ name }}", data) == "Peter" );
		CHECK( env.render("Hello {{ names.1 }}!", data) == "Hello Seb!" );
		CHECK( env.render("Hello {{ brother.name }}!", data) == "Hello Chris!" );
		CHECK( env.render("Hello {{ brother.daughter0.name }}!", data) == "Hello Maria!" );

		// CHECK_THROWS_WITH( env.render("{{unknown}}", data), "[json.exception.out_of_range.403] key 'unknown' not found" );
	}

	SECTION("other expression syntax") {
		inja::Environment env = inja::Environment();

		CHECK( env.render("Hello {{ name }}!", data) == "Hello Peter!" );

		env.set_expression("\\(&", "&\\)");

		CHECK( env.render("Hello {{ name }}!", data) == "Hello {{ name }}!" );
		CHECK( env.render("Hello (& name &)!", data) == "Hello Peter!" );
	}

	SECTION("other comment syntax") {
		inja::Environment env = inja::Environment();
		env.set_comment("\\(&", "&\\)");

		CHECK( env.render("Hello {# Test #}", data) == "Hello {# Test #}" );
		CHECK( env.render("Hello (& Test &)", data) == "Hello " );
	}
}
