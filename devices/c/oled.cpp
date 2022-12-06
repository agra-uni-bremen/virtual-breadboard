#include "oled.h"

OLED::OLED(const DeviceID& id) : CDevice(id) {
    m_pin = std::make_unique<OLED_PIN>(this);
    m_spi = std::make_unique<OLED_SPI>(this);
    m_layout = Layout{11, 6, "rgba"};
}
OLED::~OLED() = default;

const DeviceClass OLED::getClass() const { return m_classname; }

/* Graphbuf Interface */

void OLED::initializeBuffer() {
	auto *img = m_buffer.bits();
	for(unsigned x=0; x < m_buffer.width(); x++) {
		for(unsigned y=0; y < m_buffer.height(); y++) {
			const auto offs = (y * m_buffer.width() + x) * 4; // heavily depends on rgba8888
			img[offs+0] = 0;
			img[offs+1] = 0;
			img[offs+2] = 0;
			img[offs+3] = 255;
		}
	}
}

/* PIN Interface */

OLED::OLED_PIN::OLED_PIN(CDevice* device) : CDevice::PIN_Interface_C(device) {
    m_pinLayout = PinLayout();
	m_pinLayout.emplace(1, PinDesc{.dir = Dir::input, .name = "data_command", .row = 0, .index=0});
    m_pinLayout.emplace(2, PinDesc{.dir = Dir::input, .name = "cs", .row = 1, .index = 0});
}

void OLED::OLED_PIN::setPin(DevicePin num, gpio::Tristate val) {
	if(num == 1) {
		auto oled_device = static_cast<OLED*>(m_device);
		oled_device->m_is_data = val == gpio::Tristate::HIGH;
	}
}

/* SPI Interface */

OLED::OLED_SPI::OLED_SPI(CDevice* device) : CDevice::SPI_Interface_C(device) {}

uint8_t getMask(uint8_t op) {
	if(op == DISPLAY_START_LINE) {
		return 0xC0;
	}
	else if(op == COL_LOW || op == COL_HIGH || op == PAGE_ADDR) {
		return 0xF0;
	}
	else if(op == DISPLAY_ON) {
		return 0xFE;
	}
	return 0xFF;
}

std::pair<uint8_t, uint8_t> match(uint8_t cmd) {
	for(uint8_t op : COMMANDS) {
		if(((cmd^op)&getMask(op)) == 0) {
			return {op, cmd&(~getMask(op))};
		}
	}
	return {NOP,0};
}

gpio::SPI_Response OLED::OLED_SPI::send(gpio::SPI_Command byte) {
	auto oled_device = static_cast<OLED*>(m_device);
	if(oled_device->m_is_data) {
		if (oled_device->m_state.column >= m_device->getBuffer().width()) {
			return 0;
		}
		if (oled_device->m_state.page >= m_device->getBuffer().height() / 8) {
			return 0;
		}
		auto *img = m_device->getBuffer().bits();
		for(unsigned y=0; y<8; y++) {
			uint8_t pix=0;
			if(byte & 1<<y) {
				pix = 255;
			}
			const auto offs = (((oled_device->m_state.page * 8) + y) * m_device->getBuffer().width() + oled_device->m_state.column) * 4; // heavily depends on rgba8888
			img[offs+0] = pix;
			img[offs+1] = pix;
			img[offs+2] = pix;
			img[offs+3] = oled_device->m_state.contrast;
		}
		oled_device->m_state.column += 1;
	}
	else {
		std::pair<uint8_t, uint8_t> op_payload = match(byte);
		if(op_payload.first == DISPLAY_START_LINE) {
			return 0;
		}
		else if(op_payload.first == COL_LOW) {
			oled_device->m_state.column = (oled_device->m_state.column & 0xf0) | op_payload.second;
		}
		else if(op_payload.first == COL_HIGH) {
			oled_device->m_state.column = (oled_device->m_state.column & 0x0f) | (op_payload.second << 4);
		}
		else if(op_payload.first == PAGE_ADDR) {
			oled_device->m_state.page = op_payload.second;
		}
		else if(op_payload.first == DISPLAY_ON) {
			oled_device->m_state.display_on = op_payload.second;
		}
	}
	return 0;
}
