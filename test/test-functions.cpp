// Copyright (c) 2020 Pantor. All rights reserved.

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
  data["i"] = 1;
  data["is_happy"] = true;
  data["is_sad"] = false;
  data["vars"] = {2, 3, 4, 0, -1, -2, -3};

  SUBCASE("math") {
    CHECK(env.render("{{ 1 + 1 }}", data) == "2");
    CHECK(env.render("{{ 3 - 21 }}", data) == "-18");
    CHECK(env.render("{{ 1 + 1 * 3 }}", data) == "4");
    CHECK(env.render("{{ (1 + 1) * 3 }}", data) == "6");
    CHECK(env.render("{{ 5 / 2 }}", data) == "2.5");
    CHECK(env.render("{{ 5^3 }}", data) == "125");
    CHECK(env.render("{{ 5 + 12 + 4 * (4 - (1 + 1))^2 - 75 * 1 }}", data) == "-42");
  }

  SUBCASE("upper") {
    CHECK(env.render("{{ upper(name) }}", data) == "PETER");
    CHECK(env.render("{{ upper(  name  ) }}", data) == "PETER");
    CHECK(env.render("{{ upper(city) }}", data) == "NEW YORK");
    CHECK(env.render("{{ upper(upper(name)) }}", data) == "PETER");

    // CHECK_THROWS_WITH( env.render("{{ upper(5) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be string, but is number" ); CHECK_THROWS_WITH( env.render("{{
    // upper(true) }}", data), "[inja.exception.json_error] [json.exception.type_error.302] type must be string, but is
    // boolean" );
  }

  SUBCASE("lower") {
    CHECK(env.render("{{ lower(name) }}", data) == "peter");
    CHECK(env.render("{{ lower(city) }}", data) == "new york");
    // CHECK_THROWS_WITH( env.render("{{ lower(5.45) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be string, but is number" );
  }

  SUBCASE("range") {
    CHECK(env.render("{{ range(2) }}", data) == "[0,1]");
    CHECK(env.render("{{ range(4) }}", data) == "[0,1,2,3]");
    // CHECK_THROWS_WITH( env.render("{{ range(name) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be number, but is string" );
  }

  SUBCASE("length") {
    CHECK(env.render("{{ length(names) }}", data) == "4"); // Length of array
    CHECK(env.render("{{ length(name) }}", data) == "5");  // Length of string
    // CHECK_THROWS_WITH( env.render("{{ length(5) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be array, but is number" );
  }

  SUBCASE("sort") {
    CHECK(env.render("{{ sort([3, 2, 1]) }}", data) == "[1,2,3]");
    CHECK(env.render("{{ sort([\"bob\", \"charlie\", \"alice\"]) }}", data) == "[\"alice\",\"bob\",\"charlie\"]");
    // CHECK_THROWS_WITH( env.render("{{ sort(5) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be array, but is number" );
  }

  SUBCASE("at") {
    CHECK(env.render("{{ at(names, 0) }}", data) == "Jeff");
    CHECK(env.render("{{ at(names, i) }}", data) == "Seb");
    CHECK(env.render("{{ at(brother, \"name\") }}", data) == "Chris");
    CHECK(env.render("{{ at(at(brother, \"daughters\"), 0) }}", data) == "Maria");
    // CHECK(env.render("{{ at(names, 45) }}", data) == "Jeff");
  }

  SUBCASE("first") {
    CHECK(env.render("{{ first(names) }}", data) == "Jeff");
    // CHECK_THROWS_WITH( env.render("{{ first(5) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be array, but is number" );
  }

  SUBCASE("last") {
    CHECK(env.render("{{ last(names) }}", data) == "Tom");
    // CHECK_THROWS_WITH( env.render("{{ last(5) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be array, but is number" );
  }

  SUBCASE("round") {
    CHECK(env.render("{{ round(4, 0) }}", data) == "4");
    CHECK(env.render("{{ round(temperature, 2) }}", data) == "25.68");
    // CHECK_THROWS_WITH( env.render("{{ round(name, 2) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be number, but is string" );
  }

  SUBCASE("divisibleBy") {
    CHECK(env.render("{{ divisibleBy(50, 5) }}", data) == "true");
    CHECK(env.render("{{ divisibleBy(12, 3) }}", data) == "true");
    CHECK(env.render("{{ divisibleBy(11, 3) }}", data) == "false");
    // CHECK_THROWS_WITH( env.render("{{ divisibleBy(name, 2) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be number, but is string" );
  }

  SUBCASE("odd") {
    CHECK(env.render("{{ odd(11) }}", data) == "true");
    CHECK(env.render("{{ odd(12) }}", data) == "false");
    // CHECK_THROWS_WITH( env.render("{{ odd(name) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be number, but is string" );
  }

  SUBCASE("even") {
    CHECK(env.render("{{ even(11) }}", data) == "false");
    CHECK(env.render("{{ even(12) }}", data) == "true");
    // CHECK_THROWS_WITH( env.render("{{ even(name) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be number, but is string" );
  }

  SUBCASE("max") {
    CHECK(env.render("{{ max([1, 2, 3]) }}", data) == "3");
    CHECK(env.render("{{ max([-5.2, 100.2, 2.4]) }}", data) == "100.2");
    // CHECK_THROWS_WITH( env.render("{{ max(name) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be array, but is string" );
  }

  SUBCASE("min") {
    CHECK(env.render("{{ min([1, 2, 3]) }}", data) == "1");
    CHECK(env.render("{{ min([-5.2, 100.2, 2.4]) }}", data) == "-5.2");
    // CHECK_THROWS_WITH( env.render("{{ min(name) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be array, but is string" );
  }

  SUBCASE("float") {
    CHECK(env.render("{{ float(\"2.2\") == 2.2 }}", data) == "true");
    CHECK(env.render("{{ float(\"-1.25\") == -1.25 }}", data) == "true");
    // CHECK_THROWS_WITH( env.render("{{ max(name) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be array, but is string" );
  }

  SUBCASE("int") {
    CHECK(env.render("{{ int(\"2\") == 2 }}", data) == "true");
    CHECK(env.render("{{ int(\"-1.25\") == -1 }}", data) == "true");
    // CHECK_THROWS_WITH( env.render("{{ max(name) }}", data), "[inja.exception.json_error]
    // [json.exception.type_error.302] type must be array, but is string" );
  }

  SUBCASE("default") {
    CHECK(env.render("{{ default(11, 0) }}", data) == "11");
    CHECK(env.render("{{ default(nothing, 0) }}", data) == "0");
    CHECK(env.render("{{ default(name, \"nobody\") }}", data) == "Peter");
    CHECK(env.render("{{ default(surname, \"nobody\") }}", data) == "nobody");
    CHECK(env.render("{{ default(surname, \"{{ surname }}\") }}", data) == "{{ surname }}");
    CHECK_THROWS_WITH(env.render("{{ default(surname, lastname) }}", data),
                      "[inja.exception.render_error] (at 1:21) variable 'lastname' not found");
  }

  SUBCASE("exists") {
    CHECK(env.render("{{ exists(\"name\") }}", data) == "true");
    CHECK(env.render("{{ exists(\"zipcode\") }}", data) == "false");
    CHECK(env.render("{{ exists(name) }}", data) == "false");
    CHECK(env.render("{{ exists(property) }}", data) == "true");
    
    // CHECK(env.render("{{ exists(\"keywords\") and length(keywords) > 0 }}", data) == "false");
  }

  SUBCASE("existsIn") {
    CHECK(env.render("{{ existsIn(brother, \"name\") }}", data) == "true");
    CHECK(env.render("{{ existsIn(brother, \"parents\") }}", data) == "false");
    CHECK(env.render("{{ existsIn(brother, property) }}", data) == "true");
    CHECK(env.render("{{ existsIn(brother, name) }}", data) == "false");
    CHECK_THROWS_WITH(env.render("{{ existsIn(sister, \"lastname\") }}", data),
                      "[inja.exception.render_error] (at 1:13) variable 'sister' not found");
    CHECK_THROWS_WITH(env.render("{{ existsIn(brother, sister) }}", data),
                      "[inja.exception.render_error] (at 1:22) variable 'sister' not found");
  }

  SUBCASE("join") {
    CHECK(env.render("{{ join(names, \" | \") }}", data) == "Jeff | Seb | Peter | Tom");
    CHECK(env.render("{{ join(vars, \", \") }}", data) == "2, 3, 4, 0, -1, -2, -3");
  }

  SUBCASE("isType") {
    CHECK(env.render("{{ isBoolean(is_happy) }}", data) == "true");
    CHECK(env.render("{{ isBoolean(vars) }}", data) == "false");
    CHECK(env.render("{{ isNumber(age) }}", data) == "true");
    CHECK(env.render("{{ isNumber(name) }}", data) == "false");
    CHECK(env.render("{{ isInteger(age) }}", data) == "true");
    CHECK(env.render("{{ isInteger(is_happy) }}", data) == "false");
    CHECK(env.render("{{ isFloat(temperature) }}", data) == "true");
    CHECK(env.render("{{ isFloat(age) }}", data) == "false");
    CHECK(env.render("{{ isObject(brother) }}", data) == "true");
    CHECK(env.render("{{ isObject(vars) }}", data) == "false");
    CHECK(env.render("{{ isArray(vars) }}", data) == "true");
    CHECK(env.render("{{ isArray(name) }}", data) == "false");
    CHECK(env.render("{{ isString(name) }}", data) == "true");
    CHECK(env.render("{{ isString(names) }}", data) == "false");
  }
}

