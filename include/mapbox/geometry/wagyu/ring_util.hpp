#pragma once

#include <queue>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
void fixup_hole_state_of_children(ring_ptr<T> ring, ring_list<T>& rings) {
    for (auto& r : rings) {
        if (r != ring && r->points && ring == r->first_left && ring->is_hole == r->is_hole) {
            r->is_hole = !ring->is_hole;
            fixup_hole_state_of_children(r, rings);
        }
    }
}

template <typename T>
void promote_children_of_removed_ring(ring_ptr<T> ring, ring_list<T>& rings) {
    for (auto& r : rings) {
        if (r->points && (ring == parse_first_left(r->first_left) || ring == r->first_left)) {
            r->is_hole = ring->is_hole;
            r->first_left = ring->first_left;
            fixup_hole_state_of_children(r, rings);
        }
    }
}

template <typename T>
void set_hole_state(active_bound_list_itr<T>& bnd, active_bound_list<T>& active_bounds) {
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
        (*bnd)->ring->first_left = nullptr;
        (*bnd)->ring->is_hole = false;
    } else {
        (*bnd)->ring->first_left = bndTmp->ring;
        (*bnd)->ring->is_hole = !bndTmp->ring->is_hole;
    }
}

template <typename T>
void set_hole_state(active_bound_list_rev_itr<T>& bnd, active_bound_list<T>& active_bounds) {
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
        (*bnd)->ring->first_left = nullptr;
        (*bnd)->ring->is_hole = false;
    } else {
        (*bnd)->ring->first_left = bndTmp->ring;
        (*bnd)->ring->is_hole = !bndTmp->ring->is_hole;
    }
}

template <typename T>
point_ptr<T> add_first_point(active_bound_list_itr<T>& bnd,
                             active_bound_list<T>& active_bounds,
                             mapbox::geometry::point<T> const& pt,
                             ring_list<T>& rings) {
    ring_ptr<T> r = new ring<T>();
    // no ring currently set!
    rings.emplace_back(r);
    (*bnd)->ring = r;
    r->is_open = ((*bnd)->winding_delta == 0);
    point_ptr<T> new_point = new point<T>(r, pt);
    r->points = new_point;
    if (!r->is_open) {
        set_hole_state(bnd, active_bounds);
    }
    return new_point;
}

template <typename T>
point_ptr<T> add_first_point(active_bound_list_rev_itr<T>& bnd,
                             active_bound_list<T>& active_bounds,
                             mapbox::geometry::point<T> const& pt,
                             ring_list<T>& rings) {
    ring_ptr<T> r = new ring<T>();
    // no ring currently set!
    rings.emplace_back(r);
    (*bnd)->ring = r;
    r->is_open = ((*bnd)->winding_delta == 0);
    point_ptr<T> new_point = new point<T>(r, pt);
    r->points = new_point;
    if (!r->is_open) {
        set_hole_state(bnd, active_bounds);
    }
    return new_point;
}

template <typename T>
point_ptr<T> add_point_to_ring(active_bound_list_itr<T>& bnd, mapbox::geometry::point<T> const& pt) {

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
    point_ptr<T> new_point = new point<T>(ring, pt, op);
    if (to_front) {
        ring->points = new_point;
    }
    return new_point;
}

template <typename T>
point_ptr<T> add_point_to_ring(active_bound_list_rev_itr<T>& bnd, mapbox::geometry::point<T> const& pt) {

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
    point_ptr<T> new_point = new point<T>(ring, pt, op);
    if (to_front) {
        ring->points = new_point;
    }
    return new_point;
}
template <typename T>
point_ptr<T> add_point(active_bound_list_itr<T>& bnd,
                       active_bound_list<T>& active_bounds,
                       mapbox::geometry::point<T> const& pt,
                       ring_list<T>& rings) {
    if ((*bnd)->ring) {
        return add_first_point(bnd, active_bounds, pt, rings);
    } else {
        return add_point_to_ring(bnd, pt);
    }
}

template <typename T>
point_ptr<T> add_point(active_bound_list_rev_itr<T>& bnd,
                       active_bound_list<T>& active_bounds,
                       mapbox::geometry::point<T> const& pt,
                       ring_list<T>& rings) {
    if ((*bnd)->ring) {
        return add_first_point(bnd, active_bounds, pt, rings);
    } else {
        return add_point_to_ring(bnd, pt);
    }
}

