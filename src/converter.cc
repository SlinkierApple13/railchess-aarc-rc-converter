#include <queue>

#include "converter.h"

namespace converter {

void add_stations(const geometry::Map& geomap, rc::Map& rcmap) {
    // handle station groups
    for (const auto& [group_id, group] : geomap.station_groups) {
        geometry::Position norm_pos = geomap.normalized_pos(geomap.group_pos(group_id));
        rc::Station station;
        station.id = group_id;
        station.norm_x = norm_pos.x;
        station.norm_y = norm_pos.y;
        rcmap.stations[station.id] = std::move(station);
    }

    // handle ungrouped stations
    for (const auto& [point_id, point] : geomap.points) {
        if (point.type != geometry::Point::Type::Station) continue;
        if (geomap.point_to_group.contains(point_id)) continue;
        geometry::Position norm_pos = geomap.normalized_pos(point.pos);
        rc::Station station;
        station.id = point_id;
        station.norm_x = norm_pos.x;
        station.norm_y = norm_pos.y;
        rcmap.stations[station.id] = std::move(station);
    }
}

struct Track {
    int point_id;
    int line_id;
    int index_in_line;
    bool forward;
    bool is_end = false;
    int next_index = -1;

    int get_next_index() const {
        return next_index != -1 ? next_index : (forward ? index_in_line + 1 : index_in_line - 1);
    }
};

struct Point : geometry::Point {
    std::vector<Track> tracks;
};

struct RouteEntry {
    std::vector<Track> tracks;
    bool is_special = false;
    int base_size = 0;
    int station_count = 0;

    void push_back(const Track& track, const geometry::Map& geomap) {
        tracks.push_back(track);
        if (geomap.points.contains(track.point_id) && geomap.points.at(track.point_id).type == geometry::Point::Type::Station) {
            station_count++;
        }
    }

    void pop_back(const geometry::Map& geomap) {
        if (tracks.empty()) return;
        const Track& track = tracks.back();
        if (geomap.points.contains(track.point_id) && geomap.points.at(track.point_id).type == geometry::Point::Type::Station) {
            station_count--;
        }
        tracks.pop_back();
    }

