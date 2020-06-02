#include "API.h"

#include <iostream>
#include <fstream>

#include "CatalogManager.h"
#include "SQLExecError.h"
#include "Interpreter.h"

static void PrintVal(const Value &val) {
    if (auto pv = std::get_if<int>(&val)) {
        std::cout << *pv;
    } else if (auto pv = std::get_if<double>(&val)) {
        std::cout << *pv;
    } else if (auto pv = std::get_if<std::string>(&val)) {
        std::cout << "'" << *pv << "'";
    }
}

static void PrintConds(const std::vector<Condition> &conds) {
    bool fir = true;
    for (const auto &cond : conds) {
        if (fir) {
            std::cout << " where ";
            fir = false;
        } else {
            std::cout << " and ";
        }

        std::cout << cond.attrb;

        if (cond.type == CondType::EQUAL) {
            std::cout << " = ";
        } else if (cond.type == CondType::NOT_EQUAL) {
            std::cout << " <> ";
        } else if (cond.type == CondType::LESS) {
            std::cout << " < ";
        } else if (cond.type == CondType::LESSS_EQUAL) {
            std::cout << " <= ";
        } else if (cond.type == CondType::GREAT) {
            std::cout << " > ";
        } else {
            std::cout << " >= ";
        }

        PrintVal(cond.val);
    }
    if (!conds.empty()) {
        std::cout << std::endl;
    }
}

static std::string OrderStr(int i) {
    int mod = i % 10;
    if (mod == 1 && i != 11) {
        return std::to_string(i) + "st";
    } else if (mod == 2 && i != 12) {
        return std::to_string(i) + "nd";
    } else if (mod == 3 && i != 13) {
        return std::to_string(i) + "rd";
    } else {
        return std::to_string(i) + "th";
    }
}

static bool CheckType(const Attribute &attrb, const Value &val) {
    if (attrb.type == AttrbType::INT) {
        return std::get_if<int>(&val) != nullptr;
    } else if (attrb.type == AttrbType::FLOAT) {
        return std::get_if<int>(&val) != nullptr ||
               std::get_if<double>(&val) != nullptr;
    } else {
        return std::get_if<std::string>(&val) != nullptr;
    }
}

static void CreateTable(const CreateTableStmt &stmt) {
    // TODO
    if (cat_mgr.CheckName(stmt.name)) {
        throw SQLExecError("duplicated table name '" + stmt.name + "'");
    }
    cat_mgr.NewTable(stmt.name, stmt.attrbs);
}

static void CreateIndex(const CreateIndexStmt &stmt) {
    // TODO
    if (cat_mgr.CheckName(stmt.name)) {
        throw SQLExecError("duplicated index name '" + stmt.name + "'");
    }
    if (!cat_mgr.CheckTable(stmt.table_name)) {
        throw SQLExecError("no table named '" + stmt.table_name + "'");
    }
    auto table = cat_mgr.GetTable(stmt.table_name);
    if (!table.HasAttrb(stmt.attrb_name)) {
        throw SQLExecError("table '" + stmt.table_name +
            "' doesn't have an attribute named '" + stmt.attrb_name + "'");
    }
    auto attrb = table.GetAttrb(stmt.attrb_name);
    if (!attrb.is_unique) {
        throw SQLExecError("'" + attrb.name + "' is not an unique attribue");
    }
    if (attrb.index != "") {
        throw SQLExecError("an index named '" + attrb.index +
            "' has already been built on attribute '" + attrb.name + "'");
    }
    cat_mgr.NewIndex(stmt.name, stmt.table_name, stmt.attrb_name);
}

static void DropTable(const DropTableStmt &stmt) {
    // TODO
    if (!cat_mgr.CheckTable(stmt.name)) {
        throw SQLExecError("no table named '" + stmt.name + "'");
    }
    cat_mgr.DropTable(stmt.name);
}

static void DropIndex(const DropIndexStmt &stmt) {
    // TODO
    if (!cat_mgr.CheckIndex(stmt.name)) {
        throw SQLExecError("no index named '" + stmt.name + "'");
    }
    cat_mgr.DropIndex(stmt.name);
}

static void Insert(const InsertStmt &stmt) {
    // TODO
    if (!cat_mgr.CheckTable(stmt.name)) {
        throw SQLExecError("no table named '" + stmt.name + "'");
    }
    auto table = cat_mgr.GetTable(stmt.name);
    if (stmt.vals.size() != table.attrbs.size()) {
        throw SQLExecError("table '" + stmt.name + "' has " +
            std::to_string(table.attrbs.size()) + " attributes, but " +
            std::to_string(stmt.vals.size()) + " values are given");
    }
    int N = stmt.vals.size();
    for (int i = 0; i < N; i++) {
        auto attrb = table.attrbs[i];
        auto val = stmt.vals[i];
        if (!CheckType(attrb, val)) {
            throw SQLExecError("value type of the " + OrderStr(i + 1) +
                " attribute doesn't match");
        }
    }
}

