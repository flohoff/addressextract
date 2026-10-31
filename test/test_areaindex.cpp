#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

#include "AreaIndex.hpp"
#include "Boundary.hpp"
#include "PostCode.hpp"
#include "AddressHandler.hpp"
#include "testutil.hpp"

static size_t count_real(const std::vector<Boundary*>& l) {
	return std::count_if(l.begin(), l.end(), [](Boundary *b) { return !b->cache; });
}

TEST_CASE("AreaIndex without cache grid finds by envelope") {
	AreaIndex<PostCode> idx(false);
	TestArea a({{"postal_code", "11111"}});
	TestArea b({{"postal_code", "22222"}});
	PostCode p1(box_polygon(0, 0, 10, 10), a.area());
	PostCode p2(box_polygon(20, 20, 30, 30), b.area());
	idx.insert(&p1, a.area());
	idx.insert(&p2, b.area());

	OGRPoint q(5, 5);
	std::vector<PostCode*> list;
	idx.findoverlapping_geom(&q, &list);
	REQUIRE(list.size() == 1);
	REQUIRE(list[0]->postcode == "11111");

	OGRPoint nothing(15, 15);
	list.clear();
	idx.findoverlapping_geom(&nothing, &list);
	REQUIRE(list.empty());
	REQUIRE(idx.cacheentriespos == 0);

	list.clear();
	idx.findoverlapping(&p2, &list);
	REQUIRE(list.size() == 1);
	REQUIRE(list[0] == &p2);
}

TEST_CASE("AreaIndex cache grid creates cells fully within the parent") {
	AreaIndex<Boundary> idx(true);
	TestArea ta({{"name", "Town"}, {"admin_level", "8"}});
	Boundary b(box_polygon(0, 0, 10, 10), ta.area());
	idx.insert(&b, ta.area());

	REQUIRE(idx.cacheentriespos > 0);
	/* box: one cell covers it */
	REQUIRE(idx.cacheentriespos == 1);

	OGRPoint q(5, 5);
	std::vector<Boundary*> list;
	idx.findoverlapping_geom(&q, &list);
	REQUIRE(list.size() == 2);
	REQUIRE(count_real(list) == 1);
	for (auto c : list) {
		if (c->cache)
			REQUIRE(c->parent == &b);
	}
}

TEST_CASE("Cache grid skips cells outside a concave geometry") {
	AreaIndex<Boundary> idx(true);
	TestArea ta({{"name", "L"}, {"admin_level", "8"}});

	/* L shape, upper right empty */
	auto ring=std::make_unique<OGRLinearRing>();
	double xy[][2]={{0,0},{10,0},{10,5},{5,5},{5,10},{0,10},{0,0}};
	for (auto& p : xy) ring->addPoint(p[0], p[1]);
	auto poly=std::make_unique<OGRPolygon>();
	poly->addRingDirectly(ring.release());

	Boundary b(std::move(poly), ta.area());
	idx.insert(&b, ta.area());

	OGRPoint hole(8, 8);
	std::vector<Boundary*> list;
	idx.findoverlapping_geom(&hole, &list);
	for (auto c : list) {
		if (c->cache) {
			INFO("cache cell must never cover the empty quadrant");
			REQUIRE_FALSE(c->contains(&hole));
		}
	}
	REQUIRE_FALSE(b.contains(&hole));
}

TEST_CASE("Boundary without name is invalid and not indexed") {
	AreaIndex<Boundary> idx(false);
	TestArea unnamed({{"admin_level", "8"}});
	Boundary b(box_polygon(0, 0, 1, 1), unnamed.area());
	REQUIRE_FALSE(b.valid());
	REQUIRE(idx.areavector.empty());
}

TEST_CASE("Boundary sort order: most specific first, then cache before real") {
	auto mk=[](const char *level) {
		return TestArea({{"name", "n"}, {"admin_level", level}});
	};
	auto t8=mk("8"), t10=mk("10"), t8b=mk("8");
	Boundary city(box_polygon(0, 0, 1, 1), t8.area());
	Boundary suburb(box_polygon(0, 0, 1, 1), t10.area());
	Boundary cell(box_polygon(0, 0, 1, 1), t8b.area(), false);
	cell.cache=true;
	cell.id=city.id;       // cell shares parent id

	std::vector<Boundary*> l={&city, &cell, &suburb};
	std::sort(l.begin(), l.end(), compare_admin_level);
	REQUIRE(l[0] == &suburb);
	REQUIRE(l[1] == &cell);
	REQUIRE(l[2] == &city);
}

static std::unique_ptr<OGRGeometry> circle(int nodes) {
	auto ring=std::make_unique<OGRLinearRing>();
	for (int i=0;i<nodes;i++) {
		double a=2*M_PI*i/nodes;
		ring->addPoint(5+4*cos(a), 5+4*sin(a));
	}
	ring->closeRings();
	auto poly=std::make_unique<OGRPolygon>();
	poly->addRingDirectly(ring.release());
	return poly;
}

TEST_CASE("Cache grid depth grows with geometry size") {
	AreaIndex<Boundary> idx(true);
	TestArea ta({{"name", "C"}, {"admin_level", "8"}});
	Boundary small(circle(100), ta.area());
	Boundary mid(circle(1000), ta.area());
	Boundary big(circle(7000), ta.area());
	REQUIRE(idx.cache_depth(&small) == 3);
	REQUIRE(idx.cache_depth(&mid) == 4);
	REQUIRE(idx.cache_depth(&big) == 5);
}

TEST_CASE("Comparators are irreflexive and order the cache flag") {
	TestArea t({{"name", "n"}, {"admin_level", "8"}});
	Boundary real(box_polygon(0, 0, 1, 1), t.area());
	Boundary same(box_polygon(0, 0, 1, 1), t.area(), false);
	same.id=real.id;
	same.cache=true;

	REQUIRE_FALSE(compare_admin_level(&real, &real));
	REQUIRE_FALSE(compare_admin_level(&same, &same));
	REQUIRE(compare_admin_level(&same, &real));
	REQUIRE_FALSE(compare_admin_level(&real, &same));

	TestArea p({{"postal_code", "1"}});
	PostCode a(box_polygon(0, 0, 1, 1), p.area());
	PostCode b(box_polygon(0, 0, 1, 1), p.area(), false);
	b.cache=true;
	REQUIRE(postcode_compare(&b, &a));
	REQUIRE_FALSE(postcode_compare(&a, &b));
	REQUIRE_FALSE(postcode_compare(&a, &a));
	REQUIRE_FALSE(postcode_compare(&b, &b));
}
