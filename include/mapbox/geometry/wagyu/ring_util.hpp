#pragma once

#ifdef DEBUG
#include <iostream>
// Example debug print for backtrace - only works on IOS
#include <execinfo.h>
#include <stdio.h>
//
// void* callstack[128];
// int i, frames = backtrace(callstack, 128);
// char** strs = backtrace_symbols(callstack, frames);
// for (i = 0; i < frames; ++i) {
//     printf("%s\n", strs[i]);
// }
// free(strs);
#endif

#include <queue>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
void set_hole_state(active_bound_list_itr<T>& bnd, 
                    active_bound_list<T>& active_bounds,
                    ring_manager<T> & rings) {
    auto bnd2 = active_bound_list_rev_itr<T>(bnd);
    bound_ptr<T> bndTmp = nullptr;
    // Find first non line ring to the left of current bound.
    while (bnd2 != active_bounds.rend()) {
        if ((*bnd2)->ring && (*bnd2)->winding_delta != 0) {
            if (!bndTmp) {
                bndTmp = (*bnd2);
            } else if (bndTmp->ring == (*bnd2)->ring) {
                bndTmp = nullptr;
            }
        }
        ++bnd2;
    }
    if (!bndTmp) {
        (*bnd)->ring->parent = nullptr;
        rings.children.push_back((*bnd)->ring);
    } else {
        (*bnd)->ring->parent = bndTmp->ring;
        bndTmp->ring->children.push_back((*bnd)->ring);
    }
}

template <typename T>
void set_hole_state(active_bound_list_rev_itr<T>& bnd, active_bound_list<T>& active_bounds,
                    ring_manager<T> & rings) {
    auto bnd2 = std::next(bnd);
    bound_ptr<T> bndTmp = nullptr;
    // Find first non line ring to the left of current bound.
    while (bnd2 != active_bounds.rend()) {
        if ((*bnd2)->ring && (*bnd2)->winding_delta != 0) {
            if (!bndTmp) {
                bndTmp = (*bnd2);
            } else if (bndTmp->ring == (*bnd2)->ring) {
                bndTmp = nullptr;
            }
        }
        ++bnd2;
    }

    if (!bndTmp) {
        (*bnd)->ring->parent = nullptr;
        rings.children.push_back((*bnd)->ring);
    } else {
        (*bnd)->ring->parent = bndTmp->ring;
        bndTmp->ring->children.push_back((*bnd)->ring);
    }
}

template <typename T>
point_ptr<T> add_first_point(active_bound_list_itr<T>& bnd,
                             active_bound_list<T>& active_bounds,
                             mapbox::geometry::point<T> const& pt,
                             ring_manager<T>& rings) {

    ring_ptr<T> r = create_new_ring(rings);
    (*bnd)->ring = r;
    r->is_open = ((*bnd)->winding_delta == 0);
    point_ptr<T> new_point = create_new_point(r, pt, rings);
    r->points = new_point;
    if (!r->is_open) {
        set_hole_state(bnd, active_bounds, rings);
    }
    return new_point;
}

template <typename T>
point_ptr<T> add_first_point(active_bound_list_rev_itr<T>& bnd,
                             active_bound_list<T>& active_bounds,
                             mapbox::geometry::point<T> const& pt,
                             ring_manager<T>& rings) {
    ring_ptr<T> r = create_new_ring(rings);
    // no ring currently set!
    (*bnd)->ring = r;
    r->is_open = ((*bnd)->winding_delta == 0);
    point_ptr<T> new_point = create_new_point(r, pt, rings);
    r->points = new_point;
    if (!r->is_open) {
        set_hole_state(bnd, active_bounds, rings);
    }
    return new_point;
}

