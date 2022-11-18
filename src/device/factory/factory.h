#pragma once

#include "cFactory.h"
#include "luaFactory.hpp"


class Factory {
	LuaFactory m_lua_factory;
	CFactory m_c_factory = getCFactory();

public:
	void scanAdditionalDir(std::string dir, bool overwrite_existing = false);
	std::list<DeviceClass> getAvailableDevices();
	std::list<DeviceClass> getLUADevices();
	std::list<DeviceClass> getCDevices();

	bool deviceExists(const DeviceClass& classname);
	std::unique_ptr<Device> instantiateDevice(const DeviceID& id, const DeviceClass& classname);
};
