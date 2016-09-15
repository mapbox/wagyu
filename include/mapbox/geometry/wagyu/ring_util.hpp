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
#include <mapbox/geometry/wagyu/util.hpp>

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
T round_towards_min(double val) {
    // 0.5 rounds to 0
    // 0.0 rounds to 0
    // -0.5 rounds to -1
    return static_cast<T>(std::ceil(val - 0.5));
}

template <typename T>
T round_towards_max(double val) {
    // 0.5 rounds to 1
    // 0.0 rounds to 0
    // -0.5 rounds to 0
    return static_cast<T>(std::floor(val + 0.5));
}

template <typename T>
inline T get_edge_min_x(edge<T> const& edge, const T current_y) {
    if (is_horizontal(edge)) {
        if (edge.bot.x < edge.top.x) {
            return edge.bot.x;
        } else {
            return edge.top.x;
        }
    } else if (edge.dx > 0.0) {
        if (current_y == edge.top.y) {
            return edge.top.x;
        } else {
            double lower_range_y = static_cast<double>(current_y - edge.bot.y) - 0.5;
            double return_val = static_cast<double>(edge.bot.x) + edge.dx * lower_range_y;
            T value = round_towards_max<T>(return_val);
            return value;
        }
    } else {
        if (current_y == edge.bot.y) {
            return edge.bot.x;
        } else {
            double return_val = static_cast<double>(edge.bot.x) + edge.dx * (static_cast<double>(current_y - edge.bot.y) + 0.5);
            T value = round_towards_max<T>(return_val);
            return value;
        }
    }
}

template <typename T>
inline T get_edge_max_x(edge<T> const& edge, const T current_y) {
    if (is_horizontal(edge)) {
        if (edge.bot.x > edge.top.x) {
            return edge.bot.x;
        } else {
            return edge.top.x;
        }
    } else if (edge.dx < 0.0) {
        if (current_y == edge.top.y) {
            return edge.top.x;
        } else {
            double lower_range_y = static_cast<double>(current_y - edge.bot.y) - 0.5;
            double return_val = static_cast<double>(edge.bot.x) + edge.dx * lower_range_y;
            T value = round_towards_min<T>(return_val);
            return value;
        }
    } else {
        if (current_y == edge.bot.y) {
            return edge.bot.x;
        } else {
            double return_val = static_cast<double>(edge.bot.x) + edge.dx * (static_cast<double>(current_y - edge.bot.y) + 0.5);
            T value = round_towards_min<T>(return_val);
            return value;
        }
    }
}

template <typename T>
void hot_pixel_set_left_to_right(T y, 
                                 T start_x,
                                 T end_x,
                                 bound<T> & bnd,
                                 ring_manager<T> & rings, 
                                 hot_pixel_set<T>& set) {
    T x_min = get_edge_min_x(*(bnd.current_edge), y);
    x_min = std::max(x_min, start_x);
    T x_max = get_edge_max_x(*(bnd.current_edge), y);
    x_max = std::min(x_max, end_x);
    for (auto itr = set.begin(); itr != set.end(); ++itr) {
        if (*itr > x_max || *itr < x_min) {
            continue;
        }
        mapbox::geometry::point<T> pt(*itr, y);
        point_ptr<T> op = bnd.ring->points;
        bool to_front = (bnd.side == edge_left);
        if (to_front && (pt == *op)) {
            continue;
        } else if (!to_front && (pt == *op->prev)) {
            continue;
        }
        point_ptr<T> new_point = create_new_point(bnd.ring, pt, op, rings);
        if (to_front) {
            bnd.ring->points = new_point;
        }
    }
}

template <typename T>
void hot_pixel_set_right_to_left(T y,
                                 T start_x,
                                 T end_x,
                                 bound<T> & bnd,
                                 ring_manager<T> & rings, 
                                 hot_pixel_set<T>& set) {
    T x_min = get_edge_min_x(*(bnd.current_edge), y);
    x_min = std::max(x_min, end_x);
    T x_max = get_edge_max_x(*(bnd.current_edge), y);
    x_max = std::min(x_max, start_x);
    for (auto itr = set.rbegin(); itr != set.rend(); ++itr) {
        if (*itr > x_max || *itr < x_min) {
            continue;
        }
        mapbox::geometry::point<T> pt(*itr, y);
        point_ptr<T> op = bnd.ring->points;
        bool to_front = (bnd.side == edge_left);
        if (to_front && (pt == *op)) {
            continue;
        } else if (!to_front && (pt == *op->prev)) {
            continue;
        }
        point_ptr<T> new_point = create_new_point(bnd.ring, pt, op, rings);
        if (to_front) {
            bnd.ring->points = new_point;
        }
    }
}

