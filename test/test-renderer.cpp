// Copyright (c) 2020 Pantor. All rights reserved.

TEST_CASE("types") {
  inja::Environment env;
  json data;
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

    CHECK_THROWS_WITH(env.render("{{unknown}}", data), "[inja.exception.render_error] (at 1:3) variable 'unknown' not found");
  }

  SUBCASE("comments") {
    CHECK(env.render("Hello{# This is a comment #}!", data) == "Hello!");
    CHECK(env.render("{# --- #Todo --- #}", data) == "");
  }

  SUBCASE("loops") {
    CHECK(env.render("{% for name in names %}a{% endfor %}", data) == "aa");
    CHECK(env.render("Hello {% for name in names %}{{ name }} {% endfor %}!", data) == "Hello Jeff Seb !");
    CHECK(env.render("Hello {% for name in names %}{{ loop.index }}: {{ name }}, {% endfor %}!", data) ==
          "Hello 0: Jeff, 1: Seb, !");
    CHECK(env.render("{% for type, name in relatives %}{{ loop.index1 }}: {{ type }}: {{ name }}{% if loop.is_last == "
                     "false %}, {% endif %}{% endfor %}",
                     data) == "1: brother: Chris, 2: mother: Maria, 3: sister: Jenny");
    CHECK(env.render("{% for v in vars %}{% if v > 0 %}+{% endif %}{% endfor %}", data) == "+++");
    CHECK(env.render(
              "{% for name in names %}{{ loop.index }}: {{ name }}{% if not loop.is_last %}, {% endif %}{% endfor %}!",
              data) == "0: Jeff, 1: Seb!");
    CHECK(env.render("{% for name in names %}{{ loop.index }}: {{ name }}{% if loop.is_last == false %}, {% endif %}{% "
                     "endfor %}!",
                     data) == "0: Jeff, 1: Seb!");

    CHECK(env.render("{% for name in [] %}a{% endfor %}", data) == "");

    CHECK_THROWS_WITH(env.render("{% for name ins names %}a{% endfor %}", data),
                      "[inja.exception.parser_error] (at 1:13) expected 'in', got 'ins'");
    CHECK_THROWS_WITH(env.render("{% for name in empty_loop %}a{% endfor %}", data),
                      "[inja.exception.render_error] (at 1:16) variable 'empty_loop' not found");
    // CHECK_THROWS_WITH( env.render("{% for name in relatives %}{{ name }}{% endfor %}", data),
    // "[inja.exception.json_error] [json.exception.type_error.302] type must be array, but is object" );
  }

  SUBCASE("nested loops") {
    auto ldata = json::parse(R""""(
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
    CHECK(env.render("{% if age == 26 %}26{% else if age == 27 %}27{% else if age == 28 %}28{% else %}29{% endif %}",
                     data) == "29");
    CHECK(env.render("{% if age == 25 %}+{% endif %}{% if age == 29 %}+{% else %}-{% endif %}", data) == "+");

    CHECK_THROWS_WITH(env.render("{% if is_happy %}{% if is_happy %}{% endif %}", data),
                      "[inja.exception.parser_error] (at 1:46) unmatched if");
    CHECK_THROWS_WITH(env.render("{% if is_happy %}{% else if is_happy %}{% end if %}", data),
                      "[inja.exception.parser_error] (at 1:43) expected statement, got 'end'");
  }

  SUBCASE("set statements") {
    CHECK(env.render("{% set predefined=true %}{% if predefined %}a{% endif %}", data) == "a");
    CHECK(env.render("{% set predefined=false %}{% if predefined %}a{% endif %}", data) == "");
    CHECK(env.render("{% set age=30 %}{{age}}", data) == "30");
    CHECK(env.render("{% set predefined.value=1 %}{% if existsIn(predefined, \"value\") %}{{predefined.value}}{% endif %}", data) == "1");
    CHECK(env.render("{% set brother.name=\"Bob\" %}{{brother.name}}", data) == "Bob");
    CHECK_THROWS_WITH(env.render("{% if predefined %}{% endif %}", data),
                      "[inja.exception.render_error] (at 1:7) variable 'predefined' not found");
    CHECK(env.render("{{age}}", data) == "29");
    CHECK(env.render("{{brother.name}}", data) == "Chris");
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
}

TEST_CASE("templates") {
  json data;
  data["name"] = "Peter";
  data["city"] = "Brunswick";
  data["is_happy"] = true;

  SUBCASE("reuse") {
    inja::Environment env;
    inja::Template temp = env.parse("{% if is_happy %}{{ name }}{% else %}{{ city }}{% endif %}");

    CHECK(env.render(temp, data) == "Peter");

    data["is_happy"] = false;

    CHECK(env.render(temp, data) == "Brunswick");
  }

  SUBCASE("include") {
    inja::Environment env;
    inja::Template t1 = env.parse("Hello {{ name }}");
    env.include_template("greeting", t1);

    inja::Template t2 = env.parse("{% include \"greeting\" %}!");
    CHECK(env.render(t2, data) == "Hello Peter!");
    CHECK_THROWS_WITH(env.parse("{% include \"does-not-exist\" %}!"),
                      "[inja.exception.file_error] failed accessing file at 'does-not-exist'");
  }

  SUBCASE("include-in-loop") {
    json loop_data;
    loop_data["cities"] = json::array({{{"name", "Munich"}}, {{"name", "New York"}}});

    inja::Environment env;
    env.include_template("city.tpl", env.parse("{{ loop.index }}:{{ city.name }};"));

    CHECK(env.render("{% for city in cities %}{% include \"city.tpl\" %}{% endfor %}", loop_data) ==
          "0:Munich;1:New York;");
  }

  SUBCASE("count variables") {
    inja::Environment env;
    inja::Template t1 = env.parse("Hello {{ name }}");
    inja::Template t2 = env.parse("{% if is_happy %}{{ name }}{% else %}{{ city }}{% endif %}");
    inja::Template t3 = env.parse("{% if at(name, test) %}{{ name }}{% else %}{{ city }}{{ upper(city) }}{% endif %}");

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
  json data;
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
