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
		}

        if(json.contains("raster") && json["raster"].isArray()) {
            QJsonArray raster = json["raster"].toArray();
            for(auto const& row_it : raster) {
                QJsonObject row_obj = row_it.toObject();
                if(!row_obj.contains("row") || !row_obj["row"].isDouble()) {
                    cerr << "[Breadboard] Connection entry is missing row number" << endl;
                    continue;
                }
                Row row = row_obj["row"].toInt();
                if(!row_obj.contains("pins") || !row_obj["pins"].isArray()) {
                    cerr << "[Breadboard] Config entry for row " << row << " was emtpy" << endl;
                    continue;
                }
                QJsonArray pin_array = row_obj["pins"].toArray();
                for(auto const& pin_it : pin_array) {
                    QJsonObject pin_obj = pin_it.toObject();
                    if(!pin_obj.contains("global_pin") || !pin_obj.contains("name") || !pin_obj.contains("index")) {
                        cerr << "[Breadboard] Pin entry in list for row " << row << " is missing information (name, index, global)" << endl;
                        continue;
                    }
                    gpio::PinNumber global = pin_obj["global_pin"].toInt();
                    std::string name = pin_obj["name"].toString().toStdString();
                    Index index = pin_obj["index"].toInt();
                    if(pin_obj.contains("spi") && pin_obj["spi"].isObject()) {
                        QJsonObject spi_obj = pin_obj["spi"].toObject();
                        if(spi_obj.contains("noresponse")) {
                            bool noresponse = spi_obj["noresponse"].toBool();
                            addSPIToRow(row, index, global, name, noresponse);
                        }
                        continue;
                    }
                    addPinToRow(row, index, global, name);
                    if(pin_obj.contains("sync") && pin_obj["sync"].isArray()) {
                        QJsonArray sync_devices = pin_obj["sync"].toArray();
                        for(auto const& sync_it : sync_devices) {
                            QJsonObject sync = sync_it.toObject();
                            if(!sync.contains("device_id") || !sync.contains("device_pin")) {
                                cerr << "[Breadboard] Synchronous devices list entry for pin " << (int) global << " is missing device ID or device pin" << endl;
                                continue;
                            }
                            DeviceID device_id = sync["device_id"].toString().toStdString();
                            Device::PIN_Interface::DevicePin device_pin = sync["device_pin"].toInt();
                            setPinSync(global, device_pin, device_id, true);
                        }
                    }
                }

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

			printConnections();
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
    QJsonArray raster;
    for(auto const& [row, content] : m_raster) {
        QJsonObject row_obj;
        row_obj["row"] = (int)row;
        QJsonArray row_pins;
        for(const PinConnection& pin : content.pins) {
            QJsonObject pin_obj;
            pin_obj["global_pin"] = (int) pin.global_pin;
            pin_obj["name"] = QString::fromStdString(pin.name);
            pin_obj["index"] = (int) pin.index;
            QJsonObject pin_spi;
            for (auto const& [device, req] : m_spi_channels) {
                if(req.global_pin == pin.global_pin) {
                    pin_spi["noresponse"] = req.noresponse;
                    break;
                }
            }
            if(!pin_spi.empty()) {
                pin_obj["spi"] = pin_spi;
            }
            QJsonArray pin_sync_devices;
            for(auto const& [device, req] : m_pin_channels) {
                if(req.global_pin == pin.global_pin) {
                    QJsonObject sync_device;
                    sync_device["device_id"] = QString::fromStdString(device);
                    sync_device["device_pin"] = (int) req.device_pin;
                    pin_sync_devices.append(sync_device);
                }
            }
            if(!pin_sync_devices.empty()) {
                pin_obj["sync"] = pin_sync_devices;
            }
            row_pins.append(pin_obj);
        }
        row_obj["pins"] = row_pins;
    }
    current_state["raster"] = raster;
	QJsonArray devices_json;
	for(auto const& [id, device] : m_devices) {
		QJsonObject dev_json = device->toJSON();
		devices_json.append(dev_json);
	}
	current_state["devices"] = devices_json;
	QJsonDocument doc(current_state);
	confFile.write(doc.toJson());
	return true;
}
