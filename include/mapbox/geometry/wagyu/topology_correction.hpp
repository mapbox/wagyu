#pragma once

#include <algorithm>
#include <list>
#include <set>
#include <unordered_map>
#include <utility>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/join.hpp>
#include <mapbox/geometry/wagyu/join_util.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>
#include <mapbox/geometry/wagyu/ring_util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct point_ptr_pair {
    point_ptr<T> op1;
    point_ptr<T> op2;
};

template <typename T>
bool find_intersect_loop(std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                         std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>>& iList,
                         ring_ptr<T> ring_parent,
                         ring_ptr<T> ring_origin,
                         ring_ptr<T> ring_search,
                         std::set<ring_ptr<T>>& visited,
                         point_ptr<T> orig_pt,
                         point_ptr<T> prev_pt,
                         ring_list<T>& rings) {
    auto range = dupe_ring.equal_range(ring_search);
    // Check for direct connection
    for (auto it = range.first; it != range.second;) {
        ring_ptr<T> it_ring1 = get_ring(it->second.op1->ring);
        ring_ptr<T> it_ring2 = get_ring(it->second.op2->ring);
        if (it_ring1 != ring_search || (!it_ring1->is_hole && !it_ring2->is_hole)) {
            it = dupe_ring.erase(it);
            continue;
        }
        if (it_ring2 == ring_origin &&
            (ring_parent == it_ring2 || ring_parent == parse_first_left(it_ring2->first_left)) &&
            *prev_pt != *it->second.op2 && *orig_pt != *it->second.op2) {
            iList.emplace_front(ring_search, it->second);
            return true;
        }
        ++it;
    }
    range = dupe_ring.equal_range(ring_search);
    visited.insert(ring_search);
    // Check for connection through chain of other intersections
    for (auto it = range.first;
         it != range.second && it != dupe_ring.end() && it->first == ring_search; ++it) {
        ring_ptr<T> it_ring = get_ring(it->second.op2->ring);
        if (visited.count(it_ring) > 0 ||
            (ring_parent != it_ring && ring_parent != parse_first_left(it_ring->first_left)) ||
            *prev_pt == *it->second.op2) {
            continue;
        }
        if (find_intersect_loop(dupe_ring, iList, ring_parent, ring_origin, it_ring, visited,
                                orig_pt, it->second.op2, rings)) {
            iList.emplace_front(ring_search, it->second);
            return true;
        }
    }
    return false;
}

