// Copyright (c) 2019 Pantor. All rights reserved.

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"
#include "inja/inja.hpp"


using json = nlohmann::json;


TEST_CASE("copy-environment") {
    inja::Environment env;
    env.add_callback("double", 1, [](inja::Arguments& args) {
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
