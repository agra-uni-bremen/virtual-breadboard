#include "central.h"

#include <QVBoxLayout>
#include <QTimer>

/* Constructor */

Central::Central(const std::string& host, const std::string& port, QWidget *parent) : QWidget(parent) {
    m_breadboard = new Breadboard();
    m_embedded = new Embedded(host, port);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(m_embedded);
	layout->addWidget(m_breadboard);

	auto *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &Central::timerUpdate);
	timer->start(250);

	connect(m_breadboard, &Breadboard::registerIOF_PIN, m_embedded, &Embedded::registerIOF_PIN);
	connect(m_breadboard, &Breadboard::registerIOF_SPI, m_embedded, &Embedded::registerIOF_SPI);
	connect(m_breadboard, &Breadboard::closeAllIOFs, this, &Central::closeAllIOFs);
	connect(m_breadboard, &Breadboard::closeDeviceIOFs, this, &Central::closeDeviceIOFs);
    connect(m_breadboard, &Breadboard::closePinIOFs, this, &Central::closePinIOFs);
	connect(m_breadboard, &Breadboard::setBit, m_embedded, &Embedded::setBit);
	connect(m_embedded, &Embedded::connectionLost, [this](){
		emit(connectionUpdate(false));
	});
	connect(this, &Central::connectionUpdate, m_breadboard, &Breadboard::connectionUpdate);
}

Central::~Central() = default;

void Central::destroyConnection() {
	m_embedded->destroyConnection();
}

bool Central::toggleDebug() {
	return m_breadboard->toggleDebug();
}

void Central::closeAllIOFs(const std::vector<gpio::PinNumber>& gpio_offs) {
	for(gpio::PinNumber gpio : gpio_offs) {
		m_embedded->closeIOF(gpio);
	}
	m_breadboard->clearConnections();
}

void Central::closeDeviceIOFs(const std::vector<gpio::PinNumber>& gpio_offs, const DeviceID& device) {
	for(gpio::PinNumber gpio : gpio_offs) {
		m_embedded->closeIOF(gpio);
	}
	m_breadboard->removeDeviceObjects(device);
}

void Central::closePinIOFs(const std::list<gpio::PinNumber>& gpio_offs, gpio::PinNumber global) {
    for(gpio::PinNumber gpio : gpio_offs) {
        m_embedded->closeIOF(gpio);
    }
    m_breadboard->removePinObjects(global);
}

/* LOAD */

void Central::loadJSON(const QString& file) {
	emit(sendStatus("Loading config file " + file, 10000));
    if(!m_breadboard->loadConfigFile(file)) {
        std::cerr << "[Central] Could not open config file " << std::endl;
        return;
    }
	if(m_breadboard->isBreadboard()) {
		m_embedded->show();
	}
	else {
		m_embedded->hide();
	}
	if(m_embedded->gpioConnected()) {
		m_breadboard->connectionUpdate(true);
	}
}

void Central::saveJSON(const QString& file) {
	m_breadboard->saveConfigFile(file);
}

void Central::clearBreadboard() {
	emit(sendStatus("Clearing breadboard", 10000));
	m_breadboard->clear();
	m_embedded->show();
}

void Central::loadLUA(const std::string& dir, bool overwrite_integrated_devices) {
	m_breadboard->additionalLuaDir(dir, overwrite_integrated_devices);
}

/* Timer */

void Central::timerUpdate() {
 	bool reconnect = m_embedded->timerUpdate();
	if(reconnect) {
		emit(connectionUpdate(true));
	}
	if(m_embedded->gpioConnected()) {
		m_breadboard->timerUpdate(m_embedded->getState());
	}
}