template <typename T>
point_ptr<T> add_point_to_ring(active_bound_list_itr<T>& bnd,
                               mapbox::geometry::point<T> const& pt,
                               ring_manager<T>& rings) {
    assert((*bnd)->ring);
    /*
    if (pt.x == 16 && pt.y == 13) {
        void* callstack[128];
        int i, frames = backtrace(callstack, 128);
        char** strs = backtrace_symbols(callstack, frames);
        for (i = 0; i < frames; ++i) {
            printf("%s\n", strs[i]);
        }
        free(strs);
        std::clog << *(*bnd)->ring << std::endl;
    }
    */
    ring_ptr<T>& ring = (*bnd)->ring;
    // ring->points is the 'Left-most' point & ring->points->prev is the
    // 'Right-most'
    point_ptr<T> op = ring->points;

    bool to_front = ((*bnd)->side == edge_left);
    if (to_front && (pt == *op)) {
        return op;
    } else if (!to_front && (pt == *op->prev)) {
        return op->prev;
    }
    // Add touching points on "cut backs"
    if (to_front) {
        if (pt.y == op->y && 
            op->next != op && op->next != op->prev && op->y == op->next->y &&
            ((op->x < op->next->x && op->next->x < pt.x) ||
            (op->x > op->next->x && op->next->x > pt.x)))
        {
            mapbox::geometry::point<T> cut_back(op->next->x,op->next->y);
            op = create_new_point(ring, cut_back, op, rings);
        } 
    } else {
        if (pt.y == op->prev->y && 
            op->prev != op && op != op->prev->prev && op->prev->y == op->prev->prev->y && 
            ((op->prev->x > op->prev->prev->x && op->prev->prev->x > pt.x) ||
            (op->prev->x < op->prev->prev->x && op->prev->prev->x < pt.x)))
        {
            mapbox::geometry::point<T> cut_back(op->prev->prev->x,op->prev->prev->y);
            create_new_point(ring, cut_back, op, rings);
        } 
    }   
    point_ptr<T> new_point = create_new_point(ring, pt, op, rings);
    if (to_front) {
        ring->points = new_point;
    }
    return new_point;
}

template <typename T>
point_ptr<T> add_point_to_ring(active_bound_list_rev_itr<T>& bnd,
                               mapbox::geometry::point<T> const& pt,
                               ring_manager<T>& rings) {
    
    /*
    if (pt.x == 16 && pt.y == 13) {
        void* callstack[128];
        int i, frames = backtrace(callstack, 128);
        char** strs = backtrace_symbols(callstack, frames);
        for (i = 0; i < frames; ++i) {
            printf("%s\n", strs[i]);
        }
        free(strs);
        std::clog << *(*bnd)->ring << std::endl;
    }
    */
    assert((*bnd)->ring);
    ring_ptr<T>& ring = (*bnd)->ring;
    // ring->points is the 'Left-most' point & ring->points->prev is the
    // 'Right-most'
    point_ptr<T> op = ring->points;

    bool to_front = ((*bnd)->side == edge_left);
    if (to_front && (pt == *op)) {
        return op;
    } else if (!to_front && (pt == *op->prev)) {
        return op->prev;
    }
    // Add touching points on "cut backs"
    if (to_front) {
        if (pt.y == op->y && 
            op->next != op && op->next != op->prev && op->y == op->next->y &&
            ((op->x < op->next->x && op->next->x < pt.x) ||
            (op->x > op->next->x && op->next->x > pt.x)))
        {
            mapbox::geometry::point<T> cut_back(op->next->x,op->next->y);
            op = create_new_point(ring, cut_back, op, rings);
        } 
    } else {
        if (pt.y == op->prev->y && 
            op->prev != op && op != op->prev->prev && op->prev->y == op->prev->prev->y && 
            ((op->prev->x > op->prev->prev->x && op->prev->prev->x > pt.x) ||
            (op->prev->x < op->prev->prev->x && op->prev->prev->x < pt.x)))
        {
            mapbox::geometry::point<T> cut_back(op->prev->prev->x,op->prev->prev->y);
            create_new_point(ring, cut_back, op, rings);
        } 
    }   
    point_ptr<T> new_point = create_new_point(ring, pt, op, rings);
    if (to_front) {
        ring->points = new_point;
    }
    return new_point;
}

template <typename T>
point_ptr<T> add_point(active_bound_list_itr<T>& bnd,
                       active_bound_list<T>& active_bounds,
                       mapbox::geometry::point<T> const& pt,
                       ring_manager<T>& rings) {
    if (!(*bnd)->ring) {
        return add_first_point(bnd, active_bounds, pt, rings);
    } else {
        return add_point_to_ring(bnd, pt, rings);
    }
}

template <typename T>
point_ptr<T> add_point(active_bound_list_rev_itr<T>& bnd,
                       active_bound_list<T>& active_bounds,
                       mapbox::geometry::point<T> const& pt,
                       ring_manager<T>& rings) {
    if (!(*bnd)->ring) {
        return add_first_point(bnd, active_bounds, pt, rings);
    } else {
        return add_point_to_ring(bnd, pt, rings);
    }
}

