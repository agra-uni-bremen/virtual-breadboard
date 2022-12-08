#pragma once

#include <gpio-client.hpp>

#include <list>

enum IOFType {
    SPI,
    UART,
    PWM
};

struct IOF {
    IOFType type;
    bool active=false;
};

struct GPIOPin {
    gpio::PinNumber gpio_offs;
    std::list<IOF> iofs;
};
typedef std::unordered_map<gpio::PinNumber, GPIOPin> GPIOPinLayout; // GLOBAL to information