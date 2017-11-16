#include "catch.hpp"
#include "nlohmann/json.hpp"
#include "inja.hpp"



TEST_CASE("Dot to pointer notation") {
	CHECK( inja::dot_to_json_pointer_notation("person.names.surname") == "/person/names/surname" );
	CHECK( inja::dot_to_json_pointer_notation("guests.2") == "/guests/2" );
}
