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
	const std::string builtin_scripts = ":/devices/lua/";
	const std::string scriptloader = ":/src/device/factory/loadscript.lua";

	std::unordered_map<std::string,std::string> available_devices;
public:

	LuaFactory();

	void scanAdditionalDir(std::string dir, bool overwrite_existing = false);
	std::list<DeviceClass> getAvailableDevices();

	bool deviceExists(DeviceClass classname);
	std::unique_ptr<LuaDevice> instantiateDevice(DeviceID id, DeviceClass classname);
};

