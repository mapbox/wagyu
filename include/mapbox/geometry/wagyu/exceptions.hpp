#pragma once

#include <stdexcept>

namespace mapbox { namespace geometry { namespace wagyu {

class clipper_exception : public std::exception
{
private:
    std::string m_descr;
public:
    clipper_exception(const char* description): m_descr(description) {}
    virtual ~clipper_exception() throw() {}
    virtual const char* what() const throw()
    {
        return m_descr.c_str();
    }
};

}}}
