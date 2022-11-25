#include "sevensegment.h"

Sevensegment::Sevensegment(const DeviceID& id) : CDevice(id) {
    m_pin = std::make_unique<Segment_PIN>(this);
    m_layout = Layout{4, 5, "rgba"};
}

Sevensegment::~Sevensegment() = default;

const DeviceClass Sevensegment::getClass() const { return m_classname; }

void Sevensegment::initializeBuffer() {
	auto *img = m_buffer.bits();
	for(unsigned x=0; x < m_buffer.width(); x++) {
		for(unsigned y=0; y < m_buffer.height(); y++) {
			const auto offs = (y * m_buffer.width() + x) * 4; // heavily depends on rgba8888
			img[offs+0] = 0;
			img[offs+1] = 0;
			img[offs+2] = 0;
			img[offs+3] = 128;
		}
	}
	for(PIN_Interface::DevicePin num=0; num<=7; num++) {
		draw(num, false);
	}
}

void Sevensegment::draw(PIN_Interface::DevicePin num, bool val) {
	if(num > 7) { return; }
	// general display stuff
	unsigned xcol1 = 0;
	unsigned xcol2 = 3 * (m_buffer.width() / 4);
	unsigned yrow1 = 0;
	unsigned yrow2 = (m_buffer.height() - 2) / 2;
	unsigned yrow3 = m_buffer.height() - 2;
	unsigned x1=0, y1=0;
	unsigned x2=0, y2=0;
	// assign start and end point of line
	if(num == 0) {
		x1 = xcol1, y1 = yrow1;
		x2 = xcol2, y2 = yrow1;
	} else if(num == 1) {
		x1 = xcol2, y1 = yrow1;
		x2 = xcol2, y2 = yrow2;
	} else if(num == 2) {
		x1 = xcol2, y1 = yrow2;
		x2 = xcol2, y2 = yrow3;
	} else if(num == 3) {
		x1 = xcol1, y1 = yrow3;
		x2 = xcol2, y2 = yrow3;
	} else if(num == 4) {
		x1 = xcol1, y1 = yrow2;
		x1 = xcol1, y2 = yrow3;
	} else if(num == 5) {
		x1 = xcol1, y1 = yrow1;
		x2 = xcol1, y2 = yrow2;
	} else if(num == 6) {
		x1 = xcol1, y1 = yrow2;
		x2 = xcol2, y2 = yrow2;
	} else if(num == 7) {
		x1 = m_buffer.width() - 4, y1 = yrow3 - 2;
		x2 = m_buffer.width() - 2, y2 = yrow3;
	}

	auto *img = m_buffer.bits();
	for(unsigned x=x1; x<=x2+1; x++) {
		for(unsigned y=y1; y<=y2+1; y++) {
			const auto offs = (y * m_buffer.width() + x) * 4; // heavily depends on rgba8888
			img[offs+0] = val?(uint8_t)255:(uint8_t)0;
			img[offs+1] = 0;
			img[offs+2] = 0;
			img[offs+3] = 255;
		}
	}
}

/* PIN Interface */

Sevensegment::Segment_PIN::Segment_PIN(CDevice* device) : CDevice::PIN_Interface_C(device) {
    m_pinLayout = PinLayout();
	m_pinLayout.emplace(0, PinDesc{.dir=Dir::input, .name="top", .row=1, .index=0});
	m_pinLayout.emplace(1, PinDesc{.dir=Dir::input, .name="top_right", .row=3, .index=1});
	m_pinLayout.emplace(2, PinDesc{.dir=Dir::input, .name="bottom_right", .row=3, .index=3});
	m_pinLayout.emplace(3, PinDesc{.dir=Dir::input, .name="bottom", .row=1, .index=4});
	m_pinLayout.emplace(4, PinDesc{.dir=Dir::input, .name="bottom_left", .row=0, .index=3});
	m_pinLayout.emplace(5, PinDesc{.dir=Dir::input, .name="top_left", .row=0, .index=1});
	m_pinLayout.emplace(6, PinDesc{.dir=Dir::input, .name="center", .row=2, .index=0});
	m_pinLayout.emplace(7, PinDesc{.dir=Dir::input, .name="dot", .row=2, .index=4});
}

void Sevensegment::Segment_PIN::setPin(DevicePin num, gpio::Tristate val) {
	if(num <= 7) {
		auto segment_device = static_cast<Sevensegment*>(m_device);
		segment_device->draw(num, val == gpio::Tristate::HIGH);
	}
}
