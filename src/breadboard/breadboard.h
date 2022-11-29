#pragma once

#include "constants.h"
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
#include <unordered_set>
#include <list>
#include <mutex> // TODO: FIXME: Create one Lua state per device that uses asyncs like SPI and synchronous pins

class Breadboard : public QWidget {
	Q_OBJECT

    typedef unsigned Row;
    typedef unsigned Index;

	struct SPI_IOF_Request {
		gpio::PinNumber gpio_offs;	// calculated from "global pin"
		gpio::PinNumber global_pin;
		bool noresponse;
		GpioClient::OnChange_SPI fun;
	};

	struct PIN_IOF_Request {
		gpio::PinNumber gpio_offs;	// calculated from "global pin"
		gpio::PinNumber global_pin;
		Device::PIN_Interface::DevicePin device_pin;
		GpioClient::OnChange_PIN fun;
	};

    struct PinMapping {
        gpio::PinNumber gpio_offs;
        gpio::PinNumber global_pin;
        Device::PIN_Interface::DevicePin device_pin;
        DeviceID device;
    };

	struct PinConnection {
        gpio::PinNumber global_pin;
        std::string name;
        Index index;
	};

    struct DeviceConnection {
        DeviceID id;
        Device::PIN_Interface::DevicePin pin;
    };

    struct RowContent {
        std::list<DeviceConnection> devices;
        std::list<PinConnection> pins;
    };

	std::mutex m_lua_access;		//TODO: Use multiple Lua states per 'async called' device
	Factory m_factory;
	std::unordered_map<DeviceID,std::unique_ptr<Device>> m_devices;

	std::unordered_map<DeviceID,SPI_IOF_Request> m_spi_channels;
	std::unordered_map<DeviceID,PIN_IOF_Request> m_pin_channels;
    std::list<PinMapping> m_reading_connections;
    std::list<PinMapping> m_writing_connections;

    std::unordered_map<Row,RowContent> m_raster;

	bool m_debugmode = false;
	QString m_bkgnd_path;
	QPixmap m_bkgnd;

	DeviceID m_menu_device_id;
	QMenu *m_device_menu;
    QAction *m_device_menu_key;
    QAction *m_device_menu_conf;
	KeybindingDialog *m_device_keys;
	ConfigDialog *m_device_config;
	QErrorMessage *m_error_dialog;
	QMenu *m_add_device;

	void setBackground(QString path);
	void updateBackground();

	// Device
	bool addDevice(const DeviceClass& classname, QPoint pos, DeviceID id="");
	void removeDevice(const DeviceID& id);

	// Connections
    void printConnections();
    void addSPIToRow(Row row, Index index, gpio::PinNumber global, std::string name, bool noresponse=true);
    void addPinToRow(Row row, Index index, gpio::PinNumber global, std::string name);
    void addPinToRowContent(Row row, Index index, gpio::PinNumber global, std::string name);
    void createRowConnections(Row row);
    void createRowConnectionsSPI(Row row, bool noresponse=true);
    std::unordered_set<gpio::PinNumber> getPinsToDevice(DeviceID device_id);
    void registerPin(gpio::PinNumber global, Device::PIN_Interface::DevicePin device_pin, const DeviceID& device_id, bool synchronous=false);
    void setPinSync(gpio::PinNumber global, Device::PIN_Interface::DevicePin device_pin, const DeviceID& device_id, bool synchronous);
	void registerSPI(gpio::PinNumber global, const DeviceID& device_id, bool noresponse);
    void setSPI(gpio::PinNumber global, bool active);
    void setSPInoresponse(gpio::PinNumber global, bool noresponse);
    void removePin(Row row, gpio::PinNumber global);

	void writeDevice(const DeviceID& id);

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
	DeviceRow getDeviceRow(QPoint pos_on_device);
	DeviceIndex getDeviceIndex(QPoint pos_on_device);
	std::pair<DeviceRow,DeviceIndex> getDeviceRasterPosition(QPoint pos_on_device);
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

	// Remove Connection Objects
	void removeDeviceObjects(const DeviceID& id);
    void removePinObjects(const gpio::PinNumber global);

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
    void closePinIOFs(std::list<gpio::PinNumber> gpio_offs, gpio::PinNumber global);
	void setBit(gpio::PinNumber gpio_offs, gpio::Tristate state);
};
