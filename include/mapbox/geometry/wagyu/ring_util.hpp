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

void set_hole_state(edge_list_itr<T> e, edge_list<T> & active_edge_list, ring_ptr<T> ring) {
    auto e2 = edge_list_rev_itr<T>(e);
    edge_ptr<T> eTmp = nullptr;
    while (e2 != active_edge_list.rend()) {
        if (e2->ring && e2->winding_delta != 0) {
            if (!eTmp) {
                eTmp = &(*e2);
            } else if (eTmp->ring == e2->ring) {
                eTmp = nullptr;
            }
        }
        ++e2;
    }

    if (!eTmp) {
        ring->first_left = nullptr;
        ring->is_hole = false;
    } else {
        ring->first_left = eTmp->ring;
        ring->is_hole = !eTmp->is_hole;
    }
}

template <typename T>
void set_hole_state(edge_list_rev_itr<T> e, edge_list<T> & active_edge_list, ring_ptr<T> ring) {
    auto e2 = e - 1;
    edge_ptr<T> eTmp = nullptr;
    while (e2 != active_edge_list.rend()) {
        if (e2->ring && e2->winding_delta != 0) {
            if (!eTmp) {
                eTmp = &(*e2);
            } else if (eTmp->ring == e2->ring) {
                eTmp = nullptr;
            }
        }
        ++e2;
    }

    if (!eTmp) {
        ring->first_left = nullptr;
        ring->is_hole = false;
    } else {
        ring->first_left = eTmp->ring;
        ring->is_hole = !eTmp->is_hole;
    }
}

template <typename T, template <typename ...> class itr_type>
point_ptr<T> add_first_point(itr_type<T> e, edge_list<T> & active_edge_list, mapbox::geometry::point<T> const& pt, ring_list<T>& rings) {
    ring_ptr<T> & ring = e->ring;
    assert(!ring);
    // no ring currently set!
    rings.emplace_back();
    ring = &rings.back();
    ring->is_open = (e->winding_delta == 0);
    point_ptr<T> new_point = new point<T>(ring, pt);
    ring->points = new_point;
    if (!ring->is_open) {
        set_hole_state(e, active_edge_list, ring);
    }
    return new_point;
}

template <typename T, template <typename ...> class itr_type>
point_ptr<T> add_point_to_ring(itr_type<T> e, mapbox::geometry::point<T> const& pt) {
    
    assert(e->ring);
    ring_ptr<T> & ring = e->ring;
    // ring->points is the 'Left-most' point & ring->points->prev is the
    // 'Right-most'
    point_ptr<T> op = ring->points;

    bool to_front = (e->side == edge_left);
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

template <typename T, template <typename ...> class itr_type>
point_ptr<T> add_point(itr_type<T> e, edge_list<T> & active_edge_list, mapbox::geometry::point<T> const& pt, ring_list<T>& rings) {
    if (e->ring) {
        return add_first_point(e, active_edge_list, pt, rings);
    } else {
        return add_point_to_ring(e, pt);
    }
}

template <typename T>
point_ptr<T> add_local_minimum_point(edge_list_itr<T> & e1,
                                     edge_list_itr<T> & e2,
                                     edge_list<T> & active_edge_list,
                                     mapbox::geometry::point<T> const& pt,
                                     ring_list<T>& rings,
                                     join_list<T>& joins) {
    using value_type = T;
    point_ptr<value_type> result;
    edge_list_itr<value_type> e;
    edge_list_rev_itr<value_type> prev_edge;
    edge_list_rev_itr<value_type> prev_e1(e1);
    edge_list_rev_itr<value_type> prev_e2(e2);
    if (is_horizontal(*e2) || (e1->dx > e2->dx)) {
        result = add_point(e1, active_edge_list, pt, rings);
        e2->ring = e1->ring;
        e1->side = edge_left;
        e2->side = edge_right;
        e = e1;
        if (prev_e1 != active_edge_list.rend() && (e - 1) == e2) {
            prev_edge = prev_e2;
        } else {
            prev_edge = prev_e1;
        }
    } else {
        result = add_point(e2, active_edge_list, pt, rings);
        e1->ring = e2->ring;
        e1->side = edge_right;
        e2->side = edge_left;
        e = e2;
        if (prev_e2 != active_edge_list.rend() && (e - 1) == e1) {
            prev_edge = prev_e1;
        } else {
            prev_edge = prev_e2;
        }
    }

    if (prev_edge != active_edge_list.rend() && prev_edge->ring) {
        value_type x_prev = get_current_x(*prev_edge, pt.y);
        value_type x_edge = get_current_x(*e, pt.y);
        if (x_prev == x_edge && e->winding_delta != 0 && prev_edge->winding_delta != 0 &&
            slopes_equal(mapbox::geometry::point<value_type>(x_prev, pt.y), prev_edge->top,
                         mapbox::geometry::point<value_type>(x_edge, pt.y), e->top)) {
            point_ptr<T> outpt = add_point(prev_edge, active_edge_list, pt, rings);
            joins.emplace_back(result, outpt, e->top);
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
void append_ring(edge_ptr<T> e1,
                 edge_ptr<T> e2,
                 ring_list<T>& rings,
                 const_edge_ptr<T> active_edge_list) {
    // get the start and ends of both output polygons ...
    ring_ptr<T> outRec1 = rings[e1->index];
    ring_ptr<T> outRec2 = rings[e2->index];

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
    if (e1->side == edge_left) {
        if (e2->side == edge_left) {
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
        if (e2->side == edge_right) {
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
            //fixup_hole_state_of_children(outRec1, rings);
        }
    }
    outRec2->points = nullptr;
    outRec2->bottom_point = nullptr;
    outRec2->first_left = outRec1;

    int OKindex = e1->index;
    int Obsoleteindex = e2->index;

    // nb: safe because we only get here via AddLocalMaxPoly
    e1->index = EDGE_UNASSIGNED;
    e2->index = EDGE_UNASSIGNED;

    edge_ptr<T> e = active_edge_list;
    while (e) {
        if (e->index == Obsoleteindex) {
            e->index = OKindex;
            e->side = e1->side;
            break;
        }
        e = e->next_in_AEL;
    }

    outRec2->index = outRec1->index;
}

template <typename T>
void add_local_maximum_point(edge_ptr<T> e1,
                             edge_ptr<T> e2,
                             mapbox::geometry::point<T> const& pt,
                             ring_list<T>& rings,
                             const_edge_ptr<T> active_edge_list) {
    add_point(e1, pt, rings);
    if (e2->winding_delta == 0) {
        add_point(e2, pt, rings);
    }
    if (e1->index == e2->index) {
        e1->index = EDGE_UNASSIGNED;
        e2->index = EDGE_UNASSIGNED;
    } else if (e1->index < e2->index) {
        append_ring(e1, e2, rings, active_edge_list);
    } else {
        append_ring(e2, e1, rings, active_edge_list);
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
