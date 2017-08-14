#include "catch.hpp"
#include "json.hpp"
#include "inja.hpp"


using Environment = inja::Environment;
using json = nlohmann::json;


TEST_CASE("files handling") {
	Environment env = Environment();
	json data;
	data["name"] = "Jeff";
	
	SECTION("files should be loaded") {
		REQUIRE( env.load_file("../test/data/simple.txt") == "Hello {{ name }}." );
	}
	
	SECTION("files should be rendered") {
		REQUIRE( env.render_template("../test/data/simple.txt", data) == "Hello Jeff." );
	}
	
	SECTION("file includes should be rendered") {
		REQUIRE( env.render_template("../test/data/include.txt", data) == "Answer: Hello Jeff." );
	}
}

TEST_CASE("complete files") {
	Environment env = Environment("../test/data/");
	
	for (std::string test_name : {"simple-file", "nested"}) {
		SECTION(test_name) {
			REQUIRE( env.render_template_with_json_file(test_name + "/template.txt", test_name + "/data.json") == env.load_file(test_name + "/result.txt") );
		}
	}
}