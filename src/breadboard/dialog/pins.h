#pragma once

#include <device/device.hpp>

#include <QWidget>
#include <QFormLayout>
#include <QCheckBox>

class PinDialog : public QWidget {
	Q_OBJECT

public:
	typedef std::pair<Device::PIN_Interface::DevicePin, bool> ChangedSync;

private:

	QFormLayout *m_layout;
	std::unordered_map<Device::PIN_Interface::DevicePin, QCheckBox*> m_sync_boxes;

	DeviceID m_device;
	std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> m_globals_input;
	std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> m_globals_output;

	ChangedSync m_sync_output;

	void addPin(Device::PIN_Interface::DevicePin device_pin, gpio::PinNumber global, bool sync);

public:
	PinDialog();
	void setPins(DeviceID device_id, const std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber>& globals, Device::PIN_Interface::DevicePin sync);
	void removePins();

public slots:
	void accept();

signals:
	void pinsChanged(DeviceID device_id, std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> globals, ChangedSync sync);
};