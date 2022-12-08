#include "breadboard.h"

using namespace std;

void Breadboard::printConnections() {
    cout << "--------------------------" << endl;
    cout << "Raster:" << endl;
    for(auto const& [row, content] : m_raster) {
        cout << "\tRow " << row << endl;
        cout << "\t\tPins: " << endl;
        for(const PinConnection& pin : content.pins) {
            cout << "\t\t\tGlobal pin " << (int) pin.global_pin << " (" << pin.name << ") at index " << pin.index << endl;
        }
        cout << "\t\tDevices: " << endl;
        for(const DeviceConnection& device : content.devices) {
            cout << "\t\t\tDevice " << device.id << " with device pin " << device.pin << endl;
        }
    }
    cout << "Connections:" << endl;
    cout << "\tAsync writing:" << endl;
    for(const PinMapping& mapping : m_writing_connections) {
        cout << "\t\tGlobal pin " << (int) mapping.global_pin << " connected with device " << mapping.device << endl;
    }
    cout << "\tAsync reading:" << endl;
    for(const PinMapping& mapping : m_reading_connections) {
        cout << "\t\tGlobal pin " << (int) mapping.global_pin << " connected with device " << mapping.device << endl;
    }
    cout << "\tSync pins:" << endl;
    for(auto const& [device_id, req] : m_pin_channels) {
        cout << "\t\tGlobal pin " << (int) req.global_pin << " connected with device " << device_id << endl;
    }
    cout << "\tSync SPI:" << endl;
    for(auto const& [device_id, req] : m_spi_channels) {
        cout << "\t\tGlobal pin " << (int) req.global_pin << " connected with device " << device_id << endl;
    }
    cout << "--------------------------" << endl;
}

/* Register */

pair<Breadboard::Row, Breadboard::Index> Breadboard::removeConnection(const DeviceID& device_id, Device::PIN_Interface::DevicePin device_pin) {
    Row row = getRowForDevicePin(device_id, device_pin);
    Index index = getNextIndex(row);
    if(isBreadboard() && (!isValidRasterRow(row) || !isValidRasterIndex(index))) {
        cerr << "[Breadboard] Could not find the device pin in the connection raster" << endl;
        return {BB_ROWS, BB_INDEXES};
    }
    unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> current_globals = getPinsToDevicePins(device_id);
    auto old_pin = current_globals.find(device_pin);
    if(old_pin != current_globals.end() && m_embedded->isPin(old_pin->second)) {
        removePin(old_pin->second, false);
    }
    return {row, index};
}

void Breadboard::addPinToDevicePin(const DeviceID& device_id, Device::PIN_Interface::DevicePin device_pin, gpio::PinNumber global, std::string name) {
    auto device = m_devices.find(device_id);
    if(device == m_devices.end() || !device->second->m_pin) {
        m_error_dialog->showMessage("Device does not implement pin interface.");
        return;
    }
    auto const [row, index] = removeConnection(device_id, device_pin);
    addPinToRow(row, index, global, name);
}

void Breadboard::addPinToRow(Row row, Index index, gpio::PinNumber global, std::string name) {
    addPinToRowContent(row, index, global, name);
    createRowConnections(row);
}

void Breadboard::addPinToRowContent(Row row, Index index, gpio::PinNumber global, std::string name) {
    if(isBreadboard() && (!isValidRasterIndex(index) || !isValidRasterRow(row))) return;
    if(!m_embedded->isPin(global)) return;
    // TODO: check if global already exists somewhere
    PinConnection new_connection = PinConnection{
            .global_pin = global,
            .name = name,
            .index = index
    };
    auto row_obj = m_raster.find(row);
    if(row_obj != m_raster.end()) {
        // TODO: check if there already is an object at position index
        row_obj->second.pins.push_back(new_connection);
    }
    else {
        RowContent new_content;
        new_content.pins.push_back(new_connection);
        m_raster.emplace(row, new_content);
    }
}

