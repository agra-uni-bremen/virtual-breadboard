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
    m_device_menu = new QMenu(this);
	auto *delete_device = new QAction("Delete");
	connect(delete_device, &QAction::triggered, this, &Breadboard::removeActiveDevice);
	m_device_menu->addAction(delete_device);
	auto *scale_device = new QAction("Scale");
	connect(scale_device, &QAction::triggered, this, &Breadboard::scaleActiveDevice);
	m_device_menu->addAction(scale_device);
    m_device_menu_key = new QAction("Edit Keybindings");
	connect(m_device_menu_key, &QAction::triggered, this, &Breadboard::keybindingActiveDevice);
	m_device_menu->addAction(m_device_menu_key);
    m_device_menu_conf = new QAction("Edit configurations");
	connect(m_device_menu_conf, &QAction::triggered, this, &Breadboard::configActiveDevice);
	m_device_menu->addAction(m_device_menu_conf);

    m_device_keys = new KeybindingDialog(this);
	connect(m_device_keys, &KeybindingDialog::keysChanged, this, &Breadboard::changeKeybindingActiveDevice);
    m_device_config = new ConfigDialog(this);
	connect(m_device_config, &ConfigDialog::configChanged, this, &Breadboard::changeConfigActiveDevice);
    m_error_dialog = new QErrorMessage(this);

    m_add_device = new QMenu(this);
	for(const DeviceClass& device : m_factory.getAvailableDevices()) {
		auto *device_action = new QAction(QString::fromStdString(device));
		connect(device_action, &QAction::triggered, [this, device](){
			addDevice(device, mapFromGlobal(m_add_device->pos()));
		});
		m_add_device->addAction(device_action);
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
		if(getDistortedGraphicBounds(device_it->getBuffer(),
				device_it->getScale()).intersects(device_bounds)) {
			cerr << "[Breadboard] Device position invalid: Overlaps with other device." << endl;
			return {-1,-1};
		}
	}
	return getMinimumPosition(upper_left);
}

bool Breadboard::moveDevice(Device *device, QPoint position, QPoint hotspot) {
	if(!device) return false;
	unsigned scale = device->getScale();
	if(!scale) scale = 1;

	QPoint upper_left = checkDevicePosition(device->getID(), device->getBuffer(), scale, position, hotspot);

	if(upper_left.x()<0) {
		cerr << "[Breadboard] New device position is invalid." << endl;
		return false;
	}

	device->getBuffer().setOffset(upper_left);
	device->setScale(scale);
	return true;
}

bool Breadboard::addDevice(const DeviceClass& classname, QPoint pos, DeviceID id) {
	if(id.empty()) {
		if(m_devices.size() < std::numeric_limits<unsigned>::max()) {
			id = std::to_string(m_devices.size());
		}
		else {
			std::set<unsigned> used_ids;
			for(auto const& [id_it, device_it] : m_devices) {
				used_ids.insert(std::stoi(id_it));
			}
			unsigned id_int = 0;
			for(unsigned used_id : used_ids) {
				if(used_id > id_int)
					break;
				id_int++;
			}
			id = std::to_string(id_int);
		}
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
	device->createBuffer(iconSizeMinimum(), pos);
	if(moveDevice(device.get(), pos)) {
		m_devices.insert(make_pair(id, std::move(device)));
		return true;
	}
	cerr << "[Breadboard] Could not place new " << classname << " device" << endl;
	return false;
}

/* Context Menu */

void Breadboard::openContextMenu(QPoint pos) {
	for(auto const& [id, device] : m_devices) {
		if(getDistortedGraphicBounds(device->getBuffer(), device->getScale()).contains(pos)) {
            m_menu_device_id = id;
            if(device->m_conf) m_device_menu_conf->setVisible(true);
            else m_device_menu_conf->setVisible(false);
            if(device->m_input) m_device_menu_key->setVisible(true);
            else m_device_menu_key->setVisible(false);
			m_device_menu->popup(mapToGlobal(pos));
			return;
		}
	}
	if(isBreadboard() && !isOnRaster(pos)) return;
	m_add_device->popup(mapToGlobal(pos));
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
	int scale = QInputDialog::getInt(this, "Input new scale value", "Scale",
			device->second->getScale(), 1, 10, 1, &ok);
	if(ok && checkDevicePosition(device->second->getID(), device->second->getBuffer(),
			scale, getDistortedPosition(device->second->getBuffer().offset())).x()>=0) {
		device->second->setScale(scale);
	}
    m_menu_device_id = "";
}

void Breadboard::keybindingActiveDevice() {
	auto device = m_devices.find(m_menu_device_id);
	if(device == m_devices.end() || !device->second->m_input) {
		m_error_dialog->showMessage("Device does not implement input interface.");
		return;
	}
	m_device_keys->setKeys(m_menu_device_id, device->second->m_input->getKeys());
    m_menu_device_id = "";
	m_device_keys->exec();
}

void Breadboard::changeKeybindingActiveDevice(const DeviceID& device_id, Keys keys) {
	auto device = m_devices.find(device_id);
	if(device == m_devices.end() || !device->second->m_input) {
		m_error_dialog->showMessage("Device does not implement input interface.");
		return;
	}
	device->second->m_input->setKeys(keys);
}

void Breadboard::configActiveDevice() {
	auto device = m_devices.find(m_menu_device_id);
	if(device == m_devices.end() || !device->second->m_conf) {
		m_error_dialog->showMessage("Device does not implement config interface.");
		return;
	}
	m_device_config->setConfig(m_menu_device_id, device->second->m_conf->getConfig());
    m_menu_device_id = "";
	m_device_config->exec();
}

void Breadboard::changeConfigActiveDevice(const DeviceID& device_id, Config config) {
	auto device = m_devices.find(device_id);
	if(device == m_devices.end() || !device->second->m_conf) {
		m_error_dialog->showMessage("Device does not implement config interface.");
		return;
	}
	device->second->m_conf->setConfig(config);
}
