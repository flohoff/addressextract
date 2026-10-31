#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <sstream>

#include "Address.h"
#include "testutil.hpp"

using namespace Address;

static nlohmann::json to_json(const Object& o) {
	std::ostringstream os;
	os << o;
	return nlohmann::json::parse(os.str());
}

TEST_CASE("parse_from_tags picks up only address keys") {
	TestArea ta({{"addr:city", "Bielefeld"}, {"addr:street", "Hauptstr."},
		{"building", "yes"}, {"name", "x"}});
	Object o;
	o.parse_from_tags(ta.area().tags());

	REQUIRE(o.tag_num() == 2);
	REQUIRE(o.has_tag_type(Tag::TYPE_CITY));
	REQUIRE(o.has_tag_type(Tag::TYPE_STREET));
	REQUIRE_FALSE(o.has_tag_type(Tag::TYPE_POSTCODE));
}

TEST_CASE("tag_add_name adds known tags and ignores unknown ones") {
	Object o;
	o.tag_add_name("geom:city", "Bielefeld");
	o.tag_add_name("bogus:tag", "x");

	REQUIRE(o.tag_num() == 1);
	auto t=o.tag_get("geom:city");
	REQUIRE(t != nullptr);
	REQUIRE(t->value == "Bielefeld");
	REQUIRE(o.tag_get("bogus:tag") == nullptr);
}

TEST_CASE("generated geom: tags do not count as address tags") {
	Object o;
	o.tag_add_name("geom:postcode", "33602");
	REQUIRE_FALSE(o.has_tag_type(Tag::TYPE_POSTCODE));

	o.tag_add_name("addr:postcode", "33602");
	REQUIRE(o.has_tag_type(Tag::TYPE_POSTCODE));
}

TEST_CASE("tag_get_by_type distinguishes prefix") {
	Object o;
	o.tag_add_name("geom:city", "Geo");
	o.tag_add_name("addr:city", "Addr");

	auto g=o.tag_get_by_type(Tag::TYPE_CITY, Tag::PFX_GEOM);
	auto a=o.tag_get_by_type(Tag::TYPE_CITY, Tag::PFX_ADDR);
	REQUIRE(g.has_value());
	REQUIRE(a.has_value());
	REQUIRE(g->value == "Geo");
	REQUIRE(a->value == "Addr");
	REQUIRE_FALSE(o.tag_get_by_type(Tag::TYPE_STREET, Tag::PFX_ADDR).has_value());
}

TEST_CASE("source_string") {
	Object o;
	o.source=SourceNode;
	REQUIRE(std::string(o.source_string()) == "node");
	o.source=SourceWay;
	REQUIRE(std::string(o.source_string()) == "way");
	o.source=SourceRelation;
	REQUIRE(std::string(o.source_string()) == "relation");
}

TEST_CASE("json output") {
	Object o;
	o.source=SourceNode;
	o.osmobjid=42;
	o.lat=52.5;
	o.lon=8.25;
	o.bbox[0]=8.25; o.bbox[1]=52.5; o.bbox[2]=8.25; o.bbox[3]=52.5;
	o.tag_add_name("addr:street", "Hauptstr.");

	auto j=to_json(o);
	REQUIRE(j["id"] == 42);
	REQUIRE(j["source"] == "node");
	REQUIRE(j["lat"] == 52.5);
	REQUIRE(j["lon"] == 8.25);
	REQUIRE(j["street"] == "Hauptstr.");
	REQUIRE(j["bbox"].size() == 4);
	REQUIRE_FALSE(j.contains("errors"));

	o.error_add("No housenumber");
	j=to_json(o);
	REQUIRE(j["errors"].size() == 1);
	REQUIRE(j["errors"][0] == "No housenumber");
}

TEST_CASE("json output escapes untrusted values") {
	Object o;
	o.source=SourceWay;
	o.osmobjid=1;
	o.lat=0; o.lon=0;
	o.bbox[0]=0; o.bbox[1]=0; o.bbox[2]=0; o.bbox[3]=0;
	const std::string evil="Stra\"ße\\\n\t</script>";
	o.tag_add_name("addr:street", evil.c_str());
	o.error_add(evil);

	std::ostringstream os;
	os << o;
	/* No raw control characters may survive in the serialized line */
	REQUIRE(os.str().find('\n') == std::string::npos);
	REQUIRE(os.str().find('\t') == std::string::npos);

	auto j=nlohmann::json::parse(os.str());
	REQUIRE(j["street"] == evil);
	REQUIRE(j["errors"][0] == evil);
}

TEST_CASE("later duplicate tag wins in json") {
	Object o;
	o.source=SourceNode;
	o.osmobjid=1;
	o.lat=0; o.lon=0;
	o.bbox[0]=0; o.bbox[1]=0; o.bbox[2]=0; o.bbox[3]=0;
	o.tag_add_name("geom:city", "First");
	o.tag_add_name("geom:city", "Second");
	REQUIRE(to_json(o)["geomcity"] == "Second");
}
