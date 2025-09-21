#include <cmath>

#include "rc.h"

namespace rc {

nlohmann::json Map::to_json() const {
    nlohmann::json j;

    nlohmann::json stations_json = nlohmann::json::array();
    for (const auto& [id, station] : stations) {
        stations_json.push_back(std::array<int, 3>{
            station.id,
            static_cast<int>(std::round(station.norm_x * 10000)),
            static_cast<int>(std::round(station.norm_y * 10000))
        });
    }
    j["Stations"] = stations_json;

    nlohmann::json lines_json = nlohmann::json::array();
    for (const auto& [id, line] : lines) {
        lines_json.push_back(nlohmann::json{
            {"Id", line.id},
            {"Stas", line.station_ids},
            {"IsNotLoop", !line.is_loop}
        });
    }
    j["Lines"] = lines_json;

    return j;
}

} // namespace rc