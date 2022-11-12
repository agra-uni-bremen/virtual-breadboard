#pragma once
#include "types.h"

#include <gpio-common.hpp>

#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include <QImage>
#include <QJsonObject>

typedef std::string DeviceID;
typedef std::string DeviceClass;

typedef unsigned DeviceRow;
typedef unsigned DeviceIndex;

class Device {
protected:

	DeviceID m_id;
	QImage buffer;

	struct Layout {
		unsigned width = 1; // as raster rows
		unsigned height = 1; // as raster indexes
		std::string data_type = "rgba";	// Currently ignored and always RGBA8888
	};

	// TODO: Add a scheme that only alpha channel is changed?
	//       either rgb may be negative (don't change)
	//       or just another function (probably better)
	typedef unsigned Xoffset;
	typedef unsigned Yoffset;
	struct Pixel {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	};
	void setPixel(const Xoffset, const Yoffset, Pixel);
	Pixel getPixel(const Xoffset, const Yoffset);
	virtual Layout getLayout();

public:

	const DeviceID& getID() const;
	virtual const DeviceClass getClass() const = 0;

	void fromJSON(QJsonObject json);
	QJsonObject toJSON();

	virtual void initializeBuffer();
	virtual void createBuffer(unsigned iconSizeMinimum, QPoint offset=QPoint(0,0));
	void setScale(unsigned scale);
	unsigned getScale();
	QImage& getBuffer();

	class PIN_Interface {
	public:
		virtual ~PIN_Interface();
		virtual PinLayout getPinLayout() = 0;
		virtual gpio::Tristate getPin(PinNumber num) = 0;
		virtual void setPin(PinNumber num, gpio::Tristate val) = 0;
	};

	class SPI_Interface {
	public:
		virtual ~SPI_Interface();
		virtual gpio::SPI_Response send(gpio::SPI_Command byte) = 0;
	};

	class Config_Interface {
	public:
		virtual ~Config_Interface();
		virtual Config getConfig() = 0;
		virtual bool setConfig(Config conf) = 0;
	};

	class Input_Interface {
	public:
		Keys keybindings;
		virtual ~Input_Interface();
		virtual void onClick(bool active) = 0;
		virtual void onKeypress(Key key, bool active) = 0;
		void setKeys(Keys bindings);
		Keys getKeys();
	};


	std::unique_ptr<PIN_Interface> pin;
	std::unique_ptr<SPI_Interface> spi;
	std::unique_ptr<Config_Interface> conf;
	std::unique_ptr<Input_Interface> input;

	Device(DeviceID id);
	virtual ~Device();

private:
	unsigned m_scale = 1;
};
