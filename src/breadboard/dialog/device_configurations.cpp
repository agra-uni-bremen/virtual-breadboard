#include "device_configurations.h"

#include <QVBoxLayout>

DeviceConfigurations::DeviceConfigurations(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Device Configurations");

    connect(this, &QDialog::finished, this, &DeviceConfigurations::hide);

    m_tabs = new QTabWidget;
    m_conf = new ConfigDialog;
    connect(m_conf, &ConfigDialog::configChanged, this, &DeviceConfigurations::configChanged);
    m_tabs->addTab(m_conf, "Config");
    m_keys = new KeybindingDialog;
    connect(m_keys, &KeybindingDialog::keysChanged, this, &DeviceConfigurations::keysChanged);
    m_tabs->addTab(m_keys, "Keybinding");
    m_pins = new PinDialog;
    connect(m_pins, &PinDialog::pinsChanged, this, &DeviceConfigurations::pinsChanged);
    m_tabs->addTab(m_pins, "Pin Connections");

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_tabs);
    setLayout(layout);
}

void DeviceConfigurations::setConfig(DeviceID device, const Config &config) {
    m_tabs->setTabVisible(0, true);
    m_conf->setConfig(device, config);
}

void DeviceConfigurations::hideConfig() {
    m_tabs->setTabVisible(0, false);
}

void DeviceConfigurations::setKeys(DeviceID device, const Keys& keys) {
    m_tabs->setTabVisible(1, true);
    m_keys->setKeys(device, keys);
}

void DeviceConfigurations::hideKeys() {
    m_tabs->setTabVisible(1, false);
}

void DeviceConfigurations::setPins(DeviceID device, const std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber>& globals, Device::PIN_Interface::DevicePin sync) {
    m_tabs->setTabVisible(2, true);
    m_pins->setPins(device, globals, sync);
}

void DeviceConfigurations::hidePins() {
    m_tabs->setTabVisible(2, false);
}

void DeviceConfigurations::hide(int) {
    m_conf->removeConfig();
    m_keys->removeKeys();
    m_pins->removePins();
    QWidget::hide();
}