void Breadboard::createRowConnections(Row row) {
    auto row_obj = m_raster.find(row);
    if(row_obj == m_raster.end()) {
        cerr << "[Breadboard] Could not find row " << row << endl;
        return;
    }
    GPIOPinLayout embedded_pins = m_embedded->getPins();
    for (auto &device_obj: row_obj->second.devices) {
        unordered_set<gpio::PinNumber> connected = getPinsToDevice(device_obj.id);
        for(auto const& pin_obj : row_obj->second.pins) {
            if(connected.contains(pin_obj.global_pin)) continue;
            auto e_pin = embedded_pins.find(pin_obj.global_pin);
            if(e_pin == embedded_pins.end()) continue;
            bool iof_set = false;
            for(auto iof : e_pin->second.iofs) {
                if(iof.type == IOFType::SPI && iof.active) {
                    registerSPI(pin_obj.global_pin, device_obj.pin, device_obj.id, false);
                    iof_set = true;
                    break;
                }
            }
            if(!iof_set) {
                registerPin(pin_obj.global_pin, device_obj.pin, device_obj.id);
            }
        }
    }
}

unordered_set<gpio::PinNumber> Breadboard::getPinsToDevice(const DeviceID& device_id) {
    unordered_set<gpio::PinNumber> connected_global;
    auto sync_pin = m_pin_channels.find(device_id);
    if(sync_pin != m_pin_channels.end()) {
        connected_global.insert(sync_pin->second.global_pin);
    }
    auto spi = m_spi_channels.find(device_id);
    if(spi != m_spi_channels.end()) {
        connected_global.insert(spi->second.global_pin);
    }
    for(auto const& mapping : m_reading_connections) {
        if(mapping.device == device_id) {
            connected_global.insert(mapping.global_pin);
        }
    }
    for(auto const& mapping : m_writing_connections) {
        if(mapping.device == device_id) {
            connected_global.insert(mapping.global_pin);
        }
    }
    return connected_global;
}

std::string Breadboard::getPinName(gpio::PinNumber global) {
    for(auto const& [row, content] : m_raster) {
        auto pin_obj = find_if(content.pins.begin(), content.pins.end(),[global](const PinConnection& connection){return connection.global_pin==global;});
        if(pin_obj != content.pins.end()) return pin_obj->name;
    }
    return "";
}

unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> Breadboard::getPinsToDevicePins(const DeviceID& device_id) {
    unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> connected_global;
    auto device = m_devices.find(device_id);
    if(device == m_devices.end() || !device->second->m_pin) return connected_global;
    auto sync = m_pin_channels.find(device_id);
    if(sync!=m_pin_channels.end()) {
        connected_global.emplace(sync->second.device_pin, sync->second.global_pin);
    }
    auto spi = m_spi_channels.find(device_id);
    if(spi!=m_spi_channels.end()) {
        connected_global.emplace(spi->second.cs_pin, spi->second.global_pin);
    }
    for(auto const& mapping : m_reading_connections) {
        if(mapping.device == device_id) {
            connected_global.emplace(mapping.device_pin, mapping.global_pin);
        }
    }
    for(auto const& mapping : m_writing_connections) {
        if(mapping.device == device_id) {
            connected_global.emplace(mapping.device_pin, mapping.global_pin);
        }
    }
    // invalid for all other device pins
    for(auto const& [device_pin, desc] : device->second->m_pin->getPinLayout()) {
        if(!connected_global.contains(device_pin)) {
            connected_global.emplace(device_pin, numeric_limits<gpio::PinNumber>::max());
        }
    }
    return connected_global;
}

void Breadboard::registerPin(gpio::PinNumber global, Device::PIN_Interface::DevicePin device_pin, const DeviceID& device_id, bool synchronous) {
    auto device = m_devices.find(device_id);
    if(device == m_devices.end()) {
        cerr << "[Breadboard] Could not find device '" << device_id << "' when attempting to register pin " << (int) global << endl;
        removeDevice(device_id);
        return;
    }
    if(!device->second->m_pin) {
        cerr << "[Breadboard] Attempting to add pin connection for device '" << device->second->getClass() <<
             "', but device does not implement PIN interface." << endl;
        return;
    }
    const Device::PIN_Interface::PinLayout layout = device->second->m_pin->getPinLayout();
    if(layout.find(device_pin) == layout.end()) {
        cerr << "[Breadboard] Attempting to add pin '" << (int)device_pin << "' for device " <<
             device->second->getClass() << " that is not offered by device" << endl;
        return;
    }
    const Device::PIN_Interface::PinDesc& desc = layout.at(device_pin);
    if(synchronous) {
        if(desc.dir != Device::PIN_Interface::Dir::input) {
            cerr << "[Breadboard] Attempting to add pin '" << (int)device_pin << "' as synchronous for device " <<
                 device->second->getClass() << ", but device labels pin not as input."
                                       " This is not supported for inout-pins and unnecessary for output pins." << endl;
            return;
        }
        auto device_ptr = device->second.get();
        m_pin_channels.emplace(device_id, PIN_IOF_Request{
                .global_pin = global,
                .device_pin = device_pin,
                .fun = [this, device_ptr, device_pin](gpio::Tristate pin) {
                    m_lua_access.lock();
                    device_ptr->m_pin->setPin(device_pin, pin);
                    m_lua_access.unlock();
                }
        });
    }
    else {
        PinMapping mapping = PinMapping{
            .global_pin = global,
            .device_pin = device_pin,
            .device = device_id
        };
        if(desc.dir == Device::PIN_Interface::Dir::input || desc.dir == Device::PIN_Interface::Dir::inout) {
            m_reading_connections.push_back(mapping);
        }
        else if(desc.dir == Device::PIN_Interface::Dir::output) {
            m_writing_connections.push_back(mapping);
        }
    }
}

