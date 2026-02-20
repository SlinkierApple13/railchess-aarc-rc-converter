#include <algorithm>
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

// Helper enum for position relationships between two points
enum class PosRel {
    Same,        // 's' - same position
    Left,        // 'l' - purely horizontal
    LeftLeftUp,  // 'llu' - more left than up
    LeftUp,      // 'lu' - diagonal left-up (45 degrees)
    LeftUpUp,    // 'luu' - more up than left
    Up,          // 'u' - purely vertical
    UpUpRight,   // 'uur' - more up than right
    UpRight,     // 'ur' - diagonal up-right (45 degrees)
    UpRightRight // 'urr' - more right than up
};

// Helper function to determine position relationship
struct RelResult {
    PosRel pos_rel;
    bool reversed;
};

static const double EPSILON = 1e-9;

static bool is_zero(double val) {
    return std::abs(val) < EPSILON;
}

static RelResult coord_rel_diff(double x_diff, double y_diff) {
    if (is_zero(x_diff)) {
        if (is_zero(y_diff))
            return {PosRel::Same, false};
        return {PosRel::Up, y_diff > 0};
    }
    if (is_zero(y_diff)) {
        return {PosRel::Left, x_diff > 0};
    }
    if (is_zero(x_diff - y_diff)) {
        return {PosRel::LeftUp, x_diff > 0};
    }
    if (is_zero(x_diff + y_diff)) {
        return {PosRel::UpRight, y_diff > 0};
    }
    if ((y_diff > 0 && x_diff > y_diff) || (y_diff < 0 && x_diff < y_diff)) {
        return {PosRel::LeftLeftUp, y_diff > 0};
    }
    if ((x_diff > 0 && y_diff > x_diff) || (x_diff < 0 && y_diff < x_diff)) {
        return {PosRel::LeftUpUp, x_diff > 0};
    }
    if ((y_diff > 0 && -x_diff < y_diff) || (y_diff < 0 && x_diff < -y_diff)) {
        return {PosRel::UpUpRight, y_diff > 0};
    }
    return {PosRel::UpRightRight, x_diff < 0};
}

// Helper function to fill intermediate points
enum class FillType {
    Top,
    Bottom,
    MidVert,
    MidInc
};

static std::vector<Position> coord_fill_unordered(
    const Position& a, const Position& b, 
    double x_diff, double y_diff,
    PosRel pos_rel, FillType type
) {
    // For simple relations, no intermediate points needed
    if (pos_rel == PosRel::Left || pos_rel == PosRel::Up || 
        pos_rel == PosRel::LeftUp || pos_rel == PosRel::UpRight) {
        return {};
    }
    
    if (pos_rel == PosRel::LeftLeftUp) {
        double bias = -x_diff + y_diff;
        if (type == FillType::Top) {
            return {{a.x + bias, a.y}};
        } else if (type == FillType::Bottom) {
            return {{b.x - bias, b.y}};
        } else if (type == FillType::MidInc) {
            bias = bias / 2.0;
            return {{a.x + bias, a.y}, {b.x - bias, b.y}};
        } else { // MidVert
            bias = -y_diff / 2.0;
            return {{a.x + bias, a.y + bias}, {b.x - bias, b.y - bias}};
        }
    } else if (pos_rel == PosRel::LeftUpUp) {
        double bias = x_diff - y_diff;
        if (type == FillType::Top) {
            return {{b.x, b.y - bias}};
        } else if (type == FillType::Bottom) {
            return {{a.x, a.y + bias}};
        } else if (type == FillType::MidInc) {
            bias = bias / 2.0;
            return {{a.x, a.y + bias}, {b.x, b.y - bias}};
        } else { // MidVert
            bias = -x_diff / 2.0;
            return {{a.x + bias, a.y + bias}, {b.x - bias, b.y - bias}};
        }
    } else if (pos_rel == PosRel::UpUpRight) {
        double bias = -x_diff - y_diff;
        if (type == FillType::Top) {
            return {{b.x, b.y - bias}};
        } else if (type == FillType::Bottom) {
            return {{a.x, a.y + bias}};
        } else if (type == FillType::MidInc) {
            bias = bias / 2.0;
            return {{a.x, a.y + bias}, {b.x, b.y - bias}};
        } else { // MidVert
            bias = -x_diff / 2.0;
            return {{a.x + bias, a.y - bias}, {b.x - bias, b.y + bias}};
        }
    } else if (pos_rel == PosRel::UpRightRight) {
        double bias = x_diff + y_diff;
        if (type == FillType::Top) {
            return {{a.x - bias, a.y}};
        } else if (type == FillType::Bottom) {
            return {{b.x + bias, b.y}};
        } else if (type == FillType::MidInc) {
            bias = bias / 2.0;
            return {{a.x - bias, a.y}, {b.x + bias, b.y}};
        } else { // MidVert
            bias = y_diff / 2.0;
            return {{a.x + bias, a.y - bias}, {b.x - bias, b.y + bias}};
        }
    }
    
    return {};
}

