#ifndef ABSTRACTAREA_HPP
#define ABSTRACTAREA_HPP

#include <gdalcpp.hpp>
#include <osmium/osm/area.hpp>

class AbstractArea {
	public:
	std::unique_ptr<OGRGeometry>	geometry;
	uint32_t			id;

	/* Cache grid cells completely within */
	bool				cache=false;

	virtual ~AbstractArea() = default;
	AbstractArea(std::unique_ptr<OGRGeometry> geom);
	void envelope(OGREnvelope& env);
	virtual bool intersects(AbstractArea *oa);
	virtual bool intersects(OGRGeometry *other);
	virtual bool contains(OGRGeometry *other);
	void dump(void);
	virtual bool valid(void) {
		return true;
	};
};

#endif
