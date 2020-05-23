#include "Table.h"

bool Table::HasAttrb(const std::string &name) const {
    for (const auto &attrb : attrbs) {
        if (attrb.name == name) {
            return true;
        }
    }
    return false;
}

Attribute Table::GetAttrb(const std::string &name) const {
    for (const auto &attrb : attrbs) {
        if (attrb.name == name) {
            return attrb;
        }
    }
    return Attribute { };
}

int Table::Size() const {
    if (tuple_size > 0) {
        return tuple_size;
    } else {
        tuple_size = 0;
        for (const auto &attrb : attrbs) {
            tuple_size += attrb.Size();
        }
        return tuple_size;
    }
}