#include "device_configuration.h"

#include <QVBoxLayout>

DeviceConfiguration::DeviceConfiguration(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Device Configuration");

	connect(this, &QDialog::finished, this, &DeviceConfiguration::hide);

	m_tabs = new QTabWidget;
	m_conf = new ConfigDialog;
	connect(m_conf, &ConfigDialog::configChanged, this, &DeviceConfiguration::configChanged);
	m_tabs->addTab(m_conf, "Config");
	m_keys = new KeybindingDialog;
	connect(m_keys, &KeybindingDialog::keysChanged, this, &DeviceConfiguration::keysChanged);
	m_tabs->addTab(m_keys, "Keybinding");
	m_pins = new PinDialog;
	connect(m_pins, &PinDialog::pinsChanged, this, &DeviceConfiguration::pinsChanged);
	m_tabs->addTab(m_pins, "Pin Connections");

	auto *layout = new QVBoxLayout;
	layout->addWidget(m_tabs);
	setLayout(layout);
}

void DeviceConfiguration::setConfig(DeviceID device, const Config &config) {
	m_tabs->setTabEnabled(0, true);
	m_conf->setConfig(device, config);
}

void DeviceConfiguration::hideConfig() {
	m_tabs->setTabEnabled(0, false);
}

void DeviceConfiguration::setKeys(DeviceID device, const Keys& keys) {
	m_tabs->setTabEnabled(1, true);
	m_keys->setKeys(device, keys);
}

void DeviceConfiguration::hideKeys() {
	m_tabs->setTabEnabled(1, false);
}

void DeviceConfiguration::setPins(DeviceID device, const std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber>& globals, Device::PIN_Interface::DevicePin sync) {
	m_tabs->setTabEnabled(2, true);
	m_pins->setPins(device, globals, sync);
}

void DeviceConfiguration::hidePins() {
	m_tabs->setTabEnabled(2, false);
}

void DeviceConfiguration::hide(int) {
	m_conf->removeConfig();
	m_keys->removeKeys();
	m_pins->removePins();
	QWidget::hide();
}
