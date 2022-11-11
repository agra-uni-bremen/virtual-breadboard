#include "breadboard.h"
#include "raster.h"
#include "device/raster.h"

#include <QTimer>
#include <QInputDialog>

using namespace std;

Breadboard::Breadboard() : QWidget() {
	setFocusPolicy(Qt::StrongFocus);
	setAcceptDrops(true);
	setMouseTracking(true);

	QTimer *timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, [this]{update();});
	timer->start(1000/30);

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested, this, &Breadboard::openContextMenu);
	device_menu = new QMenu(this);
	QAction *delete_device = new QAction("Delete");
	connect(delete_device, &QAction::triggered, this, &Breadboard::removeActiveDevice);
	device_menu->addAction(delete_device);
	QAction *scale_device = new QAction("Scale");
	connect(scale_device, &QAction::triggered, this, &Breadboard::scaleActiveDevice);
	device_menu->addAction(scale_device);
	QAction *keybinding_device = new QAction("Edit Keybindings");
	connect(keybinding_device, &QAction::triggered, this, &Breadboard::keybindingActiveDevice);
	device_menu->addAction(keybinding_device);
	QAction *config_device = new QAction("Edit configurations");
	connect(config_device, &QAction::triggered, this, &Breadboard::configActiveDevice);
	device_menu->addAction(config_device);

	device_keys = new KeybindingDialog(this);
	connect(device_keys, &KeybindingDialog::keysChanged, this, &Breadboard::changeKeybindingActiveDevice);
	device_config = new ConfigDialog(this);
	connect(device_config, &ConfigDialog::configChanged, this, &Breadboard::changeConfigActiveDevice);
	error_dialog = new QErrorMessage(this);

	add_device = new QMenu(this);
	for(DeviceClass device : factory.getAvailableDevices()) {
		QAction *device_action = new QAction(QString::fromStdString(device));
		connect(device_action, &QAction::triggered, [this, device](){
			addDevice(device, mapFromGlobal(add_device->pos()));
		});
		add_device->addAction(device_action);
	}

	defaultBackground();
}

Breadboard::~Breadboard() {
}

std::list<DeviceClass> Breadboard::getAvailableDevices() { return factory.getAvailableDevices(); }
bool Breadboard::isBreadboard() { return bkgnd_path == DEFAULT_PATH; }
bool Breadboard::toggleDebug() {
	debugmode = !debugmode;
	return debugmode;
}

/* DEVICE */

QPoint Breadboard::checkDevicePosition(DeviceID id, QImage buffer, int scale, QPoint position, QPoint hotspot) {
	QPoint upper_left = position - hotspot;
	QRect device_bounds = QRect(upper_left, getGraphicBounds(buffer, scale).size());

	if(isBreadboard()) {
		if(bb_getRasterBounds().intersects(device_bounds)) {
			QPoint dropPositionRaster = bb_getAbsolutePosition(bb_getRow(position), bb_getIndex(position));
			upper_left = dropPositionRaster - device_getAbsolutePosition(device_getRow(hotspot),
					device_getIndex(hotspot));
			device_bounds = QRect(upper_left, device_bounds.size());
		} else {
			cerr << "[Breadboard] Device position invalid: Device should be at least partially on raster." << endl;
			return QPoint(-1,-1);
		}
	}

	if(!rect().contains(device_bounds)) {
		cerr << "[Breadboard] Device position invalid: Device may not leave window view." << endl;
		return QPoint(-1,-1);
	}

	for(const auto& [id_it, device_it] : devices) {
		if(id_it == id) continue;
		if(device_it->graph && getGraphicBounds(device_it->graph->getBuffer(),
				device_it->graph->getScale()).intersects(device_bounds)) {
			cerr << "[Breadboard] Device position invalid: Overlaps with other device." << endl;
			return QPoint(-1,-1);
		}
	}
	return upper_left;
}

bool Breadboard::moveDevice(Device *device, QPoint position, QPoint hotspot) {
	if(!device || !device->graph) return false;
	unsigned scale = device->graph->getScale();
	if(!scale) scale = 1;

	QPoint upper_left = checkDevicePosition(device->getID(), device->graph->getBuffer(), scale, position, hotspot);

	if(upper_left.x()<0) {
		cerr << "[Breadboard] New device position is invalid." << endl;
		return false;
	}

	device->graph->getBuffer().setOffset(upper_left);
	device->graph->setScale(scale);
	return true;
}

