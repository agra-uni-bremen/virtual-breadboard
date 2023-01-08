#include "device.hpp"

#include <breadboard/constants.h>

#include <QKeySequence>
#include <QJsonArray>
#include <QPixmap>

#include <iostream>

Device::Device(const DeviceID& id) : m_id(id) {}

Device::~Device() = default;

const DeviceID& Device::getID() const {
	return m_id;
}

void Device::fromJSON(QJsonObject json) {
	if(json.contains("conf") && json["conf"].isObject()) {
		if(!m_conf) {
			std::cerr << "[Device] config for device '" << getClass() << "' sets"
					" a Config Interface, but device does not implement it" << std::endl;
		}
		else {
			QJsonObject conf_obj = json["conf"].toObject();
			Config config;
			for(QJsonObject::iterator conf_it = conf_obj.begin(); conf_it != conf_obj.end(); conf_it++) {
				if(conf_it.value().isBool()) {
					config.emplace(conf_it.key().toStdString(), ConfigElem{conf_it.value().toBool()});
				}
				else if(conf_it.value().isDouble()) {
					config.emplace(conf_it.key().toStdString(), ConfigElem{static_cast<int64_t>(conf_it.value().toInt())});
				}
				else if(conf_it.value().isString()) {
					QByteArray value_bytes = conf_it.value().toString().toLocal8Bit();
					config.emplace(conf_it.key().toStdString(), ConfigElem{value_bytes.data()});
				}
				else {
					std::cerr << "[Device] Invalid conf element type" << std::endl;
				}
			}
			m_conf->setConfig(config);
		}
	}

	if(json.contains("keybindings") && json["keybindings"].isArray()) {
		if(!m_input) {
			std::cerr << "[Device] config for device '" << getClass() << "' sets"
					" keybindings, but device does not implement input interface" << std::endl;
		}
		else {
			QJsonArray bindings = json["keybindings"].toArray();
			Keys keys;
			for(const auto & binding : bindings) {
				QKeySequence binding_sequence = QKeySequence(binding.toString());
				if(binding_sequence.count()) {
					keys.emplace(binding_sequence[0]);
				}
			}
			m_input->setKeys(keys);
		}
	}

	if(json.contains("graphics") && json["graphics"].isObject()) {
		QJsonObject graphics = json["graphics"].toObject();
		if(graphics.contains("scale") && graphics["scale"].isDouble()) {
			unsigned scale = graphics["scale"].toInt();
			setScale(scale);
		}
	}
	else {
		std::cerr << "[Device] Config does not specify scale and position correctly." << std::endl;
	}
}

QJsonObject Device::toJSON() {
	QJsonObject json;
	json["class"] = QString::fromStdString(getClass());
	json["id"] = QString::fromStdString(getID());

	QJsonObject graph_json;
	QJsonArray offs_json;
	offs_json.append(getBuffer().offset().x());
	offs_json.append(getBuffer().offset().y());
	graph_json["offs"] = offs_json;
	graph_json["scale"] = (int) getScale();
	json["graphics"] = graph_json;

	if(m_conf) {
		QJsonObject conf_json;
		for(const auto& [desc, elem] : m_conf->getConfig()) {
			if(elem.type == ConfigElem::Type::integer) {
				conf_json[QString::fromStdString(desc)] = (int) elem.value.integer;
			}
			else if(elem.type == ConfigElem::Type::boolean) {
				conf_json[QString::fromStdString(desc)] = elem.value.boolean;
			}
			else if(elem.type == ConfigElem::Type::string) {
				conf_json[QString::fromStdString(desc)] = QString::fromLocal8Bit(elem.value.string);
			}
		}
		json["conf"] = conf_json;
	}
	if(m_input) {
		Keys keys = m_input->getKeys();
		if(!keys.empty()) {
			QJsonArray keybindings_json;
			for(const Key& key : keys) {
				keybindings_json.append(QJsonValue(QKeySequence(key).toString()));
			}
			json["keybindings"] = keybindings_json;
		}
	}
	return json;
}

Device::Layout Device::getLayout() {
	   return {};
}

void Device::initializeBuffer() {
	QPoint offset = m_buffer.offset();
	QSize size = m_buffer.size();
	m_buffer = QImage(":/img/default.png");
	m_buffer = m_buffer.scaled(size);
	m_buffer.setOffset(offset);
}

void Device::createBuffer(unsigned iconSizeMinimum, QPoint offset) {
	if(!m_buffer.isNull()) return;
	Layout layout = getLayout();
	m_buffer = QImage(layout.width * iconSizeMinimum, layout.height * iconSizeMinimum, QImage::Format_RGBA8888);
	m_buffer.setOffset(offset);
	initializeBuffer();
}

void Device::setScale(unsigned scale) {
	m_scale = scale;
}

unsigned Device::getScale() const { return m_scale; }

QImage& Device::getBuffer() {
	return m_buffer;
}

void Device::setPixel(const Xoffset x, const Yoffset y, Pixel p) {
	auto* img = getBuffer().bits();
	if(x >= m_buffer.width() || y >= m_buffer.height()) {
		std::cerr << "[Device] WARN: device write accessing graphbuffer out of bounds!" << std::endl;
		return;
	}
	const auto offs = (y * m_buffer.width() + x) * 4; // heavily depends on rgba8888
	img[offs+0] = p.r;
	img[offs+1] = p.g;
	img[offs+2] = p.b;
	img[offs+3] = p.a;
}

Device::Pixel Device::getPixel(const Xoffset x, const Yoffset y) {
	auto* img = getBuffer().bits();
	if(x >= m_buffer.width() || y >= m_buffer.height()) {
		std::cerr << "[Device] WARN: device read accessing graphbuffer out of bounds!" << std::endl;
		return Pixel{0,0,0,0};
	}
	const auto& offs = (y * m_buffer.width() + x) * 4; // heavily depends on rgba8888
	return Pixel{
		static_cast<uint8_t>(img[offs+0]),
				static_cast<uint8_t>(img[offs+1]),
				static_cast<uint8_t>(img[offs+2]),
				static_cast<uint8_t>(img[offs+3])
	};
}

Device::PIN_Interface::~PIN_Interface() = default;
Device::SPI_Interface::~SPI_Interface() = default;
Device::Config_Interface::~Config_Interface() = default;
Device::Input_Interface::~Input_Interface() = default;

void Device::Input_Interface::setKeys(Keys bindings) {
	keybindings = bindings;
}

Keys Device::Input_Interface::getKeys() const {
	return keybindings;
}
