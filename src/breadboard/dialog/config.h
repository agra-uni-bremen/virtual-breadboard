#pragma once

#include <device/device.hpp>

#include <QWidget>
#include <QFormLayout>

class ConfigDialog : public QWidget {
	Q_OBJECT

	QFormLayout *m_layout;
	Config m_config;
	DeviceID m_device;

	void addValue(const ConfigDescription& name, QWidget* value);
	void addInt(const ConfigDescription& name, int value);
	void addBool(const ConfigDescription& name, bool value);
	void addString(const ConfigDescription& name, const QString& value);

public:
	ConfigDialog();
	void setConfig(DeviceID device, const Config& config);
    void removeConfig();

public slots:
	void accept();

signals:
	void configChanged(DeviceID device, Config config);
};