    int stations() const {
        return station_count;
    }
};

void add_lines(const geometry::Map& geomap, rc::Map& rcmap) {
    // pre-process point information
    std::unordered_map<int, Point> points;
    for (const auto& [point_id, point] : geomap.points) {
        points[point_id] = static_cast<Point>(point);
    }
    for (const auto& [line_id, line] : geomap.lines) {
        for (size_t i = 0; i < line.point_ids.size(); ++i) {
            int pid = line.point_ids[i];
            if (!points.contains(pid)) continue;
            if (i + 1 < line.point_ids.size()) {
                points[pid].tracks.push_back(Track{
                    .point_id = pid,
                    .line_id = line_id,
                    .index_in_line = static_cast<int>(i),
                    .forward = true
                });
            }
            if (i > 0) {
                points[pid].tracks.push_back(Track{
                    .point_id = pid,
                    .line_id = line_id,
                    .index_in_line = static_cast<int>(i),
                    .forward = false
                });
            }
            if (i == 0 && line.is_loop) {
                points[pid].tracks.push_back(Track{
                    .point_id = pid,
                    .line_id = line_id,
                    .index_in_line = static_cast<int>(i),
                    .forward = false,
                    .next_index = static_cast<int>(line.point_ids.size()) - 1
                });
            }
            if (i + 1 == line.point_ids.size() && line.is_loop) {
                points[pid].tracks.push_back(Track{
                    .point_id = pid,
                    .line_id = line_id,
                    .index_in_line = static_cast<int>(i),
                    .forward = true,
                    .next_index = 0
                });
            }
            if (i == 0 && !line.is_loop) {
                points[pid].tracks.push_back(Track{
                    .point_id = pid,
                    .line_id = line_id,
                    .index_in_line = static_cast<int>(i),
                    .forward = false,
                    .is_end = true
                });
            }
            if (i + 1 == line.point_ids.size() && !line.is_loop) {
                points[pid].tracks.push_back(Track{
                    .point_id = pid,
                    .line_id = line_id,
                    .index_in_line = static_cast<int>(i),
                    .forward = true,
                    .is_end = true
                });
            }
        }
    }
    
    // helper lambdas
    auto add_line = [&](const std::vector<Track>& tracks) {
        if (tracks.size() < 2) return;
        rc::Line line;
        line.id = rcmap.lines.size() + 1;
        line.is_loop = false;
        for (const auto& track : tracks) {
            if (!geomap.points.contains(track.point_id)) continue;
            const auto& point = geomap.points.at(track.point_id);
            if (point.type != geometry::Point::Type::Station) continue;
            int id = geomap.point_to_group.contains(point.id) ? geomap.point_to_group.at(point.id)->id : point.id;
            if (!geomap.config.merge_consecutive_duplicates || line.station_ids.empty() || line.station_ids.back() != id) {
                line.station_ids.push_back(id);
            }
        }
        rcmap.lines.emplace(line.id, std::move(line));
    };

    auto next_tracks = [&](const Track& track) {
        std::vector<Track> result;
        if (track.is_end) return result;
        int next_pid = geomap.lines.at(track.line_id).point_ids[track.get_next_index()];
        for (const auto& t : points.at(next_pid).tracks) {
            if (t.line_id == track.line_id && t.index_in_line == track.get_next_index()) {
                if (t.forward == track.forward || t.is_end) {
                    result.push_back(t);
                }
                continue;
            }
            if (t.is_end) continue;
            bool is_friend = geomap.friend_lines.contains({track.line_id, t.line_id});
            bool is_merged = geomap.merged_lines.contains({track.line_id, t.line_id});
            if (is_merged) {
                result.push_back(t);
                continue;
            }
            if (!is_friend) continue;
            int pid_after_next = geomap.lines.at(t.line_id).point_ids[t.get_next_index()];
            if (geomap.can_move_through(track.point_id, next_pid, pid_after_next)) {
                result.push_back(t);
            }
        }
        if (result.size() > 1) {
            // remove the only one with is_end == true
            result.erase(std::remove_if(result.begin(), result.end(), [](const Track& t) {
                return t.is_end;
            }), result.end());
        }
        return result;
    };

    auto is_too_long = [&](const RouteEntry& entry) {
        if (entry.stations() >= geomap.config.max_length) return true;
        if (entry.is_special && 
            entry.stations() >= geomap.config.max_length_special * 2 + entry.base_size
        ) return true;
        return false;
    };

    // search possible routes
    std::queue<RouteEntry> q;
    for (const auto& [line_id, line] : geomap.lines) {
        if (line.point_ids.size() < 2) continue;
        
        RouteEntry entry1;
        entry1.push_back(Track(line.point_ids.front(), line_id, 0, true), geomap);
        entry1.is_special = geomap.special_lines.contains(line_id);
        q.push(std::move(entry1));

        RouteEntry entry2;
        entry2.push_back(Track(line.point_ids.back(), line_id, static_cast<int>(line.point_ids.size()) - 1, false), geomap);
        entry2.is_special = geomap.special_lines.contains(line_id);
        q.push(std::move(entry2));

        if (!geomap.special_lines.contains(line_id)) continue;
        for (
            size_t i = geomap.config.max_length_special; 
            i + 1 < line.point_ids.size(); 
            i += geomap.config.max_length_special
        ) {
            RouteEntry entry3;
            entry3.push_back(Track(line.point_ids[i], line_id, static_cast<int>(i), true), geomap);
            entry3.is_special = true;
            q.push(std::move(entry3));

            RouteEntry entry4;
            entry4.push_back(Track(line.point_ids[i], line_id, static_cast<int>(i), false), geomap);
            entry4.is_special = true;
            q.push(std::move(entry4));
        }
    }

    // do breadth-first search; no need to track visited states as we care about all possible routes
    while (!q.empty()) {
        RouteEntry entry = std::move(q.front());
        q.pop();
        bool is_special = entry.is_special;
        const Track& last_track = entry.tracks.back();
        std::vector<Track> nexts = next_tracks(last_track);
        if (nexts.empty() || is_too_long(entry)) {
            add_line(entry.tracks);
            continue;
        }
        for (const auto& next : nexts) {
            RouteEntry new_entry = entry;
            new_entry.push_back(next, geomap);
            if (geomap.special_lines.contains(next.line_id) && !is_special) {
                new_entry.is_special = true;
                new_entry.base_size = static_cast<int>(entry.stations());
            }
            q.push(std::move(new_entry));
        }
    }
}

void remove_duplicate_lines(rc::Map& rcmap) {
    // if line A is identical to line B or the inverse of line B, remove the one with the larger id
    // if line A or the inverse of line A is a sub-route of line B, remove line A
    for (auto it = rcmap.lines.begin(); it != rcmap.lines.end(); ) {
        bool erased = false;
        for (auto it2 = rcmap.lines.begin(); it2 != rcmap.lines.end(); ++it2) {
            if (it == it2) continue;
            auto& lineA = it->second;
            auto& lineB = it2->second;
            std::vector<int> revB = lineB.station_ids;
            std::reverse(revB.begin(), revB.end());
            // Check for identical lines first
            if (lineA.station_ids.size() == lineB.station_ids.size()) {
                if (lineA.station_ids == lineB.station_ids || lineA.station_ids == revB) {
                    if (it->first > it2->first) {
                        it = rcmap.lines.erase(it);
                        erased = true;
                        break;
                    }
                    continue; 
                }
            }
            // If not identical, check for sub-route
            auto is_subroute = [](const std::vector<int>& a, const std::vector<int>& b) {
                if (a.empty() || a.size() >= b.size()) return false;
                for (size_t i = 0; i <= b.size() - a.size(); ++i) {
                    if (std::equal(a.begin(), a.end(), b.begin() + i)) return true;
                }
                return false;
            };
            if (is_subroute(lineA.station_ids, lineB.station_ids) || is_subroute(lineA.station_ids, revB)) {
                it = rcmap.lines.erase(it);
                erased = true;
                break;
            }
        }
        if (!erased) {
            ++it;
        }
    }
}

rc::Map convert_to_rc(const geometry::Map& geomap) {
    rc::Map rcmap;
    add_stations(geomap, rcmap);
    add_lines(geomap, rcmap);
    remove_duplicate_lines(rcmap);
    return rcmap;
}

} // namespace converter