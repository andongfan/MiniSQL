#include "Table.h"

#include <algorithm>

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

std::string Table::Serialize() const {
    std::string str;

    int name_len = name.size();
    std::copy_n(reinterpret_cast<char *>(&name_len), sizeof(int),
        std::back_inserter(str));
    str += name;

    int n_col = attrbs.size();
    std::copy_n(reinterpret_cast<char *>(&n_col), sizeof(int),
        std::back_inserter(str));
    std::copy_n(reinterpret_cast<const char *>(&n_tuples), sizeof(int),
        std::back_inserter(str));
    std::copy_n(reinterpret_cast<const char *>(&n_blocks), sizeof(int),
        std::back_inserter(str));

    for (const auto &attrb : attrbs) {
        str += attrb.Serialize();
    }

    return str;
}

Table Table::Parse(const char *&pstr) {
    Table table;

    int name_len = *reinterpret_cast<const int *>(pstr);
    pstr += sizeof(int);
    std::copy_n(pstr, name_len, std::back_inserter(table.name));
    pstr += name_len;

    int n_col = *reinterpret_cast<const int *>(pstr);
    pstr += sizeof(int);
    table.n_tuples = *reinterpret_cast<const int *>(pstr);
    pstr += sizeof(int);
    table.n_blocks = *reinterpret_cast<const int *>(pstr);
    pstr += sizeof(int);

    for (int i = 0; i < n_col; i++) {
        Attribute attrb = Attribute::Parse(pstr);
        table.attrbs.emplace_back(attrb);
    }

    return table;
}