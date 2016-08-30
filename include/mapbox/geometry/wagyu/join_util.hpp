#pragma once

#include <mapbox/geometry/wagyu/active_bound_list.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/join.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

/*
template <typename T>
ring_ptr<T> get_ring(ring_ptr<T> r) {
    assert(r != nullptr);
    while (r != r->replacement_ring) {
        r = r->replacement_ring;
        assert(r != nullptr);
    }
    return r;
}
*/

template <typename T>
point_ptr<T> duplicate_point(point_ptr<T> pt, bool insert_after, ring_manager<T> & rings) {
    point_ptr<T> result = create_new_point(pt->ring, mapbox::geometry::point<T>(pt->x, pt->y), rings);

    if (insert_after) {
        result->next = pt->next;
        result->prev = pt;
        pt->next->prev = result;
        pt->next = result;
    } else {
        result->prev = pt->prev;
        result->next = pt;
        pt->prev->next = result;
        pt->prev = result;
    }
    return result;
}

template <typename T>
bool join_horizontal(point_ptr<T> op1,
                     point_ptr<T> op1b,
                     point_ptr<T> op2,
                     point_ptr<T> op2b,
                     mapbox::geometry::point<T> const& pt,
                     bool discard_left,
                     ring_manager<T> & rings) {
    horizontal_direction dir1 = (op1->x > op1b->x ? horizontal_direction::right_to_left
                                                  : horizontal_direction::left_to_right);
    horizontal_direction dir2 = (op2->x > op2b->x ? horizontal_direction::right_to_left
                                                  : horizontal_direction::left_to_right);
    if (dir1 == dir2) {
        return false;
    }

    // When discard_left, we want Op1b to be on the Left of Op1, otherwise we
    // want Op1b to be on the Right. (And likewise with Op2 and Op2b.)
    // So, to facilitate this while inserting Op1b and Op2b ...
    // when discard_left, make sure we're AT or RIGHT of pt before adding Op1b,
    // otherwise make sure we're AT or LEFT of pt. (Likewise with Op2b.)

    if (dir1 == horizontal_direction::left_to_right) {
        while (op1->next->x <= pt.x && op1->next->x >= op1->x && op1->next->y == pt.y) {
            op1 = op1->next;
        }
        if (discard_left && (op1->x != pt.x)) {
            op1 = op1->next;
        }
        op1b = duplicate_point(op1, !discard_left, rings);
        if (*op1b != pt) {
            op1 = op1b;
            op1->x = pt.x;
            op1->y = pt.y;
            op1b = duplicate_point(op1, !discard_left, rings);
        }
    } else {
        while (op1->next->x >= pt.x && op1->next->x <= op1->x && op1->next->y == pt.y) {
            op1 = op1->next;
        }
        if (!discard_left && (op1->x != pt.x)) {
            op1 = op1->next;
        }
        op1b = duplicate_point(op1, discard_left, rings);
        if (*op1b != pt) {
            op1 = op1b;
            op1->x = pt.x;
            op1->y = pt.y;
            op1b = duplicate_point(op1, discard_left, rings);
        }
    }

    if (dir2 == horizontal_direction::left_to_right) {
        while (op2->next->x <= pt.x && op2->next->x >= op2->x && op2->next->y == pt.y) {
            op2 = op2->next;
        }
        if (discard_left && (op2->x != pt.x)) {
            op2 = op2->next;
        }
        op2b = duplicate_point(op2, !discard_left, rings);
        if (*op2b != pt) {
            op2 = op2b;
            op2->x = pt.x;
            op2->y = pt.y;
            op2b = duplicate_point(op2, !discard_left, rings);
        };
    } else {
        while (op2->next->x >= pt.x && op2->next->x <= op2->x && op2->next->y == pt.y) {
            op2 = op2->next;
        }
        if (!discard_left && (op2->x != pt.x)) {
            op2 = op2->next;
        }
        op2b = duplicate_point(op2, discard_left, rings);
        if (*op2b != pt) {
            op2 = op2b;
            op2->x = pt.x;
            op2->y = pt.y;
            op2b = duplicate_point(op2, discard_left, rings);
        };
    };

    if ((dir1 == horizontal_direction::left_to_right) == discard_left) {
        op1->prev = op2;
        op2->next = op1;
        op1b->next = op2b;
        op2b->prev = op1b;
    } else {
        op1->next = op2;
        op2->prev = op1;
        op1b->prev = op2b;
        op2b->next = op1b;
    }
    return true;
}