static void Delete(const DeleteStmt &stmt) {
    // TODO
    if (!cat_mgr.CheckTable(stmt.table_name)) {
        throw SQLExecError("no table named '" + stmt.table_name + "'");
    }
    auto table = cat_mgr.GetTable(stmt.table_name);
    int i = 0;
    for (const auto &cond : stmt.conds) {
        if (!table.HasAttrb(cond.attrb)) {
            throw SQLExecError("table '" + stmt.table_name +
                "' doesn't have an attribute named '" + cond.attrb + "'");
        }
        auto attrb = table.GetAttrb(cond.attrb);
        auto val = cond.val;
        if (!CheckType(attrb, val)) {
            throw SQLExecError("value type of the " + OrderStr(++i) +
                " constrain doesn't match");
        }
    }
}

static void Select(const SelectStmt &stmt) {
    // TODO
    if (!cat_mgr.CheckTable(stmt.table_name)) {
        throw SQLExecError("no table named '" + stmt.table_name + "'");
    }
    auto table = cat_mgr.GetTable(stmt.table_name);
    int i = 0;
    for (const auto &cond : stmt.conds) {
        if (!table.HasAttrb(cond.attrb)) {
            throw SQLExecError("table '" + stmt.table_name +
                "' doesn't have an attribute named '" + cond.attrb + "'");
        }
        auto attrb = table.GetAttrb(cond.attrb);
        auto val = cond.val;
        if (!CheckType(attrb, val)) {
            throw SQLExecError("value type of the " + OrderStr(++i) +
                " constrain doesn't match");
        }
    }
}

static void Execfile(const ExecfileStmt &stmt) {
    // TODO
    std::ifstream fin(stmt.file_name);
    if (!fin) {
        throw SQLExecError("can't open '" + stmt.file_name + "'");
    }
    Interpreter interpreter;
    auto stmts = interpreter.ParseFile(stmt.file_name);

    for (const auto &stmt : stmts) {
        minisql::Execute(stmt);
    }
}

namespace minisql {

void Initialize() {
    cat_mgr.Load();
    // TODO
}

void Finalize() {
    cat_mgr.Save();
    // TODO
}

void Print(const SQLStatement &stmt) {
    if (auto p = std::get_if<CreateTableStmt>(&stmt)) {
        std::cout << "create table " << p->name << " (" << std::endl;
        for (const auto &attrb : p->attrbs) {
            std::cout << "  " << attrb.name << " ";
            if (attrb.type == AttrbType::INT) {
                std::cout << "int";
            } else if (attrb.type == AttrbType::FLOAT) {
                std::cout << "float";
            } else {
                std::cout << "char(" << attrb.char_len << ")";
            }
            if (attrb.is_primary) {
                std::cout << " primary key";
            } else if (attrb.is_unique) {
                std::cout << " unique";
            }
            std::cout << std::endl;
        }
        std::cout << ")" << std::endl;
    } else if (auto p = std::get_if<CreateIndexStmt>(&stmt)) {
        std::cout << "create index " << p->name << " on " << p->table_name <<
            "(" << p->attrb_name << ")" << std::endl;
    } else if (auto p = std::get_if<DropTableStmt>(&stmt)) {
        std::cout << "drop table " << p->name << std::endl;
    } else if (auto p = std::get_if<DropIndexStmt>(&stmt)) {
        std::cout << "drop index " << p->name << std::endl;
    } else if (auto p = std::get_if<InsertStmt>(&stmt)) {
        std::cout << "insert into " << p->name << " values (";
        for (const auto &val : p->vals) {
            PrintVal(val);
            std::cout << "  ";
        }
        std::cout << ")" << std::endl;
    } else if (auto p = std::get_if<DeleteStmt>(&stmt)) {
        std::cout << "delete from " << p->table_name << std::endl;
        PrintConds(p->conds);
    } else if (auto p = std::get_if<SelectStmt>(&stmt)) {
        std::cout << "select * from " << p->table_name << std::endl;
        PrintConds(p->conds);
    } else if (auto p = std::get_if<QuitStmt>(&stmt)) {
        std::cout << "quit" << std::endl;
    } else if (auto p = std::get_if<ExecfileStmt>(&stmt)) {
        std::cout << "execfile " << p->file_name << std::endl;
    }
}

std::chrono::duration<double> Execute(const SQLStatement &stmt) {
    auto begin = std::chrono::high_resolution_clock::now();
    if (auto p = std::get_if<CreateTableStmt>(&stmt)) {
        CreateTable(*p);
    } else if (auto p = std::get_if<CreateIndexStmt>(&stmt)) {
        CreateIndex(*p);
    } else if (auto p = std::get_if<DropTableStmt>(&stmt)) {
        DropTable(*p);
    } else if (auto p = std::get_if<DropIndexStmt>(&stmt)) {
        DropIndex(*p);
    } else if (auto p = std::get_if<InsertStmt>(&stmt)) {
        Insert(*p);
    } else if (auto p = std::get_if<DeleteStmt>(&stmt)) {
        Delete(*p);
    } else if (auto p = std::get_if<SelectStmt>(&stmt)) {
        Select(*p);
    } else if (auto p = std::get_if<ExecfileStmt>(&stmt)) {
        Execfile(*p);
    }
    auto end = std::chrono::high_resolution_clock::now();
    return end - begin;
}

}