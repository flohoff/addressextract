#include "Address.h"

#include <ostream>

#include <nlohmann/json.hpp>
#include <optional>

namespace Address {

void Object::tag_add(const struct Address::Tag::Info& taginfo, const char *value) {
	tags.emplace_back(taginfo, value);
}

void Object::tag_add_name(const char *tag, const char *value) {
	for(auto& ti : Tag::InfoList) {
		if (std::string_view(tag) != ti.tag)
			continue;
		tag_add(ti, value);
		break;
	}
}

std::optional<Tag::Object> Object::tag_get_by_type(Address::Tag::TagType_t type, Address::Tag::PrefixType_t pfx) {
	for(auto &t : tags) {
		if (t.type() == type && t.pfx() == pfx) {
			return t;
		}
	}
	return std::nullopt;
}

Tag::Object* Object::tag_get(const std::string_view tag) {
	for(auto &t : tags) {
		if (t.info.tag == tag) {
			return &t;
		}
	}
	return nullptr;
}


bool Object::has_tag_type(Address::Tag::TagType_t type){
	for(auto &t : tags) {
		if (t.type() == type && t.pfx() != Address::Tag::PFX_GEOM) {
			return true;
		}
	}
	return false;
}

int Object::tag_num(void ) {
	return tags.size();
}

const char *Object::source_string(void ) const {
	return Address::SourceStringMap.at(source);
}

void Object::error_add(const std::string& error) {
	errors.emplace_back(error);
}

void Object::parse_from_tags(const osmium::TagList& tags) {
	for(auto &addrtaginfo : Address::Tag::InfoList) {
		const char *value=tags.get_value_by_key(addrtaginfo.tag, nullptr);
		if (!value)
			continue;
		tag_add(addrtaginfo, value);
	}
}

std::ostream& operator<<(std::ostream& os, const Object& a) {
	nlohmann::json jaddr;

	jaddr["id"]=a.osmobjid;
	jaddr["source"]=a.source_string();

	jaddr["lat"]=a.lat;
	jaddr["lon"]=a.lon;

	if (a.bbox[0]) {
		nlohmann::json bbox;
		for(int i=0;i<=3;i++)
			bbox.push_back(a.bbox[i]);
		jaddr["bbox"]=bbox;
	}

	for(auto& t : a.tags)
		jaddr[t.info.tagshort]=t.value;

	if (a.errors.size() > 0) {
		nlohmann::json errors;
		for(auto& errorstring : a.errors) {
			errors.push_back(errorstring);
		}
		jaddr["errors"]=errors;
	}

	os << jaddr;

	return os;
}

}; // namespace Address
