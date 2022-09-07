#include "breadboard.h"

using namespace std;

constexpr bool debug_logging = false;

/* JSON */

void Breadboard::additionalLuaDir(string additional_device_dir, bool overwrite_integrated_devices) {
	if(additional_device_dir.size() != 0) {
		factory.scanAdditionalDir(additional_device_dir, overwrite_integrated_devices);
	}
}

bool Breadboard::loadConfigFile(QString file) {
	QFile confFile(file);
	if (!confFile.open(QIODevice::ReadOnly)) {
		cerr << "Could not open config file " << endl;
		return false;
	}

	QByteArray  raw_file = confFile.readAll();
	QJsonParseError error;
	QJsonDocument json_doc = QJsonDocument::fromJson(raw_file, &error);
	if(json_doc.isNull())
	{
		cerr << "Config seems to be invalid: ";
		cerr << error.errorString().toStdString() << endl;
		return false;
	}
	QJsonObject config_root = json_doc.object();

	QString bkgnd_path = ":/img/virtual_breadboard.png";
	QSize bkgnd_size = QSize(486, 233);
	if(config_root.contains("window") && config_root["window"].isObject()) {
		breadboard = false;
		QJsonObject window = config_root["window"].toObject();
		bkgnd_path = window["background"].toString(bkgnd_path);
		unsigned windowsize_x = window["windowsize"].toArray().at(0).toInt();
		unsigned windowsize_y = window["windowsize"].toArray().at(1).toInt();
		bkgnd_size = QSize(windowsize_x, windowsize_y);
	}

	QPixmap bkgnd(bkgnd_path);
	bkgnd = bkgnd.scaled(bkgnd_size, Qt::IgnoreAspectRatio);
	QPalette palette;
	palette.setBrush(QPalette::Window, bkgnd);
	this->setPalette(palette);
	this->setAutoFillBackground(true);

	setFixedSize(bkgnd_size);

	if(config_root.contains("devices") && config_root["devices"].isArray()) {
		QJsonArray device_descriptions = config_root["devices"].toArray();
		devices.reserve(device_descriptions.count());
		if(debug_logging)
			cout << "[config loader] reserving space for " << device_descriptions.count() << " devices." << endl;
		for(const QJsonValue& device_description : device_descriptions) {
			QJsonObject device_desc = device_description.toObject();
			const string& classname = device_desc["class"].toString("undefined").toStdString();
			const string& id = device_desc["id"].toString("undefined").toStdString();

			if(!factory.deviceExists(classname)) {
				cerr << "[config loader] device '" << classname << "' does not exist" << endl;
				continue;
			}
			if(devices.find(id) != devices.end()) {
				cerr << "[config loader] Another device with the ID '" << id << "' is already instatiated!" << endl;
				continue;
			}

			devices.emplace(id, factory.instantiateDevice(id, classname));
			Device* device = devices.at(id).get();
			device->fromJSON(device_desc);

			if(device_desc.contains("spi") && device_desc["spi"].isObject()) {
				const QJsonObject spi = device_desc["spi"].toObject();
				if(!spi.contains("cs_pin")) {
					cerr << "[config loader] config for device '" << classname << "' sets"
							" an SPI interface, but does not set a cs_pin." << endl;
					continue;
				}
				const gpio::PinNumber cs = spi["cs_pin"].toInt();
				const bool noresponse = spi["noresponse"].toBool(true);
				addSPI(cs, noresponse, device);
			}

			if(device_desc.contains("pins") && device_desc["pins"].isArray()) {
				QJsonArray pin_descriptions = device_desc["pins"].toArray();
				for (const QJsonValue& pin_desc : pin_descriptions) {
					const QJsonObject pin = pin_desc.toObject();
					if(!pin.contains("device_pin") ||
							!pin.contains("global_pin")) {
						cerr << "[config loader] config for device '" << classname << "' is"
								" missing device_pin or global_pin mappings" << endl;
						continue;
					}
					const gpio::PinNumber device_pin = pin["device_pin"].toInt();
					const gpio::PinNumber global_pin = pin["global_pin"].toInt();
					const bool synchronous = pin["synchronous"].toBool(false);
					const string pin_name = pin["name"].toString("undef").toStdString();

					addPin(synchronous, device_pin, global_pin, pin_name, device);
				}
			}

			if(device_desc.contains("graphics") && device_desc["graphics"].isObject()) {
				const QJsonObject graphics_desc = device_desc["graphics"].toObject();
				if(!(graphics_desc.contains("offs") && graphics_desc["offs"].isArray())) {
					cerr << "[config loader] config for device '" << classname << "' sets"
							" a graph buffer interface, but no valid offset given" << endl;
					continue;
				}
				const QJsonArray offs_desc = graphics_desc["offs"].toArray();
				if(offs_desc.size() != 2) {
					cerr << "[config loader] config for device '" << classname << "' sets"
							" a graph buffer interface, but offset is malformed (needs x,y, is size " << offs_desc.size() << ")" << endl;
					continue;
				}

				QPoint offs(offs_desc[0].toInt(), offs_desc[1].toInt());
				const unsigned scale = graphics_desc["scale"].toInt(1);

				addGraphics(offs, scale, device);

				// setting up the image buffer and its functions
				QImage& new_buffer = device_graphics.at(id).image;
				memset(new_buffer.bits(), 0x8F, new_buffer.sizeInBytes());

				device->graph->registerBuffer(new_buffer);
				// only called if lua implements the function
				device->graph->initializeBufferMaybe();
			}
		}

		if(debug_logging) {
			cout << "Instatiated devices:" << endl;
			for (auto& [id, device] : devices) {
				cout << "\t" << id << " of class " << device->getClass() << endl;

				if(device->pin)
					cout << "\t\timplements PIN" << endl;
				if(device->spi)
					cout << "\t\timplements SPI" << endl;
				if(device->conf)
					cout << "\t\timplements conf" << endl;
				if(device->graph)
					cout << "\t\timplements graphbuf (" << device->graph->getLayout().width << "x" <<
					device->graph->getLayout().height << " pixel)"<< endl;
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

	if(debug_logging)
		factory.printAvailableDevices();

	return true;
}

bool Breadboard::saveConfigFile(QString file) {
	return true;
}