static std::vector<Position> coord_fill(
    const Position& a, const Position& b,
    double x_diff, double y_diff,
    PosRel pos_rel, bool reversed, FillType type
) {
    auto result = coord_fill_unordered(a, b, x_diff, y_diff, pos_rel, type);
    if (reversed) {
        std::reverse(result.begin(), result.end());
    }
    return result;
}

// Helper structures for formalization
struct FormalSegment {
    Position a;
    std::vector<Position> itp;  // intermediate points
    Position b;
    int ill;  // ill-posed level: 0=good, 1=one intermediate, 2=problematic
};

// Ray structure for intersection calculations
struct Ray {
    Position source;
    Vec2 direction;  // normalized direction vector
};

static Ray create_ray(const Position& from, const Position& to) {
    Vec2 dir = to - from;
    double len = dir.length();
    if (len < EPSILON) {
        return {from, {0, 0}};
    }
    return {from, dir / len};
}

static bool rays_perpendicular(const Ray& a, const Ray& b) {
    return std::abs(a.direction.dot(b.direction)) < EPSILON;
}

static bool rays_parallel(const Ray& a, const Ray& b) {
    return std::abs(a.direction.cross(b.direction)) < EPSILON;
}

static double ray_to_point_distance(const Ray& ray, const Position& point) {
    Vec2 to_point = point - ray.source;
    // For axis-aligned and diagonal rays, calculate perpendicular distance
    double cross = std::abs(ray.direction.cross(to_point));
    return cross;
}

static Position ray_intersect(const Ray& a, const Ray& b, bool perp_only = false) {
    if (rays_parallel(a, b)) {
        return {NAN, NAN};  // No intersection
    }
    if (perp_only && !rays_perpendicular(a, b)) {
        return {NAN, NAN};
    }
    
    // Calculate intersection point using parametric form
    // a.source + t * a.direction = b.source + s * b.direction
    Vec2 diff = b.source - a.source;
    double cross = a.direction.cross(b.direction);
    
    if (std::abs(cross) < EPSILON) {
        return {NAN, NAN};
    }
    
    double t = diff.cross(b.direction) / cross;
    
    Position result = a.source + a.direction * t;
    return result;
}

static Ray rotate_ray_90(const Ray& ray) {
    return {ray.source, ray.direction.perpendicular()};
}

