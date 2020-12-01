#include <iostream>
#include <gdalcpp.hpp>
#include <osmium/osm/area.hpp>

#include "AbstractArea.hpp"

static uint64_t	globalid=0;

AbstractArea::~AbstractArea(void ) {
	delete(geometry);
}

AbstractArea::AbstractArea(std::unique_ptr<OGRGeometry> geom) :
		geometry(geom.release()) {
	id=globalid++;
}

void AbstractArea::envelope(OGREnvelope& env) {
	geometry->getEnvelope(&env);
}

bool AbstractArea::overlaps(AbstractArea *oa) {
	return geometry->Overlaps(oa->geometry)
		|| geometry->Contains(oa->geometry)
		|| geometry->Within(oa->geometry);
}

bool AbstractArea::intersects(AbstractArea *oa) {
	return geometry->Overlaps(oa->geometry);
}

void AbstractArea::dump(void ) {
	geometry->dumpReadable(stdout, nullptr, nullptr);
}
