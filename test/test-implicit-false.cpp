// Copyright (c) 2020 Pantor. All rights reserved.

#include "inja/environment.hpp"

#include "test-common.hpp"

TEST_CASE("implicit_false_for_missing_vars") {
  inja::Environment env;
  inja::json data;
  data["entity"] = {{"name", "John"}};
  data["is_happy"] = true;

  SUBCASE("default behavior - throws on missing variable") {
    // Default behavior should throw error for missing variables in conditionals
    CHECK_THROWS_WITH(env.render("{% if entity.Person %}yes{% else %}no{% endif %}", data),
                      "[inja.exception.render_error] (at 1:7) variable 'entity.Person' not found");
    CHECK_THROWS_WITH(env.render("{% if missing_var %}yes{% else %}no{% endif %}", data),
                      "[inja.exception.render_error] (at 1:7) variable 'missing_var' not found");
  }

  SUBCASE("enabled - missing variables evaluate to false") {
    env.set_implicit_false_for_missing_vars(true);

    // Missing nested variable
    CHECK(env.render("{% if entity.Person %}yes{% else %}no{% endif %}", data) == "no");

    // Missing deeply nested variable
    CHECK(env.render("{% if entity.Person.address %}yes{% else %}no{% endif %}", data) == "no");

    // Missing top-level variable
    CHECK(env.render("{% if missing_var %}yes{% else %}no{% endif %}", data) == "no");

    // Missing variable without else clause
    CHECK(env.render("{% if entity.Unknown %}yes{% endif %}", data) == "");
  }

  SUBCASE("enabled - existing variables still work correctly") {
    env.set_implicit_false_for_missing_vars(true);

    // Existing truthy variable
    data["entity"]["Person"] = true;
    CHECK(env.render("{% if entity.Person %}yes{% else %}no{% endif %}", data) == "yes");

    // Existing falsy boolean
    data["entity"]["Person"] = false;
    CHECK(env.render("{% if entity.Person %}yes{% else %}no{% endif %}", data) == "no");

    // Existing falsy number (0)
    data["entity"]["count"] = 0;
    CHECK(env.render("{% if entity.count %}yes{% else %}no{% endif %}", data) == "no");

    // Existing truthy number
    data["entity"]["count"] = 1;
    CHECK(env.render("{% if entity.count %}yes{% else %}no{% endif %}", data) == "yes");

    // Existing null
    data["entity"]["value"] = nullptr;
    CHECK(env.render("{% if entity.value %}yes{% else %}no{% endif %}", data) == "no");

    // Existing empty string (empty strings are truthy in inja)
    data["entity"]["text"] = "";
    CHECK(env.render("{% if entity.text %}yes{% else %}no{% endif %}", data) == "yes");

    // Existing non-empty string
    data["entity"]["text"] = "hello";
    CHECK(env.render("{% if entity.text %}yes{% else %}no{% endif %}", data) == "yes");
  }

  SUBCASE("enabled - expressions still throw on missing variables") {
    env.set_implicit_false_for_missing_vars(true);

    // Expression context should still throw
    CHECK_THROWS_WITH(env.render("{{ missing_var }}", data),
                      "[inja.exception.render_error] (at 1:4) variable 'missing_var' not found");
    CHECK_THROWS_WITH(env.render("{{ entity.Unknown }}", data),
                      "[inja.exception.render_error] (at 1:4) variable 'entity.Unknown' not found");
  }

  SUBCASE("enabled - works with logical operators") {
    env.set_implicit_false_for_missing_vars(true);

    // Missing variable with AND
    CHECK(env.render("{% if is_happy and entity.Person %}yes{% else %}no{% endif %}", data) == "no");

    // Missing variable with OR
    CHECK(env.render("{% if is_happy or entity.Person %}yes{% else %}no{% endif %}", data) == "yes");

    // Missing variable with NOT
    CHECK(env.render("{% if not entity.Person %}yes{% else %}no{% endif %}", data) == "yes");
  }

  SUBCASE("enabled - works with else if") {
    env.set_implicit_false_for_missing_vars(true);

    CHECK(env.render("{% if entity.Person %}a{% else if is_happy %}b{% else %}c{% endif %}", data) == "b");
    CHECK(env.render("{% if entity.Unknown1 %}a{% else if entity.Unknown2 %}b{% else %}c{% endif %}", data) == "c");
  }

  SUBCASE("enabled - works in loops") {
    env.set_implicit_false_for_missing_vars(true);
    data["items"] = {"a", "b", "c"};

    CHECK(env.render("{% for item in items %}{% if item.missing %}x{% else %}{{ item }}{% endif %}{% endfor %}", data) == "abc");
  }
}