template <typename T>
bool fix_intersects(std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                    point_ptr<T> op_j,
                    point_ptr<T> op_k,
                    ring_ptr<T> ring_j,
                    ring_ptr<T> ring_k,
                    ring_list<T>& rings) {
    if (!ring_j->is_hole && !ring_k->is_hole) {
        // Both are not holes, return nothing to do.
        return false;
    }
    ring_ptr<T> ring_origin;
    ring_ptr<T> ring_search;
    ring_ptr<T> ring_parent;
    point_ptr<T> op_origin_1;
    point_ptr<T> op_origin_2;
    if (!ring_j->is_hole) {
        ring_origin = ring_j;
        ring_parent = ring_origin;
        ring_search = ring_k;
        op_origin_1 = op_j;
        op_origin_2 = op_k;
    } else if (!ring_k->is_hole) {
        ring_origin = ring_k;
        ring_parent = ring_origin;
        ring_search = ring_j;
        op_origin_1 = op_k;
        op_origin_2 = op_j;

    } else // both are holes
    {
        // Order doesn't matter
        ring_origin = ring_j;
        ring_parent = parse_first_left(ring_origin->first_left);
        ring_search = ring_k;
        op_origin_1 = op_j;
        op_origin_2 = op_k;
    }
    if (ring_parent != parse_first_left(ring_search->first_left)) {
        // The two holes do not have the same parent, do not add them
        // simply return!
        return false;
    }
    bool found = false;
    std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>> iList;
    auto range = dupe_ring.equal_range(ring_search);
    // Check for direct connection
    for (auto it = range.first; it != range.second;) {
        ring_ptr<T> it_ring1 = get_ring(it->second.op1->ring);
        ring_ptr<T> it_ring2 = get_ring(it->second.op2->ring);
        if (ring_search != it_ring1 || ring_search == it_ring2) {
            it = dupe_ring.erase(it);
            continue;
        }
        if (it_ring2 == ring_origin) {
            found = true;
            if (*op_origin_1 != *it->second.op2) {
                iList.emplace_back(ring_search, it->second);
                break;
            }
        }
        ++it;
    }
    if (!found) {
        range = dupe_ring.equal_range(ring_search);
        std::set<ring_ptr<T>> visited;
        visited.insert(ring_search);
        // Check for connection through chain of other intersections
        for (auto it = range.first;
             it != range.second && it != dupe_ring.end() && it->first == ring_search; ++it) {
            ring_ptr<T> it_ring = get_ring(it->second.op2->ring);
            if (it_ring != ring_search && *op_origin_2 != *it->second.op2 &&
                (ring_parent == it_ring || ring_parent == parse_first_left(it_ring->first_left)) &&
                find_intersect_loop(dupe_ring, iList, ring_parent, ring_origin, it_ring, visited,
                                    op_origin_2, it->second.op2, rings)) {
                found = true;
                iList.emplace_front(ring_search, it->second);
                break;
            }
        }
    }
    if (!found) {
        point_ptr_pair<T> intPt_origin = { op_origin_1, op_origin_2 };
        point_ptr_pair<T> intPt_search = { op_origin_2, op_origin_1 };
        dupe_ring.emplace(ring_origin, intPt_origin);
        dupe_ring.emplace(ring_search, intPt_search);
        return false;
    }

    if (iList.empty()) {
        return false;
    }

    if (ring_origin->is_hole) {
        for (auto& iRing : iList) {
            ring_ptr<T> ring_itr = get_ring(iRing.first);
            if (!ring_itr->is_hole) {
                // Make the hole the origin!
                point_ptr<T> op1 = op_origin_1;
                op_origin_1 = iRing.second.op1;
                iRing.second.op1 = op1;
                point_ptr<T> op2 = op_origin_2;
                op_origin_2 = iRing.second.op2;
                iRing.second.op2 = op2;
                iRing.first = ring_origin;
                ring_origin = ring_itr;
                ring_parent = ring_origin;
                break;
            }
        }
    }

    // Switch
    point_ptr<T> op_origin_1_next = op_origin_1->next;
    point_ptr<T> op_origin_2_next = op_origin_2->next;
    op_origin_1->next = op_origin_2_next;
    op_origin_2->next = op_origin_1_next;
    op_origin_1_next->prev = op_origin_2;
    op_origin_2_next->prev = op_origin_1;

    for (auto iRing : iList) {
        point_ptr<T> op_search_1 = iRing.second.op1;
        point_ptr<T> op_search_2 = iRing.second.op2;
        point_ptr<T> op_search_1_next = op_search_1->next;
        point_ptr<T> op_search_2_next = op_search_2->next;
        op_search_1->next = op_search_2_next;
        op_search_2->next = op_search_1_next;
        op_search_1_next->prev = op_search_2;
        op_search_2_next->prev = op_search_1;
    }

    ring_ptr<T> ring_new = create_new_ring(rings);
    ring_new->is_hole = false;
    if (ring_origin->is_hole && ((area(op_origin_1) < 0.0))) {
        ring_origin->points = op_origin_1;
        ring_new->points = op_origin_2;
    } else {
        ring_origin->points = op_origin_2;
        ring_new->points = op_origin_1;
    }

    update_points_ring(ring_origin);
    update_points_ring(ring_new);

    ring_origin->bottom_point = nullptr;

    std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>> move_list;
    for (auto& iRing : iList) {
        ring_ptr<T> ring_itr = get_ring(iRing.first);
        ring_itr->points = nullptr;
        ring_itr->bottom_point = nullptr;
        ring_itr->replacement_ring = ring_origin;
        if (ring_origin->is_hole) {
            ring_itr->first_left = parse_first_left(ring_origin->first_left);
        } else {
            ring_itr->first_left = ring_origin;
        }
        ring_itr->is_hole = ring_origin->is_hole;
        fixup_first_lefts3(ring_itr, ring_origin, rings);
    }
    if (ring_origin->is_hole) {
        ring_new->first_left = ring_origin;
        fixup_first_lefts2(ring_new, ring_origin, rings);
        fixup_first_lefts1(ring_parent, ring_new, rings);
    } else {
        ring_new->first_left = ring_origin->first_left;
        fixup_first_lefts1(ring_origin, ring_new, rings);
    }
    for (auto iRing : iList) {
        auto range_itr = dupe_ring.equal_range(iRing.first);
        if (range_itr.first != range_itr.second) {
            for (auto it = range_itr.first; it != range_itr.second; ++it) {
                ring_ptr<T> it_ring = get_ring(it->second.op1->ring);
                ring_ptr<T> it_ring2 = get_ring(it->second.op2->ring);
                if (it_ring == it_ring2) {
                    continue;
                }
                ring_ptr<T> fl_ring;
                ring_ptr<T> fl_ring2;
                if (it_ring->is_hole) {
                    fl_ring = parse_first_left(it_ring->first_left);
                } else {
                    fl_ring = it_ring;
                }
                if (it_ring2->is_hole) {
                    fl_ring2 = parse_first_left(it_ring2->first_left);
                } else {
                    fl_ring2 = it_ring2;
                }
                if ((it_ring->is_hole || it_ring2->is_hole) && (fl_ring == fl_ring2)) {
                    move_list.emplace_back(it_ring, it->second);
                }
            }
            dupe_ring.erase(iRing.first);
        }
    }
    auto range_itr = dupe_ring.equal_range(ring_origin);
    for (auto it = range_itr.first; it != range_itr.second;) {
        ring_ptr<T> it_ring = get_ring(it->second.op1->ring);
        ring_ptr<T> it_ring2 = get_ring(it->second.op2->ring);
        if (it_ring == it_ring2) {
            it = dupe_ring.erase(it);
            continue;
        }
        ring_ptr<T> fl_ring;
        ring_ptr<T> fl_ring2;
        if (it_ring->is_hole) {
            fl_ring = parse_first_left(it_ring->first_left);
        } else {
            fl_ring = it_ring;
        }
        if (it_ring2->is_hole) {
            fl_ring2 = parse_first_left(it_ring2->first_left);
        } else {
            fl_ring2 = it_ring2;
        }
        if (it_ring != ring_origin) {
            if ((it_ring->is_hole || it_ring2->is_hole) && (fl_ring == fl_ring2)) {
                move_list.emplace_back(it_ring, it->second);
            }
            it = dupe_ring.erase(it);
        } else {
            if ((it_ring->is_hole || it_ring2->is_hole) && (fl_ring == fl_ring2)) {
                ++it;
            } else {
                it = dupe_ring.erase(it);
            }
        }
    }

    if (!move_list.empty()) {
        dupe_ring.insert(move_list.begin(), move_list.end());
    }
    return true;
}

