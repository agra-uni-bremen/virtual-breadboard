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

void Breadboard::addSPIToRow(Row row, Index index, gpio::PinNumber global, std::string name, bool noresponse) {
    addPinToRowContent(row, index, global, name);
    createRowConnectionsSPI(row, noresponse);
}

void Breadboard::addPinToRow(Row row, Index index, gpio::PinNumber global, std::string name) {
    addPinToRowContent(row, index, global, name);
    createRowConnections(row);
}

void Breadboard::addPinToRowContent(Row row, Index index, gpio::PinNumber global, std::string name) {
    PinConnection new_connection = PinConnection{
            .global_pin = global,
            .name = name,
            .index = index
    };
    auto row_obj = m_raster.find(row);
    if(row_obj != m_raster.end()) {
        row_obj->second.pins.push_back(new_connection);
    }
    else {
        RowContent new_content;
        new_content.pins.push_back(new_connection);
        m_raster.emplace(row, new_content);
    }
}

void Breadboard::createRowConnectionsSPI(Row row, bool noresponse) {
    auto row_obj = m_raster.find(row);
    if(row_obj == m_raster.end()) {
        cerr << "[Breadboard] Could not find row " << row << endl;
        return;
    }
    for (auto &device_obj: row_obj->second.devices) {
        unordered_set<gpio::PinNumber> connected = getPinsToDevice(device_obj.id);
        for(auto const& pin_obj : row_obj->second.pins) {
            if(connected.contains(pin_obj.global_pin)) continue;
            registerSPI(pin_obj.global_pin, device_obj.id, noresponse);
        }
    }
}

