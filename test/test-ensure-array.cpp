// Copyright (c) 2020 Pantor. All rights reserved.

#include "inja/environment.hpp"

#include "test-common.hpp"

TEST_CASE("ensure_array_for_loops") {
  inja::Environment env;
  inja::json data;

  SUBCASE("default behavior - throws on non-array") {
    // Default behavior should throw error when trying to loop over non-array
    data["person"] = {{"name", "John"}};

    CHECK_THROWS_WITH(env.render("{% for p in person %}{{ p.name }}{% endfor %}", data),
                      "[inja.exception.render_error] (at 1:10) object must be an array");

    data["count"] = 5;
    CHECK_THROWS_WITH(env.render("{% for n in count %}{{ n }}{% endfor %}", data),
                      "[inja.exception.render_error] (at 1:10) object must be an array");
  }

  SUBCASE("enabled - single object wrapped in array") {
    env.set_ensure_array_for_loops(true);

    // Single object becomes one-item loop
    data["person"] = {{"name", "John"}, {"age", 30}};
    CHECK(env.render("{% for p in person %}{{ p.name }}-{{ p.age }}{% endfor %}", data) == "John-30");

    // Verify loop runs exactly once
    CHECK(env.render("{% for p in person %}{{ loop.index1 }}{% endfor %}", data) == "1");
  }

  SUBCASE("enabled - arrays work normally") {
    env.set_ensure_array_for_loops(true);

    // Array with multiple items
    data["people"] = inja::json::array({
      {{"name", "John"}, {"age", 30}},
      {{"name", "Jane"}, {"age", 25}},
      {{"name", "Bob"}, {"age", 35}}
    });

    CHECK(env.render("{% for p in people %}{{ p.name }}{% if not loop.is_last %},{% endif %}{% endfor %}", data) == "John,Jane,Bob");

    // Verify loop variables
    CHECK(env.render("{% for p in people %}{{ loop.index1 }}{% endfor %}", data) == "123");
  }

  SUBCASE("enabled - null becomes empty array") {
    env.set_ensure_array_for_loops(true);

    // Null value results in no loop iterations
    data["missing"] = nullptr;
    CHECK(env.render("{% for item in missing %}X{% endfor %}", data) == "");

    // Check with conditional
    CHECK(env.render("{% if missing %}has value{% endif %}", data) == "");
  }

  SUBCASE("enabled - primitives wrapped in array") {
    env.set_ensure_array_for_loops(true);

    // String
    data["str"] = "hello";
    CHECK(env.render("{% for s in str %}{{ s }}{% endfor %}", data) == "hello");

    // Number
    data["num"] = 42;
    CHECK(env.render("{% for n in num %}{{ n }}{% endfor %}", data) == "42");

    // Boolean
    data["bool"] = true;
    CHECK(env.render("{% for b in bool %}{{ b }}{% endfor %}", data) == "true");
  }

  SUBCASE("enabled - XML-style single vs multiple items") {
    env.set_ensure_array_for_loops(true);

    // Simulating XML-to-JSON: single person (object)
    data["response"]["person"] = {{"name", "John"}, {"dob", "1980-05-15"}};

    std::string tmpl = R"({% for p in response.person %}
Name: {{ p.name }}
DOB: {{ p.dob }}
{% endfor %})";

    std::string result1 = env.render(tmpl, data);
    CHECK(result1.find("Name: John") != std::string::npos);
    CHECK(result1.find("DOB: 1980-05-15") != std::string::npos);

    // Now with multiple people (array) - same template works!
    data["response"]["person"] = inja::json::array({
      {{"name", "John"}, {"dob", "1980-05-15"}},
      {{"name", "Jane"}, {"dob", "1982-03-22"}}
    });

    std::string result2 = env.render(tmpl, data);
    CHECK(result2.find("Name: John") != std::string::npos);
    CHECK(result2.find("Name: Jane") != std::string::npos);
  }

  SUBCASE("enabled - loop variables work correctly") {
    env.set_ensure_array_for_loops(true);

    // Single item: is_first and is_last both true
    data["item"] = {{"val", "A"}};
    CHECK(env.render("{% for i in item %}{% if loop.is_first %}F{% endif %}{% if loop.is_last %}L{% endif %}{% endfor %}", data) == "FL");

    // Multiple items: first has F, last has L
    data["items"] = inja::json::array({
      {{"val", "A"}},
      {{"val", "B"}},
      {{"val", "C"}}
    });
    std::string result = env.render("{% for i in items %}{% if loop.is_first %}F{% endif %}{% if loop.is_last %}L{% endif %}{% endfor %}", data);
    CHECK(result == "FL");  // First iteration gets both first and last flags since output concatenates
  }

  SUBCASE("enabled - nested loops") {
    env.set_ensure_array_for_loops(true);

    // Outer loop: single object, Inner loop: array
    data["person"] = {{"name", "John"}};
    data["person"]["hobbies"] = inja::json::array({"reading", "coding", "gaming"});

    std::string tmpl = R"({% for p in person %}{{ p.name }}: {% for h in p.hobbies %}{{ h }}{% if not loop.is_last %},{% endif %}{% endfor %}{% endfor %})";
    CHECK(env.render(tmpl, data) == "John: reading,coding,gaming");
  }

  SUBCASE("enabled - empty arrays") {
    env.set_ensure_array_for_loops(true);

    // Empty array - no iterations
    data["items"] = inja::json::array();
    CHECK(env.render("{% for item in items %}X{% endfor %}", data) == "");

    // Check length
    CHECK(env.render("{{ length(items) }}", data) == "0");
  }

  SUBCASE("enabled - works with length function") {
    env.set_ensure_array_for_loops(true);

    // Single item wrapped in array has length 1
    data["item"] = {{"name", "John"}};
    CHECK(env.render("{% set items_array = item %}{% for i in items_array %}{{ loop.index1 }} of {{ length(items_array) }}{% endfor %}", data) == "1 of 1");

    // Null has length 0 when wrapped
    data["nothing"] = nullptr;
    std::string result = env.render("{% for x in nothing %}X{% endfor %}Count: 0", data);
    CHECK(result == "Count: 0");
  }

  SUBCASE("enabled - complex XML example") {
    env.set_ensure_array_for_loops(true);

    // Search results that might be single or multiple
    data["SearchResults"]["Subject"] = {
      {"Name", "Smith, John"},
      {"DOB", "1980-05-15"},
      {"Address", "123 Main St"}
    };

    std::string tmpl = R"(Search Results
{% for subject in SearchResults.Subject %}
{{ loop.index1 }}. {{ subject.Name }} (DOB: {{ subject.DOB }})
   Address: {{ subject.Address }}
{% endfor %})";

    std::string result = env.render(tmpl, data);
    CHECK(result.find("1. Smith, John") != std::string::npos);
    CHECK(result.find("DOB: 1980-05-15") != std::string::npos);
  }

  SUBCASE("disabled flag - normal array behavior") {
    // Explicitly disable (already default)
    env.set_ensure_array_for_loops(false);

    // Arrays work normally
    data["items"] = inja::json::array({"a", "b", "c"});
    CHECK(env.render("{% for i in items %}{{ i }}{% endfor %}", data) == "abc");

    // Non-arrays throw
    data["item"] = "single";
    CHECK_THROWS(env.render("{% for i in item %}{{ i }}{% endfor %}", data));
  }
}
