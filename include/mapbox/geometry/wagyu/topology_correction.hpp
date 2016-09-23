#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include <algorithm>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <utility>

#include <mapbox/geometry/wagyu/config.hpp>
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
        if (!it_ring1 || !it_ring2 || it_ring1 != ring_search || (!ring_is_hole(it_ring1) && !ring_is_hole(it_ring2))) {
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
            std::fabs(area(it_ring)) < std::numeric_limits<double>::epsilon() ||
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
    ring_ptr<T> r = pt->ring;
    while (true) {
        if (pt->next == pt) {
            r->points = nullptr;
            r->area = std::numeric_limits<double>::quiet_NaN();
            pt->ring = nullptr;
            pt = nullptr;
            break;
        } else if (*(pt) == *(pt->next)) {
            point_ptr<T> old_next = pt->next;
            old_next->next->prev = pt;
            pt->next = old_next->next;
            old_next->next = old_next;
            old_next->prev = old_next;
            if (r->points == old_next) {
                r->points = pt;
            }
            r->area = std::numeric_limits<double>::quiet_NaN();
            old_next->ring = nullptr;
        } else if (*(pt) == *(pt->prev)) {
            point_ptr<T> old_prev = pt->prev;
            old_prev->prev->next = pt;
            pt->prev = old_prev->prev;
            old_prev->next = old_prev;
            old_prev->prev = old_prev;
            if (r->points == old_prev) {
                r->points = pt;
            }
            r->area = std::numeric_limits<double>::quiet_NaN();
            old_prev->ring = nullptr;
        } else if (*(pt->next) == *(pt->prev)) {
            point_ptr<T> next = pt->next;
            point_ptr<T> prev = pt->prev;
            next->prev = prev;
            prev->next = next;
            if (r->points == pt) {
                r->points = prev;
            }
            r->area = std::numeric_limits<double>::quiet_NaN();
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
void fixup_children(ring_ptr<T> old_ring, ring_ptr<T> new_ring) {
    // Tests if any of the children from the old ring are now children of the new ring
    assert(old_ring != new_ring);
    for (auto r = old_ring->children.begin(); r != old_ring->children.end();) {
        assert((*r)->points);
        assert((*r) != old_ring);
        if ((*r) != new_ring && 
            !ring1_right_of_ring2(new_ring, (*r)) && 
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
bool fix_intersects(std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                    point_ptr<T> op_j,
                    point_ptr<T> op_k,
                    ring_ptr<T> ring_j,
                    ring_ptr<T> ring_k,
                    ring_manager<T>& rings) {
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
        if (poly2_contains_poly1(ring_search->points, ring_parent->points)) {
            ring_ptr<T> old_parent = ring_search->parent;
            ring_search->parent = ring_parent;
            if (old_parent) {
                old_parent->children.remove(ring_search);
            } else {
                rings.children.remove(ring_search);
            }
            ring_parent->children.push_back(ring_search);
        } else {
            return false;
        }
    }
    bool found = false;
    std::list<std::pair<ring_ptr<T>, point_ptr_pair<T>>> iList;
    auto range = dupe_ring.equal_range(ring_search);
    // Check for direct connection
    for (auto it = range.first; it != range.second;) {
        if (!it->second.op1->ring) {
            it = dupe_ring.erase(it);
            continue;
        } 
        if (!it->second.op2->ring) {
            it = dupe_ring.erase(it);
            continue;
        } 
        ring_ptr<T> it_ring2 = it->second.op2->ring;
        if (it_ring2 == ring_origin) {
            found = true;
            if (*op_origin_1 != *(it->second.op2)) {
                iList.emplace_back(ring_search, it->second);
                break;
            }
        }
        ++it;
    }
    if (iList.empty()) {
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
void fixup_children_new_interior_ring(ring_ptr<T> old_ring, ring_ptr<T> new_ring, ring_manager<T> & rings) {
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

    // Now we must search the siblings of the old ring
    // This is slow, but there is no other way I know of currently
    // to solve these problems.
    if (old_ring->parent == nullptr) {
        for (auto r = rings.children.begin(); r != rings.children.end();) {
            assert((*r)->points);
            bool ring_area_is_positive = area((*r)) > 0.0;
            if ((*r) != new_ring &&
                ring_area_is_positive == old_ring_area_is_positive && 
                poly2_contains_poly1((*r)->points, new_ring->points)) {
                (*r)->parent = new_ring;
                new_ring->children.push_back((*r));
                r = rings.children.erase(r);
            } else {
                ++r;
            }
        }
    } else {
        ring_ptr<T> parent = old_ring->parent;
        for (auto r = parent->children.begin(); r != parent->children.end();) {
            assert((*r)->points);
            assert((*r) != parent);
            bool ring_area_is_positive = area((*r)) > 0.0;
            if ((*r) != new_ring &&
                ring_area_is_positive == old_ring_area_is_positive && 
                poly2_contains_poly1((*r)->points, new_ring->points)) {
                (*r)->parent = new_ring;
                new_ring->children.push_back((*r));
                r = parent->children.erase(r);
            } else {
                ++r;
            }
        }
    }
}

template <typename T>
bool intersections_cross(point_ptr<T> p1, point_ptr<T> p2) {
    point_ptr<T> p1_next = p1->next;
    point_ptr<T> p2_next = p2->next;
    point_ptr<T> p1_prev = p1->prev;
    point_ptr<T> p2_prev = p2->prev;
    while (*p1_next == *p1) {
        if (p1_next == p1) {
            return false;
        }
        p1_next = p1_next->next;
    }
    while (*p2_next == *p2) {
        if (p2_next == p2) {
            return false;
        }
        p2_next = p2_next->next;
    }
    while (*p1_prev == *p1) {
        if (p1_prev == p1) {
            return false;
        }
        p1_prev = p1_prev->prev;
    }
    while (*p2_prev == *p2) {
        if (p2_prev == p2) {
            return false;
        }
        p2_prev = p2_prev->prev;
    }
    double a1_p1 = std::atan2(static_cast<double>(p1_prev->y - p1->y), static_cast<double>(p1_prev->x - p1->x));
    double a2_p1 = std::atan2(static_cast<double>(p1_next->y - p1->y), static_cast<double>(p1_next->x - p1->x));
    double a1_p2 = std::atan2(static_cast<double>(p2_prev->y - p2->y), static_cast<double>(p2_prev->x - p2->x));
    double a2_p2 = std::atan2(static_cast<double>(p2_next->y - p2->y), static_cast<double>(p2_next->x - p2->x));
    double min_p1 = std::min(a1_p1, a2_p1);
    double max_p1 = std::max(a1_p1, a2_p1);
    double min_p2 = std::min(a1_p2, a2_p2);
    double max_p2 = std::max(a1_p2, a2_p2);
    if (min_p1 < max_p2 && min_p1 > min_p2 && max_p1 > max_p2) {
        return true;
    } else if (min_p2 < max_p1 && min_p2 > min_p1 && max_p2 > max_p1) {
        return true;
    } else {
        return false;
    }
}

template <typename T>
void handle_self_intersections(point_ptr<T> op,
                               point_ptr<T> op2,
                               std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                               ring_manager<T>& rings) {
    // Check that are same ring
    assert(op->ring == op2->ring);
    ring_ptr<T> ring = op->ring;

    point_ptr<T> pt1 = op;
    remove_spikes(pt1);
    if (pt1 == nullptr) {
        ring->points = nullptr;
        ring->area = std::numeric_limits<double>::quiet_NaN();
        remove_ring(ring, rings);
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    } else if (*pt1 != *op) {
        ring->area = std::numeric_limits<double>::quiet_NaN();
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    } else if (pt1 != op) {
        op = pt1;
    }
    
    point_ptr<T> pt2 = op2;
    remove_spikes(pt2);
    if (pt2 == nullptr) {
        ring->points = nullptr;
        ring->area = std::numeric_limits<double>::quiet_NaN();
        remove_ring(ring, rings);
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    } else if (*pt2 != *op2) {
        ring->area = std::numeric_limits<double>::quiet_NaN();
        update_duplicate_point_entries(ring, dupe_ring);
        return;
    } else if (pt2 != op2) {
        op2 = pt2;
    }
    
    if (op == op2) {
        ring->area = std::numeric_limits<double>::quiet_NaN();
        return;
    }
    
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
    bool area_1_is_zero = std::fabs(area_1) < std::numeric_limits<double>::epsilon();
    bool area_2_is_zero = std::fabs(area_2) < std::numeric_limits<double>::epsilon();
    
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
        fixup_children_new_interior_ring(ring, new_ring, rings);
    } else {
        // Situation #2 - create new ring
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
void handle_collinear_rings(point_ptr<T> pt1, point_ptr<T> pt2, 
                            ring_manager<T> & rings) {
    
    ring_ptr<T> ring1 = pt1->ring;
    ring_ptr<T> ring2 = pt2->ring;
    if (!ring1 || !ring2) {
        return;
    }

    if (ring1 == ring2) {
        return;
    }
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
    
    ring1->area = std::numeric_limits<double>::quiet_NaN();
    ring2->area = std::numeric_limits<double>::quiet_NaN();

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
        if (!pt1->ring) {
            pt1 = pt2;
        }
    }
    ring1->points = pt1;
    ring2->points = nullptr;
    ring1_replaces_ring2(ring1, ring2, rings);
    update_points_ring(ring1);
}

template <typename T>
mapbox::geometry::point<T> find_rewind_point(point_ptr<T> pt) {
    mapbox::geometry::point<T> rewind;
    rewind.x = pt->x;
    rewind.y = pt->y;
    point_ptr<T> itr = pt->next;
    while (pt != itr) {
        if (itr->y > rewind.y || (itr->y == rewind.y  && itr->x < rewind.x)) {
            rewind.x = itr->x;
            rewind.y = itr->y;
        }
        itr = itr->next;
    }
    return rewind;
}

template <typename T>
bool handle_collinear_edges(point_ptr<T> pt1, 
                            point_ptr<T> pt2, 
                            std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                            ring_manager<T> & rings,
                            mapbox::geometry::point<T> & rewind_point) {
    ring_ptr<T> ring1 = pt1->ring;
    ring_ptr<T> ring2 = pt2->ring;
    if (ring1 == ring2) {
        return false;
    }
    if (ring1->parent != ring2->parent) {
        return false;
    }

    if (*(pt1->next) != *(pt2->prev) && *(pt2->next) != *(pt1->prev)) {
        return false;
    }

    mapbox::geometry::point<T> rewind_1 = find_rewind_point(pt1);
    mapbox::geometry::point<T> rewind_2 = find_rewind_point(pt2);

    // The lower right of the two points is the rewind point.
    if (rewind_1.y > rewind_2.y) {
        rewind_point = rewind_2;
    } else if (rewind_1.y < rewind_2.y) {
        rewind_point = rewind_1;
    } else if (rewind_1.x > rewind_2.x) {
        rewind_point = rewind_1;
    } else {
        rewind_point = rewind_2;
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
        ring1->area = std::numeric_limits<double>::quiet_NaN();
        remove_ring(ring1, rings);
        ring2->points = nullptr;
        ring2->area = std::numeric_limits<double>::quiet_NaN();
        remove_ring(ring2, rings);
        return false;
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
            return false;
        }
    }
    ring1->points = pt1;
    ring2->points = nullptr;
    ring1->area = std::numeric_limits<double>::quiet_NaN();
    ring2->area = std::numeric_limits<double>::quiet_NaN();
    ring1_replaces_ring2(ring1, ring2, rings);
    update_points_ring(ring1);
    
    update_duplicate_point_entries(ring2, dupe_ring);

    return true;
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
            ring_ptr<T> r = op_j->ring;
            remove_spikes(op_j);
            if (op_j == nullptr) {
                r->area = std::numeric_limits<double>::quiet_NaN();
                remove_ring(r, rings);
                continue;
            }
        }
        if (op_j && op_j->x == original_x && op_j->y == original_y) {
            point_list.push_back(op_j);
        }
    }
    return point_list;
}

template <typename T>
using angle_point = std::tuple<double, point_ptr<T>, bool, double>;

template <typename T>
using angle_point_vector = std::vector<angle_point<T>>;

#ifdef DEBUG

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const angle_point<T>& x) {
    out << "   ap: " << std::get<0>(x) << " - " << std::get<1>(x);
    if (std::get<2>(x)) {
        out << " - away";
    } else {
        out << " - towards";    
    }
    out << " - " << std::get<3>(x) << std::endl;
    return out;
}

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const angle_point_vector<T>& angles) {
    out << "Angle Point Vector:" << std::endl;
    for (auto& x : angles) {
        out << x;
    }
    return out;
}

#endif

template <typename T>
struct segment_angle_sorter {

    inline bool operator()(angle_point<T> const& p1, angle_point<T> const& p2) {
        if (std::fabs(std::get<0>(p1) - std::get<0>(p2)) < std::numeric_limits<double>::epsilon()) {
            return std::get<3>(p1) > std::get<3>(p2);
        } else {
            return std::get<0>(p1) < std::get<0>(p2);
        }
    }
};

template <typename T>
void find_repeated_point_pair(angle_point_vector<T> & angle_points,
                              point_ptr<T> first_point,
                              point_ptr<T> & point_1,
                              point_ptr<T> & point_2) {

    angle_point_vector<T> possible_match;
    auto search_itr = angle_points.begin();
    
    // Move itr to "first point"
    while (std::get<1>(*search_itr) != first_point) {
        ++search_itr;
    }
    
    angle_point<T> angle_1 = *search_itr;

    // Now find next angle for the match
    ++search_itr;
    if (search_itr == angle_points.end()) {
        search_itr = angle_points.begin();
    }

    // It is possible that we have the same angle here..
    while (std::fabs(std::get<0>(angle_1) - std::get<0>(*search_itr)) < std::numeric_limits<double>::epsilon() && std::get<2>(angle_1) == std::get<2>(*search_itr)) {
        ++search_itr;
        if (search_itr == angle_points.end()) {
            search_itr = angle_points.begin();
        }
    }

    // Find next point that is not the same
    while (std::get<1>(angle_1) == std::get<1>(*search_itr)) {
        angle_1 = *search_itr;
        ++search_itr;
        if (search_itr == angle_points.end()) {
            search_itr = angle_points.begin();
        }
    }

    angle_point<T> angle_2 = *search_itr;

    if (std::get<2>(angle_1) == std::get<2>(angle_2)) {
        throw std::runtime_error("Segments are found to be crossing");
    }
    
    point_1 = std::get<1>(angle_1);
    point_2 = std::get<1>(angle_2);
}

template <typename T>
void process_front_of_point_list(std::list<point_ptr<T>>& point_list,
                                 std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                                 ring_manager<T> & rings) {
    angle_point_vector<T> angle_points;
    point_ptr<T> first_point = point_list.front();
    assert(first_point != nullptr);
    ring_ptr<T> r = first_point->ring;
    if (r == nullptr) {
        point_list.pop_front();
        return;
    }
    
    for (auto p = point_list.begin(); p != point_list.end();) {
        if ((*p)->ring != nullptr && (*p)->ring != r) {
            ++p;
            continue;
        }
        double next_angle = calculate_segment_angle_next(*p);
        double prev_angle = calculate_segment_angle_prev(*p);
        if (std::isnan(next_angle) || std::isnan(prev_angle)) {
            ++p;
            continue;
        }
        if (std::fabs(next_angle - prev_angle) < std::numeric_limits<double>::epsilon()) {
            point_ptr<T> spike = *p;
            ring_ptr<T> spike_ring = spike->ring;
            remove_spikes(spike);
            p = point_list.erase(p);
            if (spike == nullptr) {
                spike_ring->area = std::numeric_limits<double>::quiet_NaN();
                remove_ring(spike_ring, rings);
            } 
        } else {
            double dist_next_to_prev;
            double dist_prev_to_next;
            if (next_angle > prev_angle) {
                dist_prev_to_next = next_angle - prev_angle;
                dist_next_to_prev = (2.0 * M_PI) - dist_prev_to_next;
            } else {
                dist_next_to_prev = prev_angle - next_angle;
                dist_prev_to_next = (2.0 * M_PI) - dist_next_to_prev;
            }
            angle_points.emplace_back(next_angle, *p, true, dist_next_to_prev);
            angle_points.emplace_back(prev_angle, *p, false, dist_prev_to_next);
            ++p;
        }
    }
    if (first_point->ring == nullptr) {
        return;
    }

    if (angle_points.size() <= 2) {
        point_list.pop_front();
        return;
    }
    
    // Search forward
    std::stable_sort(angle_points.begin(), angle_points.end(), segment_angle_sorter<T>());
    point_ptr<T> point_1 = nullptr;
    point_ptr<T> point_2 = nullptr;
    find_repeated_point_pair(angle_points, first_point, point_1, point_2);
    handle_self_intersections(point_1, point_2, dupe_ring,
                              rings);
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
    
#ifdef DEBUG
    for (std::size_t j = (last_index - repeated_point_count - 1); j < last_index; ++j) {
        point_ptr<T> op_j = rings.all_points[j];
        if (!op_j->ring) {
            continue;
        }
        double ring_area = area(op_j->ring);
        bool ring_is_positive = ring_area > 0.0;
        bool ring_is_zero = std::fabs(ring_area) < std::numeric_limits<double>::epsilon();
        if (!ring_is_zero) {
            if (op_j->ring->parent) {
                double parent_area = area(op_j->ring->parent);
                bool parent_is_positive = parent_area > 0.0;
                bool parent_is_zero = std::fabs(parent_area) < std::numeric_limits<double>::epsilon();
                if (!parent_is_zero && ring_is_positive == parent_is_positive) {
                    throw std::runtime_error("Created a ring with a parent having the same orientation (sign of area)");
                }
            }
            for (auto c : op_j->ring->children) {
                double c_area = area(c);
                bool c_is_positive = c_area > 0.0;
                bool c_is_zero = std::fabs(c_area) < std::numeric_limits<double>::epsilon();
                if (!c_is_zero && ring_is_positive == c_is_positive) {
                    throw std::runtime_error("Created a ring with a child having the same orientation (sign of area)");
                }
            }
        }
    }
#endif
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
            fix_intersects(dupe_ring, op_j, op_k, op_j->ring, op_k->ring, rings);
        }
    }
}

