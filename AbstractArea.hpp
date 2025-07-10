#ifndef ABSTRACTAREA_HPP
#define ABSTRACTAREA_HPP

#include <gdalcpp.hpp>
#include <osmium/osm/area.hpp>

class AbstractArea {
	public:
	const OGRGeometry	*geometry;
	uint32_t		id;
	bool			cache=false,cachewithin,cacheintersects;

	virtual ~AbstractArea();
	AbstractArea(std::unique_ptr<OGRGeometry> geom);
	void envelope(OGREnvelope& env);
	bool overlaps(AbstractArea *oa);
	bool intersects(AbstractArea *oa);
	void dump(void);
	virtual bool valid(void) {
		return true;
	};
};

#endif
