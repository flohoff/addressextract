#ifndef POSTCODE_HPP
#define POSTCODE_HPP

#include <osmium/osm/area.hpp>
#include "AbstractArea.hpp"
#include <iostream>

class PostCode : public AbstractArea {
public:
	std::string	postcode;
	PostCode	*parent;

	PostCode(std::unique_ptr<OGRGeometry> geom, const osmium::Area &area)
		: AbstractArea(std::move(geom)) {

		const osmium::TagList& taglist=area.tags();
		postcode=taglist.get_value_by_key("postal_code", "");
	}

	bool valid(void ) {
		if (postcode == "")
			return false;
		if (postcode.length() == 0)
			return false;
		return true;
	}
};

#endif
