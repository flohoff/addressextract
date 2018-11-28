#ifndef BOUNDARY_HPP
#define BOUNDARY_HPP

#include <osmium/osm/area.hpp>
#include "AbstractArea.hpp"

enum {
	BTYPE_UNKNOWN,
	BTYPE_CITY,
	BTYPE_COUNTY
};

class Boundary : public AbstractArea {
public:
	int		admin_level=99;
	std::string	admin_level_string;
	std::string	name;
	std::string	deplace_string;
	int		boundarytype=BTYPE_UNKNOWN;

	Boundary(std::unique_ptr<OGRGeometry> geom, const osmium::Area &area)
		: AbstractArea(std::move(geom), area) {

		const osmium::TagList& taglist=area.tags();

		name=taglist.get_value_by_key("name", "");

		admin_level_string=taglist.get_value_by_key("admin_level", "");
		if (admin_level_string != "")
			admin_level=std::stoi(admin_level_string);

		// Kreisfreie Städte
		if (admin_level == 6) {
			deplace_string=taglist.get_value_by_key("de:place", "");
			if (deplace_string == "county" || deplace_string == "")
				boundarytype=BTYPE_COUNTY;
			else
				boundarytype=BTYPE_CITY;
		}


		std::cerr << "Boundary constructor called for " << name
			<< " admin_level " << admin_level
			<< " de:place " << deplace_string
			<< " boundarytype " << boundarytype
			<< std::endl;
	}

	bool is_county(void ) {
		return (boundarytype == BTYPE_COUNTY);
	}
};

#endif
