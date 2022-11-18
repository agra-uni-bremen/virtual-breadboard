#include "embedded.h"

#include "gpio-helpers.h"

using namespace gpio;
using namespace std;

Embedded::Embedded(const std::string host, const std::string port) : QWidget(), host(host), port(port) {
	bkgnd = QPixmap(":/img/virtual_hifive.png");
	this->setAutoFillBackground(true);
	setMinimumSize(417, 231);
}

Embedded::~Embedded() {}

void Embedded::resizeEvent(QResizeEvent*) {
   QPixmap new_bkgnd = bkgnd.scaled(size(), Qt::IgnoreAspectRatio);
   QPalette palette;
   palette.setBrush(QPalette::Window, new_bkgnd);
   setPalette(palette);
}

/* GPIO */

bool Embedded::timerUpdate() { // return: new connection?
	if(connected && !gpio.update()) {
		emit(connectionLost());
		connected = false;
	}
	if(!connected) {
		connected = gpio.setupConnection(host.c_str(), port.c_str());
		if(connected) {
			return true;
		}
	}
	return false;
}

State Embedded::getState() {
	return gpio.state;
}

bool Embedded::gpioConnected() {
	return connected;
}

void Embedded::registerIOF_PIN(PinNumber gpio_offs, GpioClient::OnChange_PIN fun) {
	if(!gpio.isIOFactive(gpio_offs)) {
		gpio.registerPINOnChange(gpio_offs, fun);
	}
}

void Embedded::registerIOF_SPI(PinNumber gpio_offs, GpioClient::OnChange_SPI fun, bool no_response) {
	if(!gpio.isIOFactive(gpio_offs)) {
		gpio.registerSPIOnChange(gpio_offs, fun, no_response);
	}
}

void Embedded::closeIOF(PinNumber gpio_offs) {
	gpio.closeIOFunction(gpio_offs);
}

void Embedded::destroyConnection() {
	gpio.destroyConnection();
}

void Embedded::setBit(gpio::PinNumber gpio_offs, gpio::Tristate state) {
	if(connected) {
		gpio.setBit(gpio_offs, state);
	}
}