template <typename T>
void handle_collinear_rings_point_list(std::list<point_ptr<T>> & point_list, ring_manager<T>& rings) {
    
    if (point_list.size() == 1) {
        handle_collinear_rings(point_list.front(), point_list.back(), rings);
        return;
    }
    
    point_ptr<T> first_point = point_list.front();
    angle_point_vector<T> angle_points;
    ring_ptr<T> r = first_point->ring;    

    for (auto p = point_list.begin(); p != point_list.end();) {
        if ((*p)->ring != nullptr && (*p)->ring != r) {
            ++p;
            continue;
        }
        double next_angle = calculate_segment_angle_next(*p);
        double prev_angle = calculate_segment_angle_prev(*p);
        if (std::isnan(next_angle) || std::isnan(prev_angle)) {
            ++p;
            continue;
        }
        if (std::fabs(next_angle - prev_angle) < std::numeric_limits<double>::epsilon()) {
            point_ptr<T> spike = *p;
            ring_ptr<T> spike_ring = spike->ring;
            remove_spikes(spike);
            p = point_list.erase(p);
            if (spike == nullptr) {
                spike_ring->area = std::numeric_limits<double>::quiet_NaN();
                remove_ring(spike_ring, rings);
            }
        } else {
            double dist_next_to_prev;
            double dist_prev_to_next;
            if (next_angle > prev_angle) {
                dist_prev_to_next = next_angle - prev_angle;
                dist_next_to_prev = (2.0 * M_PI) - dist_prev_to_next;
            } else {
                dist_next_to_prev = prev_angle - next_angle;
                dist_prev_to_next = (2.0 * M_PI) - dist_next_to_prev;
            }
            angle_points.emplace_back(next_angle, *p, true, dist_next_to_prev);
            angle_points.emplace_back(prev_angle, *p, false, dist_prev_to_next);
            ++p;
        }
    }
    
    if (angle_points.size() <= 2) {
        return;
    }

    std::stable_sort(angle_points.begin(), angle_points.end(), segment_angle_sorter<T>());
    auto p = angle_points.begin();
    auto p_next = std::next(p);
    while (p_next != angle_points.end()) {
        if (std::fabs(std::get<0>(*p) - std::get<0>(*p_next)) < std::numeric_limits<double>::epsilon() &&
            std::get<2>(*p) != std::get<2>(*p_next) && 
            std::get<1>(*p)->ring != std::get<1>(*p_next)->ring) {
            handle_collinear_rings(std::get<1>(*p), std::get<1>(*p_next), rings);
        }
        p = p_next;
        ++p_next;
    }
}

