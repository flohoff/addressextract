
#include <osmium/tags/tags_filter.hpp>
#include <osmium/handler.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/area.hpp>

#include <regex>
#include <string>
#include <iostream>

#include "AreaIndex.hpp"
#include "Boundary.hpp"
#include "PostCode.hpp"
#include "Building.hpp"

#include "Address.h"

static bool compare_admin_level(Boundary *a, Boundary *b) {
	if (a->cache > b->cache) return true;
	if (a->cache < b->cache) return false;

	if (a->admin_level > b->admin_level) return true;
	if (a->admin_level < b->admin_level) return false;

	return false;
};

static bool postcode_compare(PostCode *a, PostCode *b) {
	return(a->cache > b->cache);
}

class AddressHandler : public osmium::handler::Handler {
	osmium::geom::OGRFactory<>      m_factory{};
	AreaIndex<Boundary>&		boundaryindex;
	AreaIndex<PostCode>&		postcodeindex;
	AreaIndex<Building>&		buildingindex;
	std::vector<Address::Object>	*AddressList;
	bool				t_errors,t_missing,t_nocache;
	std::regex			housenumber_regex,
					street_regex,
					postcode_regex,
					housename_regex,
					country_regex;
	OGRPoint			point;
	OGREnvelope			envelope;
public:
	AddressHandler(AreaIndex<Boundary>& bidx, AreaIndex<PostCode>& pidx, AreaIndex<Building>& buidx,
			bool errors, bool missing, bool nocache, std::string postcoderegexstring) :
		boundaryindex(bidx), postcodeindex(pidx), buildingindex(buidx),
			t_errors(errors), t_missing(missing), t_nocache(nocache) {

		country_regex="^[A-Z][A-Z]$";
		housenumber_regex="^ |,|;| $|[0-9] [a-zA-Z]";
		housename_regex="^ *[0-9]* *$|GmbH|e\\. *V\\.|Sparkasse|[sS]tr\\.|[Ss]traße|http[s]*://";
		street_regex="^ | $|[,:;_#+\"/]|Str\\.$|str\\.$|\\t|\\.$";
		postcode_regex=postcoderegexstring;

		AddressList=new std::vector<Address::Object>;
	}

	bool isaddress(const osmium::TagList& tags) {
		if (tags.empty())
			return false;

		for(auto &taginfo : Address::Tag::InfoList) {
			if (tags.has_key(taginfo.tag.c_str()))
				return true;
		}
		return false;
	}

	void extend_city_boundary(Address::Object& address, Boundary *b) {
		switch(b->admin_level) {
			case(8):
				address.tag_add_name("geom:city", b->name.c_str());
				break;
			case(6):
				if (b->is_county())
					address.tag_add_name("geom:county", b->name.c_str());
				else
					address.tag_add_name("geom:city", b->name.c_str());
				break;
			case(9):
			case(10):
				address.tag_add_name("geom:suburb", b->name.c_str());
				break;
		}
	}

	void extend_city_boundary_up(Address::Object& address, Boundary *b) {
		while(b->up) {
			b=b->up;
			extend_city_boundary(address, b);
		}
	}

	bool extend_city_boundary_check_intersect(Address::Object& address, Boundary *b, OGRGeometry *geom) {
		boundaryindex.compared++;
		if (geom->Intersects(b->geometry)) {
			extend_city_boundary(address, b);
			return true;
		}
		return false;
	}

	bool extend_city_boundary_check(Address::Object& address, Boundary *b, OGRGeometry *geom) {
		if (b->cache) {
			if (b->cachewithin) {
				boundaryindex.cachehitpos++;
				extend_city_boundary(address, b->parent);
				return true;
			}

			if (!b->cachewithin && !b->cacheintersects) {
				boundaryindex.cachehitneg++;
				return false;
			}
			// Intersects
		}
		return extend_city_boundary_check_intersect(address, b, geom);
	}
#if 0
	void extend_city_hierarhy(Address::Object& address, Boundary *b, Boundary *last) {

		// Insert us into last admin level if its
		// bigger - read - a smaller level.
		if (!t_nocache
			&& lastboundary
			&& b->admin_level < lastboundary->admin_level
			&& lastboundary->up == nullptr) {

			std::cerr
				<< "Set cache parent of "
				<< lastboundary->name
				<< " (" << lastboundary->admin_level << ") id (" << osmium::area_id_to_object_id(lastboundary->id) << ")"
				<< " to "
				<< b->name
				<< " (" << b->admin_level << ") id (" << osmium::area_id_to_object_id(b->id) << ") "
				<< std::endl;

			lastboundary->up=b;
		}
	}
#endif
	void extend_city(Address::Object& address, OGRGeometry *geom) {
		static std::vector<Boundary*>	boundarylist;

		if (t_missing && address.has_tag_type(Address::Tag::TYPE_CITY))
			return;

		boundarylist.clear();
		boundaryindex.findoverlapping_geom(geom, &boundarylist);
		std::sort(boundarylist.begin(), boundarylist.end(), compare_admin_level);

		boundaryindex.queries++;
		for(auto b : boundarylist) {
			boundaryindex.returned++;

			if (b->admin_level > 10)
				continue;

			bool hit=extend_city_boundary_check(address, b, geom);

			if (!t_nocache && hit) {
				extend_city_boundary_up(address, b);
				break;
			}
		}
	}