template <typename T>
bool get_overlap(const T a1, const T a2, const T b1, const T b2, T& left, T& right) {
    if (a1 < a2) {
        if (b1 < b2) {
            left = std::max(a1, b1);
            right = std::min(a2, b2);
        } else {
            left = std::max(a1, b2);
            right = std::min(a2, b1);
        }
    } else {
        if (b1 < b2) {
            left = std::max(a2, b1);
            right = std::min(a1, b2);
        } else {
            left = std::max(a2, b2);
            right = std::min(a1, b1);
        }
    }
    return left < right;
}

template <typename T>
bool join_points(join_list_itr<T> j, ring_ptr<T> ring1, ring_ptr<T> ring2, ring_manager<T> & rings) {
    point_ptr<T> op1 = j->point1, op1b;
    point_ptr<T> op2 = j->point2, op2b;

    // There are 3 kinds of joins for output polygons ...
    // 1. Horizontal joins where Join.point1 & Join.point2 are vertices anywhere
    // along (horizontal) collinear edges (& Join.off_point is on the same horizontal).
    // 2. Non-horizontal joins where Join.point1 & Join.point2 are at the same
    // location at the Bottom of the overlapping segment (& Join.off_point is above).
    // 3. StrictSimple joins where edges touch but are not collinear and where
    // Join.point1, Join.point2 & Join.off_point all share the same point.

    bool is_horizontal = (j->point1->y == j->off_point.y);

    if (is_horizontal && (j->off_point == *j->point1) && (j->off_point == *j->point2)) {
        if (ring1 != ring2) {
            // First, op1 next and op2 prev

            op1b = j->point1->next;
            while (op1b != op1 && *op1b == j->off_point) {
                op1b = op1b->next;
            }

            op2b = j->point2->prev;
            while (op2b != op2 && *op2b == j->off_point) {
                op2b = op2b->prev;
            }

            if (*op1b == *op2b) {
                point_ptr<T> op1b_prev = op1b->prev;
                point_ptr<T> op2b_next = op2b->next;

                op1b->prev = op2b;
                op2b->next = op1b;

                op1b_prev->next = op2b_next;
                op2b_next->prev = op1b_prev;
                return true;
            }

            // Second, op1 prev and op2 next

            op2b = j->point2->next;
            while (op2b != op2 && *op2b == j->off_point) {
                op2b = op2b->next;
            }

            op1b = j->point1->prev;
            while (op1b != op1 && *op1b == j->off_point) {
                op1b = op1b->prev;
            }

            if (*op2b == *op1b) {
                point_ptr<T> op2b_prev = op2b->prev;
                point_ptr<T> op1b_next = op1b->next;

                op2b->prev = op1b;
                op1b->next = op2b;

                op2b_prev->next = op1b_next;
                op1b_next->prev = op2b_prev;
                return true;
            }
            return false;
        }

        // Strictly simple join

        op1b = j->point1->next;
        while (op1b != op1 && *op1b == j->off_point) {
            op1b = op1b->next;
        }
        bool reverse1 = op1b->y > j->off_point.y;

        op2b = j->point2->next;
        while (op2b != op2 && *op2b == j->off_point) {
            op2b = op2b->next;
        }
        bool reverse2 = op2b->y > j->off_point.y;

        if (reverse1 == reverse2) {
            return false;
        }

        if (reverse1) {
            op1b = duplicate_point(op1, false, rings);
            op2b = duplicate_point(op2, true, rings);
            op1->prev = op2;
            op2->next = op1;
            op1b->next = op2b;
            op2b->prev = op1b;
            j->point1 = op1;
            j->point2 = op1b;
            return true;
        } else {
            op1b = duplicate_point(op1, true, rings);
            op2b = duplicate_point(op2, false, rings);
            op1->next = op2;
            op2->prev = op1;
            op1b->prev = op2b;
            op2b->next = op1b;
            j->point1 = op1;
            j->point2 = op1b;
            return true;
        }
    } else if (is_horizontal) {
        // treat horizontal joins differently to non-horizontal joins since with
        // them we're not yet sure where the overlapping is. point1.Pt & point2.Pt
        // may be anywhere along the horizontal edge.
        op1b = op1;
        while (op1->prev->y == op1->y && op1->prev != op1b && op1->prev != op2) {
            op1 = op1->prev;
        }
        while (op1b->next->y == op1b->y && op1b->next != op1 && op1b->next != op2) {
            op1b = op1b->next;
        }
        if (op1b->next == op1 || op1b->next == op2) {
            return false; // a flat 'polygon'
        }

        op2b = op2;
        while (op2->prev->y == op2->y && op2->prev != op2b && op2->prev != op1b) {
            op2 = op2->prev;
        }
        while (op2b->next->y == op2b->y && op2b->next != op2 && op2b->next != op1) {
            op2b = op2b->next;
        }
        if (op2b->next == op2 || op2b->next == op1) {
            return false; // a flat 'polygon'
        }

        T left, right;
        // Op1 --> Op1b & Op2 --> Op2b are the extremites of the horizontal edges
        if (!get_overlap(op1->x, op1b->x, op2->x, op2b->x, left, right)) {
            return false;
        }

        // DiscardleftSide: when overlapping edges are joined, a spike will created
        // which needs to be cleaned up. However, we don't want Op1 or Op2 caught up
        // on the discard Side as either may still be needed for other joins ...
        mapbox::geometry::point<T> pt(0, 0);
        bool discard_left_side;
        if (op1->x >= left && op1->x <= right) {
            pt.x = op1->x;
            pt.y = op1->y;
            discard_left_side = (op1->x > op1b->x);
        } else if (op2->x >= left && op2->x <= right) {
            pt.x = op2->x;
            pt.y = op2->y;
            discard_left_side = (op2->x > op2b->x);
        } else if (op1b->x >= left && op1b->x <= right) {
            pt.x = op1b->x;
            pt.y = op1b->y;
            discard_left_side = op1b->x > op1->x;
        } else {
            pt.x = op2b->x;
            pt.y = op2b->y;
            discard_left_side = (op2b->x > op2->x);
        }
        j->point1 = op1;
        j->point2 = op2;
        return join_horizontal(op1, op1b, op2, op2b, pt, discard_left_side, rings);
    } else {
        // nb: For non-horizontal joins ...
        //    1. Jr.point1.Pt.Y == Jr.point2.Pt.Y
        //    2. Jr.point1.Pt > Jr.off_point.Y

        // make sure the polygons are correctly oriented ...
        op1b = op1->next;
        while ((*op1b == *op1) && (op1b != op1)) {
            op1b = op1b->next;
        }
        bool reverse1 = ((op1b->y > op1->y) || !slopes_equal(*op1, *op1b, j->off_point));
        if (reverse1) {
            op1b = op1->prev;
            while ((*op1b == *op1) && (op1b != op1)) {
                op1b = op1b->prev;
            }
            if ((op1b->y > op1->y) || !slopes_equal(*op1, *op1b, j->off_point)) {
                return false;
            }
        };
        op2b = op2->next;
        while ((*op2b == *op2) && (op2b != op2)) {
            op2b = op2b->next;
        }
        bool reverse2 = ((op2b->y > op2->y) || !slopes_equal(*op2, *op2b, j->off_point));
        if (reverse2) {
            op2b = op2->prev;
            while ((*op2b == *op2) && (op2b != op2)) {
                op2b = op2b->prev;
            }
            if ((op2b->y > op2->y) || !slopes_equal(*op2, *op2b, j->off_point)) {
                return false;
            }
        }

        if ((op1b == op1) || (op2b == op2) || (op1b == op2b) ||
            ((ring1 == ring2) && (reverse1 == reverse2))) {
            return false;
        }

        if (reverse1) {
            op1b = duplicate_point(op1, false, rings);
            op2b = duplicate_point(op2, true, rings);
            op1->prev = op2;
            op2->next = op1;
            op1b->next = op2b;
            op2b->prev = op1b;
            j->point1 = op1;
            j->point2 = op1b;
            return true;
        } else {
            op1b = duplicate_point(op1, true, rings);
            op2b = duplicate_point(op2, false, rings);
            op1->next = op2;
            op2->prev = op1;
            op1b->prev = op2b;
            op2b->next = op1b;
            j->point1 = op1;
            j->point2 = op1b;
            return true;
        }
    }
}

