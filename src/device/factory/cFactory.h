#pragma once

#include <interface/cDevice.h>

#include <iostream>


class CFactory {
public:
	typedef std::function<std::unique_ptr<CDevice>(DeviceID id)> Creator;

	template <typename Derived>
	bool registerDeviceType() {
		return m_deviceCreators.insert(std::make_pair(Derived::m_classname, [](DeviceID id) {return std::make_unique<Derived>(id);})).second;
	}

	std::list<DeviceClass> getAvailableDevices();
	bool deviceExists(const DeviceClass& classname);
	std::unique_ptr<CDevice> instantiateDevice(DeviceID id, const DeviceClass& classname);
private:
	std::unordered_map<DeviceClass, Creator> m_deviceCreators;
};

CFactory& getCFactory();
