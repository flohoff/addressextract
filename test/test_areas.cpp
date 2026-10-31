#include <catch2/catch_test_macros.hpp>

#include <ogr_geometry.h>

#include "AbstractArea.hpp"
#include "PreparedArea.hpp"
#include "Boundary.hpp"
#include "PostCode.hpp"
#include "Building.hpp"
#include "testutil.hpp"

static OGRPoint pt(double x, double y) {
	return OGRPoint(x, y);
}

TEST_CASE("AbstractArea ids are unique and envelope matches") {
	AbstractArea a(box_polygon(0, 0, 2, 1));
	AbstractArea b(box_polygon(0, 0, 1, 1));
	REQUIRE(a.id != b.id);

	OGREnvelope env;
	a.envelope(env);
	REQUIRE(env.MinX == 0);
	REQUIRE(env.MaxX == 2);
	REQUIRE(env.MaxY == 1);
}

TEST_CASE("AbstractArea predicates") {
	AbstractArea a(box_polygon(0, 0, 10, 10));
	AbstractArea inner(box_polygon(2, 2, 3, 3));
	AbstractArea away(box_polygon(20, 20, 30, 30));
	OGRPoint in=pt(5, 5), out=pt(50, 50);

	REQUIRE(a.contains(&in));
	REQUIRE_FALSE(a.contains(&out));
	REQUIRE(a.intersects(&inner));
	REQUIRE_FALSE(a.intersects(&away));
	REQUIRE(a.valid());
}

TEST_CASE("PreparedArea matches unprepared results") {
	PreparedArea p(box_polygon(0, 0, 10, 10), true);
	PreparedArea u(box_polygon(0, 0, 10, 10), false);
	REQUIRE(p.prepgeometry);
	REQUIRE_FALSE(u.prepgeometry);

	for (double x : {-1.0, 0.0, 5.0, 10.0, 11.0}) {
		OGRPoint q=pt(x, 5);
		REQUIRE(p.contains(&q) == u.contains(&q));
		REQUIRE(p.intersects(&q) == u.intersects(&q));
	}
	/* edge: intersects, not contained */
	OGRPoint edge=pt(0, 5);
	REQUIRE(p.intersects(&edge));
	REQUIRE_FALSE(p.contains(&edge));

	AbstractArea other(box_polygon(5, 5, 15, 15));
	REQUIRE(p.intersects(&other));
}

TEST_CASE("Boundary parses name, admin_level and county type") {
	TestArea ta({{"name", "Bielefeld"}, {"admin_level", "6"}, {"de:place", "city"}});
	Boundary b(box_polygon(0, 0, 1, 1), ta.area());
	REQUIRE(b.name == "Bielefeld");
	REQUIRE(b.admin_level == 6);
	REQUIRE(b.valid());
	REQUIRE_FALSE(b.is_county());

	TestArea kreis({{"name", "Kreis X"}, {"admin_level", "6"}});
	Boundary k(box_polygon(0, 0, 1, 1), kreis.area());
	REQUIRE(k.is_county());
}

TEST_CASE("Boundary official_name wins over name") {
	TestArea ta({{"name", "Short"}, {"official_name", "Long official"}, {"admin_level", "8"}});
	Boundary b(box_polygon(0, 0, 1, 1), ta.area());
	REQUIRE(b.name == "Long official");
}

TEST_CASE("Boundary validity rules") {
	auto make=[](std::vector<std::pair<std::string, std::string>> tags) {
		TestArea ta(tags);
		Boundary b(box_polygon(0, 0, 1, 1), ta.area());
		return b.valid();
	};
	REQUIRE(make({{"name", "A"}, {"admin_level", "3"}}));
	REQUIRE(make({{"name", "A"}, {"admin_level", "10"}}));
	REQUIRE_FALSE(make({{"name", "A"}, {"admin_level", "2"}}));
	REQUIRE_FALSE(make({{"name", "A"}, {"admin_level", "11"}}));
	REQUIRE_FALSE(make({{"admin_level", "8"}}));          // no name
	REQUIRE_FALSE(make({{"name", "A"}}));                 // no admin_level
	REQUIRE_FALSE(make({{"name", "A"}, {"admin_level", "abc"}}));  // garbage level
}

TEST_CASE("PostCode validity") {
	TestArea ok({{"postal_code", "33602"}});
	PostCode p(box_polygon(0, 0, 1, 1), ok.area());
	REQUIRE(p.postcode == "33602");
	REQUIRE(p.valid());

	TestArea none({{"name", "x"}});
	PostCode q(box_polygon(0, 0, 1, 1), none.area());
	REQUIRE_FALSE(q.valid());
}

TEST_CASE("Building is valid only with address tags") {
	TestArea addr({{"building", "yes"}, {"addr:housenumber", "5"}});
	Building b(box_polygon(0, 0, 1, 1), addr.area());
	REQUIRE(b.valid());

	TestArea plain({{"building", "yes"}});
	Building c(box_polygon(0, 0, 1, 1), plain.area());
	REQUIRE_FALSE(c.valid());
}
