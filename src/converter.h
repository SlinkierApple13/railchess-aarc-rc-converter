#pragma once

#include "geometry.h"
#include "rc.h"

namespace converter {

rc::Map convert_to_rc(const geometry::Map& geomap);

} // namespace converter