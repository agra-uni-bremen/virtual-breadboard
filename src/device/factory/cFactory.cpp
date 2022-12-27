#include "cFactory.h"

#include "errors.h"

std::list<DeviceClass> CFactory::getAvailableDevices() {
	std::list<DeviceClass> devices;
	for(const auto& [classname, creator] : m_deviceCreators) {
		devices.push_back(classname);
	}
	return devices;
}

bool CFactory::deviceExists(const DeviceClass& classname) {
	return m_deviceCreators.find(classname) != m_deviceCreators.end();
}

std::unique_ptr<CDevice> CFactory::instantiateDevice(DeviceID id, const DeviceClass& classname) {
	if(!deviceExists(classname)) {
		throw (device_not_found_error(classname));
	}
	else {
		return m_deviceCreators.find(classname)->second(id);
	}
}

CFactory& getCFactory() {
	static CFactory CF;
	return CF;
}
