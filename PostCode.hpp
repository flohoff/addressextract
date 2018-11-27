#ifndef POSTCODE_HPP
#define POSTCODE_HPP

#include <osmium/osm/area.hpp>
#include "AbstractArea.hpp"

class PostCode : public AbstractArea {
public:
	std::string	postcode;

	PostCode(std::unique_ptr<OGRGeometry> geom, const osmium::Area &area)
		: AbstractArea(std::move(geom), area) {

		const osmium::TagList& taglist=area.tags();
		postcode=taglist.get_value_by_key("postal_code", "");

		if (postcode == "") {
			return;
		}

		std::cerr << "Postcode constructor called for " << postcode << std::endl;
	}
};

#endif
