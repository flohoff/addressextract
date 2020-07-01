
#include <osmium/tags/tags_filter.hpp>
#include <osmium/handler.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/area.hpp>

#include <json.hpp>
#include <regex>
#include <string>
#include <iostream>
#include <boost/format.hpp>

#include "AreaIndex.hpp"
#include "Boundary.hpp"
#include "PostCode.hpp"
#include "Building.hpp"

using json = nlohmann::json;

bool compare_admin_level(Boundary *a, Boundary *b) { return(a->admin_level > b->admin_level); };

class AddressHandler : public osmium::handler::Handler {
	osmium::geom::OGRFactory<>      m_factory{};
	AreaIndex<Boundary>&		boundaryindex;
	AreaIndex<PostCode>&		postcodeindex;
	AreaIndex<Building>&		buildingindex;
	json				j;
	bool				t_errors,t_missing,t_nocache;
	std::regex			housenumber_regex,
					street_regex,
					postcode_regex,
					housename_regex;
	std::string			timestamp;
	OGRPoint			point;
	std::vector<Boundary*>		boundarylist;
	std::vector<PostCode*>		postcodelist;
public:
	AddressHandler(AreaIndex<Boundary>& bidx, AreaIndex<PostCode>& pidx, AreaIndex<Building>& buidx,
			bool errors, bool missing, bool nocache, std::string timestamp) :
		boundaryindex(bidx), postcodeindex(pidx), buildingindex(buidx),
			t_errors(errors), t_missing(missing), t_nocache(nocache), timestamp(timestamp) {

		housenumber_regex="^ |,|;| $|[0-9] [a-zA-Z]";
		housename_regex="^ *[0-9]* *$|GmbH|e\\. *V\\.|Sparkasse|[sS]tr\\.|[Ss]traße|http[s]*://";
		street_regex="^ | $|Str\\.$|str\\.$|\\t|\\.$";
		postcode_regex="^[0-9]{5}$";

		if (!timestamp.empty())
			j["timestamp"]=timestamp;
	}

	bool isaddress(const osmium::TagList& tags) {
		if (tags.empty())
			return false;
		if (tags.has_key("addr:housenumber"))
			return true;
		if (tags.has_key("addr:street"))
			return true;
		if (tags.has_key("addr:city"))
			return true;
		return false;
	}

	void extend_city_boundary(json& address, Boundary *b) {
		switch(b->admin_level) {
			case(8):
				address["geomcity"]=b->nameofficial;
				break;
			case(6):
				if (b->is_county())
					address["geomcounty"]=b->nameofficial;
				else
					address["geomcity"]=b->nameofficial;
				break;
			case(9):
			case(10):
				address["geomsuburb"]=b->nameofficial;
				break;
		}
	}

	void extend_city(json& address, OGRGeometry *geom) {
		if (t_missing && address.count("city") > 0)
			return;

		boundarylist.clear();
		boundaryindex.findoverlapping_geom(geom, &boundarylist);
		std::sort(boundarylist.begin(), boundarylist.end(), compare_admin_level);

		Boundary	*lastboundary=nullptr;

		for(auto b : boundarylist) {
			if (b->admin_level > 10)
				continue;
			if (!geom->Intersects(b->geometry))
				continue;

			extend_city_boundary(address, b);

			if (!t_nocache && b->up) {
				while(b->up) {
					b=b->up;
					extend_city_boundary(address, b);
				}
				break;
			}

			// Insert us into last admin level if its
			// bigger - read - a smaller level.
			if (!t_nocache
				&& lastboundary
				&& b->admin_level < lastboundary->admin_level
				&& lastboundary->up == nullptr) {

				std::cerr
					<< "Set cache parent of "
					<< lastboundary->nameofficial
					<< " (" << lastboundary->admin_level << ") id (" << lastboundary->id << ")"
					<< " to "
					<< b->nameofficial
					<< " (" << b->admin_level << ") id (" << b->id << ") "
					<< std::endl;

				lastboundary->up=b;
			}
			lastboundary=b;
		}

		if (t_missing && address.count("geomcity"))
			address["city"]=address["geomcity"];
	}

	void extend_postcode(json& address, OGRGeometry *geom) {
		// If we only want to add missing information
		if (t_missing && address.count("postcode") > 0)
			return;

		postcodelist.clear();
		postcodeindex.findoverlapping_geom(geom, &postcodelist);
		for(auto i : postcodelist) {
			if (geom->Within(i->geometry)
				|| geom->Overlaps(i->geometry)) {
				address["geompostcode"]=i->postcode;
				break;
			}
		}

		if (t_missing && address.count("geompostcode"))
			address["postcode"]=address["geompostcode"];
	}

