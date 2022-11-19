#pragma once

#include <gpio-client.hpp>

#include <QWidget>

class Embedded : public QWidget {
	Q_OBJECT

	GpioClient m_gpio;

	const std::string m_host;
	const std::string m_port;
	bool m_connected = false;

	QPixmap m_bkgnd;

	void resizeEvent(QResizeEvent*) override;

public:
	Embedded(const std::string& host, const std::string& port);
	~Embedded();

	bool timerUpdate();
	gpio::State getState();
	bool gpioConnected() const;
	void destroyConnection();

public slots:
	void registerIOF_PIN(gpio::PinNumber gpio_offs, GpioClient::OnChange_PIN fun);
	void registerIOF_SPI(gpio::PinNumber gpio_offs, GpioClient::OnChange_SPI fun, bool noresponse);
	void closeIOF(gpio::PinNumber gpio_offs);
	void setBit(gpio::PinNumber gpio_offs, gpio::Tristate state);

signals:
	void connectionLost();
};
