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
    int remaining_count = 65535; // effectively infinite

    void push_back(
        const Track& track, const geometry::Map& geomap, 
        const std::unordered_map<int, int>& segmented_lines
    ) {
        tracks.push_back(track);
        if (geomap.points.contains(track.point_id) && 
            geomap.points.at(track.point_id).type == geometry::Point::Type::Station) {
            --remaining_count;
        }
        remaining_count = std::min(remaining_count,
            segmented_lines.contains(track.line_id) ? segmented_lines.at(track.line_id) : geomap.config.max_length
        );
    }

    bool full() const {
        return remaining_count <= 0;
    }
};

void remove_duplicate_lines(std::unordered_map<int, rc::Line>& lines) {
    // if line A is identical to line B or the inverse of line B, remove the one with the larger id
    // if line A or the inverse of line A is a sub-route of line B, remove line A
    for (auto it = lines.begin(); it != lines.end(); ) {
        bool erased = false;
        for (auto it2 = lines.begin(); it2 != lines.end(); ++it2) {
            if (it == it2) continue;
            auto& lineA = it->second;
            auto& lineB = it2->second;
            std::vector<int> revB = lineB.station_ids;
            std::reverse(revB.begin(), revB.end());
            // Check for identical lines first
            if (lineA.station_ids.size() == lineB.station_ids.size()) {
                if (lineA.station_ids == lineB.station_ids || lineA.station_ids == revB) {
                    if (it->first > it2->first) {
                        it = lines.erase(it);
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
                it = lines.erase(it);
                erased = true;
                break;
            }
        }
        if (!erased) {
            ++it;
        }
    }
}

std::unordered_map<int, rc::Line> get_lines(
    const geometry::Map& geomap, const rc::Map& rcmap,
    const std::unordered_map<int, int>& segmented_lines,
    const std::unordered_set<int>& lines_mask = {}
) {
    std::unordered_map<int, rc::Line> lines;

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
    
    auto add_line = [&](const std::vector<Track>& tracks) {
        if (tracks.size() < 2) return;
        rc::Line line;
        line.id = lines.size() + 1;
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
        lines.emplace(line.id, std::move(line));
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
            bool is_friend = geomap.config.friend_lines.contains({track.line_id, t.line_id});
            bool is_merged = geomap.config.merged_lines.contains({track.line_id, t.line_id});
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

    // search possible routes
    std::queue<RouteEntry> q;
    for (const auto& [line_id, line] : geomap.lines) {
        if (!lines_mask.empty() && !lines_mask.contains(line_id)) continue;
        if (line.point_ids.size() < 2) continue;

        if (line.is_simple) {
            // just remove nodes and consecutive duplicate stations
            rc::Line simple_line;
            simple_line.id = lines.size() + 1;
            simple_line.is_loop = line.is_loop;
            for (int pid : line.point_ids) {
                if (!geomap.points.contains(pid)) continue;
                const auto& point = geomap.points.at(pid);
                if (point.type != geometry::Point::Type::Station) continue;
                int id = geomap.point_to_group.contains(point.id) ? geomap.point_to_group.at(point.id)->id : point.id;
                if (!geomap.config.merge_consecutive_duplicates || simple_line.station_ids.empty() || simple_line.station_ids.back() != id) {
                    simple_line.station_ids.push_back(id);
                }
            }
            lines.emplace(lines.size() + 1, std::move(simple_line));
            continue;
        }
        
        RouteEntry entry1;
        entry1.push_back(Track(line.point_ids.front(), line_id, 0, true), geomap, segmented_lines);
        q.push(std::move(entry1));

        RouteEntry entry2;
        entry2.push_back(Track(
            line.point_ids.back(), line_id, 
            static_cast<int>(line.point_ids.size()) - 1, false), 
            geomap, segmented_lines
        );
        q.push(std::move(entry2));

        if (!segmented_lines.contains(line_id)) continue;

        int interval = segmented_lines.at(line_id) - geomap.config.max_rc_steps;
        for (size_t i = interval; i + 1 < line.point_ids.size(); i += interval) {
            RouteEntry entry3;
            entry3.push_back(Track(line.point_ids[i], line_id, static_cast<int>(i), true), geomap, segmented_lines);
            q.push(std::move(entry3));

            RouteEntry entry4;
            entry4.push_back(Track(line.point_ids[i], line_id, static_cast<int>(i), false), geomap, segmented_lines);
            q.push(std::move(entry4));
        }
    }

    // do breadth-first search; no need to track visited states as we care about all possible routes
    while (!q.empty()) {
        RouteEntry entry = std::move(q.front());
        q.pop();
        const Track& last_track = entry.tracks.back();
        std::vector<Track> nexts = next_tracks(last_track);
        if (nexts.empty() || entry.full()) {
            add_line(entry.tracks);
            continue;
        }
        for (const auto& next : nexts) {
            RouteEntry new_entry = entry;
            new_entry.push_back(next, geomap, segmented_lines);
            q.push(std::move(new_entry));
        }
    }

    remove_duplicate_lines(lines);

    return lines;
}

void add_lines(const geometry::Map& geomap, rc::Map& rcmap) {
    std::unordered_map<int, int> segmented_lines = geomap.config.segmented_lines;
    if (!geomap.config.optimize_segmentation) {
        for (auto& [line_id, seg_len] : segmented_lines) {
            if (seg_len <= 0) {
                seg_len = geomap.config.max_rc_steps << 1;
            }
        }
        rcmap.lines = get_lines(geomap, rcmap, segmented_lines);
        return;
    }

    // Group lines by their initial (negative) segmentation length value
    // Lines with the same value will be optimized together
    std::unordered_map<int, std::vector<int>> seg_groups;
    for (auto& [line_id, seg_len] : segmented_lines) {
        if (seg_len < 0) {
            int group_key = seg_len; // negative values serve as group identifiers
            seg_groups[group_key].push_back(line_id);
            seg_len = geomap.config.max_rc_steps << 1;
        }
    }

    if (seg_groups.empty()) {
        rcmap.lines = get_lines(geomap, rcmap, segmented_lines);
        return;
    }

    // Get all lines related to the groups via breadth-first search
    std::unordered_set<int> lines_mask;
    for (const auto& [group_key, line_ids] : seg_groups) {
        lines_mask.insert(line_ids.begin(), line_ids.end());
    }
    
    std::queue<int> q;
    for (int line_id : lines_mask) {
        q.push(line_id);
    }
    while (!q.empty()) {
        int line_id = q.front();
        q.pop();
        for (const auto& [l1, l2] : geomap.config.friend_lines) {
            if (l1 == line_id && !lines_mask.contains(l2)) {
                lines_mask.insert(l2);
                q.push(l2);
            }
        }
        for (const auto& [l1, l2] : geomap.config.merged_lines) {
            if (l1 == line_id && !lines_mask.contains(l2)) {
                lines_mask.insert(l2);
                q.push(l2);
            }
        }
    }

    // Gradient descent to minimize the number of lines
    auto get_line_count = [&](const std::unordered_map<int, int>& seg_config) {
        auto temp_lines = get_lines(geomap, rcmap, seg_config, lines_mask);
        return static_cast<int>(temp_lines.size());
    };

    int current_count = get_line_count(segmented_lines);
    bool improved = true;
    int max_iterations = geomap.config.max_iterations;
    int iteration = 0;

    while (improved && iteration < max_iterations) {
        improved = false;
        ++iteration;
        
        const std::vector<int> deltas = iteration < 3 ? 
            std::vector<int>{-11, -5, -2, 2, 5, 11} : 
            std::vector<int>{-5, -2, 2, 5};

        // Iterate over each group (each group shares the same segmentation length)
        for (const auto& [group_key, line_ids] : seg_groups) {
            // All lines in this group share the same segmentation length
            int current_val = segmented_lines[line_ids.front()];
            int best_val = current_val;
            int best_count = current_count;

            for (int delta : deltas) {
                int new_val = current_val + delta;
                // Ensure the value stays within reasonable bounds
                if (new_val <= geomap.config.max_rc_steps) continue;
                if (new_val >= geomap.config.max_length * 2) continue;

                auto temp_config = segmented_lines;
                // Update all lines in this group with the same value
                for (int line_id : line_ids) {
                    temp_config[line_id] = new_val;
                }
                int new_count = get_line_count(temp_config);

                if (new_count < best_count) {
                    best_count = new_count;
                    best_val = new_val;
                    improved = true;
                }
            }

            if (best_val != current_val) {
                // Update all lines in this group with the best value
                for (int line_id : line_ids) {
                    segmented_lines[line_id] = best_val;
                }
                current_count = best_count;
            }
        }
    }

    rcmap.lines = get_lines(geomap, rcmap, segmented_lines);
}

rc::Map convert_to_rc(const geometry::Map& geomap) {
    rc::Map rcmap;
    add_stations(geomap, rcmap);
    add_lines(geomap, rcmap);
    return rcmap;
}

} // namespace converter