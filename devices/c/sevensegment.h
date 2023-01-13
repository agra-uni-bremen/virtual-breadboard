#pragma once

#include <cFactory.h>

class Sevensegment : public CDevice {

public:
	Sevensegment(const DeviceID& id);
	~Sevensegment();

	inline static DeviceClass m_classname = "sevensegment";
	const DeviceClass getClass() const override;

	void initializeBuffer() override;
	void draw(PIN_Interface::DevicePin num, bool val);

	class Segment_PIN : public CDevice::PIN_Interface_C {
	public:
		Segment_PIN(CDevice* device);
		void setPin(DevicePin num, gpio::Tristate val) override;
	};
};

static const bool registeredSevensegment = getCFactory().registerDeviceType<Sevensegment>();