	void extend_postcode(Address::Object& address, OGRGeometry *geom) {
		static std::vector<PostCode*>	postcodelist;

		// If we only want to add missing information
		if (t_missing  && address.has_tag_type(Address::Tag::TYPE_POSTCODE))
			return;

		postcodelist.clear();
		postcodeindex.findoverlapping_geom(geom, &postcodelist);
		postcodeindex.queries++;
		std::sort(postcodelist.begin(), postcodelist.end(), postcode_compare);

		for(auto i : postcodelist) {
			postcodeindex.returned++;

			if (i->cache) {
				if (i->cachewithin) {
					address.tag_add_name("geom:postcode", i->parent->postcode.c_str());
					postcodeindex.cachehitpos++;
					break;
				}
				/* Would be very strange */
				if (!i->cacheintersects && !i->cachewithin) {
					postcodeindex.cachehitneg++;
					continue;
				}
			}

			postcodeindex.compared+=2;
			if (geom->Within(i->geometry)
				|| geom->Overlaps(i->geometry)) {

				address.tag_add_name("geom:postcode", i->postcode.c_str());
				break;
			}
		}
	}

	int popcount(unsigned x) noexcept {
		unsigned num=0;
		for (; x; ++num, x &= (x - 1));
		return num;
	}

	std::string mask_to_prefixlist(Address::Tag::PrefixType_t mask) {
		std::string	prefixes;
		for(auto &pfx : Address::Tag::PrefixMap) {
			if (mask & pfx.first)
				prefixes+=pfx.second + " ";
		}

		/* Remove trailing whitespace */
		if (prefixes.length())
			prefixes.pop_back();

		return prefixes;
	}

	void checkerror_prefixes(Address::Object& address) {
		/* Collect bitmask */
		std::map<Address::Tag::TagType_t,Address::Tag::PrefixType_t>	smap;
		Address::Tag::PrefixType_t	mask=0;

		for(auto &tag : address.tags) {
			/* Dont collect internals */
			if (tag.info.tagpfxid == Address::Tag::PFX_GEOM)
				continue;

			smap[tag.type()]|=tag.info.tagpfxid;
			mask|=tag.info.tagpfxid;
		}

		if (popcount(mask) > 1) {
			std::string error="Tags from multiple prefixes: " + mask_to_prefixlist(mask);
			address.error_add(error);
		}

		for(auto const& el : smap) {
			if (popcount(el.second) == 1)
				continue;

			std::string error="Tag "
				+ Address::Tag::TagTypeMap.at(el.first)
				+ " in multiple prefixes: "
				+ mask_to_prefixlist(el.second);

			address.error_add(error);
		}
	}

	void compare_enclosed_in(Address::Object& address, Address::Object& enclosed) {
		for(auto &atag : address.tags) {
			// We dont compare generated tags to enclosing
			if (atag.info.tagpfxid == Address::Tag::PFX_GEOM)
				continue;
			for(auto &etag : enclosed.tags) {
				if (atag.info.tagtypeid == etag.info.tagtypeid) {
					if (atag.value != etag.value) {
						std::string error="Enclosing object tag mismatch "
							+ atag.info.tag
							+ " with value "
							+ atag.value
							+ " to tag "
							+ etag.info.tag
							+ " with value "
							+ etag.value;
						address.error_add(error);
					}
				}
				// FIXME Do we want to require enclosure have all tags?
			}
		}
	}

	void checkerror_geom(Address::Object& address, OGRGeometry *geom) {
		if (address.source != Address::SourceNode)
			return;

		static std::vector<Building*>	list;
		list.clear();
		buildingindex.findoverlapping_geom(geom, &list);
		buildingindex.queries++;
		for(auto &i : list) {
			buildingindex.returned++;
			buildingindex.compared++;
			if (!geom->Within(i->geometry))
				continue;

			compare_enclosed_in(address, i->address);

			break;
		}
	}

