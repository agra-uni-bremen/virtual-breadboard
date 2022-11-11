#include "factory.h"

#include "errors.h"

void Factory::scanAdditionalDir(std::string dir, bool overwrite_existing) {
	lua_factory.scanDir(dir, overwrite_existing);
}

std::list<DeviceClass> Factory::getAvailableDevices() {
	std::list<DeviceClass> devices = c_factory.getAvailableDevices();
	devices.merge(lua_factory.getAvailableDevices());
	return devices;
}

std::list<DeviceClass> Factory::getLUADevices() {
       return lua_factory.getAvailableDevices();
}

std::list<DeviceClass> Factory::getCDevices() {
       return c_factory.getAvailableDevices();
}

bool Factory::deviceExists(DeviceClass classname) {
	return lua_factory.deviceExists(classname) || c_factory.deviceExists(classname);
}

std::unique_ptr<Device> Factory::instantiateDevice(DeviceID id, DeviceClass classname) {
	if(c_factory.deviceExists(classname))
		return c_factory.instantiateDevice(id, classname);
	else if (lua_factory.deviceExists(classname))
		return lua_factory.instantiateDevice(id, classname);
	else throw (device_not_found_error(classname));
}
