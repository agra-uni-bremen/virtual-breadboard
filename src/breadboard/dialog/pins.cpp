#include "pins.h"

#include <QPushButton>
#include <QSpinBox>
#include <QHBoxLayout>

PinDialog::PinDialog() : QWidget() {
	auto *saveButton = new QPushButton("Save");
	connect(saveButton, &QAbstractButton::pressed, this, &PinDialog::accept);
	saveButton->setDefault(true);

	m_layout = new QFormLayout(this);
	m_layout->addRow(saveButton);
}

void PinDialog::addPin(Device::PIN_Interface::DevicePin device_pin, gpio::PinNumber global, bool sync) {
	auto *box = new QSpinBox;
	box->setRange(0,  std::numeric_limits<gpio::PinNumber>::max());
	box->setWrapping(true);
	box->setValue(global);
	connect(box, QOverload<int>::of(&QSpinBox::valueChanged), [this, device_pin](int newValue) {
		m_globals_output.erase(device_pin);
		auto current_pin = m_globals_input.find(device_pin);
		if(current_pin != m_globals_input.end() && current_pin->second != newValue) {
			m_globals_output.emplace(device_pin, newValue);
		}
	});
	auto *pin_layout = new QHBoxLayout;
	pin_layout->addWidget(box);
	auto *sync_box = new QCheckBox("Synchronous");
	sync_box->setChecked(sync);
	connect(sync_box, &QCheckBox::stateChanged, [this, device_pin](int state) {
		if(state == Qt::Checked) {
			for (auto [dp, sb]: m_sync_boxes) {
				if (dp == device_pin) continue;
				sb->setChecked(false);
			}
		}
		m_sync_output = {device_pin, state == Qt::Checked};
	});
	m_sync_boxes.emplace(device_pin, sync_box);
	pin_layout->addWidget(sync_box);
	m_layout->insertRow(0, QString::number(device_pin), pin_layout);
}

void PinDialog::setPins(DeviceID device_id, const std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber>& globals, Device::PIN_Interface::DevicePin sync) {
	m_device = device_id;
	m_globals_input = globals;
	for(const auto& [device_pin, global] : globals) {
		addPin(device_pin, global, sync == device_pin);
	}
}

void PinDialog::removePins() {
	while(m_layout->rowCount() > 1) {
		m_layout->removeRow(0);
	}
	m_globals_input.clear();
	m_globals_output.clear();
	m_sync_output = {std::numeric_limits<Device::PIN_Interface::DevicePin>::max(), false};
	m_sync_boxes.clear();
	m_device = "";
}

void PinDialog::accept() {
	emit(pinsChanged(m_device, m_globals_output, m_sync_output));
}
