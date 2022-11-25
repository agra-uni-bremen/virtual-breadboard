#include "rgb.h"

#include <cmath>

RGB::RGB(const DeviceID& id) : CDevice(id) {
    m_pin = std::make_unique<RGB_Pin>(this);
    m_layout = Layout{1, 1, "rgba"};
}

RGB::~RGB() = default;

const DeviceClass RGB::getClass() const { return m_classname; }

/* Graph */

void RGB::initializeBuffer() {
	for(PIN_Interface::DevicePin num=0; num<=2; num++) {
		draw(0, false);
	}
}

void RGB::draw(PIN_Interface::DevicePin num, bool val) {
	if(num > 2) { return; }
	int extent_center = std::ceil(m_buffer.width() / (float)2);
	Pixel cur = getPixel(extent_center, extent_center);
	if(num == 0) cur.r = val?255:0;
	else if(num == 1) cur.g = val?255:0;
	else cur.b = val?255:0;

	auto *img = m_buffer.bits();
	for(int x=1; x < m_buffer.width(); x++) {
		for(int y=1; y < m_buffer.height(); y++) {
			float dist = std::sqrt(pow(extent_center - x, 2) + pow(extent_center - y, 2));
			int norm_lumen = std::floor((1-dist/extent_center)*255);
			if(norm_lumen < 0) norm_lumen = 0;
			if(norm_lumen > 255) norm_lumen = 255;

			const auto offs = ((y-1) * m_buffer.width() + (x - 1)) * 4; // heavily depends on rgba8888
			img[offs+0] = cur.r;
			img[offs+1] = cur.g;
			img[offs+2] = cur.b;
			img[offs+3] = (uint8_t)norm_lumen;
		}
	}
}

/* PIN */

RGB::RGB_Pin::RGB_Pin(CDevice* device) : CDevice::PIN_Interface_C(device) {
    m_pinLayout = PinLayout();
	m_pinLayout.emplace(0, PinDesc{Dir::input, "r"});
	m_pinLayout.emplace(1, PinDesc{Dir::input, "g"});
	m_pinLayout.emplace(2, PinDesc{Dir::input, "b"});
}

void RGB::RGB_Pin::setPin(DevicePin num, gpio::Tristate val) {
	if(num <= 2) {
		auto rgb_device = static_cast<RGB*>(m_device);
		rgb_device->draw(num, val == gpio::Tristate::HIGH);
	}
}
