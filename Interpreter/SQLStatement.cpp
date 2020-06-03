#include "SQLStatement.h"

template <typename T>
static bool IsCondTrue(const T &lhs, const T &rhs, CondType type) {
    if (type == CondType::EQUAL) {
        return lhs == rhs;
    } else if (type == CondType::NOT_EQUAL) {
        return lhs != rhs;
    } else if (type == CondType::LESS) {
        return lhs < rhs;
    } else if (type == CondType::LESS_EQUAL) {
        return lhs <= rhs;
    } else if (type == CondType::GREAT) {
        return lhs > rhs;
    } else {
        return lhs >= rhs;
    }
}

bool Condition::IsTrue(const Value &v) const {
    if (auto p = std::get_if<int>(&val)) {
        if (auto q = std::get_if<int>(&v)) {
            return IsCondTrue(*q, *p, type);
        } else {
            return IsCondTrue(std::get<double>(v), double(*p), type);
        }
    } else if (auto p = std::get_if<double>(&val)) {
        if (auto q = std::get_if<int>(&v)) {
            return IsCondTrue(double(*q), *p, type);
        } else {
            return IsCondTrue(std::get<double>(v), *p, type);
        }
    } else {
        auto lhs = std::get<std::string>(v);
        auto rhs = std::get<std::string>(val);
        return IsCondTrue(lhs, rhs, type);
    }
}