/*
 * scriptloader.hpp
 *
 *  Created on: Apr 29, 2022
 *      Author: pp
 */

#pragma once
#include <interface/luaDevice.hpp>


#include <unordered_map>
#include <string>
#include <list>
#include <memory> // unique_ptr

class LuaFactory {
	const std::string m_builtin_scripts = ":/devices/lua/";
	const std::string m_scriptloader = ":/src/device/factory/loadscript.lua";

	std::unordered_map<std::string,std::string> m_available_devices;
public:

	LuaFactory();

	void scanDir(std::string dir, bool overwrite_existing = false);
	std::list<DeviceClass> getAvailableDevices();

	bool deviceExists(const DeviceClass& classname);
	std::unique_ptr<LuaDevice> instantiateDevice(const DeviceID& id, const DeviceClass& classname);
};

