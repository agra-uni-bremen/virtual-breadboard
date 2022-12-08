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
    void openEmbeddedOptions();

private slots:
	void timerUpdate();
    void pinSettingsChanged(const std::list<std::pair<gpio::PinNumber, IOF>>& iofs);

signals:
	void connectionUpdate(bool active);
	void sendStatus(QString message, int ms);
};
