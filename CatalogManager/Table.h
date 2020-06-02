#pragma once

#include <vector>

#include "Attribute.h"

struct Table
{
    std::string name;
    std::vector<Attribute> attrbs;
    int n_tuples;
    int n_blocks;
    mutable int tuple_size = -1;

    bool HasAttrb(const std::string &name) const;
    Attribute GetAttrb(const std::string &name) const;
    int Size() const;

    std::string Serialize() const;
    static Table Parse(const char *&pstr);
};