template <typename T>
void process_collinear_rings(std::size_t repeated_point_count,
                             std::size_t last_index,
                             ring_manager<T>& rings) {
    for (std::size_t j = (last_index - repeated_point_count - 1); j < last_index; ++j) {
        point_ptr<T> op_j = rings.all_points[j];
        ring_ptr<T> ring_j = op_j->ring;
        if (!ring_j) {
            continue;
        }
        std::list<point_ptr<T>> point_list;
        point_list.emplace_back(op_j);
        bool has_different_ring = false;
        for (std::size_t k = j + 1; k < last_index; ++k) {
            point_ptr<T> op_k = rings.all_points[k];
            ring_ptr<T> ring_k = op_k->ring;
            if (!ring_k) {
                continue;
            }
            if (ring_k->parent != ring_j->parent) {
                continue;
            }
            if (*(op_j->next) != *(op_k->prev) && *(op_k->next) != *(op_j->prev)) {
                continue;
            }
            if (ring_k != ring_j) {
                has_different_ring = true;
            }
            point_list.emplace_back(op_k);
        }
        if (!has_different_ring) {
            continue;
        }
        handle_collinear_rings_point_list(point_list, rings);
    }
}

template <typename T>
bool process_collinear_edges(std::size_t repeated_point_count,
                             std::size_t last_index,
                             std::unordered_multimap<ring_ptr<T>, point_ptr_pair<T>>& dupe_ring,
                             ring_manager<T>& rings,
                             mapbox::geometry::point<T> & rewind_point) {
    bool rewind = false;
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
            mapbox::geometry::point<T> possible_rewind_point;
            if (handle_collinear_edges(op_j, op_k, dupe_ring, rings, possible_rewind_point)) {
                if (rewind && (possible_rewind_point.y > rewind_point.y || 
                    (possible_rewind_point.y == rewind_point.y && 
                    possible_rewind_point.x < rewind_point.x))) {
                    rewind_point = possible_rewind_point;
                } else {
                    rewind = true;
                    rewind_point = possible_rewind_point;
                }
            }
        }
    }
    return rewind;
}

template <typename T>
bool index_is_after_point(std::size_t const& i, mapbox::geometry::point<T> const& pt, ring_manager<T> const& rings) {
    if (i == 0) {
        return false;
    }
    if (rings.all_points[i]->y < pt.y) {
        return true;
    } else if (rings.all_points[i]->y > pt.y) {
        return false;
    } else if (rings.all_points[i]->x >= pt.x) {
        return true;
    } else {
        return false;
    }
}

template <typename T>
void rewind_to_point(std::size_t & i, mapbox::geometry::point<T> & pt, ring_manager<T> const& rings) {
    while (index_is_after_point(i, pt, rings)) {
        --i;
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
        mapbox::geometry::point<T> rewind_point;
        if (process_collinear_edges(count, i, dupe_ring, rings, rewind_point)) {
            rewind_to_point(i, rewind_point, rings);
        }
        count = 0;
    }
}
}
}
}
