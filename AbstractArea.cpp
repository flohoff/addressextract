#include <iostream>
#include <gdalcpp.hpp>
#include <osmium/osm/area.hpp>

#include "AbstractArea.hpp"

static uint64_t	globalid=0;

AbstractArea::AbstractArea(std::unique_ptr<OGRGeometry> geom) :
		geometry(std::move(geom)) {
	id=globalid++;
}

void AbstractArea::envelope(OGREnvelope& env) {
	geometry->getEnvelope(&env);
}

bool AbstractArea::overlaps(AbstractArea *oa) {
	return geometry->Overlaps(oa->geometry.get())
		|| geometry->Contains(oa->geometry.get())
		|| geometry->Within(oa->geometry.get());
}

bool AbstractArea::intersects(AbstractArea *oa) {
	return geometry->Overlaps(oa->geometry.get());
}

void AbstractArea::dump(void ) {
	geometry->dumpReadable(stdout, nullptr, nullptr);
}
