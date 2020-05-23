#include "SQLStmtError.h"

SQLStmtError::SQLStmtError(const std::string &stmt, const std::string &info) {
    this->stmt = stmt;
    this->info = info;
    output_str = "Error on statement '" + stmt +
        "'\nInformation: " + info + "\n";
}

const char *SQLStmtError::what() const noexcept {
    return output_str.c_str();
}