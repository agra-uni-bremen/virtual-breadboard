#include "embedded.h"

#include "gpio-helpers.h"

using namespace gpio;
using namespace std;

Embedded::Embedded(const std::string& host, const std::string& port) : QWidget(), m_host(host), m_port(port) {
    m_bkgnd = QPixmap(":/img/virtual_hifive.png");
	this->setAutoFillBackground(true);
	setMinimumSize(417, 231);
}

Embedded::~Embedded() = default;

void Embedded::resizeEvent(QResizeEvent*) {
   QPixmap new_bkgnd = m_bkgnd.scaled(size(), Qt::IgnoreAspectRatio);
   QPalette palette;
   palette.setBrush(QPalette::Window, new_bkgnd);
   setPalette(palette);
}

/* GPIO */

bool Embedded::timerUpdate() { // return: new connection?
	if(m_connected && !m_gpio.update()) {
		emit(connectionLost());
        m_connected = false;
	}
	if(!m_connected) {
        m_connected = m_gpio.setupConnection(m_host.c_str(), m_port.c_str());
		if(m_connected) {
			return true;
		}
	}
	return false;
}

State Embedded::getState() {
	return m_gpio.state;
}

bool Embedded::gpioConnected() const {
	return m_connected;
}

void Embedded::registerIOF_PIN(PinNumber gpio_offs, GpioClient::OnChange_PIN fun) {
	if(!m_gpio.isIOFactive(gpio_offs)) {
		m_gpio.registerPINOnChange(gpio_offs, fun);
	}
}

void Embedded::registerIOF_SPI(PinNumber gpio_offs, GpioClient::OnChange_SPI fun, bool no_response) {
	if(!m_gpio.isIOFactive(gpio_offs)) {
		m_gpio.registerSPIOnChange(gpio_offs, fun, no_response);
	}
}

void Embedded::closeIOF(PinNumber gpio_offs) {
	m_gpio.closeIOFunction(gpio_offs);
}

void Embedded::destroyConnection() {
	m_gpio.destroyConnection();
}

void Embedded::setBit(gpio::PinNumber gpio_offs, gpio::Tristate state) {
	if(m_connected) {
		m_gpio.setBit(gpio_offs, state);
	}
}
