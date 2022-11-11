#pragma once

#include <embedded.h>
#include <breadboard.h>

#include <QWidget>

class Central : public QWidget {
	Q_OBJECT

	Breadboard *breadboard;
	Embedded *embedded;

public:
	Central(const std::string host, const std::string port, QWidget *parent);
	~Central();
	void destroyConnection();
	bool toggleDebug();
	void saveJSON(QString file);
	void loadJSON(QString file);
	void loadLUA(std::string dir, bool overwrite_integrated_devices);

public slots:
	void clearBreadboard();

private slots:
	void timerUpdate();
	void closeAllIOFs(std::vector<gpio::PinNumber> gpio_offs);
	void closeDeviceIOFs(std::vector<gpio::PinNumber> gpio_offs, DeviceID device);

signals:
	void connectionUpdate(bool active);
	void sendStatus(QString message, int ms);
};
