#pragma once

#include <mapbox/geometry/wagyu/point.hpp>

#ifdef DEBUG
#include <iostream>
#endif

namespace mapbox {
namespace geometry {
namespace wagyu {
template <typename T>
struct ring;

template <typename T>
using ring_ptr = ring<T>*;

template <typename T>
using const_ring_ptr = ring<T>* const;

template <typename T>
struct ring {
    std::size_t index;
    std::size_t ring_index; // To support unset 0 is undefined and indexes offset by 1
    bool is_hole;
    bool is_open;
    ring_ptr<T> first_left;
    point_ptr<T> points;
    point_ptr<T> bottom_point;

    ring()
        : index(0),
          ring_index(0),
          is_hole(false),
          is_open(false),
          first_left(nullptr),
          points(nullptr),
          bottom_point(nullptr) {
    }
};

template <typename T>
using ring_list = std::vector<ring_ptr<T>>;

template <typename T>
ring_ptr<T> create_new_ring(ring_list<T>& rings) {
    ring_ptr<T> result = new ring<T>();
    rings.push_back(result);
    result->index = rings.size() - 1;
    return result;
}

template <typename T>
ring_ptr<T> parse_first_left(ring_ptr<T> first_left) {
    while (first_left && !first_left->points) {
        first_left = first_left->first_left;
    }
    return first_left;
}

template <typename T>
void set_next(const_point_ptr<T>& node, const const_point_ptr<T>& next_node) {
    node->next = next_node;
}

template <typename T>
point_ptr<T> get_next(const_point_ptr<T>& node) {
    return node->next;
}

template <typename T>
point_ptr<T> get_prev(const_point_ptr<T>& node) {
    return node->prev;
}

template <typename T>
void set_prev(const_point_ptr<T>& node, const const_point_ptr<T>& prev_node) {
    node->prev = prev_node;
}

template <typename T>
void init(const_point_ptr<T>& node) {
    set_next(node, node);
    set_prev(node, node);
}

template <typename T>
std::size_t point_count(const const_point_ptr<T>& orig_node) {
    std::size_t size = 0;
    point_ptr<T> n = orig_node;
    do {
        n = get_next(n);
        ++size;
    } while (n != orig_node);
    return size;
}

template <typename T>
void link_before(point_ptr<T>& node, point_ptr<T>& new_node) {
    point_ptr<T> prev_node = get_prev(node);
    set_prev(new_node, prev_node);
    set_next(new_node, node);
    set_prev(node, new_node);
    set_next(prev_node, new_node);
}

template <typename T>
void link_after(point_ptr<T>& node, point_ptr<T>& new_node) {
    point_ptr<T> next_node = get_next(node);
    set_prev(new_node, node);
    set_next(new_node, next_node);
    set_next(node, new_node);
    set_prev(next_node, new_node);
}

template <typename T>
void transfer_point(point_ptr<T>& p, point_ptr<T>& b, point_ptr<T>& e) {
    if (b != e) {
        point_ptr<T> prev_p = get_prev(p);
        point_ptr<T> prev_b = get_prev(b);
        point_ptr<T> prev_e = get_prev(e);
        set_next(prev_e, p);
        set_prev(p, prev_e);
        set_next(prev_b, e);
        set_prev(e, prev_b);
        set_next(prev_p, b);
        set_prev(b, prev_p);
    } else {
        link_before(p, b);
    }
}

template <typename T>
void reverse_ring(point_ptr<T> pp) {
    if (!pp) {
        return;
    }
    point_ptr<T> pp1;
    point_ptr<T> pp2;
    pp1 = pp;
    do {
        pp2 = pp1->next;
        pp1->next = pp1->prev;
        pp1->prev = pp2;
        pp1 = pp2;
    } while (pp1 != pp);
}

template <typename T>
double area(point_ptr<T> op) {
    point_ptr<T> startOp = op;
    if (!op) {
        return 0.0;
    }
    double a = 0.0;
    do {
        a += static_cast<double>(op->prev->x + op->x) * static_cast<double>(op->prev->y - op->y);
        op = op->next;
    } while (op != startOp);
    return a * 0.5;
}

template <typename T>
double area(ring<T> const& polygon_ring) {
    return area(polygon_ring.points);
}

template <typename T>
void area_and_count(point_ptr<T> op, std::size_t& count, double& area) {
    point_ptr<T> startOp = op;
    count = 0;
    if (!op) {
        area = 0.0;
        return;
    }
    area = 0.0;
    do {
        ++count;
        area += static_cast<double>(op->prev->x + op->x) * static_cast<double>(op->prev->y - op->y);
        op = op->next;
    } while (op != startOp);
    area = area * 0.5;
    return;
}

#ifdef DEBUG

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const ring<T>& r) {
    out << "  index: " << r.index << std::endl;
    auto fl = parse_first_left(r.first_left);
    if (!fl) {
        out << "  parent_index: none" << std::endl;
    } else {
        out << "  parent_index: " << fl->index << std::endl;
    }
    if (r.is_hole) {
        out << "  is_hole: true" << std::endl;
    } else {
        out << "  is_hole: false" << std::endl;
    }
    auto first_point = r.points;
    auto pt_itr = r.points;
    if (first_point) {
        out << "  area: " << area(r.points) << std::endl;
        out << "  points:" << std::endl;
        do {
            out << "    x: " << pt_itr->x << " y: " << pt_itr->y << std::endl;
            pt_itr = pt_itr->next;
        } while (pt_itr != first_point);
    } else {
        out << "  area: NONE" << std::endl;
        out << "  points: NONE" << std::endl;
    }
    return out;
}

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const ring_list<T>& rings) {
    out << "START RING LIST" << std::endl;
    std::size_t c = 0;
    for (auto& r : rings) {
        out << " ring: " << c++ << std::endl;
        out << *r;
    }
    out << "END RING LIST" << std::endl;
    return out;
}

#endif
}
}
}
