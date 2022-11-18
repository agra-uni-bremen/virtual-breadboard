#include "button.h"


Button::Button(const DeviceID& id) : CDevice(id) {
    m_input = std::make_unique<Button_Input>(this);
    m_pin = std::make_unique<Button_PIN>(this);
    m_layout = Layout{2, 2, "rgba"};
}

Button::~Button() = default;

const DeviceClass Button::getClass() const { return m_classname; }

/* Graph Interface */

void Button::initializeBuffer() {
	draw();
}

void Button::draw() {
	auto *img = m_buffer.bits();
	for(unsigned x=0; x < m_buffer.width(); x++) {
		for(unsigned y=0; y < m_buffer.height(); y++) {
			const auto offs = (y * m_buffer.width() + x) * 4; // heavily depends on rgba8888
			img[offs+0] = m_active ? (uint8_t)255 : (uint8_t)0;
			img[offs+1] = 0;
			img[offs+2] = 0;
			img[offs+3] = 128;
		}
	}
}

/* PIN Interface */

Button::Button_PIN::Button_PIN(CDevice* device) : CDevice::PIN_Interface_C(device) {
    m_pinLayout = PinLayout();
	m_pinLayout.emplace(1, PinDesc{PinDesc::Dir::output, "output"});
}

gpio::Tristate Button::Button_PIN::getPin(PinNumber num) {
	if(num == 1) {
		auto button_device = static_cast<Button*>(m_device);
		return button_device->m_active ? gpio::Tristate::LOW : gpio::Tristate::UNSET;
	}
	return gpio::Tristate::UNSET;
}

/* Input Interface */

Button::Button_Input::Button_Input(CDevice* device) : CDevice::Input_Interface_C(device) {}

void Button::Button_Input::onClick(bool active) {
	auto button_device = static_cast<Button*>(m_device);
	button_device->m_active = active;
	button_device->draw();
}

void Button::Button_Input::onKeypress(int, bool active) {
	onClick(active);
}
