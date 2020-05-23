#include "Attribute.h"

int Attribute::Size() const {
    if (type == AttrbType::INT) {
        return sizeof(int);
    } else if (type == AttrbType::FLOAT) {
        return sizeof(double);
    } else {
        return char_len * sizeof(char);
    }
}