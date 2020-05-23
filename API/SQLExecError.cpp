#include "SQLExecError.h"

SQLExecError::SQLExecError(const std::string &info) {
    this->info = info;
}

const char *SQLExecError::what() const noexcept {
    return info.c_str();
}