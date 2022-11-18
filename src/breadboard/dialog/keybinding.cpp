#include "keybinding.h"
#include "keyedit.h"

#include <QHBoxLayout>
#include <QPushButton>

KeybindingDialog::KeybindingDialog(QWidget *parent) : QDialog(parent) {
    m_layout = new QFormLayout(this);
	auto *closeButton = new QPushButton("Close");
	connect(closeButton, &QAbstractButton::pressed, this, &QDialog::reject);
	auto *saveButton = new QPushButton("Save");
	connect(saveButton, &QAbstractButton::pressed, this, &QDialog::accept);
	saveButton->setDefault(true);
	auto *buttons = new QHBoxLayout;
	buttons->addWidget(closeButton);
	buttons->addWidget(saveButton);
	auto *addButton = new QPushButton("Add");
	connect(addButton, &QPushButton::pressed, [this](){add(0);});
	m_layout->addRow(addButton);
	m_layout->addRow(buttons);

	setWindowTitle("Edit keybindings");
}

void KeybindingDialog::add(int key) {
	m_keys.emplace(key);
	auto *value_layout = new QHBoxLayout;
	auto *box = new KeyEdit(key);
	connect(box, &KeyEdit::removeKey, [this](int old_key) {
		m_keys.erase(old_key);
	});
	connect(box, &KeyEdit::newKey, [this](int new_key) {
		m_keys.emplace(new_key);
	});
	value_layout->addWidget(box);
	auto *delete_value = new QPushButton("Delete");
	connect(delete_value, &QPushButton::pressed, [this, value_layout, box](){
		m_keys.erase(box->keySequence()[0]);
		m_layout->removeRow(value_layout);
	});
	value_layout->addWidget(delete_value);
	m_layout->insertRow(0, value_layout);
}

void KeybindingDialog::accept() {
	emit(keysChanged(m_device, m_keys));
	while(m_layout->rowCount() > 2) {
		m_layout->removeRow(0);
	}
	m_keys.clear();
    m_device = "";
	QDialog::accept();
}

void KeybindingDialog::reject() {
	while(m_layout->rowCount() > 2) {
		m_layout->removeRow(0);
	}
	m_keys.clear();
    m_device = "";
	QDialog::reject();
}

void KeybindingDialog::setKeys(DeviceID device, const Keys& keys) {
	this->m_keys.clear();
	this->m_device = device;
	for(const Key& key : keys) {
		add(key);
	}
}
