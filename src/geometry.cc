#include <cmath>

#include "geometry.h"

namespace geometry {

Vec2 Vec2::operator+(const Vec2& other) const {
    return {x + other.x, y + other.y};
}

Vec2 Vec2::operator-(const Vec2& other) const {
    return {x - other.x, y - other.y};
}

Vec2 Vec2::operator*(double scalar) const {
    return {x * scalar, y * scalar};
}

Vec2 Vec2::operator/(double scalar) const {
    return {x / scalar, y / scalar};
}

double Vec2::dot(const Vec2& other) const {
    return x * other.x + y * other.y;
}

double Vec2::cross(const Vec2& other) const {
    return x * other.y - y * other.x;
}

double Vec2::length() const {
    return std::sqrt(x * x + y * y);
}

Vec2 Vec2::normalized() const {
    double len = length();
    return {x / len, y / len};
}

Vec2 Vec2::perpendicular() const {
    return {-y, x};
}

double Vec2::angle() const {
    return std::atan2(y, x);
}

Vec2 Vec2::polar(double angle, double length) {
    return {std::cos(angle) * length, std::sin(angle) * length};
}

Vec2& Vec2::operator+=(const Vec2& other) {
    x += other.x;
    y += other.y;
    return *this;
}

Vec2& Vec2::operator-=(const Vec2& other) {
    x -= other.x;
    y -= other.y;
    return *this;
}

Vec2& Vec2::operator*=(double scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
}

Vec2& Vec2::operator/=(double scalar) {
    x /= scalar;
    y /= scalar;
    return *this;
}

bool Vec2::operator==(const Vec2& other) const {
    return x == other.x && y == other.y;
}

bool Map::can_move_through(int point1_id, int point2_id, int point3_id) const {
    if (!points.contains(point1_id) || !points.contains(point2_id) || !points.contains(point3_id)) {
        return false;
    }
    const Point& p1 = points.at(point1_id);
    const Point& p2 = points.at(point2_id);
    const Point& p3 = points.at(point3_id);
    return (p2.pos - p1.pos).dot(p3.pos - p2.pos) >= 0;
}

Position Map::group_pos(int group_id) const {
    if (!station_groups.contains(group_id)) {
        return {0.0, 0.0};
    }
    const StationGroup& group = station_groups.at(group_id);
    if (group.station_ids.empty()) {
        return {0.0, 0.0};
    }
    Position sum_pos = {0.0, 0.0};
    int count = 0;
    for (int station_id : group.station_ids) {
        if (points.contains(station_id)) {
            sum_pos += points.at(station_id).pos;
            ++count;
        }
    }
    if (count == 0) {
        return {0.0, 0.0};
    }
    return sum_pos / static_cast<double>(count);
}

Position Map::normalized_pos(const Position& pos) const {
    return {pos.x / width, pos.y / height};
}

Map::Map(const nlohmann::json& aarc, const nlohmann::json& config_json) {
    int max_line_id = 0;

    // helper lambdas
    auto connect_lines = [&](int line1_id, int line2_id, bool forced = false) {
        if (line1_id == line2_id && !forced) return;
        friend_lines.insert({line1_id, line2_id});
        friend_lines.insert({line2_id, line1_id});
    };

    auto merge_lines = [&](int line1_id, int line2_id, bool forced = false) {
        if (line1_id == line2_id && !forced) return;
        merged_lines.insert({line1_id, line2_id});
        merged_lines.insert({line2_id, line1_id});
    };

    auto join_stations = [&](int station1_id, int station2_id) {
        if (station1_id == station2_id) return;
        auto it1 = point_to_group.find(station1_id);
        auto it2 = point_to_group.find(station2_id);
        if (it1 != point_to_group.end() && it2 != point_to_group.end()) {
            if (it1->second == it2->second) return;
            // merge groups
            StationGroup* group1 = it1->second;
            StationGroup* group2 = it2->second;
            for (int sid : group2->station_ids) {
                group1->station_ids.push_back(sid);
                point_to_group[sid] = group1;
            }
            station_groups.erase(group2->id);
        } else if (it1 != point_to_group.end()) {
            // add to existing group
            StationGroup* group = it1->second;
            group->station_ids.push_back(station2_id);
            point_to_group[station2_id] = group;
        } else if (it2 != point_to_group.end()) {
            // add to existing group
            StationGroup* group = it2->second;
            group->station_ids.push_back(station1_id);
            point_to_group[station1_id] = group;
        } else {
            // create new group
            int new_group_id = station1_id;
            StationGroup new_group;
            new_group.id = new_group_id;
            new_group.name = "Station Group " + std::to_string(new_group_id);
            new_group.station_ids = {station1_id, station2_id};
            station_groups[new_group_id] = new_group;
            point_to_group[station1_id] = &station_groups[new_group_id];
            point_to_group[station2_id] = &station_groups[new_group_id];
        }
    };

    // load dimensions
    if (aarc.contains("cvsSize")) {
        width = aarc["cvsSize"][0].get<double>();
        height = aarc["cvsSize"][1].get<double>();
    } else {
        width = 1024.0;
        height = 1024.0;
    }

    // load points
    if (aarc.contains("points")) {
        for (const auto& item : aarc["points"]) {
            Point p;
            p.id = item["id"].get<int>();
            if (item.contains("name")) p.name = item["name"].get<std::string>();
            p.pos = {item["pos"][0].get<double>(), item["pos"][1].get<double>()};
            p.dir = static_cast<Point::Direction>(item["dir"].get<int>());
            p.type = static_cast<Point::Type>(item["sta"].get<int>());
            points[p.id] = std::move(p);
        }
    }

    // load lines
    if (aarc.contains("lines")) {
        for (const auto& item : aarc["lines"]) {
            if (item.contains("type") && item["type"].get<int>() != 0) continue;
            if (item.contains("isFake") && item["isFake"].get<bool>()) continue;
            Line l;
            l.id = item["id"].get<int>();
            if (item.contains("name")) l.name = item["name"].get<std::string>();
            for (const auto& pid : item["pts"]) {
                l.point_ids.push_back(pid.get<int>());
            }
            l.is_loop = l.point_ids.size() >= 2 && l.point_ids.front() == l.point_ids.back();
            if (item.contains("parent")) {
                l.parent_id = item["parent"].get<int>();
                connect_lines(l.id, l.parent_id);
            } else {
                l.parent_id = -1;
            }
            if (l.id > max_line_id) max_line_id = l.id;
            lines[l.id] = std::move(l);
        }
    }
    
    // parse config
    if (config_json.contains("maximum_service_length")) {
        config.maximum_service_length = config_json["maximum_service_length"].get<int>();
    }
    if (config_json.contains("raw_group_distance")) {
        config.auto_group_distance = config_json["raw_group_distance"].get<double>();
    }
    if (config_json.contains("group_distance")) {
        config.auto_group_distance = config_json["group_distance"].get<double>() * 30.0;
    }
    if (config_json.contains("merge_consecutive_duplicates")) {
        config.merge_consecutive_duplicates = config_json["merge_consecutive_duplicates"].get<bool>();
    }

    if (config_json.contains("link_modes")) {
        for (auto& [key, value] : config_json["link_modes"].items()) {
            Config::LinkType type;
            if (key == "ThickLine") type = Config::LinkType::ThickLine;
            else if (key == "ThinLine") type = Config::LinkType::ThinLine;
            else if (key == "DottedLine1") type = Config::LinkType::DottedLine1;
            else if (key == "DottedLine2") type = Config::LinkType::DottedLine2;
            else if (key == "Group") type = Config::LinkType::Group;
            else continue;

            std::string mode_str = value.get<std::string>();
            Config::LinkMode mode;
            if (mode_str == "Connect") mode = Config::LinkMode::Connect;
            else if (mode_str == "Group") mode = Config::LinkMode::Group;
            else if (mode_str == "None") mode = Config::LinkMode::None;
            else continue;

            config.link_modes[type] = mode;
        }
    }

    if (config_json.contains("friend_lines")) {
        for (const auto& pair : config_json["friend_lines"]) {
            if (pair.size() != 2) continue;
            // the pair contains two entries, each of which can be a string (line name) or int (line id)
            int line_ids[2] = {-1, -1};
            for (int i = 0; i < 2; ++i) {
                if (pair[i].is_string()) {
                    std::string name = pair[i].get<std::string>();
                    for (const auto& [id, line] : lines) {
                        if (line.name == name) {
                            line_ids[i] = id;
                            break;
                        }
                    }
                } else if (pair[i].is_number_integer()) {
                    int id = pair[i].get<int>();
                    if (lines.find(id) != lines.end()) {
                        line_ids[i] = id;
                    }
                }
            }
            if (line_ids[0] != -1 && line_ids[1] != -1) {
                connect_lines(line_ids[0], line_ids[1], true);
            }
        }
    }

    if (config_json.contains("merged_lines")) {
        for (const auto& pair : config_json["merged_lines"]) {
            if (pair.size() != 2) continue;
            // the pair contains two entries, each of which can be a string (line name) or int (line id)
            int line_ids[2] = {-1, -1};
            for (int i = 0; i < 2; ++i) {
                if (pair[i].is_string()) {
                    std::string name = pair[i].get<std::string>();
                    for (const auto& [id, line] : lines) {
                        if (line.name == name) {
                            line_ids[i] = id;
                            break;
                        }
                    }
                } else if (pair[i].is_number_integer()) {
                    int id = pair[i].get<int>();
                    if (lines.find(id) != lines.end()) {
                        line_ids[i] = id;
                    }
                }
            }
            if (line_ids[0] != -1 && line_ids[1] != -1) {
                merge_lines(line_ids[0], line_ids[1], true);
            }
        }
    }

    // load point links
    if (aarc.contains("pointLinks")) {
        for (const auto& item : aarc["pointLinks"]) {
            int p1_id = item["pts"][0].get<int>();
            int p2_id = item["pts"][1].get<int>();
            Config::LinkType type = static_cast<Config::LinkType>(item["type"].get<int>());
            Config::LinkMode mode = config.link_modes[type];
            if (mode == Config::LinkMode::None) continue;
            if (mode == Config::LinkMode::Connect) {
                Line l;
                l.id = ++max_line_id;
                l.name = "PointLink_" + std::to_string(l.id);
                l.point_ids = {p1_id, p2_id};
                l.is_loop = false;
                l.parent_id = -1;
                lines[l.id] = std::move(l);
            }
            if (mode == Config::LinkMode::Group) {
                join_stations(p1_id, p2_id);
            }
        }
    }

    // group nearby stations
    for (const auto& [id1, p1] : points) {
        if (p1.type != Point::Type::Station) continue;
        for (const auto& [id2, p2] : points) {
            if (id1 >= id2) continue;
            if (p2.type != Point::Type::Station) continue;
            if ((p1.pos - p2.pos).length() <= config.auto_group_distance) {
                join_stations(id1, id2);
            }
        }
    }

    // connect lines with common parents
    for (const auto& [id1, line1] : lines) {
        if (line1.parent_id == -1) continue;
        for (const auto& [id2, line2] : lines) {
            if (id1 >= id2) continue;
            if (line1.parent_id == line2.parent_id) {
                connect_lines(id1, id2);
            }
        }
    }
}

} // namespace geometry