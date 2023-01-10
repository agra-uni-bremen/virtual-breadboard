#include "breadboard.h"

#include <QTimer>
#include <QInputDialog>

using namespace std;

Breadboard::Breadboard() : QWidget() {
	setFocusPolicy(Qt::StrongFocus);
	setAcceptDrops(true);
	setMouseTracking(true);

	auto *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, [this]{update();});
	timer->start(1000/30);

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested, this, &Breadboard::openContextMenu);
	m_devices_menu = new QMenu(this);
	auto *delete_device = new QAction("Delete");
	connect(delete_device, &QAction::triggered, this, &Breadboard::removeActiveDevice);
	m_devices_menu->addAction(delete_device);
	auto *scale_device = new QAction("Scale");
	connect(scale_device, &QAction::triggered, this, &Breadboard::scaleActiveDevice);
	m_devices_menu->addAction(scale_device);
	auto *open_configuration = new QAction("Device Configuration");
	connect(open_configuration, &QAction::triggered, this, &Breadboard::openDeviceConfiguration);
	m_devices_menu->addAction(open_configuration);

	m_pin_menu = new QMenu(this);
	auto *delete_pin = new QAction("Remove");
	connect(delete_pin, &QAction::triggered, this, &Breadboard::removePinTriggered);
	m_pin_menu->addAction(delete_pin);

	m_device_configuration = new DeviceConfiguration(this);
	connect(m_device_configuration, &DeviceConfiguration::keysChanged, this, &Breadboard::updateKeybinding);
	connect(m_device_configuration, &DeviceConfiguration::configChanged, this, &Breadboard::updateConfig);
	connect(m_device_configuration, &DeviceConfiguration::pinsChanged, this, &Breadboard::updatePins);
	m_error_dialog = new QErrorMessage(this);

	m_bb_menu = new QMenu(this);
	QMenu *add_device = m_bb_menu->addMenu("Add Device");
	for(const DeviceClass& device : m_factory.getAvailableDevices()) {
		auto *device_action = new QAction(QString::fromStdString(device));
		connect(device_action, &QAction::triggered, [this, device](){
			addDevice(device, mapFromGlobal(m_bb_menu->pos()));
		});
		add_device->addAction(device_action);
	}

	setMinimumSize(DEFAULT_SIZE);
	setBackground(DEFAULT_PATH);
}

Breadboard::~Breadboard() = default;

bool Breadboard::isBreadboard() { return m_bkgnd_path == DEFAULT_PATH; }
bool Breadboard::toggleDebug() {
	m_debugmode = !m_debugmode;
	return m_debugmode;
}

void Breadboard::setEmbedded(Embedded *embedded) {
	this->m_embedded = embedded;
}

void Breadboard::setOverlay(Overlay *overlay) {
	this->m_overlay = overlay;
}

/* DEVICE */

QPoint Breadboard::checkDevicePosition(const DeviceID& id, const QImage& buffer, int scale, QPoint position, QPoint hotspot) {
	QPoint upper_left = position - hotspot;
	QRect device_bounds = QRect(upper_left, getDistortedGraphicBounds(buffer, scale).size());

	if(isBreadboard()) {
		if(getRasterBounds().intersects(device_bounds)) {
			QPoint dropPositionRaster = getAbsolutePosition(getRow(position), getIndex(position));
			upper_left = dropPositionRaster - getDeviceAbsolutePosition(getDeviceRow(hotspot),
					getDeviceIndex(hotspot));
			device_bounds = QRect(upper_left, device_bounds.size());
		} else {
			cerr << "[Breadboard] Device position invalid: Device should be at least partially on raster." << endl;
			return {-1,-1};
		}
	}

	if(!rect().contains(device_bounds)) {
		cerr << "[Breadboard] Device position invalid: Device may not leave window view." << endl;
		return {-1,-1};
	}

	for(const auto& [id_it, device_it] : m_devices) {
		if(id_it == id) continue;
		m_lua_access.lock();
		auto current_buffer = device_it->getBuffer();
		auto current_scale = device_it->getScale();
		m_lua_access.unlock();
		if(getDistortedGraphicBounds(current_buffer,
				current_scale).intersects(device_bounds)) {
			cerr << "[Breadboard] Device position invalid: Overlaps with other device." << endl;
			return {-1,-1};
		}
	}
	return getMinimumPosition(upper_left);
}

bool Breadboard::moveDevice(const DeviceID& device_id, QPoint position, QPoint hotspot) {
	auto device = m_devices.find(device_id);
	if(device == m_devices.end()) return false;
	m_lua_access.lock();
	unsigned scale = device->second->getScale();
	if(!scale) scale = 1;
	auto buffer = device->second->getBuffer();
	m_lua_access.unlock();
	QPoint upper_left = checkDevicePosition(device_id, buffer, scale, position, hotspot);

	if(upper_left.x()<0) {
		cerr << "[Breadboard] New device position is invalid." << endl;
		return false;
	}

	m_lua_access.lock();
	device->second->getBuffer().setOffset(upper_left);
	device->second->setScale(scale);
	m_lua_access.unlock();

	if(!device->second->m_pin) {
		return true;
	}
	m_lua_access.lock();
	Device::PIN_Interface::PinLayout device_layout = device->second->m_pin->getPinLayout();
	m_lua_access.unlock();
	for(const auto& [device_pin, desc] : device_layout) {
		Row old_row = getRowForDevicePin(device_id, device_pin);
		Row new_row;
		if(isBreadboard()) {
			if(isValidRasterRow(old_row)) {
				auto old_row_obj = m_raster.find(old_row);
				if(old_row_obj!=m_raster.end()) {
					old_row_obj->second.devices.remove_if([device_id](const DeviceConnection& c){return c.id == device_id;});
					for(const auto& pin : old_row_obj->second.pins) {
						removePinForDevice(pin.global_pin, device_id);
						removeSPIForDevice(pin.global_pin, device_id);
					}
				}
			}
			QPoint pos_on_device = getDeviceAbsolutePosition(desc.row, desc.index);
			QPoint pos_on_raster = getDistortedPosition(upper_left) + pos_on_device;
			new_row = getRow(pos_on_raster);
		}
		else {
			if(isValidRasterRow(old_row)) {
				continue;
			}
			else {
				new_row = getNewRowNumber();
			}
		}
		if(!isValidRasterRow(new_row)) continue;
		DeviceConnection new_connection = DeviceConnection{
			.id = device_id,
			.pin = device_pin
		};
		auto row_obj = m_raster.find(new_row);
		if(row_obj != m_raster.end()) {
			row_obj->second.devices.push_back(new_connection);
		} else {
			RowContent new_content;
			new_content.devices.push_back(new_connection);
			m_raster.emplace(new_row, new_content);
		}
		createRowConnections(new_row);
	}
	return true;
}

bool Breadboard::addDevice(const DeviceClass& classname, QPoint pos, DeviceID id) {
	if(id.empty()) {
		std::set<unsigned> used_ids;
		for(const auto& [id_it, device_it] : m_devices) {
			if(find_if(id_it.begin(), id_it.end(), [](unsigned char c){return !isdigit(c);}) == id_it.end()) {
				used_ids.insert(stoi(id_it));
			}
		}
		unsigned id_int = 0;
		for(unsigned used_id : used_ids) {
			if(used_id > id_int)
				break;
			id_int++;
		}
		id = std::to_string(id_int);
	}

	if(id.empty()) {
		cerr << "[Breadboard] Device ID cannot be empty string!" << endl;
		return false;
	}
	if(!m_factory.deviceExists(classname)) {
		cerr << "[Breadboard] Class name '" << classname << "' invalid." << endl;
		return false;
	}
	if(m_devices.find(id) != m_devices.end()) {
		cerr << "[Breadboard] Another device with the ID '" << id << "' already exists!" << endl;
		return false;
	}

	unique_ptr<Device> device = m_factory.instantiateDevice(id, classname);
	m_lua_access.lock();
	device->createBuffer(iconSizeMinimum(), pos);
	m_lua_access.unlock();

	m_devices.insert(make_pair(id, std::move(device)));

	if(!moveDevice(id, pos)) {
		cerr << "[Breadboard] Could not place new " << classname << " device" << endl;
		removeDevice(id);
		return false;
	}
	return true;
}

/* Context Menu */

