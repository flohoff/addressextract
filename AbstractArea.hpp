#ifndef ABSTRACTAREA_HPP
#define ABSTRACTAREA_HPP

#include <gdalcpp.hpp>
#include <osmium/osm/area.hpp>

class AbstractArea {
	public:
	std::unique_ptr<OGRGeometry>	geometry;
	uint32_t			id;
	bool				cache=false,cachewithin,cacheintersects;

	virtual ~AbstractArea() = default;
	AbstractArea(std::unique_ptr<OGRGeometry> geom);
	void envelope(OGREnvelope& env);
	virtual bool overlaps(AbstractArea *oa);
	virtual bool intersects(AbstractArea *oa);
	/* Spatial predicates of this area's geometry against another geometry.
	 * Overridden by PreparedArea to use the prepared geometry of this side,
	 * which pays off when one area is tested against many others. */
	virtual bool intersects(OGRGeometry *other);
	virtual bool contains(OGRGeometry *other);
	void dump(void);
	virtual bool valid(void) {
		return true;
	};
};

#endif
