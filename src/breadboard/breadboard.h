#pragma once

#include "constants.h"
#include "dialog/device_configurations.h"
#include "overlay.h"

#include <factory/factory.h>
#include <embedded.h>

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

    Embedded *m_embedded;
    Overlay *m_overlay;

    typedef unsigned Row;
    typedef unsigned Index;

	struct SPI_IOF_Request {
		gpio::PinNumber global_pin;
        Device::PIN_Interface::DevicePin cs_pin;
		bool noresponse;
		GpioClient::OnChange_SPI fun;
	};

	struct PIN_IOF_Request {
		gpio::PinNumber global_pin;
		Device::PIN_Interface::DevicePin device_pin;
		GpioClient::OnChange_PIN fun;
	};

    struct PinMapping {
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
	QMenu *m_devices_menu;
    DeviceConfigurations *m_device_configurations;
	QErrorMessage *m_error_dialog;
    QMenu *m_bb_menu;

	void setBackground(QString path);
	void updateBackground();
    void updateOverlay();

	// Device
	bool addDevice(const DeviceClass& classname, QPoint pos, DeviceID id="");
	void removeDevice(const DeviceID& id);

	// Connections
    std::string getPinName(gpio::PinNumber global);
    void addPinToDevicePin(const DeviceID& device_id, Device::PIN_Interface::DevicePin device_pin, gpio::PinNumber global, std::string name);
    void addPinToRow(Row row, Index index, gpio::PinNumber global, std::string name);
    void addPinToRowContent(Row row, Index index, gpio::PinNumber global, std::string name);
    void createRowConnections(Row row);
    std::unordered_set<gpio::PinNumber> getPinsToDevice(const DeviceID& device_id);
    std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber> getPinsToDevicePins(const DeviceID& device_id);
    void registerPin(gpio::PinNumber global, Device::PIN_Interface::DevicePin device_pin, const DeviceID& device_id, bool synchronous=false);
    void setPinSync(gpio::PinNumber global, Device::PIN_Interface::DevicePin device_pin, const DeviceID& device_id, bool synchronous);
	void registerSPI(gpio::PinNumber global, Device::PIN_Interface::DevicePin cs_pin, const DeviceID& device_id, bool noresponse);
    void setSPInoresponse(gpio::PinNumber global, bool noresponse);
    Row removeConnection(const DeviceID& device_id, Device::PIN_Interface::DevicePin device_pin);
    void removeConnections(gpio::PinNumber global, bool keep_on_raster);
    void removeSPI(gpio::PinNumber global, bool keep_on_raster);
    void removeSPIForDevice(gpio::PinNumber global, const DeviceID& device_id);
    void removePin(gpio::PinNumber global, bool keep_on_raster);
    void removePinForDevice(gpio::PinNumber global, const DeviceID& device_id);
    void removePinFromRaster(gpio::PinNumber global);

	void writeDevice(const DeviceID& id);

	// Drag and Drop
	QPoint checkDevicePosition(const DeviceID& id, const QImage& buffer, int scale, QPoint position, QPoint hotspot=QPoint(0,0));
	bool moveDevice(const DeviceID& device_id, QPoint position, QPoint hotspot=QPoint(0,0));
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
    Row getNewRowNumber();
    Row getRowForDevicePin(const DeviceID& device_id, Device::PIN_Interface::DevicePin device_pin);
	DeviceRow getDeviceRow(QPoint pos_on_device);
	DeviceIndex getDeviceIndex(QPoint pos_on_device);
	std::pair<DeviceRow,DeviceIndex> getDeviceRasterPosition(QPoint pos_on_device);
	QPoint getDeviceAbsolutePosition(DeviceRow row, DeviceIndex index);

    Index getNextIndex(Row row);
    static bool isValidRasterRow(Row row);
    static bool isValidRasterIndex(Index index);
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

    void setEmbedded(Embedded *embedded);
    void setOverlay(Overlay *overlay);

	// JSON
    void fromJSON(QJsonObject json);
	QJsonObject toJSON();
	void additionalLuaDir(const std::string& additional_device_dir, bool overwrite_integrated_devices);
	void clear();

	// GPIO
	void timerUpdate(uint64_t state);
	bool isBreadboard();

    void printConnections();
    void setSPI(gpio::PinNumber global, bool active);

public slots:
	void connectionUpdate(bool active);

private slots:
	void openContextMenu(QPoint pos);
	void removeActiveDevice();
    void scaleActiveDevice();
    void openDeviceConfigurations();
	void updateKeybinding(const DeviceID& device, Keys keys);
	void updateConfig(const DeviceID& device, Config config);
    void updatePins(const DeviceID& device, const std::unordered_map<Device::PIN_Interface::DevicePin, gpio::PinNumber>& globals, std::pair<Device::PIN_Interface::DevicePin, bool> sync);
};
