#pragma once

#include <mapbox/geometry/wagyu/active_edge_list.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge_util.hpp>
#include <mapbox/geometry/wagyu/intersect.hpp>
#include <mapbox/geometry/wagyu/intersect_point.hpp>
#include <mapbox/geometry/wagyu/join.hpp>
#include <mapbox/geometry/wagyu/ring_util.hpp>
#include <mapbox/geometry/wagyu/sorted_edge_list.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct intersect_list_sorter {
    inline bool operator()(intersect_node<T> const& node1, intersect_node<T> const& node2) {
        if (node2.pt.y != node1.pt.y) {
            return node2.pt.y < node1.pt.y;
        } else {
            return (node2.edge1->edge->winding_count2 + node2.edge2->edge->winding_count2) >
                   (node1.edge1->edge->winding_count2 + node1.edge2->edge->winding_count2);
        }
    }
};

template <typename T>
void build_intersect_list(T top_y,
                          sorting_edge_list<T>& sorted_edge_list,
                          intersect_list<T>& intersects) {
    // bubblesort ...
    bool isModified;
    do {
        isModified = false;
        auto e = sorted_edge_list.begin();
        auto enext = e + 1;
        while (enext != sorted_edge_list.end()) {
            mapbox::geometry::point<T> pt;
            if (e->edge->curr.x > enext->edge->curr.x) {
                intersection_point(*(e->edge), *(enext->edge), pt);
                if (pt.y < top_y) {
                    pt = mapbox::geometry::point<T>(get_current_x(*(e->edge), top_y), top_y);
                }
                intersects.emplace_back(e, enext, pt);
                sorted_edge_list.splice(e, sorted_edge_list, enext);
                enext = e + 1;
                isModified = true;
            } else {
                e = enext;
                ++enext;
            }
        }
    } while (isModified);
}

template <typename T>
bool edges_adjacent(intersect_node<T> const& inode) {
    return ((inode.edge1 + 1) == inode.edge2) || ((inode.edge2 + 1) == inode.edge1);
}

template <typename T>
bool fixup_intersection_order(sorting_edge_list<T>& sorted_edge_list,
                              intersect_list<T>& intersects) {
    // Precondition: Intersections are sorted bottom-most first.
    // It's crucial that intersections are only made between adjacent edges,
    // so reorder the intersections to ensure this if necessary.

    // resort sorted edge list to the same as the active edge list
    std::sort(sorted_edge_list.begin(), sorted_edge_list.end(), sorting_edge_sorter<T>());

    // Sort the intersection list
    std::stable_sort(intersects.begin(), intersects.end(), intersect_list_sorter<T>());

    std::size_t n = intersects.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (!edges_adjacent(intersects[i])) {
            std::size_t j = i + 1;
            while (j < n && !edges_adjacent(intersects[j])) {
                j++;
            }
            if (j == n) {
                // Intersection could not be made adjacent
                return false;
            }
            std::swap(intersects[i], intersects[j]);
        }
        sorted_edge_list.splice(intersects[i].edge1, sorted_edge_list, intersects[i].edge2);
    }
    return true;
}

template <typename T>
void intersect_edges(edge_list_itr<T>& e1,
                     edge_list_itr<T>& e2,
                     mapbox::geometry::point<T> const& pt,
                     clip_type cliptype,
                     fill_type subject_fill_type,
                     fill_type clip_fill_type,
                     ring_list<T>& rings,
                     join_list<T>& joins,
                     edge_list<T>& active_edge_list) {
    bool e1Contributing = (e1->ring != nullptr);
    bool e2Contributing = (e2->ring != nullptr);

    // if either edge is on an OPEN path ...
    if (e1->winding_delta == 0 || e2->winding_delta == 0) {
        // ignore subject-subject open path intersections UNLESS they
        // are both open paths, AND they are both 'contributing maximas' ...
        if (e1->winding_delta == 0 && e2->winding_delta == 0) {
            return;
        }

        // if intersecting a subj line with a subj poly ...
        else if (e1->poly_type == e2->poly_type && e1->winding_delta != e2->winding_delta &&
                 cliptype == clip_type_union) {
            if (e1->winding_delta == 0) {
                if (e2Contributing) {
                    add_point(e1, active_edge_list, pt, rings);
                    if (e1Contributing) {
                        e1->ring = nullptr;
                    }
                }
            } else {
                if (e1Contributing) {
                    add_point(e2, active_edge_list, pt, rings);
                    if (e2Contributing) {
                        e2->ring = nullptr;
                    }
                }
            }
        } else if (e1->poly_type != e2->poly_type) {
            // toggle subj open path index on/off when std::abs(clip.WndCnt) == 1
            if ((e1->winding_delta == 0) && std::abs(e2->winding_count) == 1 &&
                (cliptype != clip_type_union || e2->winding_count2 == 0)) {
                add_point(e1, active_edge_list, pt, rings);
                if (e1Contributing) {
                    e1->ring = nullptr;
                }
            } else if ((e2->winding_delta == 0) && (std::abs(e1->winding_count) == 1) &&
                       (cliptype != clip_type_union || e1->winding_count2 == 0)) {
                add_point(e2, active_edge_list, pt, rings);
                if (e2Contributing) {
                    e2->ring == nullptr;
                }
            }
        }
        return;
    }

    // update winding counts...
    // assumes that e1 will be to the Right of e2 ABOVE the intersection
    if (e1->poly_type == e2->poly_type) {
        if (is_even_odd_fill_type(*(e1->bound), subject_fill_type, clip_fill_type)) {
            std::int32_t oldE1winding_count = e1->winding_count;
            e1->winding_count = e2->winding_count;
            e2->winding_count = oldE1winding_count;
        } else {
            if (e1->winding_count + e2->winding_delta == 0) {
                e1->winding_count = -e1->winding_count;
            } else {
                e1->winding_count += e2->winding_delta;
            }
            if (e2->winding_count - e1->winding_delta == 0) {
                e2->winding_count = -e2->winding_count;
            } else {
                e2->winding_count -= e1->winding_delta;
            }
        }
    } else {
        if (!is_even_odd_fill_type(*e2, subject_fill_type, clip_fill_type)) {
            e1->winding_count2 += e2->winding_delta;
        } else {
            e1->winding_count2 = (e1->winding_count2 == 0) ? 1 : 0;
        }
        if (!is_even_odd_fill_type(*e1, subject_fill_type, clip_fill_type)) {
            e2->winding_count2 -= e1->winding_delta;
        } else {
            e2->winding_count2 = (e2->winding_count2 == 0) ? 1 : 0;
        }
    }

    fill_type e1FillType, e2FillType, e1FillType2, e2FillType2;
    if (e1->poly_type == polygon_type_subject) {
        e1FillType = subject_fill_type;
        e1FillType2 = clip_fill_type;
    } else {
        e1FillType = clip_fill_type;
        e1FillType2 = subject_fill_type;
    }
    if (e2->poly_type == polygon_type_subject) {
        e2FillType = subject_fill_type;
        e2FillType2 = clip_fill_type;
    } else {
        e2FillType = clip_fill_type;
        e2FillType2 = subject_fill_type;
    }

    std::int32_t e1Wc, e2Wc;
    switch (e1FillType) {
    case fill_type_positive:
        e1Wc = e1->winding_count;
        break;
    case fill_type_negative:
        e1Wc = -e1->winding_count;
        break;
    case fill_type_even_odd:
    case fill_type_non_zero:
    default:
        e1Wc = std::abs(e1->winding_count);
    }
    switch (e2FillType) {
    case fill_type_positive:
        e2Wc = e2->winding_count;
        break;
    case fill_type_negative:
        e2Wc = -e2->winding_count;
        break;
    case fill_type_even_odd:
    case fill_type_non_zero:
    default:
        e2Wc = std::abs(e2->winding_count);
    }

    if (e1Contributing && e2Contributing) {
        if ((e1Wc != 0 && e1Wc != 1) || (e2Wc != 0 && e2Wc != 1) ||
            (e1->poly_type != e2->poly_type && cliptype != clip_type_x_or)) {
            add_local_maximum_point(e1, e2, pt, rings, active_edge_list);
        } else {
            add_point(e1, active_edge_list, pt, rings);
            add_point(e2, active_edge_list, pt, rings);
            swap_sides(*e1, *e2);
            swap_rings(*e1, *e2);
        }
    } else if (e1Contributing) {
        if (e2Wc == 0 || e2Wc == 1) {
            add_point(e1, active_edge_list, pt, rings);
            swap_sides(*e1, *e2);
            swap_rings(*e1, *e2);
        }
    } else if (e2Contributing) {
        if (e1Wc == 0 || e1Wc == 1) {
            add_point(e2, active_edge_list, pt, rings);
            swap_sides(*e1, *e2);
            swap_rings(*e1, *e2);
        }
    } else if ((e1Wc == 0 || e1Wc == 1) && (e2Wc == 0 || e2Wc == 1)) {
        // neither edge is currently contributing ...

        std::int32_t e1Wc2, e2Wc2;
        switch (e1FillType2) {
        case fill_type_positive:
            e1Wc2 = e1->winding_count2;
            break;
        case fill_type_negative:
            e1Wc2 = -e1->winding_count2;
            break;
        case fill_type_even_odd:
        case fill_type_non_zero:
        default:
            e1Wc2 = std::abs(e1->winding_count2);
        }
        switch (e2FillType2) {
        case fill_type_positive:
            e2Wc2 = e2->winding_count2;
            break;
        case fill_type_negative:
            e2Wc2 = -e2->winding_count2;
            break;
        case fill_type_even_odd:
        case fill_type_non_zero:
        default:
            e2Wc2 = std::abs(e2->winding_count2);
        }

        if (e1->poly_type != e2->poly_type) {
            add_local_minimum_point(e1, e2, active_edge_list, pt, rings, joins);
        } else if (e1Wc == 1 && e2Wc == 1) {
            switch (cliptype) {
            case clip_type_intersection:
                if (e1Wc2 > 0 && e2Wc2 > 0) {
                    add_local_minimum_point(e1, e2, active_edge_list, pt, rings, joins);
                }
                break;
            default:
            case clip_type_union:
                if (e1Wc2 <= 0 && e2Wc2 <= 0) {
                    add_local_minimum_point(e1, e2, active_edge_list, pt, rings, joins);
                }
                break;
            case clip_type_difference:
                if (((e1->poly_type == polygon_type_clip) && (e1Wc2 > 0) && (e2Wc2 > 0)) ||
                    ((e1->poly_type == polygon_type_subject) && (e1Wc2 <= 0) && (e2Wc2 <= 0))) {
                    add_local_minimum_point(e1, e2, active_edge_list, pt, rings, joins);
                }
                break;
            case clip_type_x_or:
                add_local_minimum_point(e1, e2, active_edge_list, pt, rings, joins);
            }
        } else {
            swap_sides(*e1, *e2);
        }
    }
}

