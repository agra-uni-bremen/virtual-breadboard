#pragma once

#include "config.h"
#include "keybinding.h"
#include "pins.h"

#include <QDialog>
#include <QTabWidget>

class DeviceConfiguration : public QDialog {
	Q_OBJECT

	QTabWidget *m_tabs;
	ConfigDialog *m_conf;
	KeybindingDialog *m_keys;
	PinDialog *m_pins;

	void hide(int);

public:
	DeviceConfiguration(QWidget *parent);
	void setConfig(DeviceID device, const Config& config);
	void hideConfig();
	void setKeys(DeviceID device, const Keys& keys);
	void hideKeys();
	void setPins(DeviceID device, const std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber>& globals, Device::PIN_Interface::DevicePin sync);
	void hidePins();

signals:
	void keysChanged(DeviceID device_id, Keys keys);
	void configChanged(DeviceID device, Config config);
	void pinsChanged(DeviceID device_id, std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> globals, PinDialog::ChangedSync sync);
};