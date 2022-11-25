#include "breadboard.h"

#include <QJsonArray>
#include <QJsonDocument>

using namespace std;

constexpr bool debug_logging = true;

/* JSON */

void Breadboard::setBackground(QString path) {
    m_bkgnd_path = path;
    m_bkgnd = QPixmap(m_bkgnd_path);
	updateBackground();
	setAutoFillBackground(true);
}

void Breadboard::updateBackground() {
   QPixmap new_bkgnd = m_bkgnd.scaled(size(), Qt::IgnoreAspectRatio);
   QPalette palette;
   palette.setBrush(QPalette::Window, new_bkgnd);
   setPalette(palette);
}

void Breadboard::additionalLuaDir(const string& additional_device_dir, bool overwrite_integrated_devices) {
	if(!additional_device_dir.empty()) {
		m_factory.scanAdditionalDir(additional_device_dir, overwrite_integrated_devices);
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
		m_devices.reserve(device_descriptions.count());
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
			auto device = m_devices.find(id);
			if(device == m_devices.end()) continue;

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

			if(device_desc.contains("pin") && device_desc["pin"].isObject()) {
                const QJsonObject pin = device_desc["pin"].toObject();
                if(!pin.contains("device_pin") || !pin.contains("global_pin")) {
                    cerr << "[Breadboard] config for device '" << classname << "' is missing device_pin or "
                                                                               "global_pin mappings" << endl;
                    continue;
                }
                const gpio::PinNumber global_pin = pin["global_pin"].toInt();
                const Device::PIN_Interface::DevicePin device_pin = pin["device_pin"].toInt();
                registerPin(true, device_pin, global_pin, "undef", device->second.get());
			}
		}

        if(json.contains("connections") && json["connections"].isArray()) {
            QJsonArray connections = json["connections"].toArray();
            for(auto const& connection : connections) {
                QJsonObject row_obj = connection.toObject();
                if(!row_obj.contains("row") || !row_obj["row"].isDouble()) {
                    cerr << "[Breadboard] Connection entry is missing row number" << endl;
                    continue;
                }
                Row row = row_obj["row"].toInt();
                Connection row_connections;
                if(row_obj.contains("devices") && row_obj["devices"].isArray()) {
                    QJsonArray devices = row_obj["devices"].toArray();
                    for(auto const& device : devices) {
                        QJsonObject device_obj = device.toObject();
                        if(!device_obj.contains("id") || !device_obj["id"].isString() || !device_obj.contains("pin") || !device_obj["pin"].isDouble()) {
                            cerr << "[Breadboard] Connection entry for row " << row << " is missing pin or id for a device." << endl;
                            continue;
                        }
                        DeviceID id = device_obj["id"].toString().toStdString();
                        Device::PIN_Interface::DevicePin pin = device_obj["pin"].toInt();
                        row_connections.devices.push_back(DeviceConnection{.id=id,.pin=pin});
                    }
                }
                if(row_obj.contains("pins") && row_obj["pins"].isArray()) { // TODO create and use better addConnection method (instead of registerPIN)
                    QJsonArray pins = row_obj["pins"].toArray();
                    for(auto const& pin : pins) {
                        QJsonObject pin_obj = pin.toObject();
                        if(!pin_obj.contains("global_pin") || !pin_obj["global_pin"].isDouble() || !pin_obj.contains("name") || !pin_obj["name"].isString()) {
                            cerr << "[Breadboard] Connection entry for row " << row << " is missing global pin or name for a pin." << endl;
                            continue;
                        }
                        gpio::PinNumber global_pin = pin_obj["global_pin"].toInt();
                        std::string name = pin_obj["name"].toString().toStdString();
                        Device::PIN_Interface::Dir dir = static_cast<Device::PIN_Interface::Dir>(pin_obj["dir"].toInt());
                        Index index = pin_obj["index"].toInt();
                        row_connections.pins.push_back(PinConnection{.gpio_offs = translatePinToGpioOffs(global_pin),
                                                                     .global_pin = global_pin,
                                                                     .name = name,
                                                                     .dir = dir,
                                                                     .index = index});
                    }
                }
                m_connections.emplace(row, row_connections);
            }
        }

		if(debug_logging) {
			cout << "Instatiated devices:" << endl;
			for (auto& [id, device] : m_devices) {
				cout << "\t" << id << " of class " << device->getClass() << endl;
				cout << "\t minimum buffer size " << device->getBuffer().width() << "x" << device->getBuffer().height() << " pixel." << endl;

				if(device->m_pin)
					cout << "\t\timplements PIN" << endl;
				if(device->m_spi)
					cout << "\t\timplements SPI" << endl;
				if(device->m_conf)
					cout << "\t\timplements conf" << endl;
			}

			cout << "Active pin connections:" << endl;
            for(auto const& [row, connection] : m_connections) {
                cout << "\tRow " << row << endl;
                cout << "\t\tDevices: " << connection.devices.size() << endl;
                for(const DeviceConnection& device_obj : connection.devices) {
                    cout << "\t\t\tID: " << device_obj.id << ", device pin " << (int)device_obj.pin << endl;
                }
                cout << "\t\tPins: " << connection.pins.size() << endl;
                for(const PinConnection& pin_obj : connection.pins) {
                    cout << "\t\t\t" << pin_obj.name << "(index: " << pin_obj.index << "): global pin "
                    << (int)pin_obj.global_pin << " is "
                    << (pin_obj.dir == Device::PIN_Interface::Dir::output ? "output" : "input") << endl;
                }
            }
			cout << "\tReading (synchronous): " << m_pin_channels.size() << endl;
			for(auto& conn : m_pin_channels){
				cout << "\t\t" << conn.first << ": global pin " << (int)conn.second.global_pin << endl;
			}

            cout << "\tSPI: " << m_spi_channels.size() << endl;
            for(auto& conn : m_spi_channels) {
                cout << "\t\t" << conn.first << ": global pin " << (int)conn.second.global_pin << endl;
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
		window["background"] = m_bkgnd_path;
		QJsonArray windowsize;
		windowsize.append(minimumSize().width());
		windowsize.append(minimumSize().height());
		window["windowsize"] = windowsize;
		current_state["window"] = window;
	}
    QJsonArray connections;
    for(auto const& [row, connection] : m_connections) {
        QJsonObject row_obj;
        row_obj["row"] = (int)row;
        QJsonArray row_devices;
        for(const DeviceConnection& device : connection.devices) {
            QJsonObject device_obj;
            device_obj["id"] = QString::fromStdString(device.id);
            device_obj["pin"] = (int)device.pin;
            row_devices.append(device_obj);
        }
        row_obj["devices"] = row_devices;
        QJsonArray row_pins;
        for(const PinConnection& pin : connection.pins) {
            QJsonObject pin_obj;
            pin_obj["global_pin"] = (int) pin.global_pin;
            pin_obj["name"] = QString::fromStdString(pin.name);
            pin_obj["dir"] = (int) pin.dir;
            pin_obj["index"] = (int) pin.index;
            row_pins.append(pin_obj);
        }
        row_obj["pins"] = row_pins;
    }
    current_state["connections"] = connections;
	QJsonArray devices_json;
	for(auto const& [id, device] : m_devices) {
		QJsonObject dev_json = device->toJSON();
		auto spi = m_spi_channels.find(id);
		if(spi != m_spi_channels.end()) {
			QJsonObject spi_json;
			spi_json["cs_pin"] = (int) spi->second.global_pin;
			spi_json["noresponse"] = spi->second.noresponse;
			dev_json["spi"] = spi_json;
		}
		auto pin = m_pin_channels.find(id);
		if(pin != m_pin_channels.end()) {
			QJsonObject pin_json;
			pin_json["device_pin"] = (int) pin->second.device_pin;
			pin_json["global_pin"] = (int) pin->second.global_pin;
            dev_json["pin"] = pin_json;
		}
		devices_json.append(dev_json);
	}
	current_state["devices"] = devices_json;
	QJsonDocument doc(current_state);
	confFile.write(doc.toJson());
	return true;
}