template <typename T>
point_ptr<T> add_local_minimum_point(active_bound_list_itr<T>& b1,
                                     active_bound_list_itr<T>& b2,
                                     active_bound_list<T>& active_bounds,
                                     mapbox::geometry::point<T> const& pt,
                                     ring_list<T>& rings,
                                     join_list<T>& joins) {
    point_ptr<T> result;
    active_bound_list_itr<T> b;
    active_bound_list_rev_itr<T> prev_bound;
    active_bound_list_rev_itr<T> prev_b1(b1);
    active_bound_list_rev_itr<T> prev_b2(b2);
    if (is_horizontal(*((*b2)->current_edge)) || ((*b1)->current_edge->dx > (*b2)->current_edge->dx)) {
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
        T x_prev = get_current_x(*((*prev_bound)->current_edge), pt.y);
        T x_bound = get_current_x(*((*b)->current_edge), pt.y);
        if (x_prev == x_bound && (*b)->winding_delta != 0 && (*prev_bound)->winding_delta != 0 &&
            slopes_equal(mapbox::geometry::point<T>(x_prev, pt.y), (*prev_bound)->current_edge->top,
                         mapbox::geometry::point<T>(x_bound, pt.y), (*b)->current_edge->top)) {
            point_ptr<T> outpt = add_point(prev_bound, active_bounds, pt, rings);
            joins.emplace_back(result, outpt, (*b)->current_edge->top);
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
        return area(btmPt1) > 0; // if otherwise identical use orientation
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
        ring1 = ring1->first_left;
        if (ring1 == ring2) {
            return true;
        }
    } while (ring1);
    return false;
}

template <typename T>
void append_ring(active_bound_list_itr<T>& b1,
                 active_bound_list_itr<T>& b2,
                 active_bound_list<T>& active_bounds) {
    // get the start and ends of both output polygons ...
    ring_ptr<T> outRec1 = (*b1)->ring;
    ring_ptr<T> outRec2 = (*b2)->ring;

    ring_ptr<T> holeStateRec;
    if (ring1_right_of_ring2(outRec1, outRec2)) {
        holeStateRec = outRec2;
    } else if (ring1_right_of_ring2(outRec2, outRec1)) {
        holeStateRec = outRec1;
    } else {
        holeStateRec = get_lower_most_ring(outRec1, outRec2);
    }

    // get the start and ends of both output polygons and
    // join e2 poly onto e1 poly and delete pointers to e2 ...

    point_ptr<T> p1_lft = outRec1->points;
    point_ptr<T> p1_rt = p1_lft->prev;
    point_ptr<T> p2_lft = outRec2->points;
    point_ptr<T> p2_rt = p2_lft->prev;

    // join e2 poly onto e1 poly and delete pointers to e2 ...
    if ((*b1)->side == edge_left) {
        if ((*b2)->side == edge_left) {
            // z y x a b c
            reverse_ring(p2_lft);
            p2_lft->next = p1_lft;
            p1_lft->prev = p2_lft;
            p1_rt->next = p2_rt;
            p2_rt->prev = p1_rt;
            outRec1->points = p2_rt;
        } else {
            // x y z a b c
            p2_rt->next = p1_lft;
            p1_lft->prev = p2_rt;
            p2_lft->prev = p1_rt;
            p1_rt->next = p2_lft;
            outRec1->points = p2_lft;
        }
    } else {
        if ((*b2)->side == edge_right) {
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

    outRec1->bottom_point = nullptr;
    if (holeStateRec == outRec2) {
        if (outRec2->first_left != outRec1) {
            outRec1->first_left = outRec2->first_left;
        }
        if (outRec1->is_hole != outRec2->is_hole) {
            outRec1->is_hole = outRec2->is_hole;
            // fixup_hole_state_of_children(outRec1, rings);
        }
    }
    outRec2->points = nullptr;
    outRec2->bottom_point = nullptr;
    outRec2->first_left = outRec1;

    ring_ptr<T> OkRing = (*b1)->ring;
    ring_ptr<T> ObsoleteRing = (*b2)->ring;

    // nb: safe because we only get here via AddLocalMaxPoly
    (*b1)->ring = nullptr;
    (*b2)->ring = nullptr;

    for (auto& b : active_bounds) {
        if (b->ring == ObsoleteRing) {
            b->ring = OkRing;
            b->side = (*b1)->side;
            break; // Not sure why there is a break here but was transfered logic from angus
        }
    }
    outRec2->replacement_ring = outRec1;
}

template <typename T>
void add_local_maximum_point(active_bound_list_itr<T>& b1,
                             active_bound_list_itr<T>& b2,
                             mapbox::geometry::point<T> const& pt,
                             ring_list<T>& rings,
                             active_bound_list<T>& active_bounds) {
    add_point(b1, active_bounds, pt, rings);
    if ((*b2)->winding_delta == 0) {
        add_point(b2, active_bounds, pt, rings);
    }
    if ((*b1)->ring == (*b2)->ring) {
        (*b1)->ring = nullptr;
        (*b2)->ring = nullptr;
    // I am not certain that order is important here?
    //} else if ((*b1)->index < (*b2)->index) {
    //    append_ring(b1, b2, active_bounds);
    } else {
        append_ring(b1, b2, active_bounds);
    }
}

template <typename T>
bool poly2_contains_poly1(point_ptr<T> outpt1, point_ptr<T> outpt2) {
    point_ptr<T> op = outpt1;
    do {
        // nb: PointInPolygon returns 0 if false, +1 if true, -1 if pt on polygon
        int res = point_in_polygon(*op, outpt2);
        if (res >= 0) {
            return res > 0;
        }
        op = op->next;
    } while (op != outpt1);
    return true;
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
        delete tmpPp;
    }
}
}
}
}
