#pragma once

#include <device/device.hpp>

#include <QWidget>
#include <QFormLayout>

class KeybindingDialog : public QWidget {
	Q_OBJECT

	QFormLayout *m_layout;
	Keys m_keys;
	DeviceID m_device;

	void add(int key);

public:
	KeybindingDialog();
	void setKeys(DeviceID device, const Keys& keys);
    void removeKeys();

public slots:
	void accept();

signals:
	void keysChanged(DeviceID device, Keys keys);
};