void Breadboard::setPinSync(gpio::PinNumber global, Device::PIN_Interface::DevicePin device_pin, const DeviceID& device_id, bool synchronous) {
    removeSPI(global, true);
    removePinForDevice(global, device_id);
    registerPin(global, device_pin, device_id, synchronous);
}

void Breadboard::setSPI(gpio::PinNumber global, bool active) {
    if(!active) {
        removeSPI(global, true);
    }
    else {
        removePin(global, true);
    }
    for(auto const& [row, content] : m_raster) {
        createRowConnections(row);
    }
}

void Breadboard::registerSPI(gpio::PinNumber global, Device::PIN_Interface::DevicePin cs_pin, const DeviceID& device_id, bool noresponse) {
    auto device = m_devices.find(device_id);
    if(device == m_devices.end()) {
        cerr << "[Breadboard] Could not find device '" << device_id << "' when attempting to register SPI" << endl;
        removeDevice(device_id);
        return;
    }
    if(!device->second->m_spi) {
        cerr << "[Breadboard] Attempting to add SPI connection for device '" << device->second->getClass() <<
             "', but device does not implement SPI interface." << endl;
        return;
    }
    auto device_ptr = device->second.get();
    m_spi_channels.emplace(device_id, SPI_IOF_Request{
                                   .global_pin = global,
                                   .cs_pin = cs_pin,
                                   .noresponse = noresponse,
                                   .fun = [this, device_ptr](gpio::SPI_Command cmd){
                                       m_lua_access.lock();
                                       const gpio::SPI_Response ret = device_ptr->m_spi->send(cmd);
                                       m_lua_access.unlock();
                                       return ret;
                                   }
                           }
    );
}

void Breadboard::setSPInoresponse(gpio::PinNumber global, bool noresponse) {
    for(auto& [device, spi] : m_spi_channels) {
        if(spi.global_pin == global) {
            spi.noresponse = noresponse;
        }
    }
}

/* Remove */

void Breadboard::removeSPI(gpio::PinNumber global, bool keep_on_raster) {
    bool exists = find_if(m_spi_channels.begin(), m_spi_channels.end(),
                          [global](const auto& spi_pair){
                              return spi_pair.second.global_pin == global;
                          }) != m_spi_channels.end();
    if(exists) {
        m_embedded->closeIOF(global);
        erase_if(m_spi_channels, [global](const auto& spi_pair){
            return spi_pair.second.global_pin == global;
        });
    }
    if(!keep_on_raster) {
        removePinFromRaster(global);
    }
}

void Breadboard::removeSPIForDevice(gpio::PinNumber global, const DeviceID& device_id) {
    auto req = m_spi_channels.find(device_id);
    if(req == m_spi_channels.end() || req->second.global_pin != global) return;
    m_embedded->closeIOF(global);
    m_spi_channels.erase(device_id);
}

void Breadboard::removePin(gpio::PinNumber global, bool keep_on_raster) {
    bool exists = find_if(m_pin_channels.begin(), m_pin_channels.end(),
                          [global](const auto& pin_pair){
                              return pin_pair.second.global_pin == global;
                          }) != m_pin_channels.end();
    if(exists) {
        m_embedded->closeIOF(global);
        erase_if(m_pin_channels, [global](const auto& pin_pair){
            return pin_pair.second.global_pin == global;
        });
    }
    m_writing_connections.remove_if([global](const PinMapping& mapping){return mapping.global_pin == global;});
    m_reading_connections.remove_if([global](const PinMapping& mapping){return mapping.global_pin == global;});
    if(!keep_on_raster) {
        removePinFromRaster(global);
    }
}