void Breadboard::openContextMenu(QPoint pos) {
	m_lua_access.lock();
	for(const auto& [id, device] : m_devices) {
		if(getDistortedGraphicBounds(device->getBuffer(), device->getScale()).contains(pos)) {
			m_menu_device_id = id;
			m_devices_menu->popup(mapToGlobal(pos));
			m_lua_access.unlock();
			return;
		}
	}
	m_lua_access.unlock();
	if(isBreadboard()) {
		for(const auto& [row, content] : m_raster) {
			for(const auto& pin : content.pins) {
				QRect pin_rect = QRect(getAbsolutePosition(row, pin.index), getDistortedSize(QSize(iconSizeMinimum(), iconSizeMinimum())));
				if(pin_rect.contains(pos)) {
					m_menu_pin = pin.global_pin;
					m_pin_menu->popup(mapToGlobal(pos));
					return;
				}
			}
		}
	}
	if(isBreadboard() && !isOnRaster(pos)) return;
	m_bb_menu->popup(mapToGlobal(pos));
}

void Breadboard::removePinTriggered() {
	removePin(m_menu_pin, false);
}

void Breadboard::removeActiveDevice() {
	if(m_menu_device_id.empty()) return;
	removeDevice(m_menu_device_id);
	m_menu_device_id = "";
}

void Breadboard::scaleActiveDevice() {
	auto device = m_devices.find(m_menu_device_id);
	if(device == m_devices.end()) {
		m_error_dialog->showMessage("Could not find device");
		return;
	}
	bool ok;
	m_lua_access.lock();
	auto old_scale = device->second->getScale();
	m_lua_access.unlock();
	int scale = QInputDialog::getInt(this, "Input new scale value", "Scale",
									 old_scale, 1, 10, 1, &ok);
	m_lua_access.lock();
	auto buffer = device->second->getBuffer();
	m_lua_access.unlock();
	if(ok && checkDevicePosition(m_menu_device_id, buffer,
								 scale, getDistortedPosition(buffer.offset())).x()>=0) {
		m_lua_access.lock();
		device->second->setScale(scale);
		m_lua_access.unlock();
	}
	m_menu_device_id = "";
}

void Breadboard::openDeviceConfiguration() {
	auto device = m_devices.find(m_menu_device_id);
	if(device == m_devices.end()) {
		m_error_dialog->showMessage("Could not find device");
		m_menu_device_id = "";
		return;
	}
	m_lua_access.lock();
	if(device->second->m_conf) m_device_configuration->setConfig(m_menu_device_id, device->second->m_conf->getConfig());
	else m_device_configuration->hideConfig();
	if(device->second->m_input) m_device_configuration->setKeys(m_menu_device_id, device->second->m_input->getKeys());
	else m_device_configuration->hideKeys();
	m_lua_access.unlock();
	if(device->second->m_pin) {
		Device::PIN_Interface::DevicePin sync = numeric_limits<Device::PIN_Interface::DevicePin>::max();
		auto sync_pin = m_pin_channels.find(m_menu_device_id);
		if(sync_pin != m_pin_channels.end()) {
			sync = sync_pin->second.device_pin;
		}
		m_device_configuration->setPins(m_menu_device_id, getPinsToDevicePins(m_menu_device_id), sync);
	}
	else {
		m_device_configuration->hidePins();
	}

	m_menu_device_id = "";
	m_device_configuration->exec();
}

void Breadboard::updateKeybinding(const DeviceID& device_id, Keys keys) {
	auto device = m_devices.find(device_id);
	if(device == m_devices.end() || !device->second->m_input) {
		m_error_dialog->showMessage("Device does not implement input interface.");
		return;
	}
	m_lua_access.lock();
	device->second->m_input->setKeys(keys);
	m_lua_access.unlock();
}

void Breadboard::updateConfig(const DeviceID& device_id, Config config) {
	auto device = m_devices.find(device_id);
	if(device == m_devices.end() || !device->second->m_conf) {
		m_error_dialog->showMessage("Device does not implement config interface.");
		return;
	}
	m_lua_access.lock();
	device->second->m_conf->setConfig(config);
	m_lua_access.unlock();
}

void Breadboard::updatePins(const DeviceID &device_id, const unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber>& globals, PinDialog::ChangedSync sync) {
	for(const auto& [device_pin, global] : globals) {
		addPinToDevicePin(device_id, device_pin, global, "dialog");
	}
	unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> device_pins = getPinsToDevicePins(device_id);
	auto sync_pin = device_pins.find(sync.first);
	if(sync_pin != device_pins.end()) {
		setPinSync(sync_pin->second, sync.first, device_id, sync.second);
	}
	updateOverlay();
	// TODO update dialog content?
}
