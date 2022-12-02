#pragma once

#include <gpio-client.hpp>

uint64_t translateGpioToExtPin(gpio::State reg);
gpio::PinNumber translatePinToGpioOffs(gpio::PinNumber pin);
gpio::PinNumber translateGpioOffsToPin(gpio::PinNumber pin);

bool isHifivePin(gpio::PinNumber pin);