bool Breadboard::addDevice(DeviceClass classname, QPoint pos) {
	DeviceID id;
	if(devices.size() < std::numeric_limits<unsigned>::max()) {
		id = std::to_string(devices.size());
	}
	else {
		std::set<unsigned> used_ids;
		for(auto const& [id_it, device_it] : devices) {
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

	unique_ptr<Device> device = factory.instantiateDevice(id, classname);
	if(!device->graph) return false;
	device->graph->createBuffer(pos);
	if(moveDevice(device.get(), pos)) {
		devices.insert(make_pair(id, std::move(device)));
		return true;
	}
	cerr << "[Breadboard] Could not place new " << classname << " device" << endl;
	return false;
}

unique_ptr<Device> Breadboard::createDevice(DeviceClass classname, DeviceID id) {
	if(!id.size()) {
		cerr << "[Breadboard] Device ID cannot be empty string!" << endl;
		return 0;
	}
	if(!factory.deviceExists(classname)) {
		cerr << "[Breadboard] Add device: class name '" << classname << "' invalid." << endl;
		return 0;
	}
	if(devices.find(id) != devices.end()) {
		cerr << "[Breadboard] Another device with the ID '" << id << "' is already instatiated!" << endl;
		return 0;
	}
	return factory.instantiateDevice(id, classname);
}

/* Context Menu */

void Breadboard::openContextMenu(QPoint pos) {
	for(auto const& [id, device] : devices) {
		if(device->graph && getGraphicBounds(device->graph->getBuffer(), device->graph->getScale()).contains(pos)) {
			menu_device_id = id;
			device_menu->popup(mapToGlobal(pos));
			return;
		}
	}
	if(isBreadboard() && !bb_isOnRaster(pos)) return;
	add_device->popup(mapToGlobal(pos));
}

void Breadboard::removeActiveDevice() {
	if(!menu_device_id.size()) return;
	removeDevice(menu_device_id);
	menu_device_id = "";
}

void Breadboard::scaleActiveDevice() {
	auto device = devices.find(menu_device_id);
	if(device == devices.end() || !device->second->graph) {
		error_dialog->showMessage("Device does not implement graph interface.");
		return;
	}
	bool ok;
	int scale = QInputDialog::getInt(this, "Input new scale value", "Scale",
			device->second->graph->getScale(), 1, 10, 1, &ok);
	if(ok && checkDevicePosition(device->second->getID(), device->second->graph->getBuffer(),
			scale, device->second->graph->getBuffer().offset()).x()>=0) {
		device->second->graph->setScale(scale);
	}
	menu_device_id = "";
}

void Breadboard::keybindingActiveDevice() {
	auto device = devices.find(menu_device_id);
	if(device == devices.end() || !device->second->input) {
		error_dialog->showMessage("Device does not implement input interface.");
		return;
	}
	device_keys->setKeys(menu_device_id, device->second->input->getKeys());
	menu_device_id = "";
	device_keys->exec();
}

void Breadboard::changeKeybindingActiveDevice(DeviceID device_id, Keys keys) {
	auto device = devices.find(device_id);
	if(device == devices.end() || !device->second->input) {
		error_dialog->showMessage("Device does not implement input interface.");
		return;
	}
	device->second->input->setKeys(keys);
}

void Breadboard::configActiveDevice() {
	auto device = devices.find(menu_device_id);
	if(device == devices.end() || !device->second->conf) {
		error_dialog->showMessage("Device does not implement config interface.");
		return;
	}
	device_config->setConfig(menu_device_id, device->second->conf->getConfig());
	menu_device_id = "";
	device_config->exec();
}

void Breadboard::changeConfigActiveDevice(DeviceID device_id, Config config) {
	auto device = devices.find(device_id);
	if(device == devices.end() || !device->second->conf) {
		error_dialog->showMessage("Device does not implement config interface.");
		return;
	}
	device->second->conf->setConfig(config);
}
