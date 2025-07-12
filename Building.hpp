#ifndef BUILDING_HPP
#define BUILDING_HPP

#include <osmium/osm/area.hpp>
#include <nlohmann/json.hpp>
#include "AbstractArea.hpp"

#include "Address.h"

using json = nlohmann::json;

class Building : public AbstractArea {
public:
	osmium::object_id_type	id;

	Address::Object		address;
	Building		*parent;

	Building(std::unique_ptr<OGRGeometry> geom, const osmium::Area &area)
		: AbstractArea(std::move(geom)) {

		const osmium::TagList& tags=area.tags();
		id=osmium::area_id_to_object_id(area.id());

		address.parse_from_tags(tags);
	}

	void copyvalue(const osmium::TagList& taglist, std::string& out, const char *taglistkey) {
		const char *value=taglist.get_value_by_key(taglistkey);
		if (!value)
			return;
		out=value;
	}

	bool valid(void ) {
		return address.tag_num() > 0;
	}
};

#endif
