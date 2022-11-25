#include "button.h"


Button::Button(const DeviceID& id) : CDevice(id) {
    m_input = std::make_unique<Button_Input>(this);
    m_pin = std::make_unique<Button_PIN>(this);
    m_layout = Layout{2, 2, "rgba"};
    m_conf = std::make_unique<CDevice::Config_Interface_C>(this);
    Config c;
    c.emplace("active_low", ConfigElem(true));
    m_conf->setConfig(c);
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
	m_pinLayout.emplace(1, PinDesc{.dir = Dir::output, .name = "output", .row = 0, .index=0});
}

gpio::Tristate Button::Button_PIN::getPin(DevicePin num) {
	if(num == 1) {
        bool active_low = true;
        if(m_device->m_conf) {
            Config c = m_device->m_conf->getConfig();
            auto al_it = c.find("active_low");
            if(al_it != c.end() && al_it->second.type == ConfigElem::Type::boolean) {
                active_low = al_it->second.value.boolean;
            }
        }
		auto button_device = static_cast<Button*>(m_device);
		return button_device->m_active ? (active_low ? gpio::Tristate::LOW : gpio::Tristate::HIGH) : gpio::Tristate::UNSET;
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
