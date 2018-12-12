#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>
#include <user_meeting.hpp>
#include <iostream>

using nlohmann::json;

bool operator== (const handlers::Meeting &m1, const handlers::Meeting &m2) {
	return 
			m1.name == m2.name &&
			m1.address == m2.address &&
			m1.description == m2.description &&
			m1.signup_description == m2.signup_description &&
			m1.signup_from_date == m2.signup_from_date &&
			m1.signup_to_date == m2.signup_to_date &&
			m1.from_date == m2.from_date &&
			m1.to_date == m2.to_date &&
			m1.published == m2.published;
}

TEST_CASE("json_marshal") {
//json json_sourse;
handlers::Meeting source;
source.id = 1;
source.name = "test";
source.address = "addr";
source.description ="desc";
source.signup_description = "sdesc";
source.signup_from_date = 1;
source.signup_to_date = 2;
source.from_date = 3;
source.to_date = 4;
source.published = true;

json target = R"(
	{
		"id": 1,
		"name": "test",
		"address": "addr",
		"description": "desc",
		"signup_description": "sdesc",
		"signup_from_date": 1,
		"signup_to_date": 2,
		"from_date": 3,
		"to_date": 4,
		"published": true
	}
 )"_json;


CHECK(json::diff(json(source),target).empty() == true);
}

TEST_CASE("json_unmarshal") {
	json sourse = R"(
	{
		"name": "test",
		"address": "addr",
		"description": "desc",
		"signup_description": "sdesc",
		"signup_from_date": 11,
		"signup_to_date": 22,
		"from_date": 33,
		"to_date": 44,
		"published": true
	}
	)"_json;
	handlers::Meeting sourseMeeting = sourse;
	handlers::Meeting target;
	target.name = "test";
	target.address = "addr";
	target.description = "desc";
	target.signup_description = "sdesc";
	target.signup_from_date = 11;
	target.signup_to_date = 22;
	target.from_date = 33;
	target.to_date = 44;
	target.published = true;
	CHECK((sourseMeeting == target));
}

TEST_CASE("json_unmarshal fail") {
	json sourse = R"(
	{
		"name": "test",
		"address": "addr",
		"description": "desc",
		"signup_description": "sdesc",
		"signup_from_date": 11,
		"signup_to_date": 22,
		"from_date": 33,
		"to_date": 44,
		"published": true
	}
	)"_json;
	handlers::Meeting sourseMeeting = sourse;
	handlers::Meeting target;
	target.name = "test";
	target.address = "addr";
	target.description = "desc";
	target.signup_description = "sdesc";
	target.signup_from_date = 111;
	target.signup_to_date = 22;
	target.from_date = 33;
	target.to_date = 44;
	target.published = true;
	CHECK(!(sourseMeeting == target));
}