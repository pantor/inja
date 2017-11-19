#include "catch.hpp"
#include "nlohmann/json.hpp"
#include "inja.hpp"


using json = nlohmann::json;
using Type = inja::Parsed::Type;



/* TEST_CASE("Parse structure") {
	inja::Parser parser = inja::Parser();

	SECTION("Basic string") {
		std::string test = "lorem ipsum";
		json result = {{{"type", Type::String}, {"text", "lorem ipsum"}}};
		CHECK( parser.parse(test) == result );
	}

	SECTION("Empty string") {
		std::string test = "";
		json result = {};
		CHECK( parser.parse(test) == result );
	}

	SECTION("Variable") {
		std::string test = "{{ name }}";
		json result = {{{"type", Type::Expression}, {"command", "name"}}};
		CHECK( parser.parse(test) == result );
	}

	SECTION("Combined string and variables") {
		std::string test = "Hello {{ name }}!";
		json result = {
			{{"type", Type::String}, {"text", "Hello "}},
			{{"type", Type::Expression}, {"command", "name"}},
			{{"type", Type::String}, {"text", "!"}}
		};
		CHECK( parser.parse(test) == result );
	}

	SECTION("Multiple variables") {
		std::string test = "Hello {{ name }}! I come from {{ city }}.";
		json result = {
			{{"type", Type::String}, {"text", "Hello "}},
			{{"type", Type::Expression}, {"command", "name"}},
			{{"type", Type::String}, {"text", "! I come from "}},
			{{"type", Type::Expression}, {"command", "city"}},
			{{"type", Type::String}, {"text", "."}}
		};
		CHECK( parser.parse(test) == result );
	}

	SECTION("Loops") {
		std::string test = "open {% for e in list %}lorem{% endfor %} closing";
		json result = {
			{{"type", Type::String}, {"text", "open "}},
			{{"type", Type::Loop}, {"item", "e"}, {"list", "list"}, {"children", {
				{{"type", Type::String}, {"text", "lorem"}}
			}}},
			{{"type", Type::String}, {"text", " closing"}}
		};
		CHECK( parser.parse(test) == result );
	}

	SECTION("Nested loops") {
		std::string test = "{% for e in list %}{% for b in list2 %}lorem{% endfor %}{% endfor %}";
		json result = {
			{{"type", Type::Loop}, {"item", "e"}, {"list", "list"}, {"children", {
				{{"type", Type::Loop}, {"item", "b"}, {"list", "list2"}, {"children", {
					{{"type", Type::String}, {"text", "lorem"}}
				}}}
			}}}
		};
		CHECK( parser.parse(test) == result );
	}

	SECTION("Basic conditional") {
		std::string test = "{% if true %}Hello{% endif %}";
		json result = {
			{{"type", Type::Condition}, {"children", {
				{{"type", Type::ConditionBranch}, {"command", "if true"}, {"children", {
					{{"type", Type::String}, {"text", "Hello"}}
				}}}
			}}}
		};
		CHECK( parser.parse(test) == result );
	}

	SECTION("If/else if/else conditional") {
		std::string test = "if: {% if maybe %}first if{% else if perhaps %}first else if{% else if sometimes %}second else if{% else %}test else{% endif %}";
		json result = {
			{{"type", Type::String}, {"text", "if: "}},
			{{"type", Type::Condition}, {"children", {
				{{"type", Type::ConditionBranch}, {"command", "if maybe"}, {"children", {
					{{"type", Type::String}, {"text", "first if"}}
				}}},
				{{"type", Type::ConditionBranch}, {"command", "else if perhaps"}, {"children", {
					{{"type", Type::String}, {"text", "first else if"}}
				}}},
				{{"type", Type::ConditionBranch}, {"command", "else if sometimes"}, {"children", {
					{{"type", Type::String}, {"text", "second else if"}}
				}}},
				{{"type", Type::ConditionBranch}, {"command", "else"}, {"children", {
					{{"type", Type::String}, {"text", "test else"}}
				}}},
			}}}
		};
		CHECK( parser.parse(test) == result );
	}

	SECTION("Comments") {
		std::string test = "{# lorem ipsum #}";
		json result = {{{"type", Type::Comment}, {"text", "lorem ipsum"}}};
		CHECK( parser.parse(test) == result );
	}

	SECTION("Line Statements") {
		std::string test = R"(## if true
lorem ipsum
## endif)";
		json result = {
			{{"type", Type::Condition}, {"children", {
				{{"type", Type::ConditionBranch}, {"command", "if true"}, {"children", {
					{{"type", Type::String}, {"text", "lorem ipsum"}}
				}}}
			}}}
		};
		CHECK( parser.parse(test) == result );
	}
} */
