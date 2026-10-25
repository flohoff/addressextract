#ifndef POSTCODE_HPP
#define POSTCODE_HPP

#include <osmium/osm/area.hpp>
#include "PreparedArea.hpp"
#include <iostream>

class PostCode : public PreparedArea {
public:
	std::string	postcode;
	PostCode	*parent;

	PostCode(std::unique_ptr<OGRGeometry> geom, const osmium::Area &area, bool prepared=true)
		: PreparedArea(std::move(geom), prepared) {

		const osmium::TagList& taglist=area.tags();
		postcode=taglist.get_value_by_key("postal_code", "");
	}

	bool valid(void ) {
		if (postcode.length() == 0)
			return false;
		return true;
	}
};

#endif
