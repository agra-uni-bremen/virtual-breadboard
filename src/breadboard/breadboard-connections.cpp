#include "breadboard.h"

using namespace std;

/* Register */

void Breadboard::registerPin(bool synchronous, gpio::PinNumber device_pin, gpio::PinNumber global,
		std::string name, Device *device) {
	if(!device->pin) {
		cerr << "[Breadboard] Attempting to add pin connection for device '" << device->getClass() <<
				"', but device does not implement PIN interface." << endl;
		return;
	}
	const PinLayout layout = device->pin->getPinLayout();
	if(layout.find(device_pin) == layout.end()) {
		cerr << "[Breadboard] Attempting to add pin '" << (int)device_pin << "' for device " <<
				device->getClass() << " that is not offered by device" << endl;
		return;
	}
	const PinDesc& desc = layout.at(device_pin);
	if(synchronous) {
		if(desc.dir != PinDesc::Dir::input) {
			cerr << "[Breadboard] Attempting to add pin '" << (int)device_pin << "' as synchronous for device " <<
					device->getClass() << ", but device labels pin not as input."
					" This is not supported for inout-pins and unnecessary for output pins." << endl;
			return;
		}
		pin_channels.emplace(device->getID(), PIN_IOF_Request{translatePinToGpioOffs(global),
			global, device_pin,
			[this, device, device_pin](gpio::Tristate pin) {
				lua_access.lock();
				device->pin->setPin(device_pin, pin);
				lua_access.unlock();
			}
		});
	}
	else {
		PinMapping mapping = PinMapping{translatePinToGpioOffs(global), global, device_pin, name, device};
		if(desc.dir == PinDesc::Dir::input
				|| desc.dir == PinDesc::Dir::inout) {
			reading_connections.push_back(mapping);
		}
		if(desc.dir == PinDesc::Dir::output
				|| desc.dir == PinDesc::Dir::inout) {
			writing_connections.push_back(mapping);
		}
	}
}

void Breadboard::registerSPI(gpio::PinNumber global, bool noresponse, Device *device) {
	if(!device->spi) {
		cerr << "[Breadboard] Attempting to add SPI connection for device '" << device->getClass() <<
				"', but device does not implement SPI interface." << endl;
		return;
	}
	spi_channels.emplace(device->getID(), SPI_IOF_Request{translatePinToGpioOffs(global),
		global, noresponse,
		[this, device](gpio::SPI_Command cmd){
			lua_access.lock();
			const gpio::SPI_Response ret = device->spi->send(cmd);
			lua_access.unlock();
			return ret;
		}
	}
	);
}

/* Remove */

void Breadboard::removeDevice(DeviceID id) {
	vector<gpio::PinNumber> iofs;
	auto pin = pin_channels.find(id);
	if(pin!=pin_channels.end()) {
		iofs.push_back(pin->second.gpio_offs);
	}
	auto spi = spi_channels.find(id);
	if(spi!=spi_channels.end()) {
		iofs.push_back(spi->second.gpio_offs);
	}
	emit(closeDeviceIOFs(iofs, id));
}

void Breadboard::removeDeviceObjects(DeviceID id) {
	pin_channels.erase(id);
	spi_channels.erase(id);
	writing_connections.remove_if([id](PinMapping map){return map.dev->getID() == id;});
	reading_connections.remove_if([id](PinMapping map){return map.dev->getID() == id;});
	devices.erase(id);
}

void Breadboard::clear() {
	vector<gpio::PinNumber> iofs;
	for(auto const& [id,spi] : spi_channels) {
		iofs.push_back(spi.gpio_offs);
	}
	for(auto const& [id,pin] : pin_channels) {
		iofs.push_back(pin.gpio_offs);
	}
	emit(closeAllIOFs(iofs));

	setMinimumSize(DEFAULT_SIZE);
	setBackground(DEFAULT_PATH);
}

void Breadboard::clearConnections() {
	spi_channels.clear();
	pin_channels.clear();
	writing_connections.clear();
	reading_connections.clear();
	devices.clear();
}

/* Update */

void Breadboard::timerUpdate(gpio::State state) {
	lua_access.lock();
	for (PinMapping& c : reading_connections) {
		// TODO: Only if pin changed?
		c.dev->pin->setPin(c.device_pin, state.pins[c.gpio_offs] ==
				gpio::Pinstate::HIGH ? gpio::Tristate::HIGH : gpio::Tristate::LOW);
	}

	for (PinMapping& c : writing_connections) {
		emit(setBit(c.gpio_offs, c.dev->pin->getPin(c.device_pin)));
	}
	lua_access.unlock();
}

void Breadboard::connectionUpdate(bool active) {
	if(active) {
		for(const auto& [id, req] : spi_channels) {
			emit(registerIOF_SPI(req.gpio_offs, req.fun, req.noresponse));
		}
		for(const auto& [id, req] : pin_channels) {
			emit(registerIOF_PIN(req.gpio_offs, req.fun));
		}
	}
	// else connection lost
}

void Breadboard::writeDevice(DeviceID id) {
	lua_access.lock();
	for(PinMapping w : writing_connections) {
		if(w.dev->getID() == id) {
			emit(setBit(w.gpio_offs, w.dev->pin->getPin(w.device_pin)));
		}
	}
	lua_access.unlock();
}