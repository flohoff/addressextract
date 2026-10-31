#ifndef TESTUTIL_HPP
#define TESTUTIL_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <osmium/builder/attr.hpp>
#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/area.hpp>

#include <gdalcpp.hpp>
#include <ogr_geometry.h>

/* osmium::Area with tags only */
class TestArea {
	osmium::memory::Buffer	buffer{4096, osmium::memory::Buffer::auto_grow::yes};
public:
	explicit TestArea(const std::vector<std::pair<std::string, std::string>>& tags,
			osmium::object_id_type id=2) {
		{
			osmium::builder::AreaBuilder builder{buffer};
			builder.set_id(id);
			osmium::builder::TagListBuilder tlb{builder};
			for (auto& t : tags)
				tlb.add_tag(t.first, t.second);
		}
		buffer.commit();
	}
	const osmium::Area& area() {
		return buffer.get<osmium::Area>(0);
	}
};

inline std::unique_ptr<OGRGeometry> box_polygon(double x0, double y0, double x1, double y1) {
	auto ring=std::make_unique<OGRLinearRing>();
	ring->addPoint(x0, y0);
	ring->addPoint(x0, y1);
	ring->addPoint(x1, y1);
	ring->addPoint(x1, y0);
	ring->addPoint(x0, y0);
	auto poly=std::make_unique<OGRPolygon>();
	poly->addRingDirectly(ring.release());
	return poly;
}

#endif