template <typename T>
void update_points_ring(ring_ptr<T> ring) {
    point_ptr<T> op = ring->points;
    do {
        op->ring = ring;
        op = op->prev;
    } while (op != ring->points);
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
void remove_spikes(point_ptr<T> & pt, join_list_itr<T> pos, join_list<T> & joins) {
    while (true) {
        if (pt->next == pt) {
            pt->ring = nullptr;
            pt = nullptr;
            break;
        } else if (*(pt) == *(pt->next)) {
            update_in_joins(pt->next, pt, pos, joins);
            point_ptr<T> old_next = pt->next;
            old_next->next->prev = pt;
            pt->next = old_next->next;
            old_next->next = old_next;
            old_next->prev = old_next;
            old_next->ring = nullptr;
        } else if (*(pt) == *(pt->prev)) {
            update_in_joins(pt->prev, pt, pos, joins);
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
void join_common_edges(join_list<T>& joins, ring_manager<T>& rings) {
    for (auto join = joins.begin(); join != joins.end(); ++join) {

        ring_ptr<T> ring1 = join->point1->ring;
        ring_ptr<T> ring2 = join->point2->ring;
        if (join->point1->x == 9 && join->point1->y == 26) {
            std::clog << " YEA WHAT!" << std::endl;
                std::clog << *ring1 << std::endl;
                std::clog << *ring2 << std::endl;
        }

        if (!ring1 || !ring2) {
            continue;
        }
        if (!ring1->points || !ring2->points) {
            continue;
        }
        if (ring1->is_open || ring2->is_open) {
            continue;
        }

        // Get the polygon fragment with the corringt hole state
        // before calling join_points().

        ring_ptr<T> hole_state_ring;
        if (ring1 == ring2) {
            hole_state_ring = ring1;
        } else if (ring1_right_of_ring2(ring1, ring2)) {
            hole_state_ring = ring2;
        } else if (ring1_right_of_ring2(ring2, ring1)) {
            hole_state_ring = ring1;
        } else {
            hole_state_ring = get_lower_most_ring(ring1, ring2);
        }
    
#ifdef DEBUG
        bool crossing = false;
        if (*(join->point1) == *(join->point2) && join->off_point == *(join->point1)) {
            crossing = intersections_cross(join->point1, join->point2);
            if (crossing) {
                std::clog << join->point1->x << ", " << join->point1->y << std::endl;
                std::clog << *ring1 << std::endl;
                std::clog << *ring2 << std::endl;
            }
            assert(!crossing);
        }
#endif
        double original_area;
        bool original_is_positive;
        if (ring1 == ring2) {
            original_area = area(ring1);
            original_is_positive = (original_area > 0.0);
        }

        if (!join_points(join, ring1, ring2, rings)) {
            continue;
        }

        if (ring1 == ring2) {
            // Instead of joining two polygons, we have created a new one
            // by splitting one polygon into two.
            point_ptr<T> pt1 = join->point1;
            point_ptr<T> pt2 = join->point2;
            remove_spikes(pt1, join, joins);
            remove_spikes(pt2, join, joins);
            
            if (pt1 == nullptr && pt2 == nullptr) {
                // Both sections were completely removed!
                remove_ring(ring1, rings);
                continue;
            } else if (pt1 == nullptr) {
                ring1->points = pt2;
                ring1->area = std::numeric_limits<double>::quiet_NaN();
                continue;
            } else if (pt2 == nullptr) {
                ring1->points = pt1;
                ring1->area = std::numeric_limits<double>::quiet_NaN();
                continue;
            }

            ring1->bottom_point = nullptr;
            ring2 = create_new_ring(rings);

            ring1->points = pt1;
            ring1->area = std::numeric_limits<double>::quiet_NaN();
            ring2->points = pt2;
            ring2->area = std::numeric_limits<double>::quiet_NaN();
            double ring1_area = area(ring1);
            double ring2_area = area(ring2);
            bool area_1_is_positive = (ring1_area > 0.0);
            bool area_2_is_positive = (ring2_area > 0.0);
            bool area_1_is_zero = std::fabs(ring1_area) <= 0.0;
            bool area_2_is_zero = std::fabs(ring2_area) <= 0.0;

            update_points_ring(ring2);
            
            if (area_2_is_zero || (area_1_is_positive != area_2_is_positive && area_1_is_positive == original_is_positive)) {
                // new_ring is contained by ring ...
                ring1_child_of_ring2(ring2, ring1, rings);
                fixup_children(ring2, ring1);
            } else if (area_1_is_zero || (area_1_is_positive != area_2_is_positive && area_2_is_positive == original_is_positive)) {
                // ring is contained by ring2 ...
                ring1_sibling_of_ring2(ring2, ring1, rings);
                ring1_child_of_ring2(ring1, ring2, rings);
                fixup_children(ring2, ring1);
            } else {
                // the 2 polygons are separate ...
                ring1_sibling_of_ring2(ring2, ring1, rings);
                fixup_children(ring2, ring1);
            }
            /*
            if (poly2_contains_poly1(ring2->points, ring1->points)) {
                // ring1 contains ring2 ...
                ring1_child_of_ring2(ring2, ring1, rings);
                fixup_children(ring2, ring1);

                //ring2->is_hole = !ring1->is_hole;
                //if (ring2->is_hole == (ring2_area > 0.0)) {
                //    reverse_ring(ring2->points);
                //}
            } else if (poly2_contains_poly1(ring1->points, ring2->points)) {
                // ring2 contains ring1 ...
                ring1_sibling_of_ring2(ring2, ring1, rings);
                ring1_child_of_ring2(ring1, ring2, rings);
                fixup_children(ring2, ring1);
                
                //ring2->is_hole = ring1->is_hole;
                //ring1->is_hole = !ring2->is_hole;
                //if (ring1->is_hole == (ring1_area > 0.0)) {
                //    reverse_ring(ring1->points);
                //}
            } else {
                // the 2 polygons are completely separate ...
                //ring2->is_hole = ring1->is_hole;
                ring1_sibling_of_ring2(ring2, ring1, rings);
                fixup_children(ring2, ring1);
            }
            */
        } else {
            // joined 2 polygons together ...
            ring2->points = nullptr;
            ring2->area = std::numeric_limits<double>::quiet_NaN();
            ring1->area = std::numeric_limits<double>::quiet_NaN();
            ring2->bottom_point = nullptr;

            if (hole_state_ring == ring2) {
                ring1_sibling_of_ring2(ring1, ring2, rings);
            }
            ring1_replaces_ring2(ring1, ring2, rings);
            update_points_ring(ring1);
        }
    }
}
}
}
}
