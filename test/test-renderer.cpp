// Copyright (c) 2020 Pantor. All rights reserved.

#include "inja/environment.hpp"

#include "test-common.hpp"

TEST_CASE("types") {
  inja::Environment env;
  inja::json data;
  data["name"] = "Peter";
  data["city"] = "Brunswick";
  data["age"] = 29;
  data["names"] = {"Jeff", "Seb"};
  data["brother"]["name"] = "Chris";
  data["brother"]["daughters"] = {"Maria", "Helen"};
  data["brother"]["daughter0"] = {{"name", "Maria"}};
  data["is_happy"] = true;
  data["is_sad"] = false;
  data["@name"] = "@name";
  data["$name"] = "$name";
  data["relatives"]["mother"] = "Maria";
  data["relatives"]["brother"] = "Chris";
  data["relatives"]["sister"] = "Jenny";
  data["vars"] = {2, 3, 4, 0, -1, -2, -3};
  data["max_value"] = 18446744073709551615ull;

  SUBCASE("basic") {
    CHECK(env.render("", data) == "");
    CHECK(env.render("Hello World!", data) == "Hello World!");
    CHECK_THROWS_WITH(env.render("{{ }}", data), "[inja.exception.render_error] (at 1:4) empty expression");
    CHECK_THROWS_WITH(env.render("{{", data), "[inja.exception.parser_error] (at 1:3) expected expression close, got '<eof>'");
  }

  SUBCASE("variables") {
    CHECK(env.render("Hello {{ name }}!", data) == "Hello Peter!");
    CHECK(env.render("{{ name }}", data) == "Peter");
    CHECK(env.render("{{name}}", data) == "Peter");
    CHECK(env.render("{{ name }} is {{ age }} years old.", data) == "Peter is 29 years old.");
    CHECK(env.render("Hello {{ name }}! I come from {{ city }}.", data) == "Hello Peter! I come from Brunswick.");
    CHECK(env.render("Hello {{ names.1 }}!", data) == "Hello Seb!");
    CHECK(env.render("Hello {{ brother.name }}!", data) == "Hello Chris!");
    CHECK(env.render("Hello {{ brother.daughter0.name }}!", data) == "Hello Maria!");
    CHECK(env.render("{{ \"{{ no_value }}\" }}", data) == "{{ no_value }}");
    CHECK(env.render("{{ @name }}", data) == "@name");
    CHECK(env.render("{{ $name }}", data) == "$name");
    CHECK(env.render("{{max_value}}", data) == "18446744073709551615");

    CHECK_THROWS_WITH(env.render("{{unknown}}", data), "[inja.exception.render_error] (at 1:3) variable 'unknown' not found");
  }

  SUBCASE("comments") {
    CHECK(env.render("Hello{# This is a comment #}!", data) == "Hello!");
    CHECK(env.render("{# --- #Todo --- #}", data) == "");
  }

  SUBCASE("loops") {
    CHECK(env.render("{% for name in names %}a{% endfor %}", data) == "aa");
    CHECK(env.render("Hello {% for name in names %}{{ name }} {% endfor %}!", data) == "Hello Jeff Seb !");
    CHECK(env.render("Hello {% for name in names %}{{ loop.index }}: {{ name }}, {% endfor %}!", data) == "Hello 0: Jeff, 1: Seb, !");
    CHECK(env.render("{% for type, name in relatives %}{{ loop.index1 }}: {{ type }}: {{ name }}{% if loop.is_last == "
                     "false %}, {% endif %}{% endfor %}",
                     data) == "1: brother: Chris, 2: mother: Maria, 3: sister: Jenny");
    CHECK(env.render("{% for v in vars %}{% if v > 0 %}+{% endif %}{% endfor %}", data) == "+++");
    CHECK(env.render("{% for name in names %}{{ loop.index }}: {{ name }}{% if not loop.is_last %}, {% endif %}{% endfor %}!", data) == "0: Jeff, 1: Seb!");
    CHECK(env.render("{% for name in names %}{{ loop.index }}: {{ name }}{% if loop.is_last == false %}, {% endif %}{% "
                     "endfor %}!",
                     data) == "0: Jeff, 1: Seb!");

    CHECK(env.render("{% for name in [] %}a{% endfor %}", data) == "");

    CHECK_THROWS_WITH(env.render("{% for name ins names %}a{% endfor %}", data), "[inja.exception.parser_error] (at 1:13) expected 'in', got 'ins'");
    CHECK_THROWS_WITH(env.render("{% for name in empty_loop %}a{% endfor %}", data), "[inja.exception.render_error] (at 1:16) variable 'empty_loop' not found");
    // CHECK_THROWS_WITH( env.render("{% for name in relatives %}{{ name }}{% endfor %}", data),
    // "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is object" );
  }

  SUBCASE("nested loops") {
    auto ldata = inja::json::parse(R""""(
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
)"""");

    CHECK(env.render(R""""(
{% for o in outer %}{% for i in o.inner %}{{loop.parent.index}}:{{loop.index}}::{{loop.parent.is_last}}
{% for ii in i.in2%}{{ii}},{%endfor%}
{%endfor%}{%endfor%}
)"""",
                     ldata) == "\n0:0::false\n1,2,\n0:1::false\n\n0:2::false\n\n2:0::true\n3,4,\n2:1::true\n5,6,\n\n");
  }

  SUBCASE("conditionals") {
    CHECK(env.render("{% if is_happy %}{% endif %}", data) == "");
    CHECK(env.render("{% if is_happy %}Yeah!{% endif %}", data) == "Yeah!");
    CHECK(env.render("{% if is_sad %}Yeah!{% endif %}", data) == "");
    CHECK(env.render("{% if is_sad %}Yeah!{% else %}Nooo...{% endif %}", data) == "Nooo...");
    CHECK(env.render("{% if age == 29 %}Right{% else %}Wrong{% endif %}", data) == "Right");
    CHECK(env.render("{% if age > 29 %}Right{% else %}Wrong{% endif %}", data) == "Wrong");
    CHECK(env.render("{% if age <= 29 %}Right{% else %}Wrong{% endif %}", data) == "Right");
    CHECK(env.render("{% if age != 28 %}Right{% else %}Wrong{% endif %}", data) == "Right");
    CHECK(env.render("{% if age >= 30 %}Right{% else %}Wrong{% endif %}", data) == "Wrong");
    CHECK(env.render("{% if age in [28, 29, 30] %}True{% endif %}", data) == "True");
    CHECK(env.render("{% if age == 28 %}28{% else if age == 29 %}29{% endif %}", data) == "29");
    CHECK(env.render("{% if age == 26 %}26{% else if age == 27 %}27{% else if age == 28 %}28{% else %}29{% endif %}", data) == "29");
    CHECK(env.render("{% if age == 25 %}+{% endif %}{% if age == 29 %}+{% else %}-{% endif %}", data) == "+");

    CHECK_THROWS_WITH(env.render("{% if is_happy %}{% if is_happy %}{% endif %}", data), "[inja.exception.parser_error] (at 1:46) unmatched if");
    CHECK_THROWS_WITH(env.render("{% if is_happy %}{% else if is_happy %}{% end if %}", data),
                      "[inja.exception.parser_error] (at 1:43) expected statement, got 'end'");
  }

  SUBCASE("set statements") {
    CHECK(env.render("{% set predefined=true %}{% if predefined %}a{% endif %}", data) == "a");
    CHECK(env.render("{% set predefined=false %}{% if predefined %}a{% endif %}", data) == "");
    CHECK(env.render("{% set age=30 %}{{age}}", data) == "30");
    CHECK(env.render("{% set age=2+3 %}{{age}}", data) == "5");
    CHECK(env.render("{% set predefined.value=1 %}{% if existsIn(predefined, \"value\") %}{{predefined.value}}{% endif %}", data) == "1");
    CHECK(env.render("{% set brother.name=\"Bob\" %}{{brother.name}}", data) == "Bob");
    CHECK_THROWS_WITH(env.render("{% if predefined %}{% endif %}", data), "[inja.exception.render_error] (at 1:7) variable 'predefined' not found");
    CHECK(env.render("{{age}}", data) == "29");
    CHECK(env.render("{{brother.name}}", data) == "Chris");
  }

  SUBCASE("macros") {
    CHECK(env.render("{% macro hello(n) %}Hi {{ n }}!{% endmacro %}{{ hello(\"Bob\") }}", data) == "Hi Bob!");
    CHECK(env.render("{% macro add(a, b) %}{{ a + b }}{% endmacro %}{{ add(2, 3) }}", data) == "5");
    CHECK(env.render("{% macro greet(n, g=\"Hello\") %}{{ g }}, {{ n }}!{% endmacro %}{{ greet(\"Bob\") }}", data) == "Hello, Bob!");
    CHECK(env.render("{% macro greet(n, g=\"Hello\") %}{{ g }}, {{ n }}!{% endmacro %}{{ greet(\"Bob\", \"Hey\") }}", data) == "Hey, Bob!");
    // Input data is visible inside the macro body.
    CHECK(env.render("{% macro who() %}{{ name }}{% endmacro %}{{ who() }}", data) == "Peter");
    // Local set variable from outside the macro is NOT visible.
    CHECK_THROWS_WITH(env.render("{% set x=1 %}{% macro m() %}{{ x }}{% endmacro %}{{ m() }}", data),
                      doctest::Contains("variable 'x' not found"));
    // Parameters do not leak out of the macro.
    CHECK_THROWS_WITH(env.render("{% macro m(x) %}{{ x }}{% endmacro %}{{ m(5) }} {{ x }}", data),
                      doctest::Contains("variable 'x' not found"));
    // Macro used inside a for-loop sees the loop value via parameter binding.
    CHECK(env.render("{% macro li(x) %}<li>{{ x }}</li>{% endmacro %}{% for n in [1,2] %}{{ li(n) }}{% endfor %}", data) ==
          "<li>1</li><li>2</li>");
    // Macro calling another macro.
    CHECK(env.render("{% macro a(x) %}A({{ x }}){% endmacro %}{% macro b(x) %}B[{{ a(x) }}]{% endmacro %}{{ b(\"y\") }}", data) == "B[A(y)]");
    // Recursive macro.
    CHECK(env.render("{% macro down(n) %}{% if n > 0 %}{{ n }},{{ down(n - 1) }}{% endif %}{% endmacro %}{{ down(3) }}", data) == "3,2,1,");
    // Macro defined in an included template.
    {
      inja::Environment env2;
      env2.include_template("macros.tpl", env2.parse("{% macro greet(n) %}Hi {{ n }}{% endmacro %}"));
      CHECK(env2.render("{% include \"macros.tpl\" %}{{ greet(\"Bob\") }}", data) == "Hi Bob");
    }
    // A single trailing newline before endmacro is trimmed so line-statement
    // style definitions don't introduce an unexpected trailing newline.
    CHECK(env.render("## macro foo()\ntest\n## endmacro\n[{{ foo() }}]", data) == "[test]");
    CHECK(env.render("## macro foo()\ntest\r\n## endmacro\n[{{ foo() }}]", data) == "[test]");
    CHECK(env.render("## macro foo()\na\nb\n## endmacro\n[{{ foo() }}]", data) == "[a\nb]");
    // Only the final newline is removed.
    CHECK(env.render("## macro foo()\n\n## endmacro\n[{{ foo() }}]", data) == "[]");
    // Inline definitions without a trailing newline are unaffected.
    CHECK(env.render("{% macro foo() %}test{% endmacro %}[{{ foo() }}]", data) == "[test]");
    // Errors.
    CHECK_THROWS_WITH(env.render("{% macro m(a, b) %}x{% endmacro %}{{ m(1) }}", data),
                      doctest::Contains("missing required argument 'b' for macro 'm'"));
    CHECK_THROWS_WITH(env.render("{% macro m(a) %}x{% endmacro %}{{ m(1, 2) }}", data),
                      doctest::Contains("too many arguments for macro 'm'"));
    CHECK_THROWS_WITH(env.parse("{% macro m() %}x{% endmacro %}{% macro m() %}y{% endmacro %}"),
                      doctest::Contains("macro with the name 'm' does already exist"));
    CHECK_THROWS_WITH(env.parse("{% macro m() %}x"), doctest::Contains("unmatched macro"));
    CHECK_THROWS_WITH(env.parse("{% endmacro %}"), doctest::Contains("endmacro without matching macro"));

    // Runaway recursion (no base case) throws a RenderError instead of exhausting the C++ stack.
    CHECK_THROWS_WITH(env.render("{% macro loop(n) %}{{ n }},{{ loop(n + 1) }}{% endmacro %}{{ loop(1) }}", data),
                      doctest::Contains("macro recursion depth exceeded"));

    // The recursion depth limit is configurable and enforced even for otherwise-legitimate depth.
    {
      inja::Environment env3;
      env3.set_max_macro_recursion_depth(3);
      CHECK(env3.render("{% macro down(n) %}{% if n > 0 %}{{ n }},{{ down(n - 1) }}{% endif %}{% endmacro %}{{ down(2) }}", data) == "2,1,");
      CHECK_THROWS_WITH(env3.render("{% macro down(n) %}{% if n > 0 %}{{ n }},{{ down(n - 1) }}{% endif %}{% endmacro %}{{ down(5) }}", data),
                        doctest::Contains("macro recursion depth exceeded"));
    }
  }

  SUBCASE("short circuit evaluation") {
    CHECK(env.render("{% if 0 and undefined %}do{% else %}nothing{% endif %}", data) == "nothing");
    CHECK_THROWS_WITH(env.render("{% if 1 and undefined %}do{% else %}nothing{% endif %}", data),
                      "[inja.exception.render_error] (at 1:13) variable 'undefined' not found");
  }

  SUBCASE("line statements") {
    CHECK(env.render(R""""(## if is_happy
Yeah!
## endif)"""",
                     data) == R""""(Yeah!
)"""");

    CHECK(env.render(R""""(## if is_happy
## if is_happy
Yeah!
## endif
## endif    )"""",
                     data) == R""""(Yeah!
)"""");
  }

  SUBCASE("pipe syntax") {
    CHECK(env.render("{{ brother.name | upper }}", data) == "CHRIS");
    CHECK(env.render("{{ brother.name | upper | lower }}", data) == "chris");
    CHECK(env.render("{{ [\"C\", \"A\", \"B\"] | sort | join(\",\") }}", data) == "A,B,C");
  }
}

TEST_CASE("templates") {
  inja::json data;
  data["name"] = "Peter";
  data["city"] = "Brunswick";
  data["is_happy"] = true;

  SUBCASE("reuse") {
    inja::Environment env;
    const inja::Template temp = env.parse("{% if is_happy %}{{ name }}{% else %}{{ city }}{% endif %}");

    CHECK(env.render(temp, data) == "Peter");

    data["is_happy"] = false;

    CHECK(env.render(temp, data) == "Brunswick");
  }

  SUBCASE("include") {
    inja::Environment env;
    const inja::Template t1 = env.parse("Hello {{ name }}");
    env.include_template("greeting", t1);

    const inja::Template t2 = env.parse("{% include \"greeting\" %}!");
    CHECK(env.render(t2, data) == "Hello Peter!");
    CHECK_THROWS_WITH(env.parse("{% include \"does-not-exist\" %}!"), "[inja.exception.file_error] failed accessing file at 'does-not-exist'");

    CHECK_THROWS_WITH(env.parse("{% include does-not-exist %}!"), "[inja.exception.parser_error] (at 1:12) expected string, got 'does-not-exist'");
  }

  SUBCASE("include-callback") {
    inja::Environment env;

    CHECK_THROWS_WITH(env.parse("{% include \"does-not-exist\" %}!"), "[inja.exception.file_error] failed accessing file at 'does-not-exist'");

    env.set_search_included_templates_in_files(false);
    env.set_include_callback([&env](const std::filesystem::path&, const std::string&) { return env.parse("Hello {{ name }}"); });

    const inja::Template t1 = env.parse("{% include \"greeting\" %}!");
    CHECK(env.render(t1, data) == "Hello Peter!");

    env.set_search_included_templates_in_files(true);
    env.set_include_callback([&env](const std::filesystem::path&, const std::string& name) { return env.parse("Bye " + name); });

    const inja::Template t2 = env.parse("{% include \"Jeff\" %}!");
    CHECK(env.render(t2, data) == "Bye Jeff!");
  }

  SUBCASE("include-in-loop") {
    inja::json loop_data;
    loop_data["cities"] = inja::json::array({{{"name", "Munich"}}, {{"name", "New York"}}});

    inja::Environment env;
    env.include_template("city.tpl", env.parse("{{ loop.index }}:{{ city.name }};"));

    CHECK(env.render("{% for city in cities %}{% include \"city.tpl\" %}{% endfor %}", loop_data) == "0:Munich;1:New York;");
  }

  SUBCASE("count variables") {
    inja::Environment env;
    const inja::Template t1 = env.parse("Hello {{ name }}");
    const inja::Template t2 = env.parse("{% if is_happy %}{{ name }}{% else %}{{ city }}{% endif %}");
    const inja::Template t3 = env.parse("{% if at(name, test) %}{{ name }}{% else %}{{ city }}{{ upper(city) }}{% endif %}");

    CHECK(t1.count_variables() == 1);
    CHECK(t2.count_variables() == 3);
    CHECK(t3.count_variables() == 5);
  }

  SUBCASE("whitespace control") {
    inja::Environment env;
    CHECK(env.render("{% if is_happy %}{{ name }}{% endif %}", data) == "Peter");
    CHECK(env.render("   {% if is_happy %}{{ name }}{% endif %}   ", data) == "   Peter   ");
    CHECK(env.render("   {% if is_happy %}{{ name }}{% endif %}\n ", data) == "   Peter\n ");
    CHECK(env.render("Test\n   {%- if is_happy %}{{ name }}{% endif %}   ", data) == "Test\nPeter   ");
    CHECK(env.render("   {%+ if is_happy %}{{ name }}{% endif %}", data) == "   Peter");
    CHECK(env.render("   {%- if is_happy %}{{ name }}{% endif -%}   \n   ", data) == "Peter");

    CHECK(env.render("   {{- name -}}   \n   ", data) == "Peter");
    CHECK(env.render("Test\n   {{- name }}   ", data) == "Test\nPeter   ");
    CHECK(env.render("   {{ name }}\n ", data) == "   Peter\n ");
    CHECK(env.render("{{ name }}{# name -#}    !", data) == "Peter!");
    CHECK(env.render("   {#- name -#}    !", data) == "!");

    // Nothing will be stripped if there are other characters before the start of the block.
    CHECK(env.render(".  {%- if is_happy %}{{ name }}{% endif -%}\n", data) == ".  Peter");
    CHECK(env.render(".  {#- comment -#}\n.", data) == ".  .");

    env.set_lstrip_blocks(true);
    CHECK(env.render("Hello {{ name }}!", data) == "Hello Peter!");
    CHECK(env.render("   {% if is_happy %}{{ name }}{% endif %}", data) == "Peter");
    CHECK(env.render("   {% if is_happy %}{{ name }}{% endif %}   ", data) == "Peter   ");
    CHECK(env.render("   {% if is_happy %}{{ name }}{% endif -%}   ", data) == "Peter");
    CHECK(env.render("   {%+ if is_happy %}{{ name }}{% endif %}", data) == "   Peter");
    CHECK(env.render("\n   {%+ if is_happy %}{{ name }}{% endif -%}   ", data) == "\n   Peter");
    CHECK(env.render("{% if is_happy %}{{ name }}{% endif %}\n", data) == "Peter\n");
    CHECK(env.render("   {# comment #}", data) == "");

    env.set_trim_blocks(true);
    CHECK(env.render("{% if is_happy %}{{ name }}{% endif %}", data) == "Peter");
    CHECK(env.render("{% if is_happy %}{{ name }}{% endif %}\n", data) == "Peter");
    CHECK(env.render("{% if is_happy %}{{ name }}{% endif %}   \n.", data) == "Peter.");
    CHECK(env.render("{%- if is_happy %}{{ name }}{% endif -%}   \n.", data) == "Peter.");
    CHECK(env.render("   {# comment #}   \n.", data) == ".");
  }
}

TEST_CASE("other syntax") {
  inja::json data;
  data["name"] = "Peter";
  data["city"] = "Brunswick";
  data["age"] = 29;
  data["names"] = {"Jeff", "Seb"};
  data["brother"]["name"] = "Chris";
  data["brother"]["daughters"] = {"Maria", "Helen"};
  data["brother"]["daughter0"] = {{"name", "Maria"}};
  data["is_happy"] = true;

  SUBCASE("other expression syntax") {
    inja::Environment env;

    CHECK(env.render("Hello {{ name }}!", data) == "Hello Peter!");

    env.set_expression("(&", "&)");

    CHECK(env.render("Hello {{ name }}!", data) == "Hello {{ name }}!");
    CHECK(env.render("Hello (& name &)!", data) == "Hello Peter!");
  }

  SUBCASE("other comment syntax") {
    inja::Environment env;
    env.set_comment("(&", "&)");

    CHECK(env.render("Hello {# Test #}", data) == "Hello {# Test #}");
    CHECK(env.render("Hello (& Test &)", data) == "Hello ");
  }

  SUBCASE("multiple changes") {
    inja::Environment env;
    env.set_line_statement("$$");
    env.set_expression("<%", "%>");

    std::string string_template = R""""(Hello <%name%>
$$ if name == "Peter"
    You really are <%name%>
$$ endif
)"""";

    CHECK(env.render(string_template, data) == "Hello Peter\n    You really are Peter\n");
  }
}