void Breadboard::createRowConnections(Row row) {
    auto row_obj = m_raster.find(row);
    if(row_obj == m_raster.end()) {
        cerr << "[Breadboard] Could not find row " << row << endl;
        return;
    }
    for (auto &device_obj: row_obj->second.devices) {
        unordered_set<gpio::PinNumber> connected = getPinsToDevice(device_obj.id);
        for(auto const& pin_obj : row_obj->second.pins) {
            if(connected.contains(pin_obj.global_pin)) continue;
            registerPin(pin_obj.global_pin, device_obj.pin, device_obj.id);
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

void Breadboard::registerPin(gpio::PinNumber global, Device::PIN_Interface::DevicePin device_pin, const DeviceID& device_id, bool synchronous) {
    auto device = m_devices.find(device_id);
    if(device == m_devices.end()) {
        cerr << "[Breadboard] Could not find device '" << device_id << "' when attempting to register pin" << endl;
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
                .gpio_offs= translatePinToGpioOffs(global),
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
            .gpio_offs = translatePinToGpioOffs(global),
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

void Breadboard::registerSPI(gpio::PinNumber global, const DeviceID& device_id, bool noresponse) {
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
                                   .gpio_offs = translatePinToGpioOffs(global),
                                   .global_pin = global,
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

void Breadboard::setSPI(gpio::PinNumber global, bool active) {
    // add/remove SPI entry in m_spi_channels AND remove PIN connections (sync, writing, reading) for global!
    if(!active) {
        removeSPI(global, true);
        for(auto const& [row, content] : m_raster) {
            createRowConnections(row);
        }
    }
    else {
        // remove all normal pin entries for this pin
        removePin(global, true);
        for(auto const& [row, content] : m_raster) {
            createRowConnectionsSPI(row);
        }
    }
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
        emit(closeSPI(translatePinToGpioOffs(global)));
    }
    if(!keep_on_raster) {
        removePinFromRaster(global);
    }
}

void Breadboard::removeSPIObjects(gpio::PinNumber gpio_offs) {
    erase_if(m_spi_channels, [gpio_offs](const auto& spi_pair){
        return spi_pair.second.gpio_offs == gpio_offs;
    });
}

void Breadboard::removeSPIForDevice(gpio::PinNumber global, const DeviceID& device_id) {
    auto req = m_spi_channels.find(device_id);
    if(req == m_spi_channels.end() || req->second.global_pin != global) return;
    emit(closeSPIForDevice(translatePinToGpioOffs(global), device_id));
}

void Breadboard::removeSPIObjectForDevice(const DeviceID& device_id) {
    m_spi_channels.erase(device_id);
}

void Breadboard::removePin(gpio::PinNumber global, bool keep_on_raster) {
    bool exists = find_if(m_pin_channels.begin(), m_pin_channels.end(),
                          [global](const auto& pin_pair){
                              return pin_pair.second.global_pin == global;
                          }) != m_pin_channels.end();
    if(exists) {
        emit(closePin(translatePinToGpioOffs(global)));
    }
    m_writing_connections.remove_if([global](const PinMapping& mapping){return mapping.global_pin == global;});
    m_reading_connections.remove_if([global](const PinMapping& mapping){return mapping.global_pin == global;});
    if(!keep_on_raster) {
        removePinFromRaster(global);
    }
}

void Breadboard::removePinObjects(gpio::PinNumber gpio_offs) {
    erase_if(m_pin_channels, [gpio_offs](const auto& pin_pair){
        return pin_pair.second.gpio_offs == gpio_offs;
    });
}

void Breadboard::removePinForDevice(gpio::PinNumber global, const DeviceID& device_id) {
    auto req = m_pin_channels.find(device_id);
    if(req == m_pin_channels.end() || req->second.global_pin != global) return;
    if(req != m_pin_channels.end() && req->second.global_pin == global) {
        emit(closePinForDevice(translatePinToGpioOffs(global), device_id));
    }
    m_writing_connections.remove_if([global,device_id](const PinMapping& mapping){
        return mapping.device == device_id && mapping.global_pin == global;
    });
    m_reading_connections.remove_if([global, device_id](const PinMapping& mapping){
        return mapping.device == device_id && mapping.global_pin == global;
    });
}

void Breadboard::removePinObjectsForDevice(const DeviceID& device_id) {
    m_pin_channels.erase(device_id);
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
    vector<gpio::PinNumber> iofs;
    for(auto const& [id,spi] : m_spi_channels) {
        iofs.push_back(spi.gpio_offs);
    }
    for(auto const& [id,pin] : m_pin_channels) {
        iofs.push_back(pin.gpio_offs);
    }
    emit(clearIOFs(iofs));

    setMinimumSize(DEFAULT_SIZE);
    setBackground(DEFAULT_PATH);
}

void Breadboard::clearObjects() {
    m_spi_channels.clear();
    m_pin_channels.clear();
    m_raster.clear();
    m_devices.clear();
}

/* Update */

void Breadboard::timerUpdate(gpio::State state) {
    m_lua_access.lock();
    for(auto const& mapping : m_reading_connections) {
        auto device = m_devices.find(mapping.device);
        if(device == m_devices.end()) {
            removeDevice(mapping.device);
            continue;
        }
        if(!device->second->m_pin) continue;
        device->second->m_pin->setPin(mapping.device_pin, state.pins[mapping.gpio_offs] ==
        gpio::Pinstate::HIGH ? gpio::Tristate::HIGH : gpio::Tristate::LOW); // TODO only if pin changed?
    }
    for(auto const& mapping : m_writing_connections) {
        auto device = m_devices.find(mapping.device);
        if(device == m_devices.end()) {
            removeDevice(mapping.device);
            continue;
        }
        if(!device->second->m_pin) continue;
        emit(setBit(mapping.gpio_offs, device->second->m_pin->getPin(mapping.device_pin)));
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
    auto device = m_devices.find(id);
    if(device == m_devices.end()) {
        removeDevice(id);
        return;
    }
    if(!device->second->m_pin) return;
    m_lua_access.lock();
    for(auto const& mapping : m_writing_connections) {
        if(mapping.device != id) continue;
        emit(setBit(mapping.gpio_offs, device->second->m_pin->getPin(mapping.device_pin)));
    }
    m_lua_access.unlock();
}
