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
        if (visited.count(it_ring) > 0 || it_ring == nullptr || 
            (ring_parent != it_ring && ring_parent != it_ring->parent) ||
            std::fabs(area(it_ring)) <= 0.0 ||
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
    /*bool debug = false;
    //if (op_j->x == 28 && op_j->y == 31) {
    if (ring_j->ring_index == 11 || ring_k->ring_index == 11) {
        debug = true;
        //std::clog << rings.all_rings << std::endl;
        std::clog << *ring_j << std::endl;
        std::clog << *ring_k << std::endl;
        //std::clog << dupe_ring << std::endl;
    }*/

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
            if (it_ring != ring_search && *op_origin_2 != *it->second.op2 && it_ring != nullptr &&
                (ring_parent == it_ring || ring_parent == it_ring->parent) &&
                std::fabs(area(it_ring)) > 0.0 &&
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
            ring_origin->area = std::numeric_limits<double>::quiet_NaN();
            remove_ring(ring_origin, rings);
            for (auto& iRing : iList) {
                ring_ptr<T> ring_itr = iRing.first;
                ring_itr->points = nullptr;
                ring_itr->area = std::numeric_limits<double>::quiet_NaN();
                remove_ring(ring_itr, rings);
            }
        } else {
            if (op_origin_1 == nullptr) {
                ring_origin->points = op_origin_2;
            } else {
                //(op_origin_2 == nullptr)
                ring_origin->points = op_origin_1;
            }
            ring_origin->area = std::numeric_limits<double>::quiet_NaN();
            update_points_ring(ring_origin);
            for (auto& iRing : iList) {
                ring_ptr<T> ring_itr = iRing.first;
                ring_itr->points = nullptr;
                ring_itr->area = std::numeric_limits<double>::quiet_NaN();
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
        double area_1 = area_from_point(op_origin_1);
        double area_2 = area_from_point(op_origin_2);
        if (ring_is_hole(ring_origin) && ((area_1 < 0.0))) {
            ring_origin->points = op_origin_1;
            ring_origin->area = area_1;
            ring_new->points = op_origin_2;
            ring_new->area = area_2;
        } else {
            ring_origin->points = op_origin_2;
            ring_origin->area = area_2;
            ring_new->points = op_origin_1;
            ring_new->area = area_1;
        }

        update_points_ring(ring_origin);
        update_points_ring(ring_new);

        ring_origin->bottom_point = nullptr;

        for (auto& iRing : iList) {
            ring_ptr<T> ring_itr = iRing.first;
            ring_itr->points = nullptr;
            ring_itr->area = std::numeric_limits<double>::quiet_NaN();
            ring_itr->bottom_point = nullptr;
            if (ring_is_hole(ring_origin)) {
                ring1_replaces_ring2(ring_origin, ring_itr, rings);
            } else {
                ring1_replaces_ring2(ring_origin->parent, ring_itr, rings);
            }
        }
        if (ring_is_hole(ring_origin)) {
            ring_new->parent = ring_origin;
            if (ring_new->parent == nullptr) {
                rings.children.push_back(ring_new);
            } else {
                ring_new->parent->children.push_back(ring_new);
            }
            fixup_children(ring_origin, ring_new);
            fixup_children(ring_parent, ring_new);
        } else {
            ring_new->parent = ring_origin->parent;
            if (ring_new->parent == nullptr) {
                rings.children.push_back(ring_new);
            } else {
                ring_new->parent->children.push_back(ring_new);
            }
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
void fixup_children_new_interior_ring(ring_ptr<T> old_ring, ring_ptr<T> new_ring) {
    // The only rings that could possibly be a child to a new interior ring are those
    // child rings that have the same sign of their area as the old ring.
    bool old_ring_area_is_positive = area(old_ring) > 0.0;
    assert(old_ring != new_ring);
    for (auto r = old_ring->children.begin(); r != old_ring->children.end();) {
        assert((*r)->points);
        assert((*r) != old_ring);
        bool ring_area_is_positive = area((*r)) > 0.0;
        if ((*r) != new_ring &&
            ring_area_is_positive == old_ring_area_is_positive && 
            poly2_contains_poly1((*r)->points, new_ring->points)) {
            (*r)->parent = new_ring;
            new_ring->children.push_back((*r));
            r = old_ring->children.erase(r);
        } else {
            ++r;
        }
    }
}


template <typename T>
void handle_self_intersections(point_ptr<T> op,
                               point_ptr<T> op2,
                               ring_ptr<T> ring,
                               ring_ptr<T> ring2,
                               std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                               ring_manager<T>& rings) {
    bool debug = false;
    /*if (op->x == 38 && op->y == 29) {
        //std::clog << *ring2 << std::endl;
        //std::clog << rings.all_rings << std::endl;
        std::clog << *ring << std::endl;
        debug = true;
    }*/

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
    
    double original_area = area(ring);
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
        ring->area = std::numeric_limits<double>::quiet_NaN();
        remove_ring(ring, rings);
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    } else if (op == nullptr) {
        ring->points = op2;
        ring->area = std::numeric_limits<double>::quiet_NaN();
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    } else if (op2 == nullptr) {
        ring->points = op;
        ring->area = std::numeric_limits<double>::quiet_NaN();
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    }

    ring_ptr<T> new_ring = create_new_ring(rings);
    double area_1 = area_from_point(op);
    double area_2 = area_from_point(op2);
    bool area_1_is_positive = (area_1 > 0.0);
    bool area_2_is_positive = (area_2 > 0.0);
    bool area_1_is_zero = std::fabs(area_1) <= 0.0;
    bool area_2_is_zero = std::fabs(area_2) <= 0.0;
    
    // Situation # 1 - Orientations are NOT the same:
    // - One ring contains the other and MUST be a child of that ring 
    // - The one that changed orientation is the child of the other ring
    //
    // Situation # 2 - Orientations are the same
    // - The rings are now split, such a new ring of the same orientation
    //   must be created.
    // - If the new ring is WITHIN the old ring:
    //      * It WILL be the child of a hole of that ring (this ring may not yet be created)
    //        or possible the child of a child of a child of the ring (an so on)...
    // - If the new ring is OUTSIDE the old ring:
    //      * It may contain any of the children of the old ring.
    if (area_2_is_zero || area_1_is_zero || area_1_is_positive != area_2_is_positive) {
        // Situation #1 - new_ring is contained by ring ...
        if (debug) std::clog << "Situation 1" << std::endl;
        if (area_2_is_zero || (!area_1_is_zero && area_1_is_positive == original_is_positive)) {
            ring->points = op;
            ring->area = area_1;
            new_ring->points = op2;
            new_ring->area = area_2;
        } else {
            ring->points = op2;
            ring->area = area_2;
            new_ring->points = op;
            new_ring->area = area_1;
        }
        update_points_ring(ring);
        update_points_ring(new_ring);
        new_ring->parent = ring;
        if (new_ring->parent == nullptr) {
            rings.children.push_back(new_ring);
        } else {
            new_ring->parent->children.push_back(new_ring);
        }
        fixup_children_new_interior_ring(ring, new_ring);
        if (debug) {
            std::clog << "Ring" << std::endl;
            std::clog << *ring << std::endl;
            std::clog << "New Ring" << std::endl;
            std::clog << *new_ring << std::endl;
        }
    } else {
        // Situation #2 - create new ring
        if (debug) std::clog << "Situation 2" << std::endl;
        // The largest absolute area is the parent
        if (std::fabs(area_1) > std::fabs(area_2)) {
            ring->points = op;
            ring->area = area_1;
            new_ring->points = op2;
            new_ring->area = area_2;
        } else {
            ring->points = op2;
            ring->area = area_2;
            new_ring->points = op;
            new_ring->area = area_1;
        }
        update_points_ring(ring);
        update_points_ring(new_ring);
        if (poly2_contains_poly1(new_ring->points, ring->points)) {
            // This is the situation where there is the new ring is
            // created inside the ring. Later on this should be inherited
            // as child of a newly created hole. However, we should check existing
            // holes of this polygon to see if they might belong inside this polygon.
            new_ring->parent = ring;
            if (new_ring->parent == nullptr) {
                rings.children.push_back(new_ring);
            } else {
                new_ring->parent->children.push_back(new_ring);
            }
            fixup_children(ring, new_ring);
        } else {
            // Polygons are completely seperate 
            new_ring->parent = ring->parent;
            if (new_ring->parent == nullptr) {
                rings.children.push_back(new_ring);
            } else {
                new_ring->parent->children.push_back(new_ring);
            }
            fixup_children(ring, new_ring);
        }
    }
    update_duplicate_point_entries(ring, dupe_ring);
}

template <typename T>
void handle_collinear_edges(point_ptr<T> pt1, point_ptr<T> pt2, 
                            //std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                            ring_manager<T> & rings) {
    
    ring_ptr<T> ring1 = pt1->ring;
    ring_ptr<T> ring2 = pt2->ring;
    if (ring1 == ring2) {
        return;
    }
    if (ring1->parent != ring2->parent) {
        return;
    }

    if (*(pt1->next) != *(pt2->prev) && *(pt2->next) != *(pt1->prev)) {
        return;
    }
    
    /*
    bool debug = false;
    //if (pt1->x == 25 && pt1->y == 26) {
    if (ring1->ring_index == 11 || ring2->ring_index == 11) {
        std::clog << "HERE" << std::endl;
        std::clog << *ring1 << std::endl;
        std::clog << *ring2 << std::endl;
        debug = true;
    }
    */

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
        ring1->area = std::numeric_limits<double>::quiet_NaN();
        remove_ring(ring1, rings);
        ring2->points = nullptr;
        ring2->area = std::numeric_limits<double>::quiet_NaN();
        remove_ring(ring2, rings);
        return;
    }
    if (pt2->ring) {
        remove_spikes(pt2);
        if (!pt2) {
            // rings self destructed
            ring1->points = nullptr;
            ring1->area = std::numeric_limits<double>::quiet_NaN();
            remove_ring(ring1, rings);
            ring2->points = nullptr;
            ring2->area = std::numeric_limits<double>::quiet_NaN();
            remove_ring(ring2, rings);
            return;
        }
    }
    ring1->points = pt1;
    ring2->points = nullptr;
    ring1->area = std::numeric_limits<double>::quiet_NaN();
    ring2->area = std::numeric_limits<double>::quiet_NaN();
    ring1_replaces_ring2(ring1, ring2, rings);
    update_points_ring(ring1);
    
    /*
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
    }*/
}

template <typename T>
double calculate_segment_angle_next(point_ptr<T> pt) {
    point_ptr<T> pt_next = pt->next;
    while (*pt_next == *pt) {
        if (pt_next == pt) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        pt_next = pt_next->next;
    }
    return std::atan2(static_cast<double>(pt_next->y - pt->y), static_cast<double>(pt_next->x - pt->x));
}

template <typename T>
double calculate_segment_angle_prev(point_ptr<T> pt) {
    point_ptr<T> pt_prev = pt->prev;
    while (*pt_prev == *pt) {
        if (pt_prev == pt) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        pt_prev = pt_prev->prev;
    }
    return std::atan2(static_cast<double>(pt_prev->y - pt->y), static_cast<double>(pt_prev->x - pt->x));
}

template <typename T>
std::list<point_ptr<T>> build_point_list(std::size_t repeated_point_count,
                                         std::size_t last_index,
                                         ring_manager<T>& rings) {
    std::list<point_ptr<T>> point_list;
    T original_x = rings.all_points[last_index - repeated_point_count - 1]->x;
    T original_y = rings.all_points[last_index - repeated_point_count - 1]->y;
    for (std::size_t j = (last_index - repeated_point_count - 1); j < last_index; ++j) {
        point_ptr<T> op_j = rings.all_points[j];
        if (op_j->ring) {
            remove_spikes(op_j);
        }
        if (op_j && op_j->x == original_x && op_j->y == original_y) {
            point_list.push_back(op_j);
        }
    }
    return point_list;
}

template <typename T>
using angle_point = std::pair<double, point_ptr<T>>;

template <typename T>
using angle_point_vector = std::vector<angle_point<T>>;

template <typename T>
using angle_point_vector_rev_itr = typename angle_point_vector<T>::reverse_iterator;

template <typename T>
struct segment_angle_sorter {

    point_ptr<T> first_point;

    segment_angle_sorter(point_ptr<T> first_point_): first_point(first_point_) {}

    inline bool operator()(angle_point<T> const& p1, angle_point<T> const& p2) {
        if (std::fabs(p1.first - p2.first) <= 0.0) {
            if (p1.second == first_point) {
                return p2.second != first_point;
            } else {
                return false;
            }
        } else {
            return p1.first < p2.first;
        }
    }
};

template <typename T>
struct segment_angle_sorter_rev {

    point_ptr<T> first_point;

    segment_angle_sorter_rev(point_ptr<T> first_point_): first_point(first_point_) {}

    inline bool operator()(angle_point<T> const& p1, angle_point<T> const& p2) {
        if (std::fabs(p1.first - p2.first) <= 0.0) {
            if (p1.second == first_point) {
                return p2.second != first_point;
            } else {
                return false;
            }
        } else {
            return p1.first > p2.first;
        }
    }
};

template <typename T>
void process_front_of_point_list(std::list<point_ptr<T>>& point_list,
                                 std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                                 ring_manager<T> & rings) {
    angle_point_vector<T> angle_points;
    point_ptr<T> first_point = point_list.front();
    ring_ptr<T> r = first_point->ring;
    if (first_point->x == 13 && first_point->y == 23) {
        std::clog << "yeah, this happened?" << std::endl;
        std::clog << *r << std::endl;
    }
    if (r == nullptr) {
        point_list.pop_front();
        return;
    }
    for (auto & p : point_list) {
        if (p->ring != nullptr && p->ring != r) {
            continue;
        }
        double next_angle = calculate_segment_angle_next(p);
        double prev_angle = calculate_segment_angle_prev(p);
        if (std::isnan(next_angle) || std::isnan(prev_angle)) {
            continue;
        }
        angle_points.emplace_back(next_angle, p);
        angle_points.emplace_back(prev_angle, p);
    }

    if (angle_points.size() <= 2) {
        point_list.pop_front();
        return;
    }
    
    // Search forward
    std::stable_sort(angle_points.begin(), angle_points.end(), segment_angle_sorter<T>(first_point));

    auto fp_itr = angle_points.begin();
    // Move itr to "first point"
    while (fp_itr->second != first_point) {
        ++fp_itr;
    }
    
    auto search_itr = fp_itr;
    
    // Origin points are used for the rare situation where another vertex has the same angle 
    // as the first point. Therefore we must track it as well if this occurs, because we must
    // handle the self intersection with the closest angle to the match.
    std::vector<point_ptr<T>> origin_points;

    std::vector<point_ptr<T>> possible_match;
    while (true) {
        if (search_itr == angle_points.end()) {
            search_itr = angle_points.begin();
        }

        if (!possible_match.empty()) {
            point_ptr<T> found_pt = search_itr->second;
            auto next_itr = std::next(search_itr);
            while (next_itr != angle_points.end() && std::fabs(next_itr->first - search_itr->first) <= 0.0) {
                if (std::find(possible_match.begin(), possible_match.end(), next_itr->second) != possible_match.end()) {
                    found_pt = next_itr->second;
                    break;
                }
                ++next_itr;
            }
            if (std::find(possible_match.begin(), possible_match.end(), found_pt) != possible_match.end()) {
                point_ptr<T> origin_pt = first_point;
                origin_points.erase(std::remove(origin_points.begin(), origin_points.end(), found_pt), origin_points.end());
                if (origin_points.size() >= 2) {
                    auto itr = search_itr;
                    while (std::find(origin_points.begin(), origin_points.end(), itr->second) == origin_points.end()) {
                        ++itr;
                        if (itr == angle_points.end()) {
                            itr = angle_points.begin();
                        }
                    }
                    origin_pt = itr->second;
                }
                handle_self_intersections(origin_pt, found_pt, origin_pt->ring, found_pt->ring, dupe_ring,
                                          rings);
                return;
            } else {
                // assert from "crossing" situation
                if (found_pt == first_point) {
                    throw std::runtime_error("Crossing intersection!");
                }
                break;
            }
        } else {
            bool set_origin = search_itr->second == first_point;
            if (set_origin) {
                origin_points.clear();
                origin_points.push_back(search_itr->second);
            }
            if (search_itr->second != first_point) {
                possible_match.push_back(search_itr->second);
            } else {
                auto next_itr = std::next(search_itr);
                while (next_itr != angle_points.end() && std::fabs(next_itr->first - search_itr->first) <= 0.0) {
                    if (next_itr->second != first_point) {
                        possible_match.push_back(next_itr->second);
                    }
                    if (set_origin) {
                        origin_points.push_back(next_itr->second);
                    }
                    ++next_itr;
                }
            }
        }
        ++search_itr;
    }
    
    // Search back
    std::stable_sort(angle_points.begin(), angle_points.end(), segment_angle_sorter_rev<T>(first_point));

    fp_itr = angle_points.begin();
    // Move itr to "first point"
    while (fp_itr->second != first_point) {
        ++fp_itr;
    }
    
    search_itr = fp_itr;
    possible_match.clear();
    origin_points.clear();
    while (true) {
        if (search_itr == angle_points.end()) {
            search_itr = angle_points.begin();
        }
        if (!possible_match.empty()) {
            point_ptr<T> found_pt = search_itr->second;
            auto next_itr = std::next(search_itr);
            while (next_itr != angle_points.end() && std::fabs(next_itr->first - search_itr->first) <= 0.0) {
                if (std::find(possible_match.begin(), possible_match.end(), next_itr->second) != possible_match.end()) {
                    found_pt = next_itr->second;
                    break;
                }
                ++next_itr;
            }
            if (std::find(possible_match.begin(), possible_match.end(), found_pt) != possible_match.end()) {
                point_ptr<T> origin_pt = first_point;
                origin_points.erase(std::remove(origin_points.begin(), origin_points.end(), found_pt), origin_points.end());
                if (origin_points.size() >= 2) {
                    auto itr = search_itr;
                    while (std::find(origin_points.begin(), origin_points.end(), itr->second) == origin_points.end()) {
                        ++itr;
                        if (itr == angle_points.end()) {
                            itr = angle_points.begin();
                        }
                    }
                    origin_pt = itr->second;
                }
                handle_self_intersections(origin_pt, found_pt, origin_pt->ring, found_pt->ring, dupe_ring,
                                          rings);
                return;
            } else {
                if (found_pt == first_point) {
                    throw std::runtime_error("Crossing intersection");
                }
                point_list.splice(point_list.end(), point_list, point_list.begin());
                break;
            }
        } else {
            bool set_origin = search_itr->second == first_point;
            if (set_origin) {
                origin_points.clear();
                origin_points.push_back(search_itr->second);
            }
            if (search_itr->second != first_point) {
                possible_match.push_back(search_itr->second);
            } else {
                auto next_itr = std::next(search_itr);
                while (next_itr != angle_points.end() && std::fabs(next_itr->first - search_itr->first) <= 0.0) {
                    if (next_itr->second != first_point) {
                        possible_match.push_back(next_itr->second);
                    }
                    if (set_origin) {
                        origin_points.push_back(next_itr->second);
                    }
                    ++next_itr;
                }
            }
        }
        ++search_itr;
    }
}

template <typename T>
void process_repeated_points(std::size_t repeated_point_count,
                             std::size_t last_index,
                             std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                             ring_manager<T>& rings) {

    std::list<point_ptr<T>> point_list = build_point_list(repeated_point_count, last_index, rings);
    while (point_list.size() > 1) {
        process_front_of_point_list(point_list, dupe_ring, rings);
    }
    /*
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
            */
            /*if (!op_k->ring || !op_j->ring) {
                continue;
            }
            handle_collinear_edges(op_j, op_k, dupe_ring, rings);*/
        /*}
    }*/
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
void process_collinear_rings(std::size_t repeated_point_count,
                             std::size_t last_index,
                             //std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
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
            handle_collinear_edges(op_j, op_k, rings); //dupe_ring, rings);
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
        process_collinear_rings(count, i, rings); //dupe_ring, rings);
        count = 0;
    }
    //std::clog << rings.all_rings << std::endl;
    count = 0;
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
}
}
}
}
