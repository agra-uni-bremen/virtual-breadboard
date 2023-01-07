#include "options.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>

PinOptions::PinOptions(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Embedded device options");
	auto *closeButton = new QPushButton("Close");
	connect(closeButton, &QAbstractButton::pressed, this, &QDialog::reject);
//	auto *saveButton = new QPushButton("Save");
//	connect(saveButton, &QAbstractButton::pressed, this, &QDialog::accept);
//	saveButton->setDefault(true);

	auto *buttons_close = new QHBoxLayout;
	buttons_close->addWidget(closeButton);
//	buttons_close->addWidget(saveButton);

	m_layout = new QFormLayout(this);
	m_layout->addRow(buttons_close);
}

void PinOptions::addPin(gpio::PinNumber global, const GPIOPin& pin) {
	auto *pin_layout = new QHBoxLayout;
	for(auto iof : pin.iofs) {
		QString iof_label = "";
		switch (iof.type) {
			case IOFType::UART:
				iof_label = "UART";
				break;
			case IOFType::SPI:
				iof_label = "SPI";
				break;
			case IOFType::PWM:
				iof_label = "PWM";
				break;
		}
		if(iof.type == IOFType::SPI) {
			auto *box = new QCheckBox(iof_label);
			box->setChecked(iof.active);
			box->setDisabled(true);
			connect(box, &QCheckBox::stateChanged, [this, global, iof](int state) {
				m_pins_output.remove_if([global, iof](auto iof_pair) {
					return iof_pair.first == global && iof_pair.second.type == iof.type;
				});
				auto input = m_pins_input.find(global);
				if (input == m_pins_input.end()) return;
				auto iof_input = std::find_if(input->second.iofs.begin(), input->second.iofs.end(), [iof](IOF i) {
					return i.type == iof.type;
				});
				if (iof_input == input->second.iofs.end()) return;
				if (iof_input->active != (state == Qt::Checked)) {
					m_pins_output.emplace_back(global, IOF{.type=iof.type, .active=state == Qt::Checked});
				}
			});
			pin_layout->addWidget(box);
		}
		else {
			pin_layout->addWidget(new QLabel(iof_label));
		}
	}
	QString label = "Pin " + QString::number(global);
	label += " (Offset: " + QString::number(pin.gpio_offs) + ")";
	m_layout->insertRow(0, label, pin_layout);
}

void PinOptions::setPins(const GPIOPinLayout& pins) {
	m_pins_input = pins;
	for(const auto& [global, pin] : pins) {
		addPin(global, pin);
	}
}

void PinOptions::removePins() {
	while(m_layout->rowCount() > 1) {
		m_layout->removeRow(0);
	}
	m_pins_input.clear();
	m_pins_output.clear();
}

void PinOptions::accept() {
	emit(pinsChanged(m_pins_output));
	removePins();
	QDialog::accept();
}

void PinOptions::reject() {
	removePins();
	QDialog::reject();
}