// Formalize a segment between two points
static FormalSegment formalize_segment(const Point& point_a, const Point& point_b) {
    double x_diff = point_a.pos.x - point_b.pos.x;
    double y_diff = point_a.pos.y - point_b.pos.y;
    
    RelResult rel = coord_rel_diff(x_diff, y_diff);
    PosRel pr = rel.pos_rel;
    bool rv = rel.reversed;
    
    // If points are at same position
    if (pr == PosRel::Same) {
        return {point_a.pos, {}, point_b.pos, 0};
    }
    
    const Point* p_a = &point_a;
    const Point* p_b = &point_b;
    
    if (rel.reversed) {
        std::swap(p_a, p_b);
        x_diff = -x_diff;
        y_diff = -y_diff;
    }
    
    std::vector<Position> itp;
    int ill = 0;
    
    if (p_a->dir == p_b->dir) {
        // Both points have same direction
        if (p_a->dir == Point::Direction::Diagonal) {
            itp = coord_fill(p_a->pos, p_b->pos, x_diff, y_diff, pr, rv, FillType::MidVert);
        } else {
            itp = coord_fill(p_a->pos, p_b->pos, x_diff, y_diff, pr, rv, FillType::MidInc);
        }
        
        if (itp.empty()) {
            // Check if this is ill-posed
            if ((p_a->dir == Point::Direction::Orthogonal && (pr == PosRel::LeftUp || pr == PosRel::UpRight))
                || (p_a->dir == Point::Direction::Diagonal && (pr == PosRel::Left || pr == PosRel::Up))) {
                ill = 2;  // Highly problematic
            } else {
                ill = 0;  // OK, no intermediate points needed
            }
        } else {
            ill = 1;  // Has intermediate points
        }
    } else if (p_a->dir == Point::Direction::Diagonal) {
        // Point a is diagonal, point b is orthogonal
        if (pr == PosRel::LeftUpUp || pr == PosRel::UpUpRight) {
            itp = coord_fill(p_a->pos, p_b->pos, x_diff, y_diff, pr, rv, FillType::Top);
        } else {
            itp = coord_fill(p_a->pos, p_b->pos, x_diff, y_diff, pr, rv, FillType::Bottom);
        }
    } else {
        // Point a is orthogonal, point b is diagonal
        if (pr == PosRel::LeftUpUp || pr == PosRel::UpUpRight) {
            itp = coord_fill(p_a->pos, p_b->pos, x_diff, y_diff, pr, rv, FillType::Bottom);
        } else {
            itp = coord_fill(p_a->pos, p_b->pos, x_diff, y_diff, pr, rv, FillType::Top);
        }
    }
    
    return {point_a.pos, itp, point_b.pos, ill};
}

