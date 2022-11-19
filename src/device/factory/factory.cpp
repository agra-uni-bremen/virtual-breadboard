#include "factory.h"

#include "errors.h"

void Factory::scanAdditionalDir(std::string dir, bool overwrite_existing) {
	m_lua_factory.scanDir(dir, overwrite_existing);
}

std::list<DeviceClass> Factory::getAvailableDevices() {
	std::list<DeviceClass> devices = m_c_factory.getAvailableDevices();
	devices.merge(m_lua_factory.getAvailableDevices());
	return devices;
}

std::list<DeviceClass> Factory::getLUADevices() {
       return m_lua_factory.getAvailableDevices();
}

std::list<DeviceClass> Factory::getCDevices() {
       return m_c_factory.getAvailableDevices();
}

bool Factory::deviceExists(const DeviceClass& classname) {
	return m_lua_factory.deviceExists(classname) || m_c_factory.deviceExists(classname);
}

std::unique_ptr<Device> Factory::instantiateDevice(const DeviceID& id, const DeviceClass& classname) {
	if(m_c_factory.deviceExists(classname))
		return m_c_factory.instantiateDevice(id, classname);
	else if (m_lua_factory.deviceExists(classname))
		return m_lua_factory.instantiateDevice(id, classname);
	else throw (device_not_found_error(classname));
}
