#include "central.h"

#include <QVBoxLayout>
#include <QTimer>
#include <QJsonParseError>

/* Constructor */

Central::Central(const std::string& host, const std::string& port, QWidget *parent) : QWidget(parent) {
	m_breadboard = new Breadboard();
	m_embedded = new Embedded(host, port);
	m_breadboard->setEmbedded(m_embedded);

	auto *layout = new QVBoxLayout(this);
	layout->addWidget(m_embedded);
	layout->addWidget(m_breadboard);

	m_overlay = new Overlay(this);
	m_overlay->resize(width(), height());

	m_breadboard->stackUnder(m_overlay);
	m_breadboard->setOverlay(m_overlay);
	m_embedded->stackUnder(m_overlay);

	auto *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &Central::timerUpdate);
	timer->start(250);

	connect(m_embedded, &Embedded::connectionLost, [this](){
		emit(connectionUpdate(false));
	});
	connect(m_embedded, &Embedded::pinSettingsChanged, this, &Central::pinSettingsChanged);
	connect(this, &Central::connectionUpdate, m_breadboard, &Breadboard::connectionUpdate);
}

Central::~Central() = default;

void Central::resizeEvent(QResizeEvent*) {
	m_overlay->resize(width(), height());
}

void Central::destroyConnection() {
	m_embedded->destroyConnection();
}

bool Central::toggleDebug() {
	return m_breadboard->toggleDebug();
}

void Central::openEmbeddedOptions() {
	m_embedded->openPinOptions();
}

/* LOAD */

void Central::loadJSON(const QString& file) {
	emit(sendStatus("Loading config file " + file, 10000));
	QFile confFile(file);
	if (!confFile.open(QIODevice::ReadOnly)) {
		std::cerr << "[Central] Could not open config file " << std::endl;
		return;
	}

	QByteArray raw_file = confFile.readAll();
	QJsonParseError error;
	QJsonDocument json_doc = QJsonDocument::fromJson(raw_file, &error);
	if(json_doc.isNull())
	{
		std::cerr << "[Central] Config seems to be invalid: ";
		std::cerr << error.errorString().toStdString() << std::endl;
		return;
	}

	QJsonObject json = json_doc.object();
	if(!json.contains("embedded") || !json["embedded"].isObject()) {
		std::cerr << "[Central] Config file missing/malformed entry for embedded" << std::endl;
		return;
	}
	if(!json.contains("breadboard") || !json["breadboard"].isObject()) {
		std::cerr << "[Central] Config file missing/malformed entry for breadboard" << std::endl;
		return;
	}
	m_overlay->setCables(QMap<QPoint,QPoint>());
	m_embedded->fromJSON(json["embedded"].toObject());
	m_breadboard->fromJSON(json["breadboard"].toObject());
	if(m_breadboard->isBreadboard()) {
		m_embedded->show();
	}
	else {
		m_embedded->hide();
	}
	m_breadboard->updateOverlay();
}

void Central::saveJSON(const QString& file) {
	QFile confFile(file);
	if(!confFile.open(QIODevice::WriteOnly)) {
		std::cerr << "[Breadboard] Could not open config file" << std::endl;
		std::cerr << confFile.errorString().toStdString() << std::endl;
		return;
	}
	QJsonObject bb = m_breadboard->toJSON();
	QJsonObject embedded = m_embedded->toJSON();
	QJsonObject current_state;
	current_state["breadboard"] = bb;
	current_state["embedded"] = embedded;
	QJsonDocument doc(current_state);
	confFile.write(doc.toJson());
}

void Central::clearBreadboard() {
	emit(sendStatus("Clearing breadboard", 10000));
	m_breadboard->clear();
	m_embedded->show();
}

void Central::loadLUA(const std::string& dir, bool overwrite_integrated_devices) {
	m_breadboard->additionalLuaDir(dir, overwrite_integrated_devices);
}

void Central::pinSettingsChanged(const std::list<std::pair<gpio::PinNumber, IOF>>& iofs) {
	for(const auto& [global, iof] : iofs) {
		if(iof.type == IOFType::SPI) {
			m_breadboard->setSPI(global, iof.active);
		}
	}
	m_breadboard->printConnections();
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
