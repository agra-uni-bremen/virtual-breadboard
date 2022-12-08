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

void Breadboard::fromJSON(QJsonObject json) {
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

            if(device_desc.contains("pins") && device_desc["pins"].isArray()) {
                QJsonArray device_connections = device_desc["pins"].toArray();
                for(const auto& device_connection : device_connections) {
                    QJsonObject connection_obj = device_connection.toObject();
                    if(!connection_obj.contains("device_pin") || !connection_obj.contains("global_pin")) {
                        cerr << "[Breadboard] JSON entry for a connection is missing device pin or global pin" << endl;
                        continue;
                    }
                    gpio::PinNumber global = connection_obj["global_pin"].toInt();
                    Device::PIN_Interface::DevicePin device_pin = connection_obj["device_pin"].toInt();
                    string name = connection_obj["name"].toString("undefined").toStdString();
                    bool sync = connection_obj["synchronous"].toBool(false);
                    addPinToDevicePin(id, device_pin, global, name);
                    if(sync) setPinSync(global, device_pin, id, true);
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
}

QJsonObject Breadboard::toJSON() {
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
	QJsonArray devices_json;
	for(auto const& [id, device] : m_devices) {
		QJsonObject dev_json = device->toJSON();
        unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> pins = getPinsToDevicePins(id);
        QJsonArray pins_json;
        auto sync_pin = m_pin_channels.find(id);
        for(auto const& [device_pin, global] : pins) {
            if(!m_embedded->isPin(global)) continue;
            QJsonObject pin_json;
            pin_json["global_pin"] = global;
            pin_json["device_pin"] = (int) device_pin;
            pin_json["name"] = QString::fromStdString(getPinName(global));
            if(sync_pin!=m_pin_channels.end() && sync_pin->second.global_pin==global) {
                pin_json["synchronous"] = true;
            }
            pins_json.append(pin_json);
        }
        dev_json["pins"] = pins_json;
		devices_json.append(dev_json);
	}
	current_state["devices"] = devices_json;
	return current_state;
}
