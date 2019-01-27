#include "catch/catch.hpp"
#include "inja/inja.hpp"


using json = nlohmann::json;


TEST_CASE("dot-to-pointer") {
	std::string buffer;
	CHECK( inja::convert_dot_to_json_pointer("person.names.surname", buffer) == "/person/names/surname" );
	CHECK( inja::convert_dot_to_json_pointer("guests.2", buffer) == "/guests/2" );
}

TEST_CASE("types") {
	inja::Environment env;
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
	data["vars"] = {2, 3, 4, 0, -1, -2, -3};


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
		CHECK( env.render("Hello {{ names.1 }}!", data) == "Hello Seb!" );
		CHECK( env.render("Hello {{ brother.name }}!", data) == "Hello Chris!" );
		CHECK( env.render("Hello {{ brother.daughter0.name }}!", data) == "Hello Maria!" );
		CHECK( env.render("{{ \"{{ no_value }}\" }}", data) == "{{ no_value }}" );

		CHECK_THROWS_WITH( env.render("{{unknown}}", data), "[inja.exception.render_error] variable 'unknown' not found" );
	}

	SECTION("comments") {
		CHECK( env.render("Hello{# This is a comment #}!", data) == "Hello!" );
		CHECK( env.render("{# --- #Todo --- #}", data) == "" );
	}

	SECTION("loops") {
		CHECK( env.render("{% for name in names %}a{% endfor %}", data) == "aa" );
		CHECK( env.render("Hello {% for name in names %}{{ name }} {% endfor %}!", data) == "Hello Jeff Seb !" );
		CHECK( env.render("Hello {% for name in names %}{{ loop.index }}: {{ name }}, {% endfor %}!", data) == "Hello 0: Jeff, 1: Seb, !" );
		CHECK( env.render("{% for type, name in relatives %}{{ type }}: {{ name }}, {% endfor %}", data) == "brother: Chris, mother: Maria, sister: Jenny, " );
		CHECK( env.render("{% for v in vars %}{% if v > 0 %}+{% endif %}{% endfor %}", data) == "+++" );
		CHECK( env.render("{% for name in names %}{{ loop.index }}: {{ name }}{% if not loop.is_last %}, {% endif %}{% endfor %}!", data) == "0: Jeff, 1: Seb!" );
		CHECK( env.render("{% for name in names %}{{ loop.index }}: {{ name }}{% if loop.is_last == false %}, {% endif %}{% endfor %}!", data) == "0: Jeff, 1: Seb!" );

		data["empty_loop"] = {};
		CHECK( env.render("{% for name in empty_loop %}a{% endfor %}", data) == "" );
		CHECK( env.render("{% for name in {} %}a{% endfor %}", data) == "" );

		CHECK_THROWS_WITH( env.render("{% for name ins names %}a{% endfor %}", data), "[inja.exception.parser_error] expected 'in', got 'ins'" );
		// CHECK_THROWS_WITH( env.render("{% for name in relatives %}{{ name }}{% endfor %}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is object" );
	}

	SECTION("nested loops") {
		auto ldata = json::parse(
R"DELIM(
{ "outer" : [ 
	{ "inner" : [
		{ "in2" : [ 1, 2 ] },
		{ "in2" : []},
		{ "in2" : []}
		]
	},
	{ "inner" : [] },
	{ "inner" : [
		{ "in2" : [ 3, 4 ] },
		{ "in2" : [ 5, 6 ] }
		]
	}
	]
}
)DELIM"
				);
		CHECK(env.render(R"DELIM(
{% for o in outer %}{% for i in o.inner %}{{loop.parent.index}}:{{loop.index}}::{{loop.parent.is_last}}
{% for ii in i.in2%}{{ii}},{%endfor%}
{%endfor%}{%endfor%}
)DELIM",
					ldata) == "\n0:0::false\n1,2,\n0:1::false\n\n0:2::false\n\n2:0::true\n3,4,\n2:1::true\n5,6,\n\n");
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
		CHECK( env.render("{% if age == 25 %}+{% endif %}{% if age == 29 %}+{% else %}-{% endif %}", data) == "+" );

		CHECK_THROWS_WITH( env.render("{% if is_happy %}{% if is_happy %}{% endif %}", data), "[inja.exception.parser_error] unmatched if" );
		CHECK_THROWS_WITH( env.render("{% if is_happy %}{% else if is_happy %}{% end if %}", data), "[inja.exception.parser_error] expected statement, got 'end'" );
	}

	SECTION("line statements") {
		CHECK( env.render(R"(## if is_happy
Yeah!
## endif)", data) == R"(Yeah!
)" );

		CHECK( env.render(R"(## if is_happy
## if is_happy
Yeah!
## endif
## endif    )", data) == R"(Yeah!
)" );
}
}


TEST_CASE("functions") {
	inja::Environment env;

	json data;
	data["name"] = "Peter";
	data["city"] = "New York";
	data["names"] = {"Jeff", "Seb", "Peter", "Tom"};
	data["temperature"] = 25.6789;
	data["brother"]["name"] = "Chris";
	data["brother"]["daughters"] = {"Maria", "Helen"};
	data["property"] = "name";
	data["age"] = 29;
	data["is_happy"] = true;
	data["is_sad"] = false;
	data["vars"] = {2, 3, 4, 0, -1, -2, -3};

	SECTION("upper") {
		CHECK( env.render("{{ upper(name) }}", data) == "PETER" );
		CHECK( env.render("{{ upper(  name  ) }}", data) == "PETER" );
		CHECK( env.render("{{ upper(city) }}", data) == "NEW YORK" );
		CHECK( env.render("{{ upper(upper(name)) }}", data) == "PETER" );
		// CHECK_THROWS_WITH( env.render("{{ upper(5) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be string, but is number" );
		// CHECK_THROWS_WITH( env.render("{{ upper(true) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be string, but is boolean" );
	}

	SECTION("lower") {
		CHECK( env.render("{{ lower(name) }}", data) == "peter" );
		CHECK( env.render("{{ lower(city) }}", data) == "new york" );
		// CHECK_THROWS_WITH( env.render("{{ lower(5.45) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be string, but is number" );
	}

	SECTION("range") {
		CHECK( env.render("{{ range(2) }}", data) == "[0,1]" );
		CHECK( env.render("{{ range(4) }}", data) == "[0,1,2,3]" );
		// CHECK_THROWS_WITH( env.render("{{ range(name) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("length") {
		CHECK( env.render("{{ length(names) }}", data) == "4" );
		// CHECK_THROWS_WITH( env.render("{{ length(5) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is number" );
	}

	SECTION("sort") {
		CHECK( env.render("{{ sort([3, 2, 1]) }}", data) == "[1,2,3]" );
		CHECK( env.render("{{ sort([\"bob\", \"charlie\", \"alice\"]) }}", data) == "[\"alice\",\"bob\",\"charlie\"]" );
		// CHECK_THROWS_WITH( env.render("{{ sort(5) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is number" );
	}

	SECTION("first") {
		CHECK( env.render("{{ first(names) }}", data) == "Jeff" );
		// CHECK_THROWS_WITH( env.render("{{ first(5) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is number" );
	}

	SECTION("last") {
		CHECK( env.render("{{ last(names) }}", data) == "Tom" );
		// CHECK_THROWS_WITH( env.render("{{ last(5) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is number" );
	}

	SECTION("round") {
		CHECK( env.render("{{ round(4, 0) }}", data) == "4.0" );
		CHECK( env.render("{{ round(temperature, 2) }}", data) == "25.68" );
		// CHECK_THROWS_WITH( env.render("{{ round(name, 2) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("divisibleBy") {
		CHECK( env.render("{{ divisibleBy(50, 5) }}", data) == "true" );
		CHECK( env.render("{{ divisibleBy(12, 3) }}", data) == "true" );
		CHECK( env.render("{{ divisibleBy(11, 3) }}", data) == "false" );
		// CHECK_THROWS_WITH( env.render("{{ divisibleBy(name, 2) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("odd") {
		CHECK( env.render("{{ odd(11) }}", data) == "true" );
		CHECK( env.render("{{ odd(12) }}", data) == "false" );
		// CHECK_THROWS_WITH( env.render("{{ odd(name) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("even") {
		CHECK( env.render("{{ even(11) }}", data) == "false" );
		CHECK( env.render("{{ even(12) }}", data) == "true" );
		// CHECK_THROWS_WITH( env.render("{{ even(name) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be number, but is string" );
	}

	SECTION("max") {
		CHECK( env.render("{{ max([1, 2, 3]) }}", data) == "3" );
		CHECK( env.render("{{ max([-5.2, 100.2, 2.4]) }}", data) == "100.2" );
		// CHECK_THROWS_WITH( env.render("{{ max(name) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is string" );
	}

	SECTION("min") {
		CHECK( env.render("{{ min([1, 2, 3]) }}", data) == "1" );
		CHECK( env.render("{{ min([-5.2, 100.2, 2.4]) }}", data) == "-5.2" );
		// CHECK_THROWS_WITH( env.render("{{ min(name) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is string" );
	}

	SECTION("float") {
		CHECK( env.render("{{ float(\"2.2\") == 2.2 }}", data) == "true" );
		CHECK( env.render("{{ float(\"-1.25\") == -1.25 }}", data) == "true" );
		// CHECK_THROWS_WITH( env.render("{{ max(name) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is string" );
	}

	SECTION("int") {
		CHECK( env.render("{{ int(\"2\") == 2 }}", data) == "true" );
		CHECK( env.render("{{ int(\"-1.25\") == -1 }}", data) == "true" );
		// CHECK_THROWS_WITH( env.render("{{ max(name) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is string" );
	}

	SECTION("default") {
		CHECK( env.render("{{ default(11, 0) }}", data) == "11" );
		CHECK( env.render("{{ default(nothing, 0) }}", data) == "0" );
		CHECK( env.render("{{ default(name, \"nobody\") }}", data) == "Peter" );
		CHECK( env.render("{{ default(surname, \"nobody\") }}", data) == "nobody" );
		CHECK( env.render("{{ default(surname, \"{{ surname }}\") }}", data) == "{{ surname }}" );
		CHECK_THROWS_WITH( env.render("{{ default(surname, lastname) }}", data), "[inja.exception.render_error] variable 'lastname' not found" );
	}

	SECTION("exists") {
		CHECK( env.render("{{ exists(\"name\") }}", data) == "true" );
		CHECK( env.render("{{ exists(\"zipcode\") }}", data) == "false" );
		CHECK( env.render("{{ exists(name) }}", data) == "false" );
		CHECK( env.render("{{ exists(property) }}", data) == "true" );
	}

	SECTION("existsIn") {
		CHECK( env.render("{{ existsIn(brother, \"name\") }}", data) == "true" );
		CHECK( env.render("{{ existsIn(brother, \"parents\") }}", data) == "false" );
		CHECK( env.render("{{ existsIn(brother, property) }}", data) == "true" );
		CHECK( env.render("{{ existsIn(brother, name) }}", data) == "false" );
		CHECK_THROWS_WITH( env.render("{{ existsIn(sister, \"lastname\") }}", data), "[inja.exception.render_error] variable 'sister' not found" );
		CHECK_THROWS_WITH( env.render("{{ existsIn(brother, sister) }}", data), "[inja.exception.render_error] variable 'sister' not found" );
	}

	SECTION("isType") {
		CHECK( env.render("{{ isBoolean(is_happy) }}", data) == "true" );
		CHECK( env.render("{{ isBoolean(vars) }}", data) == "false" );
		CHECK( env.render("{{ isNumber(age) }}", data) == "true" );
		CHECK( env.render("{{ isNumber(name) }}", data) == "false" );
		CHECK( env.render("{{ isInteger(age) }}", data) == "true" );
		CHECK( env.render("{{ isInteger(is_happy) }}", data) == "false" );
		CHECK( env.render("{{ isFloat(temperature) }}", data) == "true" );
		CHECK( env.render("{{ isFloat(age) }}", data) == "false" );
		CHECK( env.render("{{ isObject(brother) }}", data) == "true" );
		CHECK( env.render("{{ isObject(vars) }}", data) == "false" );
		CHECK( env.render("{{ isArray(vars) }}", data) == "true" );
		CHECK( env.render("{{ isArray(name) }}", data) == "false" );
		CHECK( env.render("{{ isString(name) }}", data) == "true" );
		CHECK( env.render("{{ isString(names) }}", data) == "false" );
	}
}


TEST_CASE("callbacks") {
	inja::Environment env;
	json data;
	data["age"] = 28;

	env.add_callback("double", 1, [](inja::Arguments& args) {
		int number = args.at(0)->get<int>();
		return 2 * number;
	});

	env.add_callback("half", 1, [](inja::Arguments args) {
		int number = args.at(0)->get<int>();
		return number / 2;
	});

	std::string greet = "Hello";
	env.add_callback("double-greetings", 0, [greet](inja::Arguments args) {
		return greet + " " + greet + "!";
	});

	env.add_callback("multiply", 2, [](inja::Arguments args) {
		double number1 = args.at(0)->get<double>();
		auto number2 = args.at(1)->get<double>();
		return number1 * number2;
	});

	env.add_callback("multiply", 3, [](inja::Arguments args) {
		double number1 = args.at(0)->get<double>();
		double number2 = args.at(1)->get<double>();
		double number3 = args.at(2)->get<double>();
		return number1 * number2 * number3;
	});

	env.add_callback("multiply", 0, [](inja::Arguments args) {
		return 1.0;
	});

	CHECK( env.render("{{ double(age) }}", data) == "56" );
	CHECK( env.render("{{ half(age) }}", data) == "14" );
	CHECK( env.render("{{ double-greetings }}", data) == "Hello Hello!" );
	CHECK( env.render("{{ double-greetings() }}", data) == "Hello Hello!" );
	CHECK( env.render("{{ multiply(4, 5) }}", data) == "20.0" );
	CHECK( env.render("{{ multiply(3, 4, 5) }}", data) == "60.0" );
	CHECK( env.render("{{ multiply }}", data) == "1.0" );
}


TEST_CASE("combinations") {
	inja::Environment env;
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
	CHECK( env.render("{% for i in range(4) %}{{ loop.index1 }}{% endfor %}", data) == "1234" );
}

TEST_CASE("templates") {
	json data;
	data["name"] = "Peter";
	data["city"] = "Brunswick";
	data["is_happy"] = true;

	SECTION("reuse") {
		inja::Environment env;
		inja::Template temp = env.parse("{% if is_happy %}{{ name }}{% else %}{{ city }}{% endif %}");

		CHECK( env.render(temp, data) == "Peter" );

		data["is_happy"] = false;

		CHECK( env.render(temp, data) == "Brunswick" );
	}

	SECTION("include") {
		inja::Environment env;
		inja::Template t1 = env.parse("Hello {{ name }}");
		env.include_template("greeting", t1);

		inja::Template t2 = env.parse("{% include \"greeting\" %}!");
		CHECK( env.render(t2, data) == "Hello Peter!" );
	}
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
		inja::Environment env;
		env.set_element_notation(inja::ElementNotation::Pointer);

		CHECK( env.render("{{ name }}", data) == "Peter" );
		CHECK( env.render("Hello {{ names/1 }}!", data) == "Hello Seb!" );
		CHECK( env.render("Hello {{ brother/name }}!", data) == "Hello Chris!" );
		CHECK( env.render("Hello {{ brother/daughter0/name }}!", data) == "Hello Maria!" );

		CHECK_THROWS_WITH( env.render("{{unknown/name}}", data), "[inja.exception.render_error] variable 'unknown/name' not found" );
	}

	SECTION("other expression syntax") {
		inja::Environment env;

		CHECK( env.render("Hello {{ name }}!", data) == "Hello Peter!" );

		env.set_expression("(&", "&)");

		CHECK( env.render("Hello {{ name }}!", data) == "Hello {{ name }}!" );
		CHECK( env.render("Hello (& name &)!", data) == "Hello Peter!" );
	}

	SECTION("other comment syntax") {
		inja::Environment env;
		env.set_comment("(&", "&)");

		CHECK( env.render("Hello {# Test #}", data) == "Hello {# Test #}" );
		CHECK( env.render("Hello (& Test &)", data) == "Hello " );
	}

	SECTION("multiple changes") {
		inja::Environment env;
		env.set_line_statement("$$");
		env.set_expression("<%", "%>");

		std::string string_template = R"DELIM(Hello <%name%>
$$ if name == "Peter"
	You really are <%name%>
$$ endif
)DELIM";
        	CHECK( env.render(string_template, data) == "Hello Peter\n	You really are Peter\n");
	}
}
