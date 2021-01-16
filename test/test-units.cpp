// Copyright (c) 2020 Pantor. All rights reserved.

TEST_CASE("source location") {
  std::string content = R""""(Lorem Ipsum
  Dolor
Amid
Set ().$
Try this

)"""";

  CHECK(inja::get_source_location(content, 0).line == 1);
  CHECK(inja::get_source_location(content, 0).column == 1);

  CHECK(inja::get_source_location(content, 10).line == 1);
  CHECK(inja::get_source_location(content, 10).column == 11);

  CHECK(inja::get_source_location(content, 25).line == 4);
  CHECK(inja::get_source_location(content, 25).column == 1);

  CHECK(inja::get_source_location(content, 29).line == 4);
  CHECK(inja::get_source_location(content, 29).column == 5);

  CHECK(inja::get_source_location(content, 43).line == 6);
  CHECK(inja::get_source_location(content, 43).column == 1);
}

TEST_CASE("copy environment") {
  inja::Environment env;
  env.add_callback("double", 1, [](inja::Arguments &args) {
    int number = args.at(0)->get<int>();
    return 2 * number;
  });

  inja::Template t1 = env.parse("{{ double(2) }}");
  env.include_template("tpl", t1);
  std::string test_tpl = "{% include \"tpl\" %}";

  REQUIRE(env.render(test_tpl, json()) == "4");

  inja::Environment copy(env);
  CHECK(copy.render(test_tpl, json()) == "4");

  // overwrite template in source env
  inja::Template t2 = env.parse("{{ double(4) }}");
  env.include_template("tpl", t2);
  REQUIRE(env.render(test_tpl, json()) == "8");

  // template is unchanged in copy
  CHECK(copy.render(test_tpl, json()) == "4");
}
