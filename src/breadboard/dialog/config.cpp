#include "config.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>

ConfigDialog::ConfigDialog(QWidget *parent) : QDialog(parent) {
	auto *closeButton = new QPushButton("Close");
	connect(closeButton, &QAbstractButton::pressed, this, &QDialog::reject);
	auto *saveButton = new QPushButton("Save");
	connect(saveButton, &QAbstractButton::pressed, this, &QDialog::accept);
	saveButton->setDefault(true);
	auto *buttons_close = new QHBoxLayout;
	buttons_close->addWidget(closeButton);
	buttons_close->addWidget(saveButton);

    m_layout = new QFormLayout(this);
	m_layout->addRow(buttons_close);

	setWindowTitle("Edit config values");
}

void ConfigDialog::addValue(const ConfigDescription& name, QWidget* value) {
	m_layout->insertRow(0, QString::fromStdString(name), value);
}

void ConfigDialog::addBool(const ConfigDescription& name, bool value) {
	auto *box = new QCheckBox("");
	box->setChecked(value);
	connect(box, &QCheckBox::stateChanged, [this, name](int state) {
		auto conf_value = m_config.find(name);
		if(conf_value != m_config.end() && conf_value->second.type == ConfigElem::Type::boolean) {
			conf_value->second.value.boolean = state == Qt::Checked;
		}
	});
	this->addValue(name, box);
}

void ConfigDialog::addInt(const ConfigDescription& name, int value) {
	auto *box = new QSpinBox;
	box->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
	box->setWrapping(true);
	box->setValue(value);
	connect(box, QOverload<int>::of(&QSpinBox::valueChanged), [this, name](int newValue) {
		auto conf_value = m_config.find(name);
		if(conf_value != m_config.end() && conf_value->second.type == ConfigElem::Type::integer) {
			conf_value->second.value.integer = newValue;
		}
	});
	this->addValue(name, box);
}

void ConfigDialog::addString(const ConfigDescription& name, const QString& value) {
	auto *box = new QLineEdit(value);
	connect(box, &QLineEdit::textChanged, [this, name](const QString& text) {
		auto conf_value = m_config.find(name);
		if(conf_value != m_config.end() && conf_value->second.type == ConfigElem::Type::string) {
			m_config.erase(name);
			QByteArray text_bytes = text.toLocal8Bit();
			m_config.emplace(name, ConfigElem{text_bytes.data()});
		}
	});
	this->addValue(name, box);
}

void ConfigDialog::setConfig(DeviceID device, const Config& config) {
	this->m_device = device;
	this->m_config = config;
	for(auto const& [description, element] : config) {
		switch(element.type) {
		case ConfigElem::Type::integer: {
			addInt(description, element.value.integer);
			break;
		}
		case ConfigElem::Type::boolean: {
			addBool(description, element.value.boolean);
			break;
		}
		case ConfigElem::Type::string: {
			addString(description, QString::fromLocal8Bit(element.value.string));
			break;
		}
		default:
			break;
		}
	}
}

void ConfigDialog::reject() {
	while(m_layout->rowCount() > 1) {
		m_layout->removeRow(0);
	}
	m_config.clear();
    m_device = "";
	QDialog::reject();
}

void ConfigDialog::accept() {
	emit(configChanged(m_device, m_config));
	while(m_layout->rowCount() > 1) {
		m_layout->removeRow(0);
	}
	m_config.clear();
    m_device = "";
	QDialog::accept();
}
