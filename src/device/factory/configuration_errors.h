#pragma once

#include <stdexcept>

#include "../configuration.h"

class device_not_found_error : std::runtime_error {

public:
	device_not_found_error(DeviceClass name) : std::runtime_error("Device "+name+" could not be found") {}
};

// TODO: Transform a lot of errors from both factories
