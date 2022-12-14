/*
 * scriptloader.cpp
 *
 *  Created on: Apr 29, 2022
 *      Author: pp
 */
#include "luaFactory.hpp"
#include "errors.h"
extern "C"
{
#if __has_include(<lua5.3/lua.h>)
	#include <lua5.3/lua.h>
	#include <lua5.3/lualib.h>
	#include <lua5.3/lauxlib.h>
#elif  __has_include(<lua.h>)
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
#else
	#error("No lua libraries found")
#endif
}
#include <LuaBridge/LuaBridge.h>

#include <QDirIterator>

#include <filesystem>
#include <exception>
#include <memory> // unique_ptr


using std::cout;
using std::cerr;
using std::endl;
using std::runtime_error;
using std::unique_ptr;

using luabridge::LuaRef;
using luabridge::LuaResult;
using luabridge::LuaException;
using luabridge::getGlobal;

using std::filesystem::path;
using std::filesystem::directory_iterator;


static lua_State* L;

/**
 * @return [false, ...] if invalid
 */
LuaRef loadScriptFromFile(lua_State* L, path p) {
	LuaRef scriptloader = getGlobal(L, "scriptloader_file");
	try {
		LuaResult r = scriptloader(p.c_str());
		if(!r.wasOk()) {
			cerr << p << ": " << r.errorMessage() << endl;
			return LuaRef(L);
		}
		if(r.size() != 1) {
			//cerr << p << " failed." << endl;
			return LuaRef(L);
		}
		if(!r[0].isTable()) {
			cerr << p << ": " << r[0] << endl;
			return LuaRef(L);
		}
		return r[0];

	} catch(LuaException& e)	{
		cerr << "serious shit got down in file " << p << endl;
		cerr << e.what() << endl;
		return LuaRef(L);
	}
}

/**
 * @return [false, ...] if invalid
 */
LuaRef loadScriptFromString(lua_State* L, std::string p, std::string name = "external script") {
	LuaRef scriptloader = getGlobal(L, "scriptloader_string");
	try {
		LuaResult r = scriptloader(p, name);
		if(!r.wasOk()) {
			cerr << p << ": " << r.errorMessage() << endl;
			return LuaRef(L);
		}
		if(r.size() != 1) {
			//cerr << p << " failed." << endl;
			return LuaRef(L);
		}
		if(!r[0].isTable()) {
			cerr << p << ": " << r[0] << endl;
			return LuaRef(L);
		}
		return r[0];

	} catch(LuaException& e)	{
		cerr << "serious shit got down in string \n" << p << endl;
		cerr << e.what() << endl;
		return LuaRef(L);
	}
}

bool isScriptValidDevice(LuaRef& chunk, std::string name = "") {
	if(chunk.isNil()) {
		cerr << "[lua]\tScript " << name << " could not be loaded" << endl;
		return false;
	}
	if(chunk["classname"].isNil() || !chunk["classname"].isString()) {
		cerr << "[lua]\tScript " << name << " does not contain a (valid) classname" << endl;
		return false;
	}
	return true;
}

LuaFactory::LuaFactory(){
	L = luaL_newstate();
	luaL_openlibs(L);

	QFile loader(m_scriptloader.c_str());
	if (!loader.open(QIODevice::ReadOnly)) {
		throw(runtime_error("Could not open scriptloader at " + m_scriptloader));
	}
	QByteArray loader_content = loader.readAll();

	if( luaL_dostring( L, loader_content) )
	{
		cerr << "Error loading loadscript:\n" <<
				 lua_tostring( L, lua_gettop( L ) ) << endl;
		lua_pop( L, 1 );
		throw(runtime_error("Loadscript not valid"));
	}

	//cout << "Scanning built-in devices..." << endl;

	scanDir(m_builtin_scripts);
}

void LuaFactory::scanDir(std::string dir, bool overwrite_existing) {
	QDirIterator it(dir.c_str(),
			QStringList() << "*.lua",
			QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		it.next();
		//cout << "\t" << it.fileName().toStdString() << endl;
		QFile script_file(it.filePath());
		if (!script_file.open(QIODevice::ReadOnly)) {
			throw(runtime_error("Could not open file " + it.fileName().toStdString()));
		}
		QByteArray script = script_file.readAll();
		const auto filepath = it.filePath().toStdString();

		auto chunk = loadScriptFromString(L, script.toStdString(), it.fileName().toStdString());
		if(!isScriptValidDevice(chunk, filepath))
			continue;
		const auto maybe_classname = chunk["classname"].cast<std::string>();
		if(!maybe_classname) {
			cerr << "[lua] Warn: script does not state a classname at '" << filepath << "'" << endl;
			cerr << "\tError code is: " << maybe_classname.message() << endl;
			continue;
		}
		const auto classname = maybe_classname.value();
		if(m_available_devices.find(classname) != m_available_devices.end()) {
			if(!overwrite_existing) {
				cerr << "[lua] Warn: '" << classname << "' from '" << filepath << "' was ignored as it "
						"would overwrite device from '" << m_available_devices.at(classname) << "'" << endl;
				continue;
			} else {
				cerr << "[lua] Warn: '" << classname << "' from '" << filepath << "' "
						"overwrites device from '" << m_available_devices.at(classname) << "'" << endl;
			}
			continue;
		}
		m_available_devices.emplace(classname, filepath);
	}
}

std::list<DeviceClass> LuaFactory::getAvailableDevices() {
	std::list<DeviceClass> devices;
	for(const auto& [name, file] : m_available_devices) {
		devices.push_back(name);
	}
	return devices;
}

bool LuaFactory::deviceExists(const DeviceClass& classname) {
	return m_available_devices.find(classname) != m_available_devices.end();
}

unique_ptr<LuaDevice> LuaFactory::instantiateDevice(const DeviceID& id, const DeviceClass& classname) {
	if(!deviceExists(classname)) {
		throw (device_not_found_error(classname));
	}
	QFile script_file(m_available_devices[classname].c_str());
	if (!script_file.open(QIODevice::ReadOnly)) {
		throw(runtime_error("Could not open file " + m_available_devices[classname]));
	}
	QByteArray script = script_file.readAll();

	return std::make_unique<LuaDevice>(id, loadScriptFromString(L, script.toStdString(), classname), L);
}

