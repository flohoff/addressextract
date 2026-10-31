#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <iostream>
#include <sstream>

#include <osmium/builder/attr.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>

#include "AddressHandler.hpp"

using namespace osmium::builder::attr;

namespace {

struct Fixture {
	AreaIndex<Boundary>	boundaries{false};
	AreaIndex<PostCode>	postcodes{false};
	AreaIndex<Building>	buildings{false};
	std::vector<Address::Object>	written;
	AddressHandler	handler;

	Fixture() : handler(
		[this](const Address::Object& o) { written.push_back(o); },
		boundaries, postcodes, buildings, false, false, false, "^[0-9]{5}$") {
	}
};

std::string cerr_of(std::function<void()> fn) {
	std::ostringstream captured;
	auto *old=std::cerr.rdbuf(captured.rdbuf());
	fn();
	std::cerr.rdbuf(old);
	return captured.str();
}

osmium::memory::Buffer buffer() {
	return osmium::memory::Buffer{4096, osmium::memory::Buffer::auto_grow::yes};
}

}

TEST_CASE("way without address tags is ignored") {
	Fixture f;
	auto buf=buffer();
	osmium::builder::add_way(buf, _id(1),
		_nodes({{1, {8.5, 52.0}}, {2, {8.6, 52.1}}}), _tag("highway", "residential"));

	auto msg=cerr_of([&] { f.handler.way(buf.get<osmium::Way>(0)); });
	REQUIRE(f.written.empty());
	REQUIRE(msg.empty());
}

TEST_CASE("address way is written with centroid and bbox") {
	Fixture f;
	auto buf=buffer();
	osmium::builder::add_way(buf, _id(7),
		_nodes({{1, {8.0, 52.0}}, {2, {9.0, 52.0}}}),
		_tags({{"addr:street", "Weg"}, {"addr:housenumber", "1"}}));

	f.handler.way(buf.get<osmium::Way>(0));
	REQUIRE(f.written.size() == 1);
	auto& a=f.written[0];
	REQUIRE(a.source == Address::SourceWay);
	REQUIRE(a.osmobjid == 7);
	REQUIRE(a.lon == 8.5);
	REQUIRE(a.lat == 52.0);
	REQUIRE(a.bbox[0] == 8.0);
	REQUIRE(a.bbox[2] == 9.0);
	REQUIRE(a.tag_num() == 2);
}

TEST_CASE("way with unknown node location reports the way id") {
	Fixture f;
	auto buf=buffer();
	osmium::builder::add_way(buf, _id(12),
		_nodes({{1, {8.0, 52.0}}, {2, osmium::Location{}}}),
		_tag("addr:street", "Fehlt"));

	auto msg=cerr_of([&] { f.handler.way(buf.get<osmium::Way>(0)); });
	REQUIRE(f.written.empty());
	REQUIRE(msg == "invalid location wayid 12\n");
}

TEST_CASE("way with a single node reports a geometry error") {
	Fixture f;
	auto buf=buffer();
	osmium::builder::add_way(buf, _id(13),
		_nodes({{1, {8.0, 52.0}}}), _tag("addr:street", "Kurz"));

	auto msg=cerr_of([&] { f.handler.way(buf.get<osmium::Way>(0)); });
	REQUIRE(f.written.empty());
	REQUIRE(msg == "geometry error wayid 13\n");
}

TEST_CASE("address node is written, others are ignored") {
	Fixture f;
	auto buf=buffer();
	osmium::builder::add_node(buf, _id(5), _location(8.25, 52.5),
		_tags({{"addr:housenumber", "3"}}));
	osmium::builder::add_node(buf, _id(6), _location(8.0, 52.0), _tag("amenity", "bench"));

	f.handler.node(buf.get<osmium::Node>(0));
	f.handler.node(*std::next(buf.begin<osmium::Node>()));
	REQUIRE(f.written.size() == 1);
	REQUIRE(f.written[0].source == Address::SourceNode);
	REQUIRE(f.written[0].osmobjid == 5);
	REQUIRE(f.written[0].lon == 8.25);
	REQUIRE(f.written[0].lat == 52.5);
}
