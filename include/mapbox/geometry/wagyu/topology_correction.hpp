#pragma once

#include <algorithm>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <utility>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/join.hpp>
#include <mapbox/geometry/wagyu/join_util.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>
#include <mapbox/geometry/wagyu/ring_util.hpp>

#ifdef DEBUG
#include <iostream>
#endif

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct point_ptr_pair {
    point_ptr<T> op1;
    point_ptr<T> op2;
};

#ifdef DEBUG

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring) {
    
    out << " BEGIN CONNECTIONS: " << std::endl;
    for (auto & r : dupe_ring) {
        out << "  Ring: ";
        if (r.second.op1->ring) {
            out << r.second.op1->ring->ring_index;
        } else {
            out << "---";
        }
        out << " to ";
        if (r.second.op2->ring) {
            out << r.second.op2->ring->ring_index;
        } else {
            out << "---";
        }
        out << "  ( at " << r.second.op1->x << ", " << r.second.op1->y << " )" << std::endl;
    }
    out << " END CONNECTIONS: " << std::endl;
    return out;
}

#endif


template <typename T>
bool find_intersect_loop(std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                         std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>>& iList,
                         ring_ptr<T> ring_parent,
                         ring_ptr<T> ring_origin,
                         ring_ptr<T> ring_search,
                         std::set<ring_ptr<T>>& visited,
                         point_ptr<T> orig_pt,
                         point_ptr<T> prev_pt,
                         ring_manager<T>& rings) {
    auto range = dupe_ring.equal_range(ring_search);
    // Check for direct connection
    for (auto it = range.first; it != range.second;) {
        ring_ptr<T> it_ring1 = it->second.op1->ring;
        ring_ptr<T> it_ring2 = it->second.op2->ring;
        if (it_ring1 != ring_search || (!ring_is_hole(it_ring1) && !ring_is_hole(it_ring2))) {
            it = dupe_ring.erase(it);
            continue;
        }
        if (it_ring2 == ring_origin &&
            (ring_parent == it_ring2 || ring_parent == it_ring2->parent) &&
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
        ring_ptr<T> it_ring = it->second.op2->ring;
        if (visited.count(it_ring) > 0 ||
            (ring_parent != it_ring && ring_parent != it_ring->parent) ||
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
void remove_spikes(point_ptr<T> & pt) {
    while (true) {
        if (pt->next == pt) {
            pt->ring = nullptr;
            pt = nullptr;
            break;
        } else if (*(pt) == *(pt->next)) {
            point_ptr<T> old_next = pt->next;
            old_next->next->prev = pt;
            pt->next = old_next->next;
            old_next->next = old_next;
            old_next->prev = old_next;
            old_next->ring = nullptr;
        } else if (*(pt) == *(pt->prev)) {
            point_ptr<T> old_prev = pt->prev;
            old_prev->prev->next = pt;
            pt->prev = old_prev->prev;
            old_prev->next = old_prev;
            old_prev->prev = old_prev;
            old_prev->ring = nullptr;
        } else if (*(pt->next) == *(pt->prev)) {
            point_ptr<T> next = pt->next;
            point_ptr<T> prev = pt->prev;
            next->prev = prev;
            prev->next = next;
            pt->ring = nullptr;
            pt->next = pt;
            pt->prev = pt;
            pt = next;
        } else {
            break;
        }
    }
}

template <typename T>
bool fix_intersects(std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                    point_ptr<T> op_j,
                    point_ptr<T> op_k,
                    ring_ptr<T> ring_j,
                    ring_ptr<T> ring_k,
                    ring_manager<T>& rings,
                    bool add_if_not_found) {
    if (ring_j == ring_k) {
        return false;
    }
    if (!ring_is_hole(ring_j) && !ring_is_hole(ring_k)) {
        // Both are not holes, return nothing to do.
        return false;
    }
    ring_ptr<T> ring_origin;
    ring_ptr<T> ring_search;
    ring_ptr<T> ring_parent;
    point_ptr<T> op_origin_1;
    point_ptr<T> op_origin_2;
    if (!ring_is_hole(ring_j)) {
        ring_origin = ring_j;
        ring_parent = ring_origin;
        ring_search = ring_k;
        op_origin_1 = op_j;
        op_origin_2 = op_k;
    } else if (!ring_is_hole(ring_k)) {
        ring_origin = ring_k;
        ring_parent = ring_origin;
        ring_search = ring_j;
        op_origin_1 = op_k;
        op_origin_2 = op_j;

    } else {
        // both are holes
        // Order doesn't matter
        ring_origin = ring_j;
        ring_parent = ring_origin->parent;
        ring_search = ring_k;
        op_origin_1 = op_j;
        op_origin_2 = op_k;
    }
    if (ring_parent != ring_search->parent) {
        // The two holes do not have the same parent, do not add them
        // simply return!
        return false;
    }
    bool found = false;
    std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>> iList;
    auto range = dupe_ring.equal_range(ring_search);
    // Check for direct connection
    for (auto it = range.first; it != range.second;) {
        ring_ptr<T> it_ring2 = it->second.op2->ring;
        if (it_ring2 == ring_origin) {
            if (*op_origin_1 != *(it->second.op2)) {
                found = true;
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
            ring_ptr<T> it_ring = it->second.op2->ring;
            if (it_ring != ring_search && *op_origin_2 != *it->second.op2 &&
                (ring_parent == it_ring || ring_parent == it_ring->parent) &&
                find_intersect_loop(dupe_ring, iList, ring_parent, ring_origin, it_ring, visited,
                                    op_origin_2, it->second.op2, rings)) {
                found = true;
                iList.emplace_front(ring_search, it->second);
                break;
            }
        }
    }
    if (!found && add_if_not_found) {
        point_ptr_pair<T> intPt_origin = { op_origin_1, op_origin_2 };
        point_ptr_pair<T> intPt_search = { op_origin_2, op_origin_1 };
        dupe_ring.emplace(ring_origin, intPt_origin);
        dupe_ring.emplace(ring_search, intPt_search);
        return false;
    }

    if (iList.empty()) {
        return false;
    }

    if (ring_is_hole(ring_origin)) {
        for (auto& iRing : iList) {
            ring_ptr<T> ring_itr = iRing.first;
            if (!ring_is_hole(ring_itr)) {
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

    remove_spikes(op_origin_1);
    remove_spikes(op_origin_2);

    if (op_origin_1 == nullptr || op_origin_2 == nullptr) {
        if (op_origin_1 == nullptr && op_origin_2 == nullptr) {
            // Self destruction!
            ring_origin->points = nullptr;
            remove_ring(ring_origin, rings);
            for (auto& iRing : iList) {
                ring_ptr<T> ring_itr = iRing.first;
                ring_itr->points = nullptr;
                remove_ring(ring_itr, rings);
            }
        } else {
            if (op_origin_1 == nullptr) {
                ring_origin->points = op_origin_2;
            } else {
                //(op_origin_2 == nullptr)
                ring_origin->points = op_origin_1;
            }
            for (auto& iRing : iList) {
                ring_ptr<T> ring_itr = iRing.first;
                ring_itr->points = nullptr;
                ring_itr->bottom_point = nullptr;
                if (ring_is_hole(ring_origin)) {
                    ring1_replaces_ring2(ring_origin, ring_itr, rings);
                } else {
                    ring1_replaces_ring2(ring_origin->parent, ring_itr, rings);
                }
            }
        }
    } else {
        ring_ptr<T> ring_new = create_new_ring(rings);
        if (ring_is_hole(ring_origin) && ((area(op_origin_1) < 0.0))) {
            ring_origin->points = op_origin_1;
            ring_new->points = op_origin_2;
        } else {
            ring_origin->points = op_origin_2;
            ring_new->points = op_origin_1;
        }

        update_points_ring(ring_origin);
        update_points_ring(ring_new);

        ring_origin->bottom_point = nullptr;

        for (auto& iRing : iList) {
            ring_ptr<T> ring_itr = iRing.first;
            ring_itr->points = nullptr;
            ring_itr->bottom_point = nullptr;
            if (ring_is_hole(ring_origin)) {
                ring1_replaces_ring2(ring_origin, ring_itr, rings);
            } else {
                ring1_replaces_ring2(ring_origin->parent, ring_itr, rings);
            }
        }
        if (ring_is_hole(ring_origin)) {
            ring1_child_of_ring2(ring_new, ring_origin, rings);
            fixup_children(ring_origin, ring_new);
            fixup_children(ring_parent, ring_new);
        } else {
            ring1_sibling_of_ring2(ring_new, ring_origin, rings);
            fixup_children(ring_origin, ring_new);
        }
    }

    std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>> move_list;

    for (auto iRing : iList) {
        auto range_itr = dupe_ring.equal_range(iRing.first);
        if (range_itr.first != range_itr.second) {
            for (auto it = range_itr.first; it != range_itr.second; ++it) {
                ring_ptr<T> it_ring = it->second.op1->ring;
                ring_ptr<T> it_ring2 = it->second.op2->ring;
                if (it_ring == nullptr || it_ring2 == nullptr || it_ring == it_ring2) {
                    continue;
                }
                ring_ptr<T> fl_ring;
                ring_ptr<T> fl_ring2;
                if (ring_is_hole(it_ring)) {
                    fl_ring = it_ring->parent;
                } else {
                    fl_ring = it_ring;
                }
                if (ring_is_hole(it_ring2)) {
                    fl_ring2 = it_ring2->parent;
                } else {
                    fl_ring2 = it_ring2;
                }
                if ((ring_is_hole(it_ring) || ring_is_hole(it_ring2)) && (fl_ring == fl_ring2)) {
                    move_list.emplace_back(it_ring, it->second);
                }
            }
            dupe_ring.erase(iRing.first);
        }
    }
    
    auto range_itr = dupe_ring.equal_range(ring_origin);
    for (auto it = range_itr.first; it != range_itr.second;) {
        ring_ptr<T> it_ring = it->second.op1->ring;
        ring_ptr<T> it_ring2 = it->second.op2->ring;
        if (it_ring == nullptr || it_ring2 == nullptr || it_ring == it_ring2) {
            it = dupe_ring.erase(it);
            continue;
        }
        ring_ptr<T> fl_ring;
        ring_ptr<T> fl_ring2;
        if (ring_is_hole(it_ring)) {
            fl_ring = it_ring->parent;
        } else {
            fl_ring = it_ring;
        }
        if (ring_is_hole(it_ring2)) {
            fl_ring2 = it_ring2->parent;
        } else {
            fl_ring2 = it_ring2;
        }
        if (it_ring != ring_origin) {
            if ((ring_is_hole(it_ring) || ring_is_hole(it_ring2)) && (fl_ring == fl_ring2)) {
                move_list.emplace_back(it_ring, it->second);
            }
            it = dupe_ring.erase(it);
        } else {
            if ((ring_is_hole(it_ring) || ring_is_hole(it_ring2)) && (fl_ring == fl_ring2)) {
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
        } else if (!op1->ring || !op2->ring) {
            return true;
        } else {
            return (op1->ring->ring_index) < (op2->ring->ring_index);
        }
    }
};

template <typename T>
void update_duplicate_point_entries(
    ring_ptr<T> ring, std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring) {
    auto range = dupe_ring.equal_range(ring);
    std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>> move_list;
    for (auto it = range.first; it != range.second;) {
        ring_ptr<T> it_ring = it->second.op1->ring;
        ring_ptr<T> it_ring_2 = it->second.op2->ring;
        if (it_ring == nullptr || it_ring_2 == nullptr) {
            it = dupe_ring.erase(it);
            continue;
        }
        ring_ptr<T> fl_ring;
        ring_ptr<T> fl_ring_2;
        if (ring_is_hole(it_ring)) {
            fl_ring = it_ring->parent;
        } else {
            fl_ring = it_ring;
        }
        if (ring_is_hole(it_ring_2)) {
            fl_ring_2 = it_ring_2->parent;
        } else {
            fl_ring_2 = it_ring_2;
        }
        if (it_ring != ring) {
            if ((ring_is_hole(it_ring) || ring_is_hole(it_ring_2)) && (fl_ring == fl_ring_2)) {
                move_list.emplace_back(it_ring, it->second);
            }
            it = dupe_ring.erase(it);
        } else {
            if ((ring_is_hole(it_ring) || ring_is_hole(it_ring_2)) && (fl_ring == fl_ring_2)) {
                ++it;
            } else {
                it = dupe_ring.erase(it);
            }
        }
    }
    if (!move_list.empty()) {
        dupe_ring.insert(move_list.begin(), move_list.end());
    }
}

template <typename T>
bool parent_in_tree(ring_ptr<T> r, ring_ptr<T> possible_parent) {

    ring_ptr<T> current_ring = r->parent;
    while (current_ring != nullptr) {
        if (current_ring == possible_parent) {
            return true;
        }
        current_ring = current_ring->parent;
    }
    return false;
}

template <typename T>
void handle_self_intersections(point_ptr<T> op,
                               point_ptr<T> op2,
                               ring_ptr<T> ring,
                               ring_ptr<T> ring2,
                               std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                               ring_manager<T>& rings) {
    /*
    bool debug = false;
    if (op->x == 31 && op->y == 8) {
        std::clog << "FOUND" << std::endl;
        std::clog << *ring << std::endl;
        std::clog << *ring2 << std::endl;
        debug = true;
    }
    */
    // Check that are same ring
    if (ring != ring2) {
        return;
    }

    remove_spikes(op);
    if (!op || *op != *op2 || !op2->ring) {
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    }
    remove_spikes(op2);
    if (!op2 || *op != *op2 || !op->ring) {
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    }

    assert(op != op2);
    
    double original_area = area(ring->points);
    bool original_is_positive = (original_area > 0.0);
#ifdef DEBUG
    bool crossing = intersections_cross(op, op2);
    assert(!crossing);
#endif

    // split the polygon into two ...
    point_ptr<T> op3 = op->prev;
    point_ptr<T> op4 = op2->prev;
    op->prev = op4;
    op4->next = op;
    op2->prev = op3;
    op3->next = op2;
    
    remove_spikes(op);
    remove_spikes(op2);

    if (op == nullptr && op2 == nullptr) {
        // Self destruction!
        ring->points = nullptr;
        remove_ring(ring, rings);
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    } else if (op == nullptr) {
        ring->points = op2;
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    } else if (op2 == nullptr) {
        ring->points = op;
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    }

    ring_ptr<T> new_ring = create_new_ring(rings);
    
    // We are select where op is assigned because it provides
    // a performance increase in poly2_contains_poly1 below.
    if (point_count(op) > point_count(op2)) {
        ring->points = op;
        new_ring->points = op2;
    } else {
        ring->points = op2;
        new_ring->points = op;
    }

    update_points_ring(ring);
    update_points_ring(new_ring);
    double area_1 = area(ring->points);
    double area_2 = area(new_ring->points);
    bool area_1_is_positive = (area_1 > 0.0);
    bool area_2_is_positive = (area_2 > 0.0);
    bool area_1_is_zero = std::fabs(area_1) <= 0.0;
    bool area_2_is_zero = std::fabs(area_2) <= 0.0;

    if (area_2_is_zero || (area_1_is_positive != area_2_is_positive && area_1_is_positive == original_is_positive)) {
        // new_ring is contained by ring ...
        ring1_child_of_ring2(new_ring, ring, rings);
        fixup_children(new_ring, ring);
    } else if (area_1_is_zero || (area_1_is_positive != area_2_is_positive && area_2_is_positive == original_is_positive)) {
        // ring is contained by new_ring ...
        ring1_sibling_of_ring2(new_ring, ring, rings);
        ring1_child_of_ring2(ring, new_ring, rings);
        fixup_children(new_ring, ring);
    } else {
        // the 2 polygons are separate ...
        ring1_sibling_of_ring2(new_ring, ring, rings);
        fixup_children(new_ring, ring);
    }
    update_duplicate_point_entries(ring, dupe_ring);
}

template <typename T>
void handle_collinear_edges(point_ptr<T> pt1, point_ptr<T> pt2, 
                            std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                            ring_manager<T> & rings) {

    ring_ptr<T> ring1 = pt1->ring;
    ring_ptr<T> ring2 = pt2->ring;
    assert(ring1 != ring2);
    if (ring1->parent != ring2->parent) {
        return;
    }

    if (*(pt1->next) != *(pt2->prev) && *(pt2->next) != *(pt1->prev)) {
        return;
    }

    // swap points
    point_ptr<T> pt3 = pt1->prev;
    point_ptr<T> pt4 = pt2->prev;
    pt1->prev = pt4;
    pt4->next = pt1;
    pt2->prev = pt3;
    pt3->next = pt2;

    // remove spikes
    remove_spikes(pt1);
    if (!pt1) {
        // rings self destructed
        ring1->points = nullptr;
        remove_ring(ring1, rings);
        ring2->points = nullptr;
        remove_ring(ring2, rings);
        return;
    }
    if (pt2->ring) {
        remove_spikes(pt2);
        if (!pt2) {
            // rings self destructed
            ring1->points = nullptr;
            remove_ring(ring1, rings);
            ring2->points = nullptr;
            remove_ring(ring2, rings);
            return;
        }
    }
    ring1->points = pt1;
    ring1_replaces_ring2(ring1, ring2, rings);
    update_points_ring(ring1);
    update_duplicate_point_entries(ring2, dupe_ring);
    
    if (ring_is_hole(ring1)) {
        std::vector<point_ptr_pair<T>> point_list;
        auto range = dupe_ring.equal_range(ring1);
        for (auto it = range.first; it != range.second;) {
            point_list.push_back(it->second);
            ++it;
        }
        for (auto & i : point_list) {
            if (fix_intersects(dupe_ring, i.op1, i.op2, i.op1->ring, i.op2->ring, rings, false)) {
                break;
            }
        }
    }
}

template <typename T>
void process_repeated_points(std::size_t repeated_point_count,
                             std::size_t last_index,
                             std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                             ring_manager<T>& rings) {

    for (std::size_t j = (last_index - repeated_point_count - 1); j < last_index; ++j) {
        point_ptr<T> op_j = rings.all_points[j];
        if (!op_j->ring) {
            continue;
        }
        for (std::size_t k = j + 1; k < last_index; ++k) {
            point_ptr<T> op_k = rings.all_points[k];
            if (!op_k->ring || !op_j->ring) {
                continue;
            }
            handle_self_intersections(op_j, op_k, op_j->ring, op_k->ring, dupe_ring,
                                      rings);
            /*if (!op_k->ring || !op_j->ring) {
                continue;
            }
            handle_collinear_edges(op_j, op_k, dupe_ring, rings);
            */
        }
    }
}

template <typename T>
void process_chains(std::size_t repeated_point_count,
                             std::size_t last_index,
                             std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                             ring_manager<T>& rings) {
    for (std::size_t j = (last_index - repeated_point_count - 1); j < last_index; ++j) {
        point_ptr<T> op_j = rings.all_points[j];
        if (!op_j->ring) {
            continue;
        }
        for (std::size_t k = j + 1; k < last_index; ++k) {
            point_ptr<T> op_k = rings.all_points[k];
            if (!op_k->ring || !op_j->ring) {
                continue;
            }
            fix_intersects(dupe_ring, op_j, op_k, op_j->ring, op_k->ring, rings, true);
        }
    }
}

template <typename T>
void do_simple_polygons(ring_manager<T>& rings) {

    std::stable_sort(rings.all_points.begin(), rings.all_points.end(), point_ptr_cmp<T>());
    std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>> dupe_ring;
    dupe_ring.reserve(rings.all_rings.size());

    // Find sets of repeated points and process them
    std::size_t count = 0;
    for (std::size_t i = 1; i < rings.all_points.size(); ++i) {
        if (*rings.all_points[i] == *rings.all_points[i - 1]) {
            ++count;
            continue;
        }
        if (count == 0) {
            continue;
        }
        process_repeated_points(count, i, dupe_ring, rings);
        process_chains(count, i, dupe_ring, rings);
        count = 0;
    }
    /*
    count = 0;
    for (std::size_t i = 1; i < rings.all_points.size(); ++i) {
        if (*rings.all_points[i] == *rings.all_points[i - 1]) {
            ++count;
            continue;
        }
        if (count == 0) {
            continue;
        }
        process_chains(count, i, dupe_ring, rings);
        count = 0;
    }*/
}
}
}
}
