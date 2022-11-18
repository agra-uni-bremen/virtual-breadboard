#pragma once

#include "constants.h"
#include "types.h"
#include "dialog/keybinding.h"
#include "dialog/config.h"

#include <gpio-helpers.h>
#include <factory/factory.h>

#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QMenu>
#include <QErrorMessage>

#include <unordered_map>
#include <list>
#include <mutex> // TODO: FIXME: Create one Lua state per device that uses asyncs like SPI and synchronous pins

class Breadboard : public QWidget {
	Q_OBJECT

	struct SPI_IOF_Request {
		gpio::PinNumber gpio_offs;	// calculated from "global pin"
		gpio::PinNumber global_pin;
		bool noresponse;
		GpioClient::OnChange_SPI fun;
	};

	struct PIN_IOF_Request {
		gpio::PinNumber gpio_offs;	// calculated from "global pin"
		gpio::PinNumber global_pin;
		gpio::PinNumber device_pin;
		GpioClient::OnChange_PIN fun;
	};

	struct PinMapping{
		gpio::PinNumber gpio_offs;	// calculated from "global pin"
		gpio::PinNumber global_pin;
		gpio::PinNumber device_pin;
		std::string name;
		Device* dev;
	};

	std::mutex lua_access;		//TODO: Use multiple Lua states per 'async called' device
	Factory factory;
	std::unordered_map<DeviceID,std::unique_ptr<Device>> m_devices;
	std::unordered_map<DeviceID,SPI_IOF_Request> spi_channels;
	std::unordered_map<DeviceID,PIN_IOF_Request> pin_channels;

	std::list<PinMapping> reading_connections;		// Semantic subject to change
	std::list<PinMapping> writing_connections;

	bool debugmode = false;
	QString bkgnd_path;
	QPixmap bkgnd;

	DeviceID menu_device_id;
	QMenu *device_menu;
	KeybindingDialog *device_keys;
	ConfigDialog *device_config;
	QErrorMessage *error_dialog;
	QMenu *add_device;

	void setBackground(QString path);
	void updateBackground();

	// Device
	bool addDevice(const DeviceClass& classname, QPoint pos, DeviceID id="");
	void removeDevice(DeviceID id);

	// Connections
	void registerPin(bool synchronous, gpio::PinNumber device_pin, gpio::PinNumber global, std::string name, Device *device);
	void registerSPI(gpio::PinNumber global, bool noresponse, Device *device);

	void writeDevice(DeviceID id);

	// Drag and Drop
	QPoint checkDevicePosition(const DeviceID& id, const QImage& buffer, int scale, QPoint position, QPoint hotspot=QPoint(0,0));
	bool moveDevice(Device *device, QPoint position, QPoint hotspot=QPoint(0,0));
	void dropEvent(QDropEvent *e) override;
	void dragEnterEvent(QDragEnterEvent *e) override;
	void dragMoveEvent(QDragMoveEvent *e) override;

	// QT
	void paintEvent(QPaintEvent *e) override;
	void keyPressEvent(QKeyEvent *e) override;
	void keyReleaseEvent(QKeyEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;

	// Raster
	DeviceRow getDeviceRow(QPoint pos);
	DeviceIndex getDeviceIndex(QPoint pos);
	std::pair<DeviceRow,DeviceIndex> getDeviceRasterPosition(QPoint pos);
	QPoint getDeviceAbsolutePosition(DeviceRow row, DeviceIndex index);

	QRect getRasterBounds();
	bool isOnRaster(QPoint pos);
	Row getRow(QPoint pos);
	Index getIndex(QPoint pos);
	std::pair<Row,Index> getRasterPosition(QPoint pos);
	QPoint getAbsolutePosition(Row row, Index index);

	unsigned iconSizeMinimum();
	QRect getDistortedRect(unsigned x, unsigned y, unsigned width, unsigned height);
	QRect getDistortedGraphicBounds(const QImage& buffer, unsigned scale);
	QPoint getDistortedPosition(QPoint pos);
	QSize getDistortedSize(QSize minimum);
	QPoint getMinimumPosition(QPoint pos);

public:
	Breadboard();
	~Breadboard();

	bool toggleDebug();

	// JSON
    bool loadConfigFile(const QString& file);
	bool saveConfigFile(const QString& file);
	void additionalLuaDir(const std::string& additional_device_dir, bool overwrite_integrated_devices);
	void clear();
	void clearConnections();

	// GPIO
	void timerUpdate(gpio::State state);
	bool isBreadboard();

	// Devices
	void removeDeviceObjects(DeviceID id);

public slots:
	void connectionUpdate(bool active);

private slots:
	void openContextMenu(QPoint pos);
	void removeActiveDevice();
	void scaleActiveDevice();
	void keybindingActiveDevice();
	void changeKeybindingActiveDevice(const DeviceID& device, Keys keys);
	void configActiveDevice();
	void changeConfigActiveDevice(const DeviceID& device, Config config);

signals:
	void registerIOF_PIN(gpio::PinNumber gpio_offs, GpioClient::OnChange_PIN fun);
	void registerIOF_SPI(gpio::PinNumber gpio_offs, GpioClient::OnChange_SPI fun, bool noresponse);
	void closeAllIOFs(std::vector<gpio::PinNumber> gpio_offs);
	void closeDeviceIOFs(std::vector<gpio::PinNumber> gpio_offs, DeviceID device_id);
	void setBit(gpio::PinNumber gpio_offs, gpio::Tristate state);
};