	void checkerror(json& address, OGRGeometry *geom) {
		if (address.count("housenumber") == 0)
			address["errors"].push_back("No housenumber");
		if (address.count("city") == 0)
			address["errors"].push_back("No city");
		if (address.count("postcode") == 0)
			address["errors"].push_back("No postcode");
		if (address.count("street") == 0 && address.count("place") == 0)
			address["errors"].push_back("No addr:street or addr:place");

		if (address.count("street") > 0) {
			std::string	street=address["street"].get<std::string>();
			if (std::regex_search(street, street_regex)) {
				address["errors"].push_back("Street format issues");
			}
		}

		if (address.count("postcode") > 0) {
			std::string	postcode=address["postcode"].get<std::string>();
			if (!std::regex_search(postcode, postcode_regex)) {
				address["errors"].push_back("Postcode format issues");
			}
		}

		if (address.count("postcode") > 0 && address.count("geompostcode") > 0) {
			if (address["postcode"] != address["geompostcode"]) {
				address["errors"].push_back("Postcode mismatch");
			}
		}

		if (address.count("city") > 0  && address.count("geomcity") > 0) {
			if (address["city"] != address["geomcity"]) {
				address["errors"].push_back("City mismatch");
			}
		}

		if (address.count("housename") > 0) {
			std::string	hn=address["housename"].get<std::string>();

			if (std::regex_search(hn, housename_regex)) {
				address["errors"].push_back("Housename suspicious");
			}
		}

		if (address.count("housenumber") > 0) {
			// leading spaces
			// trailing spaces
			// 20,22
			// 20;22
			// 100 a
			std::string	hn=address["housenumber"].get<std::string>();
			if (std::regex_search(hn, housenumber_regex)) {
				address["errors"].push_back("Housenumber format issues");
			}
		}

		if (address["source"] == "node") {
			static std::vector<Building*>	list;

			list.clear();

			buildingindex.findoverlapping_geom(geom, &list);

			for(auto i : list) {
				if (!geom->Within(i->geometry))
					continue;

				std::vector<std::string> comparekeys={ "city", "street", "postcode", "housenumber" };

				for(auto key : comparekeys) {
					if (i->j[key].is_string()
						&& address[key].is_string()
						&& i->j[key] != address[key]) {

						address["errors"].push_back(str(boost::format("addr:%1% mismatch to enclosing building outline") % key));
					}
				}

				break;
			}
		}
	}

	void tag2json(json& address, const osmium::TagList& tags, const char *tagname, const char *jsonkey) {
		const char *value=tags.get_value_by_key(tagname, nullptr);
		if (value)
			address[jsonkey]=value;
	}

	void parseaddr(json& address, OGRGeometry *geom, const osmium::TagList& tags) {
		tag2json(address, tags, "addr:street", "street");
		tag2json(address, tags, "addr:housenumber", "housenumber");
		tag2json(address, tags, "addr:city", "city");
		tag2json(address, tags, "addr:suburb", "suburb");
		tag2json(address, tags, "addr:postcode", "postcode");
		tag2json(address, tags, "addr:place", "place");
		tag2json(address, tags, "addr:housename", "housename");

		if (geom) {
			extend_city(address, geom);
			extend_postcode(address, geom);
		}

		if (t_errors)
			checkerror(address, geom);

		j["addresses"].push_back(address);
	}

	void way(osmium::Way& way) {

		if (!isaddress(way.tags()))
			return;

		OGRGeometry	*geom=nullptr;

		json		address;
		address["source"]="way";
		address["id"]=std::to_string(way.id());

		try {
			geom=m_factory.create_linestring(way).release();

			geom->Centroid(&point);
			address["lat"]=std::to_string(point.getY());
			address["lon"]=std::to_string(point.getX());

			parseaddr(address, &point, way.tags());

                } catch (gdalcpp::gdal_error) {
                        std::cerr << "gdal_error while creating feature wayid " << way.id()<< std::endl;
                } catch (osmium::invalid_location) {
                        std::cerr << "invalid location wayid " << way.id() << std::endl;
                } catch (osmium::geometry_error) {
                        std::cerr << "geometry error wayid " << way.id() << std::endl;
                }

		if (geom)
			free(geom);
	}

	void node(const osmium::Node& node) {
		if (!isaddress(node.tags()))
			return;

		json			address;

		address["source"]="node";
		address["id"]=std::to_string(node.id());

		osmium::Location location=node.location();
		point.setX(location.lon());
		point.setY(location.lat());

		address["lat"]=std::to_string(location.lat());
		address["lon"]=std::to_string(location.lon());

		parseaddr(address, &point, node.tags());
	}

	void area(const osmium::Area& area) {
		if (!isaddress(area.tags()))
			return;

		json		address;

		address["source"]="relation";
		address["id"]=std::to_string(area.id());

		parseaddr(address, nullptr, area.tags());
	}

	void dump(void ) {
		std::cout << j << std::endl;;
	}
};
