#pragma once

#include <unordered_map>
#include <unordered_set>

#include "json.hpp"

namespace rc {

struct Station {
    int id;
    double norm_x;
    double norm_y;
};

struct Line {
    int id;
    std::vector<int> station_ids;
    bool is_loop;
};

struct Map {
    std::unordered_map<int, Station> stations;
    std::unordered_map<int, Line> lines;

    nlohmann::json to_json() const;
};

} // namespace rc