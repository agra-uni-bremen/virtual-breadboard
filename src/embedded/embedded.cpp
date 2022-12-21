#include "embedded.h"

#include <QJsonArray>

using namespace gpio;
using namespace std;

Embedded::Embedded(const std::string& host, const std::string& port) : QWidget(), m_host(host), m_port(port) {
    setMinimumSize(m_windowsize);
    setBackground(m_bkgnd_path);

    m_pin_dialog = new PinOptions(this);
    connect(m_pin_dialog, &PinOptions::pinsChanged, this, &Embedded::pinsChanged);
}

Embedded::~Embedded() = default;

PinNumber Embedded::translatePinToGpioOffs(PinNumber pin) {
    auto pin_obj = m_pins.find(pin);
    if(pin_obj == m_pins.end()) {
        return invalidPin();
    }
    return pin_obj->second.gpio_offs;
}

uint64_t Embedded::translateGpioToGlobal(State state) {
    uint64_t ext = 0;
    for(auto const& [global, pin_obj] : m_pins) {
        ext |= (state.pins[pin_obj.gpio_offs] == gpio::Pinstate::HIGH ? 1 : 0) << (pin_obj.gpio_offs-global);
    }
    return ext;
}

gpio::PinNumber Embedded::invalidPin() {
    static_assert(gpio::max_num_pins < numeric_limits<gpio::PinNumber>::max(),
                  "Invalid Pin hides a valid pin number");
    return numeric_limits<gpio::PinNumber>::max();
}

GPIOPinLayout Embedded::getPins() {
    return m_pins;
}

bool Embedded::isPin(gpio::PinNumber pin) {
    return m_pins.find(pin) != m_pins.end();
}

/* GPIO */

bool Embedded::timerUpdate() { // return: new connection?
	if(m_connected && !m_gpio.update()) {
		emit(connectionLost());
        m_connected = false;
	}
	if(!m_connected) {
        m_connected = m_gpio.setupConnection(m_host.c_str(), m_port.c_str());
		if(m_connected) {
			return true;
		}
	}
	return false;
}

uint64_t Embedded::getState() {
    return translateGpioToGlobal(m_gpio.state);
}

bool Embedded::gpioConnected() const {
	return m_connected;
}

void Embedded::registerIOF_PIN(PinNumber global, GpioClient::OnChange_PIN fun) {
    gpio::PinNumber gpio_offs = translatePinToGpioOffs(global);
	if(!m_gpio.isIOFactive(gpio_offs)) {
		m_gpio.registerPINOnChange(gpio_offs, fun);
	}
}

void Embedded::registerIOF_SPI(PinNumber global, GpioClient::OnChange_SPI fun, bool no_response) {
    gpio::PinNumber gpio_offs = translatePinToGpioOffs(global);
	if(!m_gpio.isIOFactive(gpio_offs)) {
		m_gpio.registerSPIOnChange(gpio_offs, fun, no_response);
	}
}

void Embedded::closeIOF(PinNumber global) {
	m_gpio.closeIOFunction(translatePinToGpioOffs(global));
}

void Embedded::destroyConnection() {
	m_gpio.destroyConnection();
}

void Embedded::setBit(gpio::PinNumber global, gpio::Tristate state) {
	if(m_connected) {
		m_gpio.setBit(translatePinToGpioOffs(global), state);
	}
}

/* JSON */

void Embedded::resizeEvent(QResizeEvent*) {
    updateBackground();
}

void Embedded::setBackground(QString path) {
    setAutoFillBackground(true);
    m_bkgnd_path = path;
    m_bkgnd = QPixmap(m_bkgnd_path);
    updateBackground();
}

void Embedded::updateBackground() {
    QPixmap new_bkgnd = m_bkgnd.scaled(size(), Qt::IgnoreAspectRatio);
    QPalette palette;
    palette.setBrush(QPalette::Window, new_bkgnd);
    setPalette(palette);
}

void Embedded::fromJSON(QJsonObject json) {
    if(!json.contains("pins") || !json["pins"].isArray()) {
        cerr << "[Embedded] JSON missing pins entry" << endl;
        return;
    }
    if(json.contains("window") && json["window"].isObject()) {
        QJsonObject window = json["window"].toObject();
        unsigned windowsize_x = window["windowsize"].toArray().at(0).toInt();
        unsigned windowsize_y = window["windowsize"].toArray().at(1).toInt();

        setMinimumSize(windowsize_x, windowsize_y);
        setBackground(window["background"].toString());
    }

    m_pins.clear();
    QJsonArray pins = json["pins"].toArray();
    for(const auto& pin_obj : pins) {
        QJsonObject pin = pin_obj.toObject();
        gpio::PinNumber global = pin["global"].toInt();
        gpio::PinNumber gpio_offs = pin["gpio_offs"].toInt();
        list<IOF> iofs;
        QJsonArray iofs_obj = pin["iofs"].toArray();
        for(const auto& iof_obj : iofs_obj) {
            QJsonObject iof = iof_obj.toObject();
            bool active = iof["active"].toBool();
            IOFType type;
            QString type_str = iof["type"].toString();
            if(type_str == "UART") type = IOFType::UART;
            else if(type_str == "SPI") type = IOFType::SPI;
            else if(type_str == "PWM") type = IOFType::PWM;
            else continue;
            iofs.push_back(IOF{.type=type,.active=active});
        }
        m_pins.emplace(global, GPIOPin{.gpio_offs=gpio_offs,.iofs=iofs});
    }
}

QJsonObject Embedded::toJSON() {
    QJsonObject json;
    QJsonObject window;
    window["background"] = m_bkgnd_path;
    QJsonArray windowsize;
    windowsize.append(minimumSize().width());
    windowsize.append(minimumSize().height());
    window["windowsize"] = windowsize;
    json["window"] = window;
    QJsonArray pins;
    for(auto const& [global, pin] : m_pins) {
        QJsonObject pin_obj;
        pin_obj["global"] = global;
        pin_obj["gpio_offs"] = pin.gpio_offs;
        QJsonArray iofs;
        for(auto const& iof : pin.iofs) {
            QJsonObject iof_obj;
            QString type;
            switch(iof.type) {
                case IOFType::PWM:
                    type = "PWM";
                    break;
                case SPI:
                    type = "SPI";
                    break;
                case UART:
                    type = "UART";
                    break;
            }
            iof_obj["type"] = type;
            iof_obj["active"] = iof.active;
            iofs.append(iof_obj);
        }
        if(!iofs.empty()) {
            pin_obj["iofs"] = iofs;
        }
        pins.append(pin_obj);
    }
    json["pins"] = pins;
    return json;
}

/* DIALOG */

void Embedded::pinsChanged(std::list<std::pair<gpio::PinNumber, IOF>> iofs) {
    for(auto const& [global, iof] : iofs) {
        auto pin = m_pins.find(global);
        if(pin == m_pins.end()) continue;
        IOFType type = iof.type;
        auto pin_iof = std::find_if(pin->second.iofs.begin(), pin->second.iofs.end(),[type](IOF i){
            return type == i.type;
        });
        if(pin_iof == pin->second.iofs.end()) continue;
        pin_iof->active = iof.active;
    }
    emit(pinSettingsChanged(iofs));
}

void Embedded::openPinOptions() {
    m_pin_dialog->setPins(getPins());
    m_pin_dialog->exec();
}
