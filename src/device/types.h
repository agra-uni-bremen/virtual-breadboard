#pragma once

#include <string>
#include <cstring>
#include <unordered_map>
#include <functional>
#include <set>

// Input Interface

typedef int Key;
typedef std::set<Key> Keys;

//ConfigInterface

struct ConfigElem {
	enum class Type {
		invalid = 0,
		integer,
		boolean,
		string,
	} type;
	union Value {
		int64_t integer;
		bool boolean;
		char* string;
	} value;

	ConfigElem() : type(Type::invalid){};
	ConfigElem(const ConfigElem& e) {
		type = e.type;
		if(type == Type::string) {
			value.string = new char[strlen(e.value.string)+1];
			strcpy(value.string, e.value.string);
		}
		else {
			value = e.value;
		}
	}
	~ConfigElem() {
		if(type == Type::string) {
			delete[] value.string;
		}
	}

	ConfigElem(int64_t val){
		type = Type::integer;
		value.integer = val;
	};
	ConfigElem(bool val) {
		type = Type::boolean;
		value.boolean = val;
	};
	ConfigElem(const char* val){
		type = Type::string;
		value.string = new char[strlen(val)+1];
		strcpy(value.string, val);
	};
};
typedef std::string ConfigDescription;
typedef std::unordered_map<ConfigDescription,ConfigElem> Config;