void Breadboard::removePinForDevice(gpio::PinNumber global, const DeviceID& device_id) {
    auto req = m_pin_channels.find(device_id);
    if(req != m_pin_channels.end() && req->second.global_pin == global) {
        m_embedded->closeIOF(global);
        m_pin_channels.erase(device_id);
    }
    m_writing_connections.remove_if([global,device_id](const PinMapping& mapping){
        return mapping.device == device_id && mapping.global_pin == global;
    });
    m_reading_connections.remove_if([global, device_id](const PinMapping& mapping){
        return mapping.device == device_id && mapping.global_pin == global;
    });
}

void Breadboard::removePinFromRaster(gpio::PinNumber global) {
    for(auto& [row, content] : m_raster) {
        content.pins.remove_if([global](const PinConnection& c_obj){return c_obj.global_pin == global;});
    }
}

void Breadboard::removeConnections(gpio::PinNumber global, bool keep_on_raster) {
    removeSPI(global, keep_on_raster);
    removePin(global, keep_on_raster);
}

void Breadboard::removeDevice(const DeviceID& id) {
    if(m_pin_channels.contains(id)) removePinForDevice(m_pin_channels.find(id)->second.global_pin, id);
    if(m_spi_channels.contains(id)) removeSPIForDevice(m_spi_channels.find(id)->second.global_pin, id);
    m_writing_connections.remove_if([id](const PinMapping& mapping){return mapping.device == id;});
    m_reading_connections.remove_if([id](const PinMapping& mapping){return mapping.device == id;});
    for(auto& [row, content] : m_raster) {
        content.devices.remove_if([id](const DeviceConnection& c_obj){return c_obj.id == id;});
    }
    m_devices.erase(id);
}

void Breadboard::clear() {
    for(auto const& [id,spi] : m_spi_channels) {
        m_embedded->closeIOF(spi.global_pin);
    }
    for(auto const& [id,pin] : m_pin_channels) {
        m_embedded->closeIOF(pin.global_pin);
    }
    m_spi_channels.clear();
    m_pin_channels.clear();
    m_writing_connections.clear();
    m_reading_connections.clear();
    m_raster.clear();
    m_devices.clear();

    setMinimumSize(DEFAULT_SIZE);
    setBackground(DEFAULT_PATH);
}

/* Update */

void Breadboard::timerUpdate(uint64_t state) {
    m_lua_access.lock();
    for(auto const& mapping : m_reading_connections) {
        auto device = m_devices.find(mapping.device);
        if(device == m_devices.end()) {
            removeDevice(mapping.device);
            continue;
        }
        if(!device->second->m_pin) continue;
        device->second->m_pin->setPin(mapping.device_pin, ((state >> mapping.global_pin)&1 ? gpio::Tristate::HIGH : gpio::Tristate::LOW));
    }
    for(auto const& mapping : m_writing_connections) {
        auto device = m_devices.find(mapping.device);
        if(device == m_devices.end()) {
            removeDevice(mapping.device);
            continue;
        }
        if(!device->second->m_pin) continue;
        m_embedded->setBit(mapping.global_pin, device->second->m_pin->getPin(mapping.device_pin));
    }
    m_lua_access.unlock();
}

void Breadboard::connectionUpdate(bool active) {
    if(active) {
        for(const auto& [id, req] : m_spi_channels) {
            m_embedded->registerIOF_SPI(req.global_pin, req.fun, req.noresponse);
        }
        for(const auto& [id, req] : m_pin_channels) {
            m_embedded->registerIOF_PIN(req.global_pin, req.fun);
        }
    }
    // else connection lost
}

void Breadboard::writeDevice(const DeviceID& id) {
    auto device = m_devices.find(id);
    if(device == m_devices.end()) {
        removeDevice(id);
        return;
    }
    if(!device->second->m_pin) return;
    m_lua_access.lock();
    for(auto const& mapping : m_writing_connections) {
        if(mapping.device != id) continue;
        m_embedded->setBit(mapping.global_pin, device->second->m_pin->getPin(mapping.device_pin));
    }
    m_lua_access.unlock();
}
