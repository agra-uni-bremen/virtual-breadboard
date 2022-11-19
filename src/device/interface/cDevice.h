#pragma once

#include <device.hpp>

class CDevice : public Device {
protected:
	Layout m_layout;
public:
	class PIN_Interface_C : public Device::PIN_Interface {
	protected:
		CDevice* m_device;
		PinLayout m_pinLayout;
	public:
		PIN_Interface_C(CDevice* device);
		~PIN_Interface_C();
		PinLayout getPinLayout() override;
		gpio::Tristate getPin(PinNumber num) override; // implement this
		void setPin(PinNumber num, gpio::Tristate val) override;	// implement this
	};

	class SPI_Interface_C : public Device::SPI_Interface {
	protected:
		CDevice* m_device;
	public:
		SPI_Interface_C(CDevice* device);
		~SPI_Interface_C();
		gpio::SPI_Response send(gpio::SPI_Command byte) override; // implement this
	};

	class Config_Interface_C : public Device::Config_Interface {
	protected:
		CDevice* m_device;
		Config m_config;
	public:
		Config_Interface_C(CDevice *device);
		~Config_Interface_C();
		Config getConfig() override;
		bool setConfig(Config conf) override;
	};

	class Input_Interface_C : public Device::Input_Interface {
	protected:
		CDevice* m_device;
	public:
		Input_Interface_C(CDevice* device);
		~Input_Interface_C();
		void onClick(bool active) override;
		void onKeypress(Key key, bool active) override;
	};

	CDevice(const DeviceID& id);
	~CDevice();

	Layout getLayout() override;
};
