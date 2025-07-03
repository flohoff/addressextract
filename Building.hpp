#ifndef BUILDING_HPP
#define BUILDING_HPP

#include <osmium/osm/area.hpp>
#include <nlohmann/json.hpp>
#include "AbstractArea.hpp"

using json = nlohmann::json;

class Building : public AbstractArea {
public:
	osmium::object_id_type	id;

	std::string		county,suburb,country,city,postcode,street,housenumber;

	Building(std::unique_ptr<OGRGeometry> geom, const osmium::Area &area)
		: AbstractArea(std::move(geom)) {

		const osmium::TagList& taglist=area.tags();
		id=osmium::area_id_to_object_id(area.id());

		copyvalue(taglist, city, "addr:city");
		copyvalue(taglist, postcode, "addr:postcode");
		copyvalue(taglist, street, "addr:street");
		copyvalue(taglist, housenumber, "addr:housenumber");
		copyvalue(taglist, county, "addr:county");
		copyvalue(taglist, suburb, "addr:suburb");
		copyvalue(taglist, country, "addr:country");
	}

	void copyvalue(const osmium::TagList& taglist, std::string& out, const char *taglistkey) {
		const char *value=taglist.get_value_by_key(taglistkey);
		if (!value)
			return;
		out=value;
	}
};

#endif