// Correct ill-posed segments using neighboring segments
static void ill_posed_segment_justify(std::vector<FormalSegment>& segs) {
    if (segs.size() <= 1) {
        return;
    }
    
    // Find all ill-posed segments
    std::vector<size_t> ill_idxs;
    for (size_t i = 0; i < segs.size(); i++) {
        if (segs[i].ill > 0) {
            ill_idxs.push_back(i);
        }
    }
    
    // Correct each ill-posed segment
    for (size_t i : ill_idxs) {
        FormalSegment& this_seg = segs[i];
        
        if (i > 0 && i < segs.size() - 1) {
            // Middle segment: correct using both neighbors
            const FormalSegment& prev_seg = segs[i - 1];
            const FormalSegment& next_seg = segs[i + 1];
            
            bool prev_helps = prev_seg.ill < this_seg.ill;
            bool next_helps = next_seg.ill < this_seg.ill;
            
            if (prev_helps && next_helps) {
                // Get reference points
                Position prev_ref = prev_seg.itp.empty() ? prev_seg.a : prev_seg.itp.back();
                Position next_ref = next_seg.itp.empty() ? next_seg.b : next_seg.itp.front();
                
                // Create rays
                Ray prev_ray = create_ray(prev_ref, prev_seg.b);
                Ray next_ray = create_ray(next_ref, next_seg.a);
                
                // Find intersection
                Position itsc = ray_intersect(prev_ray, next_ray, true);
                if (!std::isnan(itsc.x)) {
                    this_seg.itp = {itsc};
                }
            }
        } else {
            // End segment: correct using nearest neighbor
            auto correct_end = [&](const Position& neib_ref, const Position& share,
                                  const Position* this_ref, const Position& this_tip) -> Position {
                Ray neib_ray = create_ray(neib_ref, share);
                
                if (!this_ref) {
                    // If only the tip point exists in the segment
                    if (ray_to_point_distance(neib_ray, this_tip) < EPSILON) {
                        // Tip is already on the neighbor ray extension
                        return {NAN, NAN};
                    }
                    // Create perpendicular ray from tip
                    Ray this_ray = rotate_ray_90(neib_ray);
                    this_ray.source = this_tip;
                    return ray_intersect(neib_ray, this_ray, true);
                } else {
                    // If segment has other points besides tip
                    Ray this_ray = create_ray(*this_ref, share);
                    this_ray.source = this_tip;
                    if (rays_perpendicular(neib_ray, this_ray)) {
                        return ray_intersect(neib_ray, this_ray, true);
                    }
                }
                return {NAN, NAN};
            };
            
            Position itsc{NAN, NAN};
            
            if (i == segs.size() - 1) {
                // Last segment
                const FormalSegment& prev_seg = segs[i - 1];
                bool can_help = prev_seg.ill <= this_seg.ill && prev_seg.ill < 2;
                bool need_help = this_seg.ill > 0;
                
                if (need_help && can_help) {
                    Position neib_ref = prev_seg.itp.empty() ? prev_seg.a : prev_seg.itp.back();
                    Position share = this_seg.a;
                    const Position* this_ref = this_seg.itp.size() > 1 ? &this_seg.itp[0] : nullptr;
                    Position this_tip = this_seg.b;
                    itsc = correct_end(neib_ref, share, this_ref, this_tip);
                }
            } else if (i == 0) {
                // First segment
                const FormalSegment& next_seg = segs[i + 1];
                bool can_help = next_seg.ill <= this_seg.ill && next_seg.ill < 2;
                bool need_help = this_seg.ill > 0;
                
                if (can_help && need_help) {
                    Position neib_ref = next_seg.itp.empty() ? next_seg.b : next_seg.itp.front();
                    Position share = this_seg.b;
                    const Position* this_ref = this_seg.itp.size() > 1 ? &this_seg.itp[1] : nullptr;
                    Position this_tip = this_seg.a;
                    itsc = correct_end(neib_ref, share, this_ref, this_tip);
                }
            }
            
            if (!std::isnan(itsc.x)) {
                this_seg.itp = {itsc};
            }
        }
    }
}

