#pragma once

#include <queue>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {
template <typename T>
void set_hole_state(edge_ptr<T> e, ring_ptr<T> ring, ring_list<T>& rings) {
    edge_ptr<T> e2 = e->prev_in_AEL;
    edge_ptr<T> eTmp = nullptr;
    while (e2) {
        if (e2->index >= 0 && e2->winding_delta != 0) {
            if (!eTmp) {
                eTmp = e2;
            } else if (eTmp->index == e2->index) {
                eTmp = nullptr;
            }
        }
        e2 = e2->prev_in_AEL;
    }

    if (!eTmp) {
        ring->first_left = nullptr;
        ring->is_hole = false;
    } else {
        ring->first_left = rings[eTmp->index];
        ring->is_hole = !ring->first_left->is_hole;
    }
}

template <typename T>
point_ptr<T> add_point(edge_ptr<T> e, mapbox::geometry::point<T> const& pt, ring_list<T>& rings) {
    if (e->index < 0) {
        rings_ptr<T> ring = create_new_ring(rings);
        ring->is_open = (e->winding_delta == 0);
        point_ptr<T> new_point = new point<T>(ring->index, pt);
        ring->points = new_point;
        if (!ring->is_open) {
            set_hole_state(e, ring, rings);
        }
        e->index = ring->index;
        return new_point;
    } else {
        ring_ptr<T> ring = rings[e->index];
        // ring->points is the 'Left-most' point & ring->points->prev is the
        // 'Right-most'
        point_ptr<T> op = ring->points;

        bool ToFront = (e->side == edge_left);
        if (ToFront && (pt == *op)) {
            return op;
        } else if (!ToFront && (pt == *op->prev)) {
            return op->prev;
        }
        point_ptr<T> new_point = new point<T>(ring->index, pt, op);
        if (ToFront) {
            ring->points = new_point;
        }
        return new_point;
    }
}

template <typename T>
point_ptr<T> add_local_minimum_point(edge_ptr<T> e1,
                                     edge_ptr<T> e2,
                                     mapbox::geometry::point<T> const& pt,
                                     ring_list<T>& rings,
                                     join_list<T>& joins) {
    using value_type = T;
    point_ptr<value_type> result;
    edge_ptr<value_type> e;
    edge_ptr<value_type> prev_edge;
    if (is_horizontal(*e2) || (e1->dx > e2->dx)) {
        result = add_point(e1, pt, rings);
        e2->index = e1->index;
        e1->side = edge_left;
        e2->side = edge_right;
        e = e1;
        if (e->prev_in_AEL == e2) {
            prev_edge = e2->prev_in_AEL;
        } else {
            prev_edge = e->prev_in_AEL;
        }
    } else {
        result = add_point(e2, pt, rings);
        e1->index = e2->index;
        e1->side = edge_right;
        e2->side = edge_left;
        e = e2;
        if (e->prev_in_AEL == e1) {
            prev_edge = e1->prev_in_AEL;
        } else {
            prev_edge = e->prev_in_AEL;
        }
    }

    if (prev_edge && prev_edge->index >= 0) {
        value_type x_prev = get_current_x(*prev_edge, pt.y);
        value_type x_edge = get_current_x(*e, pt.y);
        if (xprev == x_edge && e->winding_delta != 0 && prev_edge->winding_delta != 0 &&
            slopes_equal(mapbox::geometry::point<value_type>(xprev, pt.y), prev_edge->top,
                         mapbox::geometry::point<value_type>(x_edge, pt.y), e->top)) {
            point_ptr<T> outpt = add_point(prev_edge, pt, rings);
            joins.emplace_back(result, outpt, e->top);
        }
    }
    return result;
}
}
}
}