template <typename T>
struct point_ptr_cmp {
    inline bool operator()(point_ptr<T> op1, point_ptr<T> op2) {
        if (op1->y > op2->y) {
            return true;
        } else if (op1->y < op2->y) {
            return false;
        } else if (op1->x < op2->x) {
            return true;
        } else if (op1->x > op2->x) {
            return false;
        } else {
            return (op1->ring) < (op2->ring);
        }
    }
};

template <typename T>
void do_simple_polygons(ring_list<T>& rings) {
    std::vector<point_ptr<T>> out_points;
    {
        std::size_t i = 0;
        while (i < rings.size()) {
            ring_ptr<T> ring = rings[i++];
            point_ptr<T> op = ring->points;
            if (!op || ring->is_open)
                continue;
            do {
                out_points.push_back(op);
                op = op->next;
            } while (op != ring->points);
        }
    }
    std::stable_sort(out_points.begin(), out_points.end(), point_ptr_cmp<T>());
    std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>> dupe_ring;
    dupe_ring.reserve(rings.size());
    std::size_t count = 0;
    for (std::size_t i = 1; i < out_points.size(); ++i) {
        if (*out_points[i] == *out_points[i - 1]) {
            ++count;
            continue;
        }
        if (count > 0) {
            for (std::size_t j = (i - count - 1); j < i; ++j) {
                if (!out_points[j]->ring) {
                    continue;
                }
                ring_ptr<T> ring_j = get_ring(out_points[j]->ring);
                for (std::size_t k = j + 1; k < i; ++k) {
                    if (!out_points[k]->ring) {
                        continue;
                    }
                    ring_ptr<T> ring_k = get_ring(out_points[k]->ring);
                    if (ring_k == ring_j) {
                        point_ptr<T> op = out_points[j];
                        point_ptr<T> op2 = out_points[k];
                        ring_ptr<T> ring = ring_j;
                        if (op != op2 && op2->next != op && op2->prev != op) {
                            // split the polygon into two ...
                            point_ptr<T> op3 = op->prev;
                            point_ptr<T> op4 = op2->prev;
                            op->prev = op4;
                            op4->next = op;
                            op2->prev = op3;
                            op3->next = op2;

                            ring_ptr<T> ring2 = create_new_ring(rings);
                            if (point_count(op) > point_count(op2)) {
                                ring->points = op;
                                ring2->points = op2;
                            } else {
                                ring->points = op2;
                                ring2->points = op;
                            }
                            update_points_ring(ring);
                            update_points_ring(ring2);
                            if (poly2_contains_poly1(ring2->points, ring->points)) {
                                // Out_ring2 is contained by Out_ring1 ...
                                ring2->is_hole = !ring->is_hole;
                                ring2->first_left = ring;
                                fixup_first_lefts2(ring2, ring, rings);
                                auto range = dupe_ring.equal_range(ring_j);
                                std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>> move_list;
                                for (auto it = range.first; it != range.second;) {
                                    ring_ptr<T> it_ring = get_ring(it->second.op1->ring);
                                    ring_ptr<T> it_ring2 = get_ring(it->second.op2->ring);
                                    ring_ptr<T> fl_ring;
                                    ring_ptr<T> fl_ring2;
                                    if (it_ring->is_hole) {
                                        fl_ring = parse_first_left(it_ring->first_left);
                                    } else {
                                        fl_ring = it_ring;
                                    }
                                    if (it_ring2->is_hole) {
                                        fl_ring2 = parse_first_left(it_ring2->first_left);
                                    } else {
                                        fl_ring2 = it_ring2;
                                    }
                                    if (it_ring != ring_j) {
                                        if ((it_ring->is_hole || it_ring2->is_hole) &&
                                            (fl_ring == fl_ring2)) {
                                            move_list.emplace_back(it_ring, it->second);
                                        }
                                        it = dupe_ring.erase(it);
                                    } else {
                                        if ((it_ring->is_hole || it_ring2->is_hole) &&
                                            (fl_ring == fl_ring2)) {
                                            ++it;
                                        } else {
                                            it = dupe_ring.erase(it);
                                        }
                                    }
                                }
                                if (!move_list.empty()) {
                                    dupe_ring.insert(move_list.begin(), move_list.end());
                                }
                                if (!ring->is_hole) {
                                    point_ptr_pair<T> intPt1 = { ring->points, ring2->points };
                                    point_ptr_pair<T> intPt2 = { ring2->points, ring->points };
                                    dupe_ring.emplace(ring, intPt1);
                                    dupe_ring.emplace(ring2, intPt2);
                                }
                            } else if (poly2_contains_poly1(ring->points, ring2->points)) {
                                // Out_ring1 is contained by Out_ring2 ...
                                ring2->is_hole = ring->is_hole;
                                ring->is_hole = !ring2->is_hole;
                                ring2->first_left = ring->first_left;
                                ring->first_left = ring2;
                                fixup_first_lefts2(ring, ring2, rings);
                                auto range = dupe_ring.equal_range(ring_j);
                                std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>> move_list;
                                for (auto it = range.first; it != range.second;) {
                                    ring_ptr<T> it_ring = get_ring(it->second.op1->ring);
                                    ring_ptr<T> it_ring2 = get_ring(it->second.op2->ring);
                                    ring_ptr<T> fl_ring;
                                    ring_ptr<T> fl_ring2;
                                    if (it_ring->is_hole) {
                                        fl_ring = parse_first_left(it_ring->first_left);
                                    } else {
                                        fl_ring = it_ring;
                                    }
                                    if (it_ring2->is_hole) {
                                        fl_ring2 = parse_first_left(it_ring2->first_left);
                                    } else {
                                        fl_ring2 = it_ring2;
                                    }
                                    if (it_ring != ring_j) {
                                        if ((it_ring->is_hole || it_ring2->is_hole) &&
                                            (fl_ring == fl_ring2)) {
                                            move_list.emplace_back(it_ring, it->second);
                                        }
                                        it = dupe_ring.erase(it);
                                    } else {
                                        if ((it_ring->is_hole || it_ring2->is_hole) &&
                                            (fl_ring == fl_ring2)) {
                                            ++it;
                                        } else {
                                            it = dupe_ring.erase(it);
                                        }
                                    }
                                }
                                if (!move_list.empty()) {
                                    dupe_ring.insert(move_list.begin(), move_list.end());
                                }
                                if (!ring2->is_hole) {
                                    point_ptr_pair<T> intPt1 = { ring->points, ring2->points };
                                    point_ptr_pair<T> intPt2 = { ring2->points, ring->points };
                                    dupe_ring.emplace(ring, intPt1);
                                    dupe_ring.emplace(ring2, intPt2);
                                }
                            } else {
                                // the 2 polygons are separate ...
                                ring2->is_hole = ring->is_hole;
                                ring2->first_left = ring->first_left;
                                fixup_first_lefts1(ring, ring2, rings);
                                auto range = dupe_ring.equal_range(ring_j);
                                std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>> move_list;
                                for (auto it = range.first; it != range.second;) {
                                    ring_ptr<T> it_ring = get_ring(it->second.op1->ring);
                                    ring_ptr<T> it_ring2 = get_ring(it->second.op2->ring);
                                    ring_ptr<T> fl_ring;
                                    ring_ptr<T> fl_ring2;
                                    if (it_ring->is_hole) {
                                        fl_ring = parse_first_left(it_ring->first_left);
                                    } else {
                                        fl_ring = it_ring;
                                    }
                                    if (it_ring2->is_hole) {
                                        fl_ring2 = parse_first_left(it_ring2->first_left);
                                    } else {
                                        fl_ring2 = it_ring2;
                                    }
                                    if (it_ring != ring_j) {
                                        if ((it_ring->is_hole || it_ring2->is_hole) &&
                                            (fl_ring == fl_ring2)) {
                                            move_list.emplace_back(it_ring, it->second);
                                        }
                                        it = dupe_ring.erase(it);
                                    } else {
                                        if ((it_ring->is_hole || it_ring2->is_hole) &&
                                            (fl_ring == fl_ring2)) {
                                            ++it;
                                        } else {
                                            it = dupe_ring.erase(it);
                                        }
                                    }
                                }
                                if (!move_list.empty()) {
                                    dupe_ring.insert(move_list.begin(), move_list.end());
                                }
                                if (ring2->is_hole) {
                                    point_ptr_pair<T> intPt1 = { ring->points, ring2->points };
                                    point_ptr_pair<T> intPt2 = { ring2->points, ring->points };
                                    dupe_ring.emplace(ring, intPt1);
                                    dupe_ring.emplace(ring2, intPt2);
                                }
                            }
                            ring_j = get_ring(out_points[j]->ring);
                        }
                        continue;
                    }
                    if (fix_intersects(dupe_ring, out_points[j], out_points[k], ring_j, ring_k,
                                       rings)) {
                        ring_j = get_ring(out_points[j]->ring);
                    }
                }
            }
            count = 0;
        }
    }
}
}
}
}