template <typename T>
point_ptr<T> add_local_minimum_point(active_bound_list_itr<T> b1,
                                     active_bound_list_itr<T> b2,
                                     active_bound_list<T>& active_bounds,
                                     mapbox::geometry::point<T> const& pt,
                                     ring_manager<T>& rings) {
    point_ptr<T> result;
    active_bound_list_itr<T> b;
    active_bound_list_rev_itr<T> prev_bound;
    active_bound_list_rev_itr<T> prev_b1(b1);
    active_bound_list_rev_itr<T> prev_b2(b2);
    if (is_horizontal(*((*b2)->current_edge)) ||
        ((*b1)->current_edge->dx > (*b2)->current_edge->dx)) {
        result = add_point(b1, active_bounds, pt, rings);
        (*b2)->ring = (*b1)->ring;
        (*b1)->side = edge_left;
        (*b2)->side = edge_right;
        b = b1;
        if (prev_b1 != active_bounds.rend() && std::prev(b) == b2) {
            prev_bound = prev_b2;
        } else {
            prev_bound = prev_b1;
        }
    } else {
        result = add_point(b2, active_bounds, pt, rings);
        (*b1)->ring = (*b2)->ring;
        (*b1)->side = edge_right;
        (*b2)->side = edge_left;
        b = b2;
        if (prev_b2 != active_bounds.rend() && std::prev(b) == b1) {
            prev_bound = prev_b1;
        } else {
            prev_bound = prev_b2;
        }
    }

    if (prev_bound != active_bounds.rend() && (*prev_bound)->ring) {
        T x_prev = std::llround(get_current_x(*((*prev_bound)->current_edge), pt.y));
        T x_bound = std::llround(get_current_x(*((*b)->current_edge), pt.y));
        if (x_prev == x_bound && (*b)->winding_delta != 0 && (*prev_bound)->winding_delta != 0 &&
            slopes_equal(mapbox::geometry::point<T>(x_prev, pt.y), (*prev_bound)->current_edge->top,
                         mapbox::geometry::point<T>(x_bound, pt.y), (*b)->current_edge->top)) {
            add_point(prev_bound, active_bounds, pt, rings);
        }
    }
    return result;
}

template <typename T>
inline double get_dx(point<T> const& pt1, point<T> const& pt2) {
    if (pt1.y == pt2.y) {
        return std::numeric_limits<double>::infinity();
    } else {
        return static_cast<double>(pt2.x - pt2.x) / static_cast<double>(pt2.y - pt1.y);
    }
}

template <typename T>
bool first_is_bottom_point(const_point_ptr<T> btmPt1, const_point_ptr<T> btmPt2) {
    point_ptr<T> p = btmPt1->prev;
    while ((*p == *btmPt1) && (p != btmPt1)) {
        p = p->prev;
    }
    double dx1p = std::fabs(get_dx(*btmPt1, *p));

    p = btmPt1->next;
    while ((*p == *btmPt1) && (p != btmPt1)) {
        p = p->next;
    }
    double dx1n = std::fabs(get_dx(*btmPt1, *p));

    p = btmPt2->prev;
    while ((*p == *btmPt2) && (p != btmPt2)) {
        p = p->prev;
    }
    double dx2p = std::fabs(get_dx(*btmPt2, *p));

    p = btmPt2->next;
    while ((*p == *btmPt2) && (p != btmPt2)) {
        p = p->next;
    }
    double dx2n = std::fabs(get_dx(*btmPt2, *p));

    if (std::fabs(std::max(dx1p, dx1n) - std::max(dx2p, dx2n)) <
            std::numeric_limits<double>::epsilon() &&
        std::fabs(std::min(dx1p, dx1n) - std::min(dx2p, dx2n)) <
            std::numeric_limits<double>::epsilon()) {
        return area_from_point(btmPt1) > 0; // if otherwise identical use orientation
    } else {
        return (dx1p >= dx2p && dx1p >= dx2n) || (dx1n >= dx2p && dx1n >= dx2n);
    }
}

template <typename T>
point_ptr<T> get_bottom_point(point_ptr<T> pp) {
    point_ptr<T> dups = 0;
    point_ptr<T> p = pp->next;
    while (p != pp) {
        if (p->y > pp->y) {
            pp = p;
            dups = 0;
        } else if (p->y == pp->y && p->x <= pp->x) {
            if (p->x < pp->x) {
                dups = 0;
                pp = p;
            } else {
                if (p->next != pp && p->prev != pp) {
                    dups = p;
                }
            }
        }
        p = p->next;
    }
    if (dups) {
        // there appears to be at least 2 vertices at bottom_point so ...
        while (dups != p) {
            if (!first_is_bottom_point(p, dups)) {
                pp = dups;
            }
            dups = dups->next;
            while (*dups != *pp) {
                dups = dups->next;
            }
        }
    }
    return pp;
}

