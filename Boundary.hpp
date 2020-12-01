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
	osmium::unsigned_object_id_type	id;
	int		admin_level=99;
	std::string	admin_level_string;

	std::string	name;
	std::string	deplace;
	std::string	nameprefix;
	std::string	namesuffix;

	std::string	nameofficial;

	int		boundarytype=BTYPE_UNKNOWN;

	Boundary*	up=nullptr;

	Boundary(std::unique_ptr<OGRGeometry> geom, const osmium::Area &area)
		: AbstractArea(std::move(geom)) {

		const osmium::TagList& taglist=area.tags();

		id=area.id();

		name=taglist.get_value_by_key("name", "");
		nameprefix=taglist.get_value_by_key("name:prefix", "");
		namesuffix=taglist.get_value_by_key("name:suffix", "");

		//
		// Build official name - No documentation on the osm wiki for this
		//
		// Examples show:
		//
		//   name:prefix=Stadt
		//   name=Werther
		//   name:suffix=(Westf.)
		//
		// Or
		//
		//   name:prefix=Stadt
		//   name=Halle (Westf.)
		//
		nameofficial=name;
		if (namesuffix != "")
			nameofficial.append(" ").append(namesuffix);

		admin_level_string=taglist.get_value_by_key("admin_level", "");
		if (admin_level_string != "")
			admin_level=std::stoi(admin_level_string);

		// Kreisfreie Städte
		if (admin_level == 6) {
			deplace=taglist.get_value_by_key("de:place", "");
			if (deplace == "county" || deplace == "")
				boundarytype=BTYPE_COUNTY;
			else
				boundarytype=BTYPE_CITY;
		}


		std::cerr << "Boundary constructor called for "
			<< nameofficial
			<< std::endl << "\t"
			<< " id " << osmium::area_id_to_object_id(id)
			<< std::endl << "\t"
			<< " name " << name
			<< " name:prefix " << nameprefix
			<< " name:suffix " << namesuffix
			<< std::endl << "\t"
			<< " admin_level " << admin_level
			<< " de:place " << deplace
			<< " boundarytype " << boundarytype
			<< std::endl;
	}

	bool is_county(void ) {
		return (boundarytype == BTYPE_COUNTY);
	}
};

#endif