template <typename T>
void process_intersect_list(intersect_list<T> const& intersects,
                            clip_type cliptype,
                            fill_type subject_fill_type,
                            fill_type clip_fill_type,
                            ring_list<T>& rings,
                            join_list<T>& joins,
                            edge_list<T>& active_edge_list) {
    for (auto const& node : intersects) {
        intersect_edges(node.edge1, node.edge2, node.pt, cliptype, subject_fill_type,
                        clip_fill_type, rings, joins, active_edge_list);
        active_edge_list.splice(node.edge1->edge, active_edge_list, node.edge2->edge);
    }
}

template <typename T>
bool process_intersections(T top_y,
                           edge_list<T>& active_edge_list,
                           clip_type cliptype,
                           fill_type subject_fill_type,
                           fill_type clip_fill_type,
                           ring_list<T>& rings,
                           join_list<T>& joins) {
    if (!active_edge_list.empty()) {
        return true;
    }
    sorting_edge_list<T> sorted_edge_list;
    std::size_t index = 0;
    // create sorted edge list from AEL
    for (auto edge_itr = active_edge_list.begin(); edge_itr != active_edge_list.end(); ++edge_itr) {
        edge_itr->curr.x = get_current_x(*edge_itr, top_y);
        sorted_edge_list.emplace_back(edge_itr, index);
        ++index;
    }

    intersect_list<T> intersects;
    build_intersect_list(top_y, sorted_edge_list, intersects);

    if (intersects.empty()) {
        return true;
    }
    if (intersests.size() > 1) {
        bool success = fixup_intersection_order(sorted_edge_list, intersects);
        if (!success) {
            return false;
        }
    }
    process_intersect_list(intersects, cliptype, subject_fill_type, clip_fill_type, rings, joins,
                           active_edge_list);
    return true;
}
}
}
}