template <typename T>
ring_ptr<T> get_lower_most_ring(ring_ptr<T> outRec1, ring_ptr<T> outRec2) {
    // work out which polygon fragment has the correct hole state ...
    if (!outRec1->bottom_point) {
        outRec1->bottom_point = get_bottom_point(outRec1->points);
    }
    if (!outRec2->bottom_point) {
        outRec2->bottom_point = get_bottom_point(outRec2->points);
    }
    point_ptr<T> OutPt1 = outRec1->bottom_point;
    point_ptr<T> OutPt2 = outRec2->bottom_point;
    if (OutPt1->y > OutPt2->y) {
        return outRec1;
    } else if (OutPt1->y < OutPt2->y) {
        return outRec2;
    } else if (OutPt1->x < OutPt2->x) {
        return outRec1;
    } else if (OutPt1->x > OutPt2->x) {
        return outRec2;
    } else if (OutPt1->next == OutPt1) {
        return outRec2;
    } else if (OutPt2->next == OutPt2) {
        return outRec1;
    } else if (first_is_bottom_point(OutPt1, OutPt2)) {
        return outRec1;
    } else {
        return outRec2;
    }
}

template <typename T>
bool ring1_right_of_ring2(ring_ptr<T> ring1, ring_ptr<T> ring2) {
    do {
        ring1 = ring1->parent;
        if (ring1 == ring2) {
            return true;
        }
    } while (ring1);
    return false;
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
void append_ring(active_bound_list_itr<T>& b1,
                 active_bound_list_itr<T>& b2,
                 active_bound_list<T>& active_bounds,
                 ring_manager<T> & manager) {
    // get the start and ends of both output polygons ...
    ring_ptr<T> outRec1 = (*b1)->ring;
    ring_ptr<T> outRec2 = (*b2)->ring;

    ring_ptr<T> keep_ring;
    bound_ptr<T> keep_bound;
    ring_ptr<T> remove_ring;
    bound_ptr<T> remove_bound;
    if (ring1_right_of_ring2(outRec1, outRec2)) {
        keep_ring = outRec2;
        keep_bound = *b2;
        remove_ring = outRec1;
        remove_bound = *b1;
    } else if (ring1_right_of_ring2(outRec2, outRec1)) {
        keep_ring = outRec1;
        keep_bound = *b1;
        remove_ring = outRec2;
        remove_bound = *b2;
    } else if (outRec1 == get_lower_most_ring(outRec1, outRec2)) {
        keep_ring = outRec1;
        keep_bound = *b1;
        remove_ring = outRec2;
        remove_bound = *b2;
    } else {
        keep_ring = outRec2;
        keep_bound = *b2;
        remove_ring = outRec1;
        remove_bound = *b1;
    }

    // get the start and ends of both output polygons and
    // join b2 poly onto b1 poly and delete pointers to b2 ...

    point_ptr<T> p1_lft = keep_ring->points;
    point_ptr<T> p1_rt = p1_lft->prev;
    point_ptr<T> p2_lft = remove_ring->points;
    point_ptr<T> p2_rt = p2_lft->prev;

    // join b2 poly onto b1 poly and delete pointers to b2 ...
    if (keep_bound->side == edge_left) {
        if (remove_bound->side == edge_left) {
            // z y x a b c
            reverse_ring(p2_lft);
            p2_lft->next = p1_lft;
            p1_lft->prev = p2_lft;
            p1_rt->next = p2_rt;
            p2_rt->prev = p1_rt;
            keep_ring->points = p2_rt;
        } else {
            // x y z a b c
            p2_rt->next = p1_lft;
            p1_lft->prev = p2_rt;
            p2_lft->prev = p1_rt;
            p1_rt->next = p2_lft;
            keep_ring->points = p2_lft;
        }
    } else {
        if (remove_bound->side == edge_right) {
            // a b c z y x
            reverse_ring(p2_lft);
            p1_rt->next = p2_rt;
            p2_rt->prev = p1_rt;
            p2_lft->next = p1_lft;
            p1_lft->prev = p2_lft;
        } else {
            // a b c x y z
            p1_rt->next = p2_lft;
            p2_lft->prev = p1_rt;
            p1_lft->prev = p2_rt;
            p2_rt->next = p1_lft;
        }
    }

    keep_ring->bottom_point = nullptr;
    bool keep_is_hole = ring_is_hole(keep_ring);
    bool remove_is_hole = ring_is_hole(remove_ring);
    
    remove_ring->points = nullptr;
    remove_ring->bottom_point = nullptr;
    if (keep_is_hole != remove_is_hole) {
        ring1_replaces_ring2(keep_ring->parent, remove_ring, manager);
    } else {
        ring1_replaces_ring2(keep_ring, remove_ring, manager);
    }

    update_points_ring(keep_ring);

    // nb: safe because we only get here via AddLocalMaxPoly
    keep_bound->ring = nullptr;
    remove_bound->ring = nullptr;

    for (auto& b : active_bounds) {
        if (b->ring == remove_ring) {
            b->ring = keep_ring;
            b->side = keep_bound->side;
            break; // Not sure why there is a break here but was transfered logic from angus
        }
    }
}

template <typename T>
void add_local_maximum_point(active_bound_list_itr<T>& b1,
                             active_bound_list_itr<T>& b2,
                             mapbox::geometry::point<T> const& pt,
                             ring_manager<T>& rings,
                             active_bound_list<T>& active_bounds) {
    add_point(b1, active_bounds, pt, rings);
    if ((*b2)->winding_delta == 0) {
        add_point(b2, active_bounds, pt, rings);
    }
    if ((*b1)->ring == (*b2)->ring) {
        (*b1)->ring = nullptr;
        (*b2)->ring = nullptr;
        // I am not certain that order is important here?
    } else if ((*b1)->ring->ring_index < (*b2)->ring->ring_index) {
        append_ring(b1, b2, active_bounds, rings);
    } else {
        append_ring(b2, b1, active_bounds, rings);
    }
}

enum point_in_polygon_result : std::int8_t {
    point_on_polygon = -1,
    point_inside_polygon = 0,
    point_outside_polygon = 1
};

template <typename T>
point_in_polygon_result point_in_polygon(point<T> const& pt, point_ptr<T> op) {
    // returns 0 if false, +1 if true, -1 if pt ON polygon boundary
    point_in_polygon_result result = point_outside_polygon;
    point_ptr<T> startOp = op;
    do {
        if (op->next->y == pt.y) {
            if ((op->next->x == pt.x) ||
                (op->y == pt.y && ((op->next->x > pt.x) == (op->x < pt.x)))) {
                return point_on_polygon;
            }
        }
        if ((op->y < pt.y) != (op->next->y < pt.y)) {
            if (op->x >= pt.x) {
                if (op->next->x > pt.x) {
                    // Switch between point outside polygon and point inside
                    // polygon
                    if (result == point_outside_polygon) {
                        result = point_inside_polygon;
                    } else {
                        result = point_outside_polygon;
                    }
                } else {
                    double d =
                        static_cast<double>(op->x - pt.x) *
                            static_cast<double>(op->next->y - pt.y) -
                        static_cast<double>(op->next->x - pt.x) * static_cast<double>(op->y - pt.y);
                    if (std::fabs(d) <= 0) {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (op->next->y > op->y)) {
                        // Switch between point outside polygon and point inside
                        // polygon
                        if (result == point_outside_polygon) {
                            result = point_inside_polygon;
                        } else {
                            result = point_outside_polygon;
                        }
                    }
                }
            } else {
                if (op->next->x > pt.x) {
                    double d =
                        static_cast<double>(op->x - pt.x) *
                            static_cast<double>(op->next->y - pt.y) -
                        static_cast<double>(op->next->x - pt.x) * static_cast<double>(op->y - pt.y);
                    if (std::fabs(d) <= 0) {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (op->next->y > op->y)) {
                        // Switch between point outside polygon and point inside
                        // polygon
                        if (result == point_outside_polygon) {
                            result = point_inside_polygon;
                        } else {
                            result = point_outside_polygon;
                        }
                    }
                }
            }
        }
        op = op->next;
    } while (startOp != op);
    return result;
}

template <typename T>
point_in_polygon_result point_in_polygon(mapbox::geometry::point<double> const& pt, point_ptr<T> op) {
    // returns 0 if false, +1 if true, -1 if pt ON polygon boundary
    point_in_polygon_result result = point_outside_polygon;
    point_ptr<T> startOp = op;
    do {
        if (std::fabs(op->next->y - pt.y) <= 0.0) {
            if (std::fabs(op->next->x - pt.x) <= 0.0 ||
                (std::fabs(op->y - pt.y) <= 0.0 && ((op->next->x > pt.x) == (op->x < pt.x)))) {
                return point_on_polygon;
            }
        }
        if ((op->y < pt.y) != (op->next->y < pt.y)) {
            if (op->x >= pt.x) {
                if (op->next->x > pt.x) {
                    // Switch between point outside polygon and point inside
                    // polygon
                    if (result == point_outside_polygon) {
                        result = point_inside_polygon;
                    } else {
                        result = point_outside_polygon;
                    }
                } else {
                    double d =
                        static_cast<double>(op->x - pt.x) *
                            static_cast<double>(op->next->y - pt.y) -
                        static_cast<double>(op->next->x - pt.x) * static_cast<double>(op->y - pt.y);
                    if (std::fabs(d) <= 0) {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (op->next->y > op->y)) {
                        // Switch between point outside polygon and point inside
                        // polygon
                        if (result == point_outside_polygon) {
                            result = point_inside_polygon;
                        } else {
                            result = point_outside_polygon;
                        }
                    }
                }
            } else {
                if (op->next->x > pt.x) {
                    double d =
                        static_cast<double>(op->x - pt.x) *
                            static_cast<double>(op->next->y - pt.y) -
                        static_cast<double>(op->next->x - pt.x) * static_cast<double>(op->y - pt.y);
                    if (std::fabs(d) <= 0) {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (op->next->y > op->y)) {
                        // Switch between point outside polygon and point inside
                        // polygon
                        if (result == point_outside_polygon) {
                            result = point_inside_polygon;
                        } else {
                            result = point_outside_polygon;
                        }
                    }
                }
            }
        }
        op = op->next;
    } while (startOp != op);
    return result;
}

template <typename T>
point_in_polygon_result inside_or_outside_special(point_ptr<T> first_pt, point_ptr<T> other_poly) {

    if (std::fabs(area(first_pt->ring)) <= 0.0) {
        return point_inside_polygon;
    }
    if (std::fabs(area(other_poly->ring)) <= 0.0) {
        return point_outside_polygon;
    }
    point_ptr<T> pt = first_pt;
    do {
        if (*pt == *(pt->prev) || *pt == *(pt->next) || *(pt->next) == *(pt->prev) ||
            slopes_equal(*(pt->prev), *pt, *(pt->next))) {
            pt = pt->next;
            continue;
        }
        double dx = ((pt->prev->x - pt->x) / 3.0) + ((pt->next->x - pt->x) / 3.0);
        double dy = ((pt->prev->y - pt->y) / 3.0) + ((pt->next->y - pt->y) / 3.0);
        mapbox::geometry::point<double> offset_pt(pt->x + dx, pt->y + dy);
        point_in_polygon_result res = point_in_polygon(offset_pt, pt);
        if (res != point_inside_polygon) {
            offset_pt.x = pt->x - dx;
            offset_pt.y = pt->y - dy;
            res = point_in_polygon(offset_pt, pt);
            if (res != point_inside_polygon) {
                pt = pt->next;
                continue;
            }
        }
        res = point_in_polygon(offset_pt, other_poly);
        if (res == point_on_polygon) {
            pt = pt->next;
            continue;
        }
        return res;
    } while (pt != first_pt);
    return point_inside_polygon;
}

template <typename T>
bool poly2_contains_poly1(point_ptr<T> outpt1, point_ptr<T> outpt2) {
    point_ptr<T> op = outpt1;
    do {
        // nb: PointInPolygon returns 0 if false, +1 if true, -1 if pt on polygon
        point_in_polygon_result res = point_in_polygon(*op, outpt2);
        if (res != point_on_polygon) {
            return res == point_inside_polygon;
        }
        op = op->next;
    } while (op != outpt1);
    point_in_polygon_result res = inside_or_outside_special(outpt1, outpt2);
    return res == point_inside_polygon;
}

template <typename T>
void dispose_out_points(point_ptr<T>& pp) {
    if (pp == nullptr) {
        return;
    }
    pp->prev->next = nullptr;
    while (pp) {
        point_ptr<T> tmpPp = pp;
        pp = pp->next;
        tmpPp->next = tmpPp;
        tmpPp->prev = tmpPp;
        tmpPp->ring = nullptr;
        //delete tmpPp;
    }
}
}
}
}
