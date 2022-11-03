#pragma once

#include <interface/cDevice.h>

class CFactory {
public:
	typedef std::function<std::unique_ptr<CDevice>(DeviceID id)> Creator;

	template <typename Derived>
	bool registerDeviceType() {
		return deviceCreators.insert(std::make_pair(Derived::classname, [](DeviceID id) {return std::make_unique<Derived>(id);})).second;
	}

	std::list<DeviceClass> getAvailableDevices();
	bool deviceExists(DeviceClass classname);
	std::unique_ptr<CDevice> instantiateDevice(DeviceID id, DeviceClass classname);
private:
	std::unordered_map<DeviceClass, Creator> deviceCreators;
};

CFactory& getCFactory();