	void checkerror(Address::Object& address, OGRGeometry *geom) {
		if (!address.has_tag_type(Address::Tag::TYPE_HOUSENUMBER))
			address.error_add("No housenumber");
		if (!address.has_tag_type(Address::Tag::TYPE_CITY))
			address.error_add("No city");
		if (!address.has_tag_type(Address::Tag::TYPE_POSTCODE))
			address.error_add("No postcode");
		if (!address.has_tag_type(Address::Tag::TYPE_STREET) && !address.has_tag_type(Address::Tag::TYPE_PLACE))
			address.error_add("No addr:street or addr:place");

		checkerror_prefixes(address);

		if (address.has_tag_type(Address::Tag::TYPE_COUNTRY)) {
			const Address::Tag::Object *tag=address.tag_get_by_type(Address::Tag::TYPE_COUNTRY);

			if (!std::regex_search(tag->value, country_regex)) {
				address.error_add("Country format issues");
			}
		}

		if (address.has_tag_type(Address::Tag::TYPE_STREET)) {
			const Address::Tag::Object *tag=address.tag_get_by_type(Address::Tag::TYPE_STREET);

			if (std::regex_search(tag->value, street_regex)) {
				address.error_add("Street format issues");
			}
		}

		if (address.has_tag_type(Address::Tag::TYPE_POSTCODE)) {
			const Address::Tag::Object *tag=address.tag_get_by_type(Address::Tag::TYPE_POSTCODE);

			if (!std::regex_search(tag->value, postcode_regex)) {
				address.error_add("Postcode format issues");
			}
		}

		// FIXME - Untested
		const Address::Tag::Object *geompostcode=address.tag_get("geom:postcode");
		if (geompostcode) {
			for(auto& tag : address.tags) {
				if (tag.pfx() == Address::Tag::PFX_GEOM)
					continue;
				if (tag.type() != Address::Tag::TYPE_POSTCODE)
					continue;

				if (tag.value == geompostcode->value)
					continue;

				std::string error="Postcode mismatch "
					+ tag.value
					+ " vs. "
					+ geompostcode->value;

				address.error_add(error);
			}
		}

		const Address::Tag::Object *geomcity=address.tag_get("geom:city");
		if (geomcity) {
			for(auto& tag : address.tags) {
				if (tag.pfx() == Address::Tag::PFX_GEOM)
					continue;
				if (tag.type() != Address::Tag::TYPE_CITY)
					continue;

				if (tag.value == geomcity->value)
					continue;

				std::string error="City mismatch "
					+ tag.value
					+ " vs. "
					+ geomcity->value;

				address.error_add(error);
			}
		}

		if (address.has_tag_type(Address::Tag::TYPE_HOUSENUMBER)) {
			for(auto& tag : address.tags) {
				if (tag.type() != Address::Tag::TYPE_HOUSENUMBER)
					continue;

				if (std::regex_search(tag.value, housenumber_regex)) {
					address.error_add("Housenumber format issues " + tag.value);
				}
			}
		}

		checkerror_geom(address, geom);
	}

	void parseaddr(Address::Object& address, OGRGeometry *geom, const osmium::TagList& tags) {

		address.parse_from_tags(tags);

		if (geom) {
			extend_city(address, geom);
			extend_postcode(address, geom);
		}

		if (t_errors)
			checkerror(address, geom);
	}

	void way(osmium::Way& way) {
		if (!isaddress(way.tags()))
			return;

		OGRGeometry	*geom=nullptr;

		Address::Object	address;
		address.source=Address::SourceWay;
		address.osmobjid=way.id();

		try {
			geom=m_factory.create_linestring(way).release();
			const osmium::Box envelope = way.envelope();

			address.bbox[0]=envelope.bottom_left().lon();
			address.bbox[1]=envelope.bottom_left().lat();
			address.bbox[2]=envelope.top_right().lon();
			address.bbox[3]=envelope.top_right().lat();

			geom->Centroid(&point);
			address.lat=point.getY();
			address.lon=point.getX();

			parseaddr(address, &point, way.tags());

			AddressList->push_back(address);

                } catch (const gdalcpp::gdal_error& e) {
                        std::cerr << "gdal_error while creating feature wayid " << way.id()<< std::endl;
                } catch (const osmium::invalid_location& e) {
                        std::cerr << "invalid location wayid " << way.id() << std::endl;
                } catch (const osmium::geometry_error& e) {
                        std::cerr << "geometry error wayid " << way.id() << std::endl;
                }

		if (geom)
			free(geom);
	}

	void node(const osmium::Node& node) {
		Address::Object	address;

		if (!isaddress(node.tags()))
			return;

		address.source=Address::SourceNode;
		address.osmobjid=node.id();

		osmium::Location location=node.location();
		point.setX(location.lon());
		point.setY(location.lat());

		address.lat=location.lat();
		address.lon=location.lon();

		address.bbox[0]=location.lon();
		address.bbox[1]=location.lat();
		address.bbox[2]=location.lon();
		address.bbox[3]=location.lat();

		parseaddr(address, &point, node.tags());

		AddressList->push_back(address);
	}

	void area(const osmium::Area& area) {
		Address::Object	address;

		if (!isaddress(area.tags()))
			return;

		address.source=Address::SourceRelation;
		address.osmobjid=area.id();

		parseaddr(address, nullptr, area.tags());

		AddressList->push_back(address);
	}

	std::vector<Address::Object> *addresslist(void ) {
		return AddressList;
	}
};
