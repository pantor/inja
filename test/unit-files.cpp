#include "catch/catch.hpp"
#include "inja/inja.hpp"


using json = nlohmann::json;


const std::string test_file_directory {"../test/data/"};

TEST_CASE("loading") {
	inja::Environment env;
	json data;
	data["name"] = "Jeff";

	SECTION("Files should be loaded") {
		CHECK( env.load_file(test_file_directory + "simple.txt") == "Hello {{ name }}." );
	}

	SECTION("Files should be rendered") {
		CHECK( env.render_file(test_file_directory + "simple.txt", data) == "Hello Jeff." );
	}

	SECTION("File includes should be rendered") {
		CHECK( env.render_file(test_file_directory + "include.txt", data) == "Answer: Hello Jeff." );
	}

	SECTION("File error should throw") {
		std::string path(test_file_directory + "does-not-exist");
		CHECK_THROWS_WITH( env.load_file(path), "[inja.exception.file_error] failed accessing file at '" + path + "'" );
		CHECK_THROWS_WITH( env.load_json(path), "[inja.exception.file_error] failed accessing file at '" + path + "'" );
	}
}

TEST_CASE("complete-files") {
	inja::Environment env {test_file_directory};

	for (std::string test_name : {"simple-file", "nested", "nested-line", "html"}) {
		SECTION(test_name) {
			CHECK( env.render_file_with_json_file(test_name + "/template.txt", test_name + "/data.json") == env.load_file(test_name + "/result.txt") );
		}
	}
}

TEST_CASE("complete-files-whitespace-control") {
	inja::Environment env {test_file_directory};
	env.set_trim_blocks(true);
	env.set_lstrip_blocks(true);
	
	for (std::string test_name : {"nested-whitespace"}) {
		SECTION(test_name) {
			CHECK( env.render_file_with_json_file(test_name + "/template.txt", test_name + "/data.json") == env.load_file(test_name + "/result.txt") );
		}
	}
}

TEST_CASE("global-path") {
	inja::Environment env {test_file_directory, "./"};
	inja::Environment env_result {"./"};
	json data;
	data["name"] = "Jeff";

	SECTION("Files should be written") {
		env.write("simple.txt", data, "global-path-result.txt");
		CHECK( env_result.load_file("global-path-result.txt") == "Hello Jeff." );
	}
}
