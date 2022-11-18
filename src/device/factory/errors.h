#pragma once

#include "../types.h"

#include <stdexcept>

class device_not_found_error : std::runtime_error {

public:
	device_not_found_error(const DeviceClass& name) : std::runtime_error("Device "+name+" could not be found") {}
};

// TODO: Transform a lot of errors from both factories
