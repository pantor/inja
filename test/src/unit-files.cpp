#include "catch.hpp"
#include "json.hpp"
#include "inja.hpp"


using json = nlohmann::json;


TEST_CASE("Files handling") {
	inja::Environment env = inja::Environment();
	json data;
	data["name"] = "Jeff";

	SECTION("Files should be loaded") {
		CHECK( env.load_file("../test/data/simple.txt") == "Hello {{ name }}." );
	}

	SECTION("Files should be rendered") {
		CHECK( env.render_template("../test/data/simple.txt", data) == "Hello Jeff." );
	}

	SECTION("File includes should be rendered") {
		CHECK( env.render_template("../test/data/include.txt", data) == "Answer: Hello Jeff." );
	}
}

TEST_CASE("Complete files") {
	inja::Environment env = inja::Environment("../test/data/");

	for (std::string test_name : {"simple-file", "nested", "nested-line"}) {
		SECTION(test_name) {
			CHECK( env.render_template_with_json_file(test_name + "/template.txt", test_name + "/data.json") == env.load_file(test_name + "/result.txt") );
		}
	}
}
