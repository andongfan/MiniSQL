#pragma once

#include <string>

enum class AttrbType {
    INT,
    FLOAT,
    CHAR,
};

struct Attribute {
    std::string name;
    AttrbType type;
    int char_len = 0;
    bool is_unique = false;
    bool is_primary = false;
    std::string index = "";

    int Size() const;

    std::string Serialize() const;
    static Attribute Parse(const char *&pstr);
};