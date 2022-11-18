#include "breadboard.h"

#include <QJsonArray>
#include <QJsonDocument>

using namespace std;

constexpr bool debug_logging = false;

/* JSON */

void Breadboard::setBackground(QString path) {
	bkgnd_path = path;
	bkgnd = QPixmap(bkgnd_path);
	updateBackground();
	setAutoFillBackground(true);
}

void Breadboard::updateBackground() {
   QPixmap new_bkgnd = bkgnd.scaled(size(), Qt::IgnoreAspectRatio);
   QPalette palette;
   palette.setBrush(QPalette::Window, new_bkgnd);
   setPalette(palette);
}

void Breadboard::additionalLuaDir(const string& additional_device_dir, bool overwrite_integrated_devices) {
	if(!additional_device_dir.empty()) {
		factory.scanAdditionalDir(additional_device_dir, overwrite_integrated_devices);
	}
}

bool Breadboard::loadConfigFile(const QString& file) {
    QFile confFile(file);
    if (!confFile.open(QIODevice::ReadOnly)) {
        std::cerr << "[Breadboard] Could not open config file " << std::endl;
        return false;
    }

    QByteArray raw_file = confFile.readAll();
    QJsonParseError error;
    QJsonDocument json_doc = QJsonDocument::fromJson(raw_file, &error);
    if(json_doc.isNull())
    {
        std::cerr << "[Breadboard] Config seems to be invalid: ";
        std::cerr << error.errorString().toStdString() << std::endl;
        return false;
    }
    QJsonObject json = json_doc.object();

    clear();

    if(json.contains("window") && json["window"].isObject()) {
        QJsonObject window = json["window"].toObject();
        unsigned windowsize_x = window["windowsize"].toArray().at(0).toInt();
        unsigned windowsize_y = window["windowsize"].toArray().at(1).toInt();

        setMinimumSize(windowsize_x, windowsize_y);
        setBackground(window["background"].toString());
    }

	if(json.contains("devices") && json["devices"].isArray()) {
		QJsonArray device_descriptions = json["devices"].toArray();
		devices.reserve(device_descriptions.count());
		if(debug_logging)
			cout << "[Breadboard] reserving space for " << device_descriptions.count() << " devices." << endl;
		for(const auto& device_description : device_descriptions) {
			QJsonObject device_desc = device_description.toObject();
			const string& classname = device_desc["class"].toString("undefined").toStdString();
			const string& id = device_desc["id"].toString("undefined").toStdString();
			QJsonObject graphics = device_desc["graphics"].toObject();
			const QJsonArray offs_desc = graphics["offs"].toArray();
			QPoint offs(offs_desc[0].toInt(), offs_desc[1].toInt());

			if(!addDevice(classname, getDistortedPosition(offs), id)) {
				cerr << "[Breadboard] could not create device '" << classname << "'." << endl;
				continue;
			}
			auto device = devices.find(id);
			if(device == devices.end()) continue;

			device->second->fromJSON(device_desc);

			if(device_desc.contains("spi") && device_desc["spi"].isObject()) {
				const QJsonObject spi = device_desc["spi"].toObject();
				if(!spi.contains("cs_pin")) {
					cerr << "[Breadboard] config for device '" << classname << "' sets"
							" an SPI interface, but does not set a cs_pin." << endl;
					continue;
				}
				const gpio::PinNumber cs = spi["cs_pin"].toInt();
				const bool noresponse = spi["noresponse"].toBool(true);
				registerSPI(cs, noresponse, device->second.get());
			}

			if(device_desc.contains("pins") && device_desc["pins"].isArray()) {
				QJsonArray pin_descriptions = device_desc["pins"].toArray();
				for (const auto& pin_desc : pin_descriptions) {
					const QJsonObject pin = pin_desc.toObject();
					if(!pin.contains("device_pin") ||
							!pin.contains("global_pin")) {
						cerr << "[Breadboard] config for device '" << classname << "' is"
								" missing device_pin or global_pin mappings" << endl;
						continue;
					}
					const gpio::PinNumber device_pin = pin["device_pin"].toInt();
					const gpio::PinNumber global_pin = pin["global_pin"].toInt();
					const bool synchronous = pin["synchronous"].toBool(false);
					const string pin_name = pin["name"].toString("undef").toStdString();

					registerPin(synchronous, device_pin, global_pin, pin_name, device->second.get());
				}
			}
		}

		if(debug_logging) {
			cout << "Instatiated devices:" << endl;
			for (auto& [id, device] : devices) {
				cout << "\t" << id << " of class " << device->getClass() << endl;
				cout << "\t minimum buffer size " << device->getBuffer().width() << "x" << device->getBuffer().height() << " pixel." << endl;

				if(device->pin)
					cout << "\t\timplements PIN" << endl;
				if(device->spi)
					cout << "\t\timplements SPI" << endl;
				if(device->conf)
					cout << "\t\timplements conf" << endl;
			}

			cout << "Active pin connections:" << endl;
			cout << "\tReading (async): " << reading_connections.size() << endl;
			for(auto& conn : reading_connections){
				cout << "\t\t" << conn.dev->getID() << " (" << conn.name << "): global pin " << (int)conn.global_pin <<
						" to device pin " << (int)conn.device_pin << endl;
			}
			cout << "\tReading (synchronous): " << pin_channels.size() << endl;
			for(auto& conn : pin_channels){
				cout << "\t\t" << conn.first << ": global pin " << (int)conn.second.global_pin << endl;
			}
			cout << "\tWriting: " << writing_connections.size() << endl;
			for(auto& conn : writing_connections){
				cout << "\t\t" << conn.dev->getID() << " (" << conn.name << "): device pin " << (int)conn.device_pin <<
						" to global pin " << (int)conn.global_pin << endl;
			}
		}
	}

	return true;
}

bool Breadboard::saveConfigFile(const QString& file) {
	QFile confFile(file);
	if(!confFile.open(QIODevice::WriteOnly)) {
		cerr << "[Breadboard] Could not open config file" << endl;
		cerr << confFile.errorString().toStdString() << endl;
		return false;
	}
	QJsonObject current_state;
	if(!isBreadboard()) {
		QJsonObject window;
		window["background"] = bkgnd_path;
		QJsonArray windowsize;
		windowsize.append(minimumSize().width());
		windowsize.append(minimumSize().height());
		window["windowsize"] = windowsize;
		current_state["window"] = window;
	}
	QJsonArray devices_json;
	for(auto const& [id, device] : devices) {
		QJsonObject dev_json = device->toJSON();
		auto spi = spi_channels.find(id);
		if(spi != spi_channels.end()) {
			QJsonObject spi_json;
			spi_json["cs_pin"] = (int) spi->second.global_pin;
			spi_json["noresponse"] = spi->second.noresponse;
			dev_json["spi"] = spi_json;
		}
		QJsonArray pins_json;
		for(PinMapping w : writing_connections) {
			if(w.dev->getID() == id) {
				QJsonObject pin_json;
				pin_json["device_pin"] = (int) w.device_pin;
				pin_json["global_pin"] = (int) w.global_pin;
				pin_json["name"] = QString::fromStdString(w.name);
				pins_json.append(pin_json);
			}
		}
		for(PinMapping w : reading_connections) {
			if(w.dev->getID() == id) {
				QJsonObject pin_json;
				pin_json["device_pin"] = (int) w.device_pin;
				pin_json["global_pin"] = (int) w.global_pin;
				pin_json["name"] = QString::fromStdString(w.name);
				pins_json.append(pin_json);
			}
		}
		auto pin = pin_channels.find(id);
		if(pin != pin_channels.end()) {
			QJsonObject pin_json;
			pin_json["synchronous"] = true;
			pin_json["device_pin"] = pin->second.device_pin;
			pin_json["global_pin"] = pin->second.global_pin;
			pins_json.append(pin_json);
		}
		if(!pins_json.empty()) {
			dev_json["pins"] = pins_json;
		}
		devices_json.append(dev_json);
	}
	current_state["devices"] = devices_json;
	QJsonDocument doc(current_state);
	confFile.write(doc.toJson());
	return true;
}