TEST_CASE("callbacks") {
  inja::Environment env;
  json data;
  data["age"] = 28;

  env.add_callback("double", 1, [](inja::Arguments &args) {
    int number = args.at(0)->get<int>();
    return 2 * number;
  });

  env.add_callback("half", 1, [](inja::Arguments args) {
    int number = args.at(0)->get<int>();
    return number / 2;
  });

  std::string greet = "Hello";
  env.add_callback("double-greetings", 0, [greet](inja::Arguments) { return greet + " " + greet + "!"; });

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

  env.add_callback("length", 1, [](inja::Arguments args) {
    auto number1 = args.at(0)->get<std::string>();
    return number1.length();
  });

  env.add_void_callback("log", 1, [](inja::Arguments) {
    
  });

  env.add_callback("multiply", 0, [](inja::Arguments) { return 1.0; });

  CHECK(env.render("{{ double(age) }}", data) == "56");
  CHECK(env.render("{{ half(age) }}", data) == "14");
  CHECK(env.render("{{ log(age) }}", data) == "");
  CHECK(env.render("{{ double-greetings }}", data) == "Hello Hello!");
  CHECK(env.render("{{ double-greetings() }}", data) == "Hello Hello!");
  CHECK(env.render("{{ multiply(4, 5) }}", data) == "20.0");
  CHECK(env.render("{{ multiply(length(\"tester\"), 5) }}", data) == "30.0");
  CHECK(env.render("{{ multiply(5, length(\"t\")) }}", data) == "5.0");
  CHECK(env.render("{{ multiply(3, 4, 5) }}", data) == "60.0");
  CHECK(env.render("{{ multiply }}", data) == "1.0");

  SUBCASE("Variadic") {
    env.add_callback("argmax", [](inja::Arguments& args) {
      auto result = std::max_element(args.begin(), args.end(), [](const json* a, const json* b) { return *a < *b;});
      return std::distance(args.begin(), result);
    });

    CHECK(env.render("{{ argmax(4, 2, 6) }}", data) == "2");
    CHECK(env.render("{{ argmax(0, 2, 6, 8, 3) }}", data) == "3");
  }
}

