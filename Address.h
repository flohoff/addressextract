#ifndef ADDRESS_H
#define ADDRESS_H

#include <osmium/tags/tags_filter.hpp>

#include <vector>
#include <string>
#include <map>
#include <list>

namespace Address {
	namespace Tag {
		typedef uint16_t	TagType_t;
		typedef uint8_t		PrefixType_t;

		enum PREFIX {
			PFX_GEOM=1<<0,
			PFX_ADDR=1<<1,
			PFX_OBJECT=1<<2,
			PFX_CONTACT=1<<3
		};

		const std::map<PrefixType_t,std::string> PrefixMap = {
			{ PFX_GEOM, "geom" },
			{ PFX_ADDR, "addr" },
			{ PFX_OBJECT, "object" },
			{ PFX_CONTACT, "contact" }
		};

		enum TYPE {
			TYPE_COUNTRY=1<<0,
			TYPE_COUNTY=1<<1,
			TYPE_CITY=1<<2,
			TYPE_PLACE=1<<3,
			TYPE_SUBURB=1<<4,
			TYPE_POSTCODE=1<<5,
			TYPE_STREET=1<<6,
			TYPE_HOUSENUMBER=1<<7,
			TYPE_HOUSENAME=1<<8
		};

		struct Info {
			std::string	tag;
			std::string	tagshort;
			PrefixType_t	tagpfxid;
			TagType_t	tagtypeid;
		};

		const std::map<TagType_t, std::string> TagTypeMap = {
			{ TYPE_COUNTRY,		"country" },
			{ TYPE_COUNTY,		"county" },
			{ TYPE_CITY,		"city" },
			{ TYPE_PLACE,		"place" },
			{ TYPE_SUBURB,		"suburb" },
			{ TYPE_POSTCODE,	"postcode" },
			{ TYPE_STREET,		"street" },
			{ TYPE_HOUSENUMBER,	"housenumber" },
			{ TYPE_HOUSENAME,	"housename" }
		};

		const std::vector<struct Info> InfoList= {
			{ "addr:city",		"city",		PFX_ADDR,	TYPE_CITY },
			{ "addr:suburb",	"suburb",	PFX_ADDR,	TYPE_SUBURB },
			{ "addr:place",		"place",	PFX_ADDR,	TYPE_PLACE },
			{ "addr:postcode",	"postcode",	PFX_ADDR,	TYPE_POSTCODE },
			{ "addr:street",	"street",	PFX_ADDR,	TYPE_STREET },
			{ "addr:housenumber",	"housenumber",	PFX_ADDR,	TYPE_HOUSENUMBER },
			{ "addr:housename",	"housename",	PFX_ADDR,	TYPE_HOUSENAME },
			{ "addr:country",	"country",	PFX_ADDR,	TYPE_COUNTRY },
			{ "addr:county",	"county",	PFX_ADDR,	TYPE_COUNTY },

			{ "object:city",	"city",		PFX_OBJECT,	TYPE_CITY },
			{ "object:suburb",	"suburb",	PFX_OBJECT,	TYPE_SUBURB },
			{ "object:place",	"place",	PFX_OBJECT,	TYPE_PLACE },
			{ "object:postcode",	"postcode",	PFX_OBJECT,	TYPE_POSTCODE },
			{ "object:street",	"street",	PFX_OBJECT,	TYPE_STREET },
			{ "object:housenumber",	"housenumber",	PFX_OBJECT,	TYPE_HOUSENUMBER },
			{ "object:housename",	"housename",	PFX_OBJECT,	TYPE_HOUSENAME },
			{ "object:country",	"country",	PFX_OBJECT,	TYPE_COUNTRY },
			{ "object:county",	"county",	PFX_OBJECT,	TYPE_COUNTRY },

			/* Internal types */
			{ "geom:city",		"geomcity",	PFX_GEOM,	TYPE_CITY },
			{ "geom:county",	"geomcounty",	PFX_GEOM,	TYPE_COUNTY },
			{ "geom:suburb",	"geomsuburb",	PFX_GEOM,	TYPE_SUBURB },
			{ "geom:postcode",	"geompostcode",	PFX_GEOM,	TYPE_POSTCODE },
		};

		class Object {
			public:
			const struct Info&	info;
			std::string	value;

			Object(const struct Address::Tag::Info& info, const char *value) : info(info), value(value) { };
			TagType_t type(void ) {
				return info.tagtypeid;
			}
			PrefixType_t pfx(void ) {
				return info.tagpfxid;
			}
		};
	}; // namespace Tag

	typedef uint8_t	SourceType_t;

	enum SOURCE {
		SourceRelation,
		SourceWay,
		SourceNode
	};

	const std::map<int, const char *> SourceStringMap={
		{ SourceRelation, "relation" },
		{ SourceWay, "way" },
		{ SourceNode, "node" }
	};

	class Object {
		public:
			std::list<Tag::Object>	tags;
			std::list<std::string>	errors;
			double		lat,lon;
			double		bbox[4];
			SourceType_t	source;
			uint64_t	osmobjid;
		public:
			void tag_add(const struct Address::Tag::Info& taginfo, const char *value);
			void tag_add_name(const char *tag, const char *value);
			Tag::Object* tag_get(const std::string tag);
			void error_add(const std::string error);
			bool has_tag_type(Address::Tag::TagType_t type);
			int tag_num(void );
			Tag::Object* tag_get_by_type(Address::Tag::TagType_t type);
			std::string *tagvalue_get_by_type(Address::Tag::TagType_t type);
			const char *source_string(void );
			void parse_from_tags(const osmium::TagList& tags);
	};
}; // namespace Address
#endif