void add_auxiliary_points(Map& map) {
    auto get_max_point_id = [&]() {
        int max_id = 0;
        for (const auto& [id, point] : map.points) {
            if (id > max_id) max_id = id;
        }
        return max_id;
    };
    int next_id = get_max_point_id() + 1;

    // Process each line to add auxiliary points
    for (auto& [line_id, line] : map.lines) {
        if (line.point_ids.size() < 2) {
            continue;
        }

        // Check if this is a loop/ring (first and last points are the same)
        bool is_ring = line.is_loop;
        
        std::vector<FormalSegment> formal_segs;
        
        if (!is_ring) {
            // Non-ring line: process segments normally
            for (size_t i = 0; i < line.point_ids.size() - 1; i++) {
                int point_a_id = line.point_ids[i];
                int point_b_id = line.point_ids[i + 1];
                
                if (!map.points.contains(point_a_id) || !map.points.contains(point_b_id)) {
                    continue;
                }
                
                const Point& point_a = map.points.at(point_a_id);
                const Point& point_b = map.points.at(point_b_id);
                
                FormalSegment seg = formalize_segment(point_a, point_b);
                formal_segs.push_back(seg);
            }
        } else {
            // Ring line: add margin segments for proper correction
            // Add head margin segment (second-to-last to first)
            if (line.point_ids.size() >= 3) {
                int a_id = line.point_ids[line.point_ids.size() - 2];
                int b_id = line.point_ids[0];
                
                if (map.points.contains(a_id) && map.points.contains(b_id)) {
                    const Point& point_a = map.points.at(a_id);
                    const Point& point_b = map.points.at(b_id);
                    FormalSegment head_margin = formalize_segment(point_a, point_b);
                    formal_segs.push_back(head_margin);
                }
            }
            
            // Add normal segments
            for (size_t i = 0; i < line.point_ids.size() - 1; i++) {
                int point_a_id = line.point_ids[i];
                int point_b_id = line.point_ids[i + 1];
                
                if (!map.points.contains(point_a_id) || !map.points.contains(point_b_id)) {
                    continue;
                }
                
                const Point& point_a = map.points.at(point_a_id);
                const Point& point_b = map.points.at(point_b_id);
                
                FormalSegment seg = formalize_segment(point_a, point_b);
                formal_segs.push_back(seg);
            }
            
            // Add tail margin segment (last to second)
            if (line.point_ids.size() >= 3) {
                int c_id = line.point_ids[line.point_ids.size() - 1];
                int d_id = line.point_ids[1];
                
                if (map.points.contains(c_id) && map.points.contains(d_id)) {
                    const Point& point_c = map.points.at(c_id);
                    const Point& point_d = map.points.at(d_id);
                    FormalSegment tail_margin = formalize_segment(point_c, point_d);
                    formal_segs.push_back(tail_margin);
                }
            }
        }
        
        // Apply ill-posed correction
        ill_posed_segment_justify(formal_segs);
        
        if (formal_segs.empty()) {
            continue;
        }
        
        // Remove margin segments for rings
        if (is_ring && formal_segs.size() > 2) {
            formal_segs.erase(formal_segs.begin());  // Remove head margin
            formal_segs.pop_back();                   // Remove tail margin
        }
        
        // Build new point list with auxiliary points
        std::vector<int> new_point_ids;
        new_point_ids.push_back(line.point_ids[0]);
        
        for (size_t i = 0; i < formal_segs.size(); i++) {
            const FormalSegment& seg = formal_segs[i];
            
            // Add intermediate points for this segment
            for (const Position& aux_pos : seg.itp) {
                Point aux_point;
                aux_point.id = next_id++;
                aux_point.name = "";
                aux_point.pos = aux_pos;
                aux_point.dir = Point::Direction::Orthogonal;
                aux_point.type = Point::Type::Node;
                
                map.points[aux_point.id] = aux_point;
                new_point_ids.push_back(aux_point.id);
            }
            
            // Add the end point of this segment (which is the start of next segment)
            // For the last segment, add the actual endpoint
            if (i < line.point_ids.size() - 1) {
                new_point_ids.push_back(line.point_ids[i + 1]);
            }
        }
        
        // For non-ring lines, ensure the last point is included
        if (!is_ring && !line.point_ids.empty()) {
            if (new_point_ids.empty() || new_point_ids.back() != line.point_ids.back()) {
                new_point_ids.push_back(line.point_ids.back());
            }
        }
        
        // Update the line with new point IDs including auxiliary points
        line.point_ids = new_point_ids;
    }
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
        config.friend_lines.insert({line1_id, line2_id});
        config.friend_lines.insert({line2_id, line1_id});
    };

    auto merge_lines = [&](int line1_id, int line2_id, bool forced = false) {
        if (line1_id == line2_id && !forced) return;
        config.merged_lines.insert({line1_id, line2_id});
        config.merged_lines.insert({line2_id, line1_id});
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

    // load line width -> point size map
    std::unordered_map<int, double> line_width_to_point_size;
    if (aarc.contains("config")) {
        const auto& conf = aarc["config"];
        if (conf.contains("lineWidthMapped")) {
            const auto& lw_map = conf["lineWidthMapped"];
            if (lw_map.is_object()) {
                for (auto& [key, value] : lw_map.items()) {
                    double line_width;
                    try {
                        line_width = std::stod(key);
                    } catch (...) {
                        continue;
                    }
                    if (value.contains("staSize")) {
                        double point_size = value["staSize"].get<double>();
                        line_width_to_point_size[static_cast<int>(line_width * 100.0 + 0.5)] = point_size;
                    }
                }
            }
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

            // get point size
            double point_size = 0.0;
            if (item.contains("ptSize")) {
                if (item["ptSize"].is_number_integer()) {
                    point_size = static_cast<double>(item["ptSize"].get<int>());
                } else if (item["ptSize"].is_number_float()) {
                    point_size = item["ptSize"].get<double>();
                } else if (item["ptSize"].is_string()) {
                    try {
                        point_size = std::stod(item["ptSize"].get<std::string>());
                    } catch (...) {
                        point_size = 1.0;
                    }
                } else {
                    point_size = 1.0;
                }
            }
            if (point_size < 1e-3) {
                if (item.contains("width")) {
                    double line_width;
                    if (item["width"].is_number_integer()) {
                        line_width = static_cast<double>(item["width"].get<int>());
                    } else if (item["width"].is_number_float()) {
                        line_width = item["width"].get<double>();
                    } else if (item["width"].is_string()) {
                        try {
                            line_width = std::stod(item["width"].get<std::string>());
                        } catch (...) {
                            line_width = 1.0;
                        }
                    } else {
                        line_width = 1.0;
                    }
                    int lw_key = static_cast<int>(line_width * 100.0 + 0.5);
                    if (line_width_to_point_size.contains(lw_key)) {
                        point_size = line_width_to_point_size[lw_key];
                    } else {
                        point_size = line_width;
                    }
                } else {
                    point_size = 1.0;
                }
            }

            // update point size for all points in this line
            for (int pid : l.point_ids) {
                if (points.contains(pid)) {
                    Point& p = points.at(pid);
                    p.size = std::max(p.size, point_size);
                }
            }

            lines[l.id] = std::move(l);
        }
    }

    // set size to 1.0 for points that are not on any line
    for (auto& [pid, point] : points) {
        if (point.size < 1e-3) {
            point.size = 1.0;
        }
    }
    
    // parse config
    if (config_json.contains("max_length")) {
        int max_length = config_json["max_length"].get<int>();
        if (max_length > 0) {
            config.max_length = max_length;
        }
    }
    if (config_json.contains("max_rc_steps")) {
        int max_rc_steps = config_json["max_rc_steps"].get<int>();
        if (max_rc_steps > 0) {
            config.max_rc_steps = max_rc_steps;
        }
    }
    if (config_json.contains("max_iterations")) {
        int max_iterations = config_json["max_iterations"].get<int>();
        if (max_iterations > 0) {
            config.max_iterations = max_iterations;
        }
    }
    if (config_json.contains("merge_consecutive_duplicates")) {
        config.merge_consecutive_duplicates = config_json["merge_consecutive_duplicates"].get<bool>();
    }
    if (config_json.contains("optimize_segmentation")) {
        config.optimize_segmentation = config_json["optimize_segmentation"].get<bool>();
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

    if (config_json.contains("segmented_lines")) {
        int param_ind = 0;
        for (const auto& entry : config_json["segmented_lines"]) {
            ++param_ind;
            if (entry.is_array()) {
                for (const auto& sub_entry : entry) {
                    if (sub_entry.is_string() || sub_entry.is_number_integer()) {
                        int line_id = -1;
                        if (sub_entry.is_string()) {
                            std::string name = sub_entry.get<std::string>();
                            for (const auto& [id, line] : lines) {
                                if (line.name == name) {
                                    line_id = id;
                                    break;
                                }
                            }
                        } else if (sub_entry.is_number_integer()) {
                            int id = sub_entry.get<int>();
                            if (lines.find(id) != lines.end()) {
                                line_id = id;
                            }
                        }
                        if (line_id != -1) {
                            config.segmented_lines[line_id] = -param_ind;
                        }
                    }
                }
                continue;
            }
            if (entry.is_string() || entry.is_number_integer()) {
                int line_id = -1;
                if (entry.is_string()) {
                    std::string name = entry.get<std::string>();
                    for (const auto& [id, line] : lines) {
                        if (line.name == name) {
                            line_id = id;
                            break;
                        }
                    }
                } else if (entry.is_number_integer()) {
                    int id = entry.get<int>();
                    if (lines.find(id) != lines.end()) {
                        line_id = id;
                    }
                }
                if (line_id != -1) {
                    config.segmented_lines[line_id] = -param_ind;
                }
                continue;
            }
            if (!entry.contains("line") && !entry.contains("lines")) continue;
            int seg_len = -param_ind;
            if (entry.contains("segment_length")) {
                int seg_len1 = entry["segment_length"].get<int>();
                if (seg_len1 > 0) {
                    seg_len = seg_len1;
                }
            }
            if (entry.contains("line") && entry["line"].is_string()) {
                std::string name = entry["line"].get<std::string>();
                for (const auto& [id, line] : lines) {
                    if (line.name == name) {
                        config.segmented_lines[id] = seg_len;
                        break;
                    }
                }
            } else if (entry.contains("line") && entry["line"].is_number_integer()) {
                int id = entry["line"].get<int>();
                if (lines.find(id) != lines.end()) {
                    config.segmented_lines[id] = seg_len;
                }
            } else if (entry.contains("lines") && entry["lines"].is_array()) {
                for (const auto& sub_entry : entry["lines"]) {
                    if (sub_entry.is_string()) {
                        std::string name = sub_entry.get<std::string>();
                        for (const auto& [id, line] : lines) {
                            if (line.name == name) {
                                config.segmented_lines[id] = seg_len;
                                break;
                            }
                        }
                    } else if (sub_entry.is_number_integer()) {
                        int id = sub_entry.get<int>();
                        if (lines.find(id) != lines.end()) {
                            config.segmented_lines[id] = seg_len;
                        }
                    }
                }
            }
        }
    }

    // add auxiliary points
    add_auxiliary_points(*this);

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
            double group_distance = config.auto_group_distance;
            group_distance *= (p1.size + p2.size) / 2.0;
            if ((p1.pos - p2.pos).length() <= group_distance + 1e-3) {
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

    // ensure that specified segmentation length is strictly greater than max_rc_steps
    for (auto& [line_id, seg_len] : config.segmented_lines) {
        if (seg_len >= 0 && seg_len <= config.max_rc_steps) {
            seg_len = config.max_rc_steps + 1;
        }
    }

    // check loop lines more thoroughly
    for (auto& [line_id, line] : lines) {
        if (line.is_loop) continue;
        int period = 0;
        for (int i = 1; i < line.point_ids.size(); ++i) {
            if (period == 0 && line.point_ids[i] == line.point_ids[0]) {
                period = i;
            } else if (period != 0 && line.point_ids[i] != line.point_ids[i % period]) {
                period = 0;
                break;
            }
        }
        if (period) {
            line.is_loop = true;
            line.point_ids.resize(period + 1);
        }
    }

    // if a line is not in the segmentation list, has no friends or merges, 
    // and has no duplicate stations, it is considered simple
    for (auto& [line_id, line] : lines) {
        line.is_simple = false;
        if (config.segmented_lines.contains(line_id)) continue;
        bool has_friends = false;
        for (const auto& [l1, l2] : config.friend_lines) {
            if (l1 == line_id || l2 == line_id) {
                has_friends = true;
                break;
            }
        }
        if (has_friends) continue;
        bool has_merges = false;
        for (const auto& [l1, l2] : config.merged_lines) {
            if (l1 == line_id || l2 == line_id) {
                has_merges = true;
                break;
            }
        }
        if (has_merges) continue;
        std::unordered_set<int> station_set;
        bool has_duplicates = false;
        // if loop line, ignore the last point (same as first)
        int limit = line.is_loop ? static_cast<int>(line.point_ids.size()) - 1 : static_cast<int>(line.point_ids.size());
        for (int i = 0; i < limit; ++i) {
            int pid = line.point_ids[i];
            if (points.contains(pid) && points.at(pid).type == Point::Type::Station) {
                if (station_set.contains(pid)) {
                    has_duplicates = true;
                    break;
                }
                station_set.insert(pid);
            }
        }
        if (has_duplicates) continue;
        line.is_simple = true;
    }
}

} // namespace geometry