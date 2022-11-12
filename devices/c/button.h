#pragma once

#include <cFactory.h>

class Button : public CDevice {
	bool active = false;

public:
	Button(DeviceID id);
	~Button();

	inline static DeviceClass classname = "button";
	const DeviceClass getClass() const override;

	void initializeBuffer() override;
	void draw();

	class Button_PIN : public CDevice::PIN_Interface_C {
	public:
		Button_PIN(CDevice* device);
		gpio::Tristate getPin(PinNumber num) override;
	};

	class Button_Input : public CDevice::Input_Interface_C {
	public:
		Button_Input(CDevice* device);
		void onClick(bool active) override;
		void onKeypress(int key, bool active) override;
	};
};

static const bool registeredButton = getCFactory().registerDeviceType<Button>();
