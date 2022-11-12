#include "cDevice.h"

#include <stdexcept>
#include <iostream>

CDevice::CDevice(DeviceID id) : Device(id) {}
CDevice::~CDevice() {}

Device::Layout CDevice::getLayout() { return layout; }

/* PIN Interface */

CDevice::PIN_Interface_C::PIN_Interface_C(CDevice* device) : device(device) {}
CDevice::PIN_Interface_C::~PIN_Interface_C() {}

PinLayout CDevice::PIN_Interface_C::getPinLayout() {
	return pinLayout;
}

void CDevice::PIN_Interface_C::setPin(PinNumber, gpio::Tristate) {
	std::cerr << "[CDevice] Warning: setPin was not implemented "
			"for device " << device->getClass() << "." << std::endl;
}

gpio::Tristate CDevice::PIN_Interface_C::getPin(PinNumber) {
	std::cerr << "[CDevice] Warning: getPin was not implemented "
			"for device " << device->getClass() << "." << std::endl;
	return gpio::Tristate::LOW;
}

/* SPI Interface */

CDevice::SPI_Interface_C::SPI_Interface_C(CDevice* device) : device(device) {}
CDevice::SPI_Interface_C::~SPI_Interface_C() {}

gpio::SPI_Response CDevice::SPI_Interface_C::send(gpio::SPI_Command) {
	std::cerr << "[CDevice] Warning: SPI::send was not implemented "
			"for device " << device->getClass() << "." << std::endl;
	return 0;
}

/* Config Interface */

CDevice::Config_Interface_C::Config_Interface_C(CDevice* device) : device(device) {}
CDevice::Config_Interface_C::~Config_Interface_C() {}

Config CDevice::Config_Interface_C::getConfig() {
	return config;
}

bool CDevice::Config_Interface_C::setConfig(Config conf) {
	config = conf;
	return true;
}

/* Input interface */

CDevice::Input_Interface_C::Input_Interface_C(CDevice* device) : device(device) {}
CDevice::Input_Interface_C::~Input_Interface_C() {}

void CDevice::Input_Interface_C::onClick(bool) {
	std::cerr << "[CDevice] Warning: onClick was not implemented "
			"for device " << device->getClass() << "." << std::endl;
}

void CDevice::Input_Interface_C::onKeypress(Key, bool) {
	std::cerr << "[CDevice] Warning: onKeypress was not implemented "
			"for device " << device->getClass() << "." << std::endl;
}
