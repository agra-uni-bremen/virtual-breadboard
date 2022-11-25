#pragma once

#include <cFactory.h>

class RGB : public CDevice {

public:
	RGB(const DeviceID& id);
	~RGB();

	inline static DeviceClass m_classname = "rgb";
	const DeviceClass getClass() const override;

	void initializeBuffer() override;
	void draw(PIN_Interface::DevicePin num, bool val);

	class RGB_Pin : public CDevice::PIN_Interface_C {
	public:
		RGB_Pin(CDevice* device);
		void setPin(DevicePin num, gpio::Tristate val) override;
	};
};

static const bool registeredRGB = getCFactory().registerDeviceType<RGB>();
