#pragma once

#include <device/device.hpp>

#include <QDialog>
#include <QFormLayout>

class ConfigDialog : public QDialog {
	Q_OBJECT

	QFormLayout *m_layout;
	Config m_config;
	DeviceID m_device;

	void addValue(const ConfigDescription& name, QWidget* value);
	void addInt(const ConfigDescription& name, int value);
	void addBool(const ConfigDescription& name, bool value);
	void addString(const ConfigDescription& name, const QString& value);

public:
	ConfigDialog(QWidget* parent);
	void setConfig(DeviceID device, const Config& config);

public slots:
	void accept() override;
	void reject() override;

signals:
	void configChanged(DeviceID device, Config config);
};
