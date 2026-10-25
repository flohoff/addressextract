#ifndef PREPAREDAREA_HPP
#define PREPAREDAREA_HPP

#include <gdalcpp.hpp>
#include <osmium/osm/area.hpp>
#include "AbstractArea.hpp"

class PreparedArea : public AbstractArea {
	public:
	OGRPreparedGeometryUniquePtr	prepgeometry;

	PreparedArea(std::unique_ptr<OGRGeometry> geom)
		: AbstractArea(std::move(geom)),
		prepgeometry(OGRCreatePreparedGeometry(static_cast<OGRGeometryH>(geometry.get()))) {
	}

	// OGR only exposes OGRPreparedGeometryIntersects() and
	// OGRPreparedGeometryContains() for prepared geometries. Intersects()
	// subsumes the strict Overlaps()/Contains()/Within() combination used by
	// AbstractArea::overlaps() (it additionally reports boundary-touching
	// geometries, which the base class ignores).
	bool overlaps(AbstractArea *oa) override {
		return intersects(oa);
	}

	bool intersects(AbstractArea *oa) override {
		return intersects(oa->geometry.get());
	}

	bool intersects(OGRGeometry *other) override {
		if (!prepgeometry)
			return geometry->Intersects(other);
		return OGRPreparedGeometryIntersects(prepgeometry.get(),
				static_cast<OGRGeometryH>(other)) != 0;
	}

	bool contains(OGRGeometry *other) override {
		if (!prepgeometry)
			return geometry->Contains(other);
		return OGRPreparedGeometryContains(prepgeometry.get(),
				static_cast<OGRGeometryH>(other)) != 0;
	}
};

#endif
