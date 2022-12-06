#include "pins.h"

#include <QPushButton>
#include <QSpinBox>

#include <iostream>

PinDialog::PinDialog() : QWidget() {
    auto *saveButton = new QPushButton("Save");
    connect(saveButton, &QAbstractButton::pressed, this, &PinDialog::accept);
    saveButton->setDefault(true);

    m_layout = new QFormLayout(this);
    m_layout->addRow(saveButton);
}

void PinDialog::addPin(Device::PIN_Interface::DevicePin device_pin, gpio::PinNumber global) {
    auto *box = new QSpinBox;
    box->setRange(0,  std::numeric_limits<gpio::PinNumber>::max());
    box->setWrapping(true);
    box->setValue(global);
    connect(box, QOverload<int>::of(&QSpinBox::valueChanged), [this, device_pin](int newValue) {
        auto current_pin = m_globals_input.find(device_pin);
        if(current_pin != m_globals_input.end() && current_pin->second != newValue) {
            m_globals_output.erase(device_pin);
            m_globals_output.emplace(device_pin, newValue);
        }
    });
    m_layout->insertRow(0, QString::number(device_pin), box);
}

void PinDialog::setPins(DeviceID device_id, std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> globals) {
    m_device = device_id;
    m_globals_input = globals;
    m_globals_output.clear();
    for(auto const& [device_pin, global] : globals) {
        addPin(device_pin, global);
    }
}

void PinDialog::removePins() {
    while(m_layout->rowCount() > 1) {
        m_layout->removeRow(0);
    }
    m_globals_input.clear();
    m_globals_output.clear();
    m_device = "";
}

void PinDialog::accept() {
    emit(pinsChanged(m_device, m_globals_output));
}
