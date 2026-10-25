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

bool AbstractArea::intersects(AbstractArea *oa) {
	return intersects(oa->geometry.get());
}

bool AbstractArea::intersects(OGRGeometry *other) {
	return geometry->Intersects(other);
}

bool AbstractArea::contains(OGRGeometry *other) {
	return geometry->Contains(other);
}

void AbstractArea::dump(void ) {
	geometry->dumpReadable(stdout, nullptr, nullptr);
}
