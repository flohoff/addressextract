#ifndef BUILDING_HPP
#define BUILDING_HPP

#include <osmium/osm/area.hpp>
#include <nlohmann/json.hpp>
#include "AbstractArea.hpp"

using json = nlohmann::json;

class Building : public AbstractArea {
public:
	osmium::object_id_type	id;

	json			j;
	std::string		city,postcode,street,housenumber;

	Building(std::unique_ptr<OGRGeometry> geom, const osmium::Area &area)
		: AbstractArea(std::move(geom)) {

		const osmium::TagList& taglist=area.tags();

		id=osmium::area_id_to_object_id(area.id());

		copyvalue(taglist, "city", "addr:city");
		copyvalue(taglist, "postcode", "addr:postcode");
		copyvalue(taglist, "street", "addr:street");
		copyvalue(taglist, "housenumber", "addr:housenumber");
	}

	void copyvalue(const osmium::TagList& taglist, const char *jsonkey, const char *taglistkey) {
		const char *value=taglist.get_value_by_key(taglistkey);
		if (!value)
			return;
		j[jsonkey]=value;
	}
};

#endif
