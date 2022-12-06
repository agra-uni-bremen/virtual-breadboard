#pragma once

#include <device/device.hpp>

#include <QWidget>
#include <QFormLayout>

class PinDialog : public QWidget {
    Q_OBJECT

    QFormLayout *m_layout;
    DeviceID m_device;
    std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> m_globals_input;
    std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> m_globals_output;

    void addPin(Device::PIN_Interface::DevicePin device_pin, gpio::PinNumber global);

public:
    PinDialog();
    void setPins(DeviceID device_id, std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> globals);
    void removePins();

public slots:
    void accept();

signals:
    void pinsChanged(DeviceID device_id, std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> globals);
};