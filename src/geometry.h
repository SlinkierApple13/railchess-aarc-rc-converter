#pragma once

#include <unordered_map>
#include <unordered_set>

#include "json.hpp"

// A hash function for std::pair
namespace std {
    template <>
    struct hash<std::pair<int, int>> {
        size_t operator()(const std::pair<int, int>& p) const {
            auto h1 = std::hash<int>{}(p.first);
            auto h2 = std::hash<int>{}(p.second);
            return h1 ^ (h2 << 1);
        }
    };
}

namespace geometry {

struct Vec2 {
    double x;
    double y;

    Vec2 operator+(const Vec2& other) const;
    Vec2 operator-(const Vec2& other) const;
    Vec2 operator*(double scalar) const;
    Vec2 operator/(double scalar) const;
    double dot(const Vec2& other) const;
    double cross(const Vec2& other) const;
    double length() const;
    Vec2 normalized() const;
    Vec2 perpendicular() const;
    double angle() const;
    static Vec2 polar(double angle, double length);
    Vec2& operator+=(const Vec2& other);
    Vec2& operator-=(const Vec2& other);
    Vec2& operator*=(double scalar);
    Vec2& operator/=(double scalar);
    bool operator==(const Vec2& other) const;
};

using Position = Vec2;

struct Point {
    int id;
    double size = 1.0;
    std::string name;
    Position pos;

    enum class Direction {
        Orthogonal,
        Diagonal
    } dir;
    
    enum class Type {
        Node,
        Station
    } type;
};

struct Line {
    int id;
    std::string name;
    std::vector<int> point_ids;
    bool is_loop;
    bool is_simple;
    int parent_id;
};

struct StationGroup {
    int id;
    std::string name;
    std::vector<int> station_ids;
};

struct Map {
    struct Config {
        int max_length = 128;
        int max_rc_steps = 16;
        double auto_group_distance = 25.0;
        bool merge_consecutive_duplicates = true;
        bool optimize_segmentation = false;
        int max_iterations = 4;
        
        enum class LinkMode {
            Connect,
            Group,
            None
        };

        enum class LinkType {
            ThickLine,
            ThinLine,
            DottedLine1,
            DottedLine2,
            Group
        };

        std::unordered_map<LinkType, LinkMode> link_modes = {
            {LinkType::ThickLine, LinkMode::Connect},
            {LinkType::ThinLine, LinkMode::Connect},
            {LinkType::DottedLine1, LinkMode::None},
            {LinkType::DottedLine2, LinkMode::None},
            {LinkType::Group, LinkMode::Group}
        };

        std::unordered_set<std::pair<int, int>> friend_lines;
        std::unordered_set<std::pair<int, int>> merged_lines;
        std::unordered_map<int, int> segmented_lines; // line_id -> segment_length
    } config;

    double width;
    double height;

    std::unordered_map<int, Point> points;
    std::unordered_map<int, Line> lines;

    std::unordered_map<int, StationGroup> station_groups;
    std::unordered_map<int, StationGroup*> point_to_group;

    bool can_move_through(int point1_id, int point2_id, int point3_id) const;
    Position group_pos(int group_id) const;
    Position normalized_pos(const Position& pos) const;

    Map(const nlohmann::json& aarc, const nlohmann::json& config_json);
};

} // namespace geometry