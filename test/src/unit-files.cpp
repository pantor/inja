#include "catch.hpp"
#include "nlohmann/json.hpp"
#include "inja.hpp"


using json = nlohmann::json;


TEST_CASE("loading") {
	inja::Environment env = inja::Environment();
	json data;
	data["name"] = "Jeff";

	SECTION("Files should be loaded") {
		CHECK( env.load_global_file("data/simple.txt") == "Hello {{ name }}." );
	}

	SECTION("Files should be rendered") {
		CHECK( env.render_template("data/simple.txt", data) == "Hello Jeff." );
	}

	SECTION("File includes should be rendered") {
		CHECK( env.render_template("data/include.txt", data) == "Answer: Hello Jeff." );
	}
}

TEST_CASE("complete-files") {
	inja::Environment env = inja::Environment("data/");

	for (std::string test_name : {"simple-file", "nested", "nested-line"}) {
		SECTION(test_name) {
			CHECK( env.render_template_with_json_file(test_name + "/template.txt", test_name + "/data.json") == env.load_global_file(test_name + "/result.txt") );
		}
	}
}

TEST_CASE("global-path") {
	inja::Environment env = inja::Environment("data/");
	json data;
	data["name"] = "Jeff";

	SECTION("Files should be written") {
		env.write("simple.txt", data, "result.txt");
		CHECK( env.load_global_file("result.txt") == "Hello Jeff." );
	}
}

TEST_CASE("input-output-path") {
	inja::Environment env = inja::Environment("data/", "data/");
	json data;
	data["name"] = "Jeff";

	SECTION("Files should be written") {
		env.write("simple.txt", data, "simple-result.txt");
		CHECK( env.load_global_file("simple-result.txt") == "Hello Jeff." );
	}
}
