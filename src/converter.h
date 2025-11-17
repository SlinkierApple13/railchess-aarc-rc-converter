#pragma once

#include <atomic>
#include "geometry.h"
#include "rc.h"

namespace converter {

rc::Map convert_to_rc(const geometry::Map& geomap, std::atomic<bool>* cancel_flag = nullptr);

} // namespace converter