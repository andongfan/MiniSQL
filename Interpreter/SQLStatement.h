#pragma once

#include <string>
#include <variant>
#include <vector>

#include "Attribute.h"

using Value = std::variant<int, double, std::string>;

enum class CondType {
    EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_EQUAL,
    GREAT,
    GREAT_EQUAL
};

struct Condition {
    Condition(const std::string &attrb, const Value &val, CondType type) :
        attrb(attrb), val(val), type(type) {}

    bool IsTrue(const Value &v) const;

    std::string attrb;
    Value val;
    CondType type;
};

struct CreateTableStmt {
    std::string name;
    std::vector<Attribute> attrbs;
};

struct CreateIndexStmt {
    std::string name;
    std::string table_name;
    std::string attrb_name;
};

struct DropTableStmt {
    std::string name;
};

struct DropIndexStmt {
    std::string name;
};

struct InsertStmt {
    std::string name;
    std::vector<Value> vals;
};

struct DeleteStmt {
    std::string table_name;
    std::vector<Condition> conds;
};

struct SelectStmt {
    std::string table_name;
    bool all = false;
    std::vector<std::string> attrb_names;
    std::vector<Condition> conds;
};

struct QuitStmt {};

struct ExecfileStmt {
    std::string file_name;
};

using SQLStatement = std::variant<CreateTableStmt, CreateIndexStmt,
    DropTableStmt, DropIndexStmt, InsertStmt, SelectStmt, DeleteStmt,
    QuitStmt, ExecfileStmt>;