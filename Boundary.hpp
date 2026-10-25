#ifndef BOUNDARY_HPP
#define BOUNDARY_HPP

#include <osmium/osm/area.hpp>
#include "PreparedArea.hpp"

enum {
	BTYPE_UNKNOWN,
	BTYPE_CITY,
	BTYPE_COUNTY
};

class Boundary : public PreparedArea {
public:
	osmium::unsigned_object_id_type	id;
	int		admin_level=99;
	std::string	admin_level_string;

	std::string	name;
	std::string	deplace;

	int		boundarytype=BTYPE_UNKNOWN;

	Boundary*	up=nullptr;
	Boundary*	parent=nullptr;

	Boundary(std::unique_ptr<OGRGeometry> geom, const osmium::Area &area, bool prepared=true)
		: PreparedArea(std::move(geom), prepared) {

		const osmium::TagList& taglist=area.tags();

		id=area.orig_id();

		name=taglist.get_value_by_key("official_name", "");
		if (name == "")
			name=taglist.get_value_by_key("name", "");

		try {
			admin_level_string=taglist.get_value_by_key("admin_level", "");
			if (admin_level_string != "")
				admin_level=std::stoi(admin_level_string);
		} catch(const std::invalid_argument &c) {
		}

		// Kreisfreie Städte
		if (admin_level == 6) {
			deplace=taglist.get_value_by_key("de:place", "");
			if (deplace == "county" || deplace == "")
				boundarytype=BTYPE_COUNTY;
			else
				boundarytype=BTYPE_CITY;
		}

		if (!valid()) {
			std::cerr << "Boundary constructor called for "
				<< (area.from_way() ? "way" : "relation")
				<< " " << area.orig_id()
				<< std::endl << "\t"
				<< " name " << name
				<< std::endl << "\t"
				<< " admin_level " << admin_level
				<< " de:place " << deplace
				<< " boundarytype " << boundarytype
				<< std::endl;
		}
	}

	bool is_county(void ) {
		return (boundarytype == BTYPE_COUNTY);
	}

	/* Only specific admin levels and we need to have a name */
	bool valid(void ) override {
		return admin_level < 11
			&& admin_level > 2
			&& name.length() > 0;
	}
};

#endif
