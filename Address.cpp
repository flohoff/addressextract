#include "Address.h"

#include <iostream>

namespace Address {

void Object::tag_add(const struct Address::Tag::Info& taginfo, const char *value) {
	tags.push_back(Address::Tag::Object(taginfo, value));
}

void Object::tag_add_name(const char *tag, const char *value) {
	for(auto& ti : Tag::InfoList) {
		if (tag != ti.tag)
			continue;
		tag_add(ti, value);
		break;
	}
}

Tag::Object* Object::tag_get_by_type(Address::Tag::TagType_t type) {
	for(auto &t : tags) {
		if (t.type() == type) {
			return &t;
		}
	}
	return nullptr;
}

Tag::Object* Object::tag_get(const std::string tag) {
	for(auto &t : tags) {
		if (t.info.tag == tag) {
			return &t;
		}
	}
	return nullptr;
}


bool Object::has_tag_type(Address::Tag::TagType_t type){
	for(auto &t : tags) {
		if (t.type() == type) {
			return true;
		}
	}
	return false;
}

int Object::tag_num(void ) {
	return tags.size();
}

const char *Object::source_string(void ) {
	return Address::SourceStringMap.at(source);
}

void Object::error_add(const std::string error) {
	errors.push_back(error);
}

void Object::parse_from_tags(const osmium::TagList& tags) {
	for(auto &addrtaginfo : Address::Tag::InfoList) {
		const char *value=tags.get_value_by_key(addrtaginfo.tag.c_str(), nullptr);
		if (!value)
			continue;
		tag_add(addrtaginfo, value);
	}
}

}; // namespace Address
