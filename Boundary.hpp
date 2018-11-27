#ifndef BOUNDARY_HPP
#define BOUNDARY_HPP

#include <osmium/osm/area.hpp>
#include "AbstractArea.hpp"

enum {
	DE_PLACE_UNKNOWN,
	DE_PLACE_CITY,
	DE_PLACE_COUNTY
};

class Boundary : public AbstractArea {
public:
	int		admin_level=99;
	std::string	admin_level_string;
	std::string	name;
	int		deplace=DE_PLACE_UNKNOWN;

	Boundary(std::unique_ptr<OGRGeometry> geom, const osmium::Area &area)
		: AbstractArea(std::move(geom), area) {

		const osmium::TagList& taglist=area.tags();

		name=taglist.get_value_by_key("name", "");

		admin_level_string=taglist.get_value_by_key("admin_level", "");
		if (admin_level_string != "")
			admin_level=std::stoi(admin_level_string);

		// Kreisfreie Städte
		if (admin_level == 6) {
			const char *s=taglist.get_value_by_key("de:place", "");
			if (!s || s == "county")
				deplace=DE_PLACE_COUNTY;
			else
				deplace=DE_PLACE_CITY;
		}


		std::cerr << "Boundary constructor called for " << name << " admin_level " << admin_level << " de:place " << deplace << std::endl;
	}

	bool is_county(void ) {
		return (deplace == DE_PLACE_COUNTY);
	}
};

#endif
