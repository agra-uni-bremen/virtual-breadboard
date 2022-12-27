#pragma once

#include "types.h"
#include "options.h"

#include <gpio-client.hpp>

#include <QWidget>
#include <QJsonObject>
#include <QDialog>
#include <QMouseEvent>

const QString DRAG_TYPE_CABLE = "cable";

class Embedded : public QWidget {
	Q_OBJECT

public:
	typedef uint64_t PinRegister;

private:
	PinOptions *m_pin_dialog;
	GPIOPinLayout m_pins;

	GpioClient m_gpio;

	const std::string m_host;
	const std::string m_port;
	bool m_connected = false;

	QPixmap m_bkgnd;
	QString m_bkgnd_path = ":/img/virtual_hifive.png";
	QSize m_windowsize = QSize(420, 231);

	QSize getDistortedSize(QSize minimum);

	void mousePressEvent(QMouseEvent *e) override;
	void paintEvent(QPaintEvent *e) override;
	void resizeEvent(QResizeEvent*) override;
	void setBackground(QString path);
	void updateBackground();

	gpio::PinNumber translatePinToGpioOffs(gpio::PinNumber pin);
	PinRegister translateGpioToGlobal(gpio::State state);

public:
	Embedded(const std::string& host, const std::string& port);
	~Embedded();

	bool timerUpdate();
	PinRegister getState();
	bool gpioConnected() const;
	void destroyConnection();
	GPIOPinLayout getPins();
	bool isPin(gpio::PinNumber pin);
	gpio::PinNumber invalidPin();

	void fromJSON(QJsonObject json);
	QJsonObject toJSON();

	void openPinOptions();

	QPoint getDistortedPositionPin(gpio::PinNumber global);
	unsigned iconSizeMinimum();
	QPoint getDistortedPosition(QPoint pos);

private slots:
	void pinsChanged(std::list<std::pair<gpio::PinNumber, IOF>> iofs);

public slots:
	void registerIOF_PIN(gpio::PinNumber global, GpioClient::OnChange_PIN fun);
	void registerIOF_SPI(gpio::PinNumber global, GpioClient::OnChange_SPI fun, bool noresponse);
	void closeIOF(gpio::PinNumber global);
	void setBit(gpio::PinNumber global, gpio::Tristate state);

signals:
	void connectionLost();
	void pinSettingsChanged(std::list<std::pair<gpio::PinNumber, IOF>> iofs);
};
