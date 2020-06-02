#include "Attribute.h"

#include <algorithm>

int Attribute::Size() const
{
    if (type == AttrbType::INT)
    {
        return sizeof(int);
    }
    else if (type == AttrbType::FLOAT)
    {
        return sizeof(double);
    }
    else
    {
        return (char_len + 1) * sizeof(char);
    }
}

std::string Attribute::Serialize() const {
    std::string str;

    int name_len = name.size();
    std::copy_n(reinterpret_cast<char *>(&name_len), sizeof(int),
        std::back_inserter(str));
    str += name;

    int flag = int(type);
    if (is_unique) {
        flag |= 4;
    }
    if (is_primary) {
        flag |= 8;
    }
    std::copy_n(reinterpret_cast<char *>(&flag), sizeof(int),
        std::back_inserter(str));
    std::copy_n(reinterpret_cast<const char *>(&char_len), sizeof(int),
        std::back_inserter(str));

    int ind_len = index.size();
    std::copy_n(reinterpret_cast<char *>(&ind_len), sizeof(int),
        std::back_inserter(str));
    str += index;

    return str;
}

Attribute Attribute::Parse(const char *&pstr) {
    Attribute attrb;

    int name_len = *reinterpret_cast<const int *>(pstr);
    pstr += sizeof(int);
    std::copy_n(pstr, name_len, std::back_inserter(attrb.name));
    pstr += name_len;

    int flag = *reinterpret_cast<const int *>(pstr);
    attrb.type = AttrbType(flag & 3);
    attrb.is_unique = (flag & 4) != 0;
    attrb.is_primary = (flag & 8) != 0;
    pstr += sizeof(int);

    attrb.char_len = *reinterpret_cast<const int *>(pstr);
    pstr += sizeof(int);

    int ind_len = *reinterpret_cast<const int *>(pstr);
    pstr += sizeof(int);
    std::copy_n(pstr, ind_len, std::back_inserter(attrb.index));
    pstr += ind_len;

    return attrb;
}