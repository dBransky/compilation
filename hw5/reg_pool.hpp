#ifndef REGPOOL_HPP
#define REGPOOL_HPP

#include "bp.hpp"
#include <vector>
#include <string>
class RegPool
{
    unsigned int current;
    RegPool() : current(0){};
    string get_reg()
    {
        return "t" + to_string(current++);
    }
}
#endif