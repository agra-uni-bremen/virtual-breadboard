#pragma once

#include <device/device.hpp>

#include <QDialog>
#include <QFormLayout>

class KeybindingDialog : public QDialog {
	Q_OBJECT

	QFormLayout *m_layout;
	Keys m_keys;
	DeviceID m_device;

	void add(int key);

public:
	KeybindingDialog(QWidget* parent);
	void setKeys(DeviceID device, const Keys& keys);

public slots:
	void accept() override;
	void reject() override;

signals:
	void keysChanged(DeviceID device, Keys keys);
};
