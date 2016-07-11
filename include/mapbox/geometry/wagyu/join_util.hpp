#pragma once

#include <mapbox/geometry/wagyu/active_bound_list.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/join.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
ring_ptr<T> get_ring(ring_ptr<T> r) {
    while (r != r->replacement_ring) {
        r = r->replacement_ring;
    }
    return r;
}

template <typename T>
point_ptr<T> duplicate_point(point_ptr<T> pt, bool insert_after) {
    point_ptr<T> result = new point<T>(pt->x, pt->y);
    result->ring = pt->ring;

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
                     const point<T> pt,
                     bool discard_left) {
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
        op1b = duplicate_point(op1, !discard_left);
        if (*op1b != pt) {
            op1 = op1b;
            *op1 = pt;
            op1b = duplicate_point(op1, !discard_left);
        }
    } else {
        while (op1->next->x >= pt.x && op1->next->x <= op1->x && op1->next->y == pt.y) {
            op1 = op1->next;
        }
        if (!discard_left && (op1->x != pt.x)) {
            op1 = op1->next;
        }
        op1b = duplicate_point(op1, discard_left);
        if (*op1b != pt) {
            op1 = op1b;
            *op1 = pt;
            op1b = duplicate_point(op1, discard_left);
        }
    }

    if (dir2 == horizontal_direction::left_to_right) {
        while (op2->next->x <= pt.x && op2->next->x >= op2->x && op2->next->y == pt.y) {
            op2 = op2->next;
        }
        if (discard_left && (op2->x != pt.x)) {
            op2 = op2->next;
        }
        op2b = duplicate_point(op2, !discard_left);
        if (*op2b != pt) {
            op2 = op2b;
            *op2 = pt;
            op2b = duplicate_point(op2, !discard_left);
        };
    } else {
        while (op2->next->x >= pt.x && op2->next->x <= op2->x && op2->next->y == pt.y) {
            op2 = op2->next;
        }
        if (!discard_left && (op2->x != pt.x)) {
            op2 = op2->next;
        }
        op2b = duplicate_point(op2, discard_left);
        if (*op2b != pt) {
            op2 = op2b;
            *op2 = pt;
            op2b = duplicate_point(op2, discard_left);
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
bool join_points(join_ptr<T> j, ring_ptr<T> ring1, ring_ptr<T> ring2) {
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
            op1b = duplicate_point(op1, false);
            op2b = duplicate_point(op2, true);
            op1->prev = op2;
            op2->next = op1;
            op1b->next = op2b;
            op2b->prev = op1b;
            j->point1 = op1;
            j->point2 = op1b;
            return true;
        } else {
            op1b = duplicate_point(op1, true);
            op2b = duplicate_point(op2, false);
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
        point<T> pt(0, 0);
        bool discard_left_side;
        if (op1->x >= left && op1->x <= right) {
            pt = *op1;
            discard_left_side = (op1->x > op1b->x);
        } else if (op2->x >= left && op2->x <= right) {
            pt = *op2;
            discard_left_side = (op2->x > op2b->x);
        } else if (op1b->x >= left && op1b->x <= right) {
            pt = *op1b;
            discard_left_side = op1b->x > op1->x;
        } else {
            pt = *op2b;
            discard_left_side = (op2b->x > op2->x);
        }
        j->point1 = op1;
        j->point2 = op2;
        return join_horizontal(op1, op1b, op2, op2b, pt, discard_left_side);
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
            op1b = duplicate_point(op1, false);
            op2b = duplicate_point(op2, true);
            op1->prev = op2;
            op2->next = op1;
            op1b->next = op2b;
            op2b->prev = op1b;
            j->point1 = op1;
            j->point2 = op1b;
            return true;
        } else {
            op1b = duplicate_point(op1, true);
            op2b = duplicate_point(op2, false);
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
void fixup_first_lefts1(ring_ptr<T> old_ring, ring_ptr<T> new_ring, ring_list<T>& rings) {
    // tests if new_ring contains the polygon before reassigning first_left
    for (auto& ring : rings) {
        ring_ptr<T> first_left = parse_first_left(ring->first_left);
        if (ring->points && first_left == old_ring) {
            if (poly2_contains_poly1(ring->points, new_ring->points)) {
                if (ring->is_hole == new_ring->is_hole) {
                    ring->is_hole = !ring->is_hole;
                    reverse_ring(ring->points);
                }
                ring->first_left = new_ring;
            }
        }
    }
}

template <typename T>
void fixup_first_lefts2(ring_ptr<T> inner_ring, ring_ptr<T> outer_ring, ring_list<T>& rings) {
    // A polygon has split into two such that one is now the inner of the other.
    // It's possible that these polygons now wrap around other polygons, so check
    // every polygon that's also contained by outer_ring's first_left container
    //(including 0) to see if they've become inner to the new inner polygon ...
    for (auto& ring : rings) {

        if (!ring->points || ring == outer_ring || ring == inner_ring) {
            continue;
        }
        ring_ptr<T> first_left = parse_first_left(ring->first_left);
        if (first_left != inner_ring && first_left != outer_ring) {
            continue;
        }
        if (poly2_contains_poly1(ring->points, inner_ring->points)) {
            if (ring->is_hole == inner_ring->is_hole) {
                ring->is_hole = !ring->is_hole;
                reverse_ring(ring->points);
            }
            ring->first_left = inner_ring;
        } else {
            if (ring->is_hole == outer_ring->is_hole) {
                if (poly2_contains_poly1(ring->points, outer_ring->points)) {
                    ring->first_left = outer_ring;
                } else {
                    ring->first_left = parse_first_left(outer_ring->first_left);
                }
            } else {
                if (std::fabs(area(ring->points)) <= 0.0 &&
                    !poly2_contains_poly1(ring->points, outer_ring->points)) {
                    ring->is_hole = !ring->is_hole;
                    ring->first_left = parse_first_left(outer_ring->first_left);
                    reverse_ring(ring->points);
                } else {
                    ring->first_left = outer_ring;
                }
            }
        }
    }
}

template <typename T>
void fixup_first_lefts3(ring_ptr<T> old_ring, ring_ptr<T> new_ring, ring_list<T>& rings) {
    // reassigns first_left WITHOUT testing if new_ring contains the polygon
    for (auto& ring : rings) {
        if (ring->points && parse_first_left(ring->first_left) == old_ring) {
            ring->first_left = new_ring;
        }
    }
}

template <typename T>
void join_common_edges(join_list<T>& joins, ring_list<T>& rings) {
    for (size_t i = 0; i < joins.size(); i++) {
        join_ptr<T> join = &joins[i];

        ring_ptr<T> ring1 = get_ring(join->point1->ring);
        ring_ptr<T> ring2 = get_ring(join->point2->ring);

        if (!ring1->points || !ring2->points) {
            continue;
        }
        if (ring1->is_open || ring2->is_open) {
            continue;
        }

        // Get the polygon fragment with the corringt hole state (first_left)
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

        if (!join_points(join, ring1, ring2)) {
            continue;
        }
        if (ring1 == ring2) {
            // Instead of joining two polygons, we have created a new one
            // by splitting one polygon into two.

            ring1->bottom_point = nullptr;
            ring2 = create_new_ring(rings);
            std::size_t p1_count;
            std::size_t p2_count;
            double p1_area;
            double p2_area;
            double ring1_area;
            double ring2_area;
            area_and_count(join->point1, p1_count, p1_area);
            area_and_count(join->point2, p2_count, p2_area);
            if (p1_count > p2_count) {
                ring1->points = join->point1;
                ring1_area = p1_area;
                ring2->points = join->point2;
                ring2_area = p2_area;
            } else {
                ring1->points = join->point2;
                ring1_area = p2_area;
                ring2->points = join->point1;
                ring2_area = p1_area;
            }

            update_points_ring(ring2);

            if (poly2_contains_poly1(ring2->points, ring1->points)) {
                // ring1 contains ring2 ...
                ring2->is_hole = !ring1->is_hole;
                ring2->first_left = ring1;

                fixup_first_lefts2(ring2, ring1, rings);
                if (ring2->is_hole == (ring2_area > 0.0)) {
                    reverse_ring(ring2->points);
                }
            } else if (poly2_contains_poly1(ring1->points, ring2->points)) {
                // ring2 contains ring1 ...
                ring2->is_hole = ring1->is_hole;
                ring1->is_hole = !ring2->is_hole;
                ring2->first_left = ring1->first_left;
                ring1->first_left = ring2;

                fixup_first_lefts2(ring1, ring2, rings);
                if (ring1->is_hole == (ring1_area > 0.0)) {
                    reverse_ring(ring1->points);
                }
            } else {
                // the 2 polygons are completely separate ...
                ring2->is_hole = ring1->is_hole;
                ring2->first_left = ring1->first_left;

                // fixup first_left pointers that may need reassigning to ring2
                fixup_first_lefts1(ring1, ring2, rings);
            }

        } else {
            // joined 2 polygons together ...
            ring2->points = nullptr;
            ring2->bottom_point = nullptr;
            ring2->replacement_ring = ring1;

            ring1->is_hole = hole_state_ring->is_hole;
            if (hole_state_ring == ring2) {
                ring1->first_left = ring2->first_left;
            }
            ring2->first_left = ring1;
            fixup_first_lefts3(ring2, ring1, rings);
        }
    }
}
}
}
}
