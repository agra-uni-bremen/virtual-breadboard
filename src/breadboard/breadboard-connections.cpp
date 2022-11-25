#include "breadboard.h"

using namespace std;

/* Register */

void Breadboard::registerPin(bool synchronous, Device::PIN_Interface::DevicePin device_pin, gpio::PinNumber global,
                             std::string name, Device *device) { // TODO eigentlich komplett neu
    cout << "Called the method -------------------" << endl;
    if(!device->m_pin) {
        cerr << "[Breadboard] Attempting to add pin connection for device '" << device->getClass() <<
             "', but device does not implement PIN interface." << endl;
        return;
    }
    const Device::PIN_Interface::PinLayout layout = device->m_pin->getPinLayout();
    if(layout.find(device_pin) == layout.end()) {
        cerr << "[Breadboard] Attempting to add pin '" << (int)device_pin << "' for device " <<
             device->getClass() << " that is not offered by device" << endl;
        return;
    }
    const Device::PIN_Interface::PinDesc& desc = layout.at(device_pin);
    if(synchronous) {
        cout << "Arrived here for device " << device->getID() << endl;
        if(desc.dir != Device::PIN_Interface::Dir::input) {
            cerr << "[Breadboard] Attempting to add pin '" << (int)device_pin << "' as synchronous for device " <<
                 device->getClass() << ", but device labels pin not as input."
                                       " This is not supported for inout-pins and unnecessary for output pins." << endl;
            return;
        }
        m_pin_channels.emplace(device->getID(), PIN_IOF_Request{
                .gpio_offs= translatePinToGpioOffs(global),
                .global_pin = global,
                .device_pin = device_pin,
                .fun = [this, device, device_pin](gpio::Tristate pin) {
                    m_lua_access.lock();
                    device->m_pin->setPin(device_pin, pin);
                    m_lua_access.unlock();
                }
        });
    }
    else { // TODO check for already existing row entry
        QPoint deviceRasterPosition = getDeviceAbsolutePosition(desc.row, desc.index);
        QPoint devicePosition = getDistortedPosition(device->getBuffer().offset()+deviceRasterPosition);
        Row row = getRow(devicePosition);
        Index index = getIndex(devicePosition);
        Connection c;
        c.pins.push_back(PinConnection{
                .gpio_offs = translatePinToGpioOffs(global),
                .global_pin = global,
                .name = name,
                .dir = desc.dir,
                .index = index,
        });
        m_connections.emplace(row, c);
    }
}

void Breadboard::registerSPI(gpio::PinNumber global, bool noresponse, Device *device) {
    if(!device->m_spi) {
        cerr << "[Breadboard] Attempting to add SPI connection for device '" << device->getClass() <<
             "', but device does not implement SPI interface." << endl;
        return;
    }
    m_spi_channels.emplace(device->getID(), SPI_IOF_Request{
                                   .gpio_offs = translatePinToGpioOffs(global),
                                   .global_pin = global,
                                   .noresponse = noresponse,
                                   .fun = [this, device](gpio::SPI_Command cmd){
                                       m_lua_access.lock();
                                       const gpio::SPI_Response ret = device->m_spi->send(cmd);
                                       m_lua_access.unlock();
                                       return ret;
                                   }
                           }
    );
}

/* Remove */

void Breadboard::removeDevice(const DeviceID& id) {
    vector<gpio::PinNumber> iofs;
    auto pin = m_pin_channels.find(id);
    if(pin != m_pin_channels.end()) {
        iofs.push_back(pin->second.gpio_offs);
    }
    auto spi = m_spi_channels.find(id);
    if(spi != m_spi_channels.end()) {
        iofs.push_back(spi->second.gpio_offs);
    }
    emit(closeDeviceIOFs(iofs, id));
}

void Breadboard::removeDeviceObjects(const DeviceID& id) {
    m_pin_channels.erase(id);
    m_spi_channels.erase(id);
    m_devices.erase(id);
}

void Breadboard::clear() {
    vector<gpio::PinNumber> iofs;
    for(auto const& [id,spi] : m_spi_channels) {
        iofs.push_back(spi.gpio_offs);
    }
    for(auto const& [id,pin] : m_pin_channels) {
        iofs.push_back(pin.gpio_offs);
    }
    emit(closeAllIOFs(iofs));

    setMinimumSize(DEFAULT_SIZE);
    setBackground(DEFAULT_PATH);
}

void Breadboard::clearConnections() {
    m_spi_channels.clear();
    m_pin_channels.clear();
    for(auto& [row, connection] : m_connections) {
        connection.pins.clear();
    }
    m_devices.clear();
}

/* Update */

void Breadboard::timerUpdate(gpio::State state) {
    m_lua_access.lock();
    for(auto const& [row, connection] : m_connections) {
        for(const DeviceConnection& device_obj : connection.devices) {
            auto device = m_devices.find(device_obj.id);
            if(device == m_devices.end() || !device->second->m_pin) continue;
            for(const PinConnection& pin_obj : connection.pins) {
                if(pin_obj.dir == Device::PIN_Interface::Dir::input
                || pin_obj.dir == Device::PIN_Interface::Dir::inout) { // TODO only if pin changed?
                    device->second->m_pin->setPin(device_obj.pin, state.pins[pin_obj.gpio_offs] ==
                    gpio::Pinstate::HIGH ? gpio::Tristate::HIGH : gpio::Tristate::LOW);
                }
                else if(pin_obj.dir == Device::PIN_Interface::Dir::output) {
                    emit(setBit(pin_obj.gpio_offs, device->second->m_pin->getPin(device_obj.pin)));
                }
            }
        }
    }
    m_lua_access.unlock();
}

void Breadboard::connectionUpdate(bool active) {
    if(active) {
        for(const auto& [id, req] : m_spi_channels) {
            emit(registerIOF_SPI(req.gpio_offs, req.fun, req.noresponse));
        }
        for(const auto& [id, req] : m_pin_channels) {
            emit(registerIOF_PIN(req.gpio_offs, req.fun));
        }
    }
    // else connection lost
}

void Breadboard::writeDevice(const DeviceID& id) {
    m_lua_access.lock();
    for(auto const& [row, connection] : m_connections) {
        for(const DeviceConnection& device_obj : connection.devices) {
            if(device_obj.id != id) continue;
            auto device = m_devices.find(id);
            if(device == m_devices.end() || !device->second->m_pin) continue;
            for(const PinConnection& pin_obj : connection.pins) {
                emit(setBit(pin_obj.gpio_offs, device->second->m_pin->getPin(device_obj.pin)));
            }
        }
    }
    m_lua_access.unlock();
}
