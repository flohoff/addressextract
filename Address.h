#ifndef ADDRESS_H
#define ADDRESS_H

#include <osmium/tags/tags_filter.hpp>

#include <iosfwd>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <optional>

namespace Address {
	namespace Tag {
		typedef uint16_t	TagType_t;
		typedef uint8_t		PrefixType_t;

		enum PREFIX {
			PFX_GEOM=1<<0,
			PFX_ADDR=1<<1
		};

		inline const std::map<PrefixType_t,std::string_view> PrefixMap = {
			{ PFX_GEOM, "geom" },
			{ PFX_ADDR, "addr" }
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
			const char		*tag;
			const char		*tagshort;
			PrefixType_t		tagpfxid;
			TagType_t		tagtypeid;
		};

		inline const std::map<TagType_t, std::string_view> TagTypeMap = {
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

		inline const std::vector<struct Info> InfoList= {
			{ "addr:city",		"city",		PFX_ADDR,	TYPE_CITY },
			{ "addr:suburb",	"suburb",	PFX_ADDR,	TYPE_SUBURB },
			{ "addr:place",		"place",	PFX_ADDR,	TYPE_PLACE },
			{ "addr:postcode",	"postcode",	PFX_ADDR,	TYPE_POSTCODE },
			{ "addr:street",	"street",	PFX_ADDR,	TYPE_STREET },
			{ "addr:housenumber",	"housenumber",	PFX_ADDR,	TYPE_HOUSENUMBER },
			{ "addr:housename",	"housename",	PFX_ADDR,	TYPE_HOUSENAME },
			{ "addr:country",	"country",	PFX_ADDR,	TYPE_COUNTRY },
			{ "addr:county",	"county",	PFX_ADDR,	TYPE_COUNTY },

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

	inline const std::map<int, const char *> SourceStringMap={
		{ SourceRelation, "relation" },
		{ SourceWay, "way" },
		{ SourceNode, "node" }
	};

	class Object {
		public:
			std::vector<Tag::Object>	tags;
			std::vector<std::string>	errors;
			double				lat,lon;
			double				bbox[4];
			SourceType_t			source;
			uint64_t			osmobjid;
		public:
			void tag_add(const struct Address::Tag::Info& taginfo, const char *value);
			void tag_add_name(const char *tag, const char *value);
			Tag::Object* tag_get(const std::string_view tag);
			void error_add(const std::string& error);
			bool has_tag_type(Address::Tag::TagType_t type);
			int tag_num(void );
			std::optional<Tag::Object> tag_get_by_type(Address::Tag::TagType_t type, Address::Tag::PrefixType_t pfx);
			const char *source_string(void ) const;
			void parse_from_tags(const osmium::TagList& tags);
	};

	/* Json serializer - one line record per address */
	std::ostream& operator<<(std::ostream& os, const Object& a);

}; // namespace Address
#endif
