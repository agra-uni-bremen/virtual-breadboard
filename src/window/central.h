#pragma once

#include <embedded.h>
#include <breadboard.h>

#include <QWidget>

class Central : public QWidget {
	Q_OBJECT

	Breadboard *m_breadboard;
	Embedded *m_embedded;

public:
	Central(const std::string& host, const std::string& port, QWidget *parent);
	~Central();
	void destroyConnection();
	bool toggleDebug();
	void saveJSON(const QString& file);
	void loadJSON(const QString& file);
	void loadLUA(const std::string& dir, bool overwrite_integrated_devices);

public slots:
	void clearBreadboard();

private slots:
	void timerUpdate();
	void clearIOFs(const std::vector<gpio::PinNumber>& gpio_offs);
    void closeSPI(gpio::PinNumber gpio_offs);
    void closeSPIForDevice(gpio::PinNumber gpio_offs, const DeviceID& device_id);
    void closePin(gpio::PinNumber gpio_offs);
    void closePinForDevice(gpio::PinNumber gpio_offs, const DeviceID& device_id);

signals:
	void connectionUpdate(bool active);
	void sendStatus(QString message, int ms);
};