template <typename T>
void insert_hot_pixels_in_path(bound<T> & bnd, 
                               mapbox::geometry::point<T> end_pt,
                               ring_manager<T>& rings) {
    if (end_pt == bnd.last_point) {
        return;
    }
    if (!bnd.ring) {
        bnd.last_point = end_pt;
        return;
    }

    T start_y = bnd.last_point.y;
    T start_x = bnd.last_point.x;
    T end_y = end_pt.y;
    T end_x = end_pt.x;

    if (start_x > end_x) {
        for (auto hp : rings.hot_pixels) {
            if (hp.first > start_y || hp.first < end_y) {
                continue;
            }
            hot_pixel_set_right_to_left(hp.first, start_x, end_x, bnd, rings, hp.second);
        }
    } else {
        for (auto hp : rings.hot_pixels) {
            if (hp.first > start_y || hp.first < end_y) {
                continue;
            }
            hot_pixel_set_left_to_right(hp.first, start_x, end_x, bnd, rings, hp.second);
        }
    }
    bnd.last_point = end_pt;
}

template <typename T>
void add_to_hot_pixels(mapbox::geometry::point<T> const& pt, ring_manager<T>& rings) {
    
    auto hp_itr = rings.hot_pixels.find(pt.y);
    if (hp_itr == rings.hot_pixels.end()) {
        hot_pixel_set<T> hp_set;
        hp_set.emplace(pt.x);
        rings.hot_pixels.emplace(pt.y, hp_set);
    } else {
        hp_itr->second.insert(pt.x);
    }
}

template <typename T>
void add_first_point(active_bound_list_itr<T>& bnd,
                             active_bound_list<T>& active_bounds,
                             mapbox::geometry::point<T> const& pt,
                             ring_manager<T>& rings) {

    ring_ptr<T> r = create_new_ring(rings);
    (*bnd)->ring = r;
    r->is_open = ((*bnd)->winding_delta == 0);
    r->points = create_new_point(r, pt, rings);
    if (!r->is_open) {
        set_hole_state(bnd, active_bounds, rings);
    }
    (*bnd)->last_point = pt;
    add_to_hot_pixels(pt, rings);
}

template <typename T>
void add_first_point(active_bound_list_rev_itr<T>& bnd,
                             active_bound_list<T>& active_bounds,
                             mapbox::geometry::point<T> const& pt,
                             ring_manager<T>& rings) {
    ring_ptr<T> r = create_new_ring(rings);
    // no ring currently set!
    (*bnd)->ring = r;
    r->is_open = ((*bnd)->winding_delta == 0);
    r->points = create_new_point(r, pt, rings);
    if (!r->is_open) {
        set_hole_state(bnd, active_bounds, rings);
    }
    (*bnd)->last_point = pt;
    add_to_hot_pixels(pt, rings);
}

template <typename T>
void add_point_to_ring(bound<T> & bnd,
                               mapbox::geometry::point<T> const& pt,
                               ring_manager<T>& rings) {
    assert(bnd.ring);
    // Handle hot pixels
    insert_hot_pixels_in_path(bnd, pt, rings); 
    
    // bnd.ring->points is the 'Left-most' point & bnd.ring->points->prev is the
    // 'Right-most'
    point_ptr<T> op = bnd.ring->points;
    bool to_front = (bnd.side == edge_left);
    if (to_front && (pt == *op)) {
        return;
    } else if (!to_front && (pt == *op->prev)) {
        return;
    }
    
    point_ptr<T> new_point = create_new_point(bnd.ring, pt, bnd.ring->points, rings);
    if (to_front) {
        bnd.ring->points = new_point;
    }
    add_to_hot_pixels(pt, rings);
}

template <typename T>
void add_point(active_bound_list_itr<T>& bnd,
                       active_bound_list<T>& active_bounds,
                       mapbox::geometry::point<T> const& pt,
                       ring_manager<T>& rings) {
    if (!(*bnd)->ring) {
        add_first_point(bnd, active_bounds, pt, rings);
    } else {
        add_point_to_ring(*(*bnd), pt, rings);
    }
}

template <typename T>
void add_point(active_bound_list_rev_itr<T>& bnd,
                       active_bound_list<T>& active_bounds,
                       mapbox::geometry::point<T> const& pt,
                       ring_manager<T>& rings) {
    if (!(*bnd)->ring) {
        add_first_point(bnd, active_bounds, pt, rings);
    } else {
        add_point_to_ring(*(*bnd), pt, rings);
    }
}

template <typename T>
void add_local_minimum_point(active_bound_list_itr<T> b1,
                                     active_bound_list_itr<T> b2,
                                     active_bound_list<T>& active_bounds,
                                     mapbox::geometry::point<T> const& pt,
                                     ring_manager<T>& rings) {
    active_bound_list_itr<T> b;
    active_bound_list_rev_itr<T> prev_bound;
    active_bound_list_rev_itr<T> prev_b1(b1);
    active_bound_list_rev_itr<T> prev_b2(b2);
    if (is_horizontal(*((*b2)->current_edge)) ||
        ((*b1)->current_edge->dx > (*b2)->current_edge->dx)) {
        add_point(b1, active_bounds, pt, rings);
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
        add_point(b2, active_bounds, pt, rings);
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
    insert_hot_pixels_in_path(*(*b2), pt, rings);
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
