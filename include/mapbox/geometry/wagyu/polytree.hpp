#pragma once

#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/wagyu/config.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
class polygon_node;

template <typename T>
using polygon_node_ptr = polygon_node<T> *;

template <typename T>
using const_polygon_node_ptr = polygon_node<T> * const;

template <typename T>
using polygon_node_list = std::vector<polygon_node_ptr<T> >;

template <typename T>
class polygon_node
{
    using value_type = T;
protected:
    mapbox::geometry::linear_ring<value_type> Contour;
    polygon_node_list<value_type>             Childs;
    std::size_t                               Index; //node index in Parent.Childs
    polygon_node_ptr<value_type>              Parent;
    join_type                                 m_jointype;
    end_type                                  m_endtype;
    bool                                      m_IsOpen;

public:
    polygon_node() :
        Contour(),
        Childs(),
        Index(0),
        Parent(nullptr),
        m_jointype(join_type_square),
        m_endtype(end_type_closed_polygon),
        m_IsOpen(false) {}
    
    ~polygon_node() {}
    
    std::size_t index() const
    {
        return Index;
    }

    void index(std::size_t value)
    {
        Index = value;
    }

    polygon_node_ptr<value_type> GetNext() const
    { 
        if (!Childs.empty())
        {
            return Childs[0];
        }
        else
        {
            return GetNextSiblingUp();
        }
    }  

    bool IsHole() const
    { 
        bool result = true;
        polygon_node_ptr<value_type> node = Parent;
        while (node)
        {
            result = !result;
            node = node->Parent;
        }
        return result;
    }  

    bool IsOpen() const
    {
        return m_IsOpen;
    }

    std::size_t ChildCount() const
    {
        return Childs.size();
    }

protected:
    polygon_node_ptr<value_type> GetNextSiblingUp() const
    {
        if (!Parent) //protects against PolyTree.GetNextSiblingUp()
        {
            return nullptr;
        }
        else if (Index == Parent->Childs.size() - 1)
        {
            return Parent->GetNextSiblingUp();
        }
        else
        {
            return Parent->Childs[Index + 1];
        }

    }
    
    void AddChild(polygon_node<value_type> & child)
    {
        std::size_t cnt = Childs.size();
        Childs.push_back(&child);
        child.Parent = this;
        child.Index = cnt;
    }
};

template <typename T>
class polygon_tree;

template <typename T>
using polygon_tree_ptr = polygon_tree<T> *;

template <typename T>
using const_polygon_tree_ptr = polygon_tree<T> * const;

template <typename T>
class polygon_tree : public polygon_node<T>
{
    using value_type = T;
    using polygon_node<value_type>::Childs;

private:
    polygon_node_list<value_type> AllNodes;

public:
    ~polygon_tree()
    {
        clear();
    }

    polygon_node_ptr<value_type> GetFirst() const
    {
        if (!Childs.empty())
        {
            return Childs[0];
        }
        return nullptr;
    }
    
    void clear()
    {
        for (std::size_t i = 0; i < AllNodes.size(); ++i)
        {
            delete AllNodes[i];
        }
        AllNodes.resize(0); 
        Childs.resize(0);
    }
    
    std::size_t size() const
    {
        std::size_t result = AllNodes.size();
        //with negative offsets, ignore the hidden outer polygon ...
        if (result > 0 && Childs[0] != AllNodes[0])
        {
            result--;
        }
        return result;
    }
};



}}}