TEST_CASE("combinations") {
  inja::Environment env;
  json data;
  data["name"] = "Peter";
  data["city"] = "Brunswick";
  data["age"] = 29;
  data["names"] = {"Jeff", "Seb", "Chris"};
  data["brother"]["name"] = "Chris";
  data["brother"]["daughters"] = {"Maria", "Helen"};
  data["brother"]["daughter0"] = {{"name", "Maria"}};
  data["is_happy"] = true;
  data["list_of_objects"] = {{{"a", 2}}, {{"b", 3}}, {{"c", 4}}, {{"d", 5}}};

  CHECK(env.render("{% if upper(\"Peter\") == \"PETER\" %}TRUE{% endif %}", data) == "TRUE");
  CHECK(env.render("{% if lower(upper(name)) == \"peter\" %}TRUE{% endif %}", data) == "TRUE");
  CHECK(env.render("{% for i in range(4) %}{{ loop.index1 }}{% endfor %}", data) == "1234");
  CHECK(env.render("{{ upper(last(brother.daughters)) }}", data) == "HELEN");
  CHECK(env.render("{{ length(name) * 2.5 }}", data) == "12.5");
  CHECK(env.render("{{ upper(first(sort(brother.daughters)) + \"_test\") }}", data) == "HELEN_TEST");
  CHECK(env.render("{% for i in range(3) %}{{ at(names, i) }}{% endfor %}", data) == "JeffSebChris");
  CHECK(env.render("{% if not is_happy or age > 26 %}TRUE{% endif %}", data) == "TRUE");
  CHECK(env.render("{{ last(list_of_objects).d * 2}}", data) == "10");
  CHECK(env.render("{{ last(range(5)) * 2 }}", data) == "8");
  CHECK(env.render("{{ last(range(5 * 2)) }}", data) == "9");
  CHECK(env.render("{{ last(range(5 * 2)) }}", data) == "9");
  CHECK(env.render("{{ not true }}", data) == "false");
  CHECK(env.render("{{ not (true) }}", data) == "false");
  CHECK(env.render("{{ true or (true or true) }}", data) == "true");
  CHECK(env.render("{{ at(list_of_objects, 1).b }}", data) == "3");
}
