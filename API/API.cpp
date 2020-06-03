#include "API.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>

#include "SQLExecError.h"
#include "Interpreter.h"

static std::string Val2Str(const Value &val, bool quote = false) {
    std::string str;
    if (auto pv = std::get_if<int>(&val)) {
        str = std::to_string(*pv);
    } else if (auto pv = std::get_if<double>(&val)) {
        str = std::to_string(*pv);
    } else if (auto pv = std::get_if<std::string>(&val)) {
        if (quote) {
            str = "'" + *pv + "'";
        } else {
            str = *pv;
        }
    }
    return str;
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
        } else if (cond.type == CondType::LESS_EQUAL) {
            std::cout << " <= ";
        } else if (cond.type == CondType::GREAT) {
            std::cout << " > ";
        } else {
            std::cout << " >= ";
        }

        std::cout << Val2Str(cond.val, true);
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

static void PrintTable(const Table &table,
        const std::vector<std::vector<Value>> &datas,
        const std::vector<std::string> &attrb_names) {
    int N = table.attrbs.size();
    if (!attrb_names.empty()) {
        N = attrb_names.size();
    }
    std::vector<int> ord(N);
    if (attrb_names.empty()) {
        for (int i = 0; i < N; i++) ord[i] = i;
    } else {
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < table.attrbs.size(); j++) {
                if (table.attrbs[j].name == attrb_names[i]) {
                    ord[i] = j;
                    break;
                }
            }
        }
    }
    std::vector<size_t> vec_len(N);
    for (int i = 0; i < N; i++) {
        vec_len[i] = table.attrbs[ord[i]].name.size();
    }
    for (const auto &row : datas) {
        for (int i = 0; i < N; i++) {
            vec_len[i] = std::max(vec_len[i], Val2Str(row[ord[i]]).size());
        }
    }
    std::string split_str = "+";
    for (auto &i : vec_len) {
        ++i;
        split_str += std::string(i, '-') + "+";
    }

    std::cout << "\n" << std::left << split_str << std::endl;
    for (int i = 0; i < N; i++) {
        std::cout << "|" << std::setw(vec_len[i]) << table.attrbs[ord[i]].name;
    }
    std::cout << "|\n" << split_str << std::endl;
    for (const auto &row : datas) {
        for (int i = 0; i < N; i++) {
            std::cout << "|" << std::setw(vec_len[i]) << Val2Str(row[ord[i]]);
        }
        std::cout << "|\n" << split_str << std::endl;
    }

    int n_row = datas.size();
    if (n_row > 1) {
        std::cout << n_row << " rows returned" << std::endl;
    } else {
        std::cout << n_row << " row returned" << std::endl;
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

static bool MergeConditions(const Table &table, std::vector<Condition> &conds) {
    std::sort(conds.begin(), conds.end(),
        [](const Condition &a, const Condition &b) {
            return a.attrb < b.attrb;
        });

    std::vector<Condition> new_conds;
    for (int i = 0, last = 0; i < conds.size(); i++) {
        if (i + 1 == conds.size() || conds[i].attrb != conds[i + 1].attrb) {
            auto attrb = conds[i].attrb;
            auto ty = table.GetAttrb(attrb).type;
            if (ty == AttrbType::INT) {
                int lower_bound = std::numeric_limits<int>::min();
                int upper_bound = std::numeric_limits<int>::max();
                int eq_val;
                bool has_eq_val = false, has_lb = false, has_ub = false;
                for (int j = last; j <= i; j++) {
                    auto cty = conds[j].type;
                    int tmp = std::get<int>(conds[j].val);
                    if (cty == CondType::EQUAL) {
                        if (has_eq_val && tmp != eq_val) {
                            return true;
                        }
                        has_eq_val = true;
                        eq_val = tmp;
                        lower_bound = std::max(lower_bound, tmp);
                        upper_bound = std::min(upper_bound, tmp);
                    } else if (cty == CondType::LESS) {
                        upper_bound = std::min(upper_bound, tmp - 1);
                        has_ub = true;
                    } else if (cty == CondType::LESS_EQUAL) {
                        upper_bound = std::min(upper_bound, tmp);
                        has_ub = true;
                    } else if (cty == CondType::GREAT) {
                        lower_bound = std::max(lower_bound, tmp + 1);
                        has_lb = true;
                    } else if (cty == CondType::LESS) {
                        lower_bound = std::max(lower_bound, tmp);
                        has_lb = true;
                    }
                }
                if (has_eq_val) {
                    for (int j = last; j <= i; j++) {
                        if (conds[j].type == CondType::NOT_EQUAL) {
                            int tmp = std::get<int>(conds[j].val);
                            if (tmp == eq_val) {
                                return true;
                            }
                        }
                    }
                }
                if (lower_bound > upper_bound) {
                    return true;
                }
                if (has_eq_val) {
                    new_conds.emplace_back(attrb, eq_val, CondType::EQUAL);
                } else {
                    if (has_lb) {
                        new_conds.emplace_back(attrb, lower_bound,
                            CondType::GREAT_EQUAL);
                    }
                    if (has_ub) {
                        new_conds.emplace_back(attrb, upper_bound,
                            CondType::LESS_EQUAL);
                    }
                }
                for (int j = last; j <= i; j++) {
                    if (conds[j].type == CondType::NOT_EQUAL) {
                        int tmp = std::get<int>(conds[j].val);
                        if (tmp >= lower_bound && tmp <= upper_bound) {
                            new_conds.emplace_back(attrb, tmp,
                                CondType::NOT_EQUAL);
                        }
                    }
                }
            } else if (ty == AttrbType::FLOAT) {
                double lb_e = std::numeric_limits<double>::min();
                double lb_ne = std::numeric_limits<double>::min();
                double ub_e = std::numeric_limits<double>::max();
                double ub_ne = std::numeric_limits<double>::max();
                double eq_val;
                bool has_eq_val = false, has_lb = false, has_ub = false;
                for (int j = last; j <= i; j++) {
                    auto cty = conds[j].type;
                    double tmp;
                    if (auto p = std::get_if<int>(&conds[j].val)) {
                        tmp = *p;
                    } else {
                        tmp = std::get<double>(conds[j].val);
                    }
                    if (cty == CondType::EQUAL) {
                        if (has_eq_val && tmp != eq_val) {
                            return true;
                        }
                        has_eq_val = true;
                        eq_val = tmp;
                        lb_e = std::max(lb_e, tmp);
                        ub_e = std::min(ub_e, tmp);
                    } else if (cty == CondType::LESS) {
                        ub_ne = std::min(ub_ne, tmp);
                        has_ub = true;
                    } else if (cty == CondType::LESS_EQUAL) {
                        ub_e = std::min(ub_e, tmp);
                        has_ub = true;
                    } else if (cty == CondType::GREAT) {
                        lb_ne = std::max(lb_ne, tmp);
                        has_lb = true;
                    } else if (cty == CondType::LESS) {
                        lb_e = std::max(lb_e, tmp);
                        has_lb = true;
                    }
                }
                if (has_eq_val) {
                    for (int j = last; j <= i; j++) {
                        if (conds[j].type == CondType::NOT_EQUAL) {
                            double tmp;
                            if (auto p = std::get_if<int>(&conds[j].val)) {
                                tmp = *p;
                            } else {
                                tmp = std::get<double>(conds[j].val);
                            }
                            if (tmp == eq_val) {
                                return true;
                            }
                        }
                    }
                }
                double lower_bound;
                double upper_bound;
                bool has_ne = false;
                if (ub_ne <= ub_e) {
                    if (has_ub) {
                        new_conds.emplace_back(attrb, ub_ne, CondType::LESS);
                    }
                    upper_bound = ub_ne;
                    has_ne = true;
                } else {
                    if (has_ub) {
                        new_conds.emplace_back(attrb, ub_e, CondType::LESS_EQUAL);
                    }
                    upper_bound = ub_e;
                }
                if (lb_ne >= lb_e) {
                    if (has_lb) {
                        new_conds.emplace_back(attrb, lb_ne, CondType::GREAT);
                    }
                    lower_bound = lb_ne;
                    has_ne = true;
                } else {
                    if (has_lb) {
                        new_conds.emplace_back(attrb, lb_e, CondType::GREAT_EQUAL);
                    }
                    lower_bound = lb_e;
                }
                if (lower_bound > upper_bound ||
                        (lower_bound == upper_bound && has_ne)) {
                    return true;
                }
                if (has_eq_val) {
                    new_conds.pop_back();
                    int tn = new_conds.size() - 1;
                    new_conds[tn] = Condition(attrb, eq_val, CondType::EQUAL);
                }
                for (int j = last; j <= i; j++) {
                    if (conds[j].type == CondType::NOT_EQUAL) {
                        double tmp;
                        if (auto p = std::get_if<int>(&conds[j].val)) {
                            tmp = *p;
                        } else {
                            tmp = std::get<double>(conds[j].val);
                        }
                        if (tmp >= lower_bound && tmp <= upper_bound) {
                            new_conds.emplace_back(attrb, tmp,
                                CondType::NOT_EQUAL);
                        }
                    }
                }
            } else {
                std::string lb_e = "\xff";
                std::string lb_ne = "\xff";
                std::string ub_e = "\0";
                std::string ub_ne = "\0";
                std::string eq_val;
                bool has_eq_val = false, has_lb = false, has_ub = false;
                for (int j = last; j <= i; j++) {
                    auto cty = conds[j].type;
                    auto tmp = std::get<std::string>(conds[j].val);
                    if (cty == CondType::EQUAL) {
                        if (has_eq_val && tmp != eq_val) {
                            return true;
                        }
                        has_eq_val = true;
                        eq_val = tmp;
                    } else if (cty == CondType::LESS) {
                        ub_ne = std::min(ub_ne, tmp);
                        has_ub = true;
                    } else if (cty == CondType::LESS_EQUAL) {
                        ub_e = std::min(ub_e, tmp);
                        has_ub = true;
                    } else if (cty == CondType::GREAT) {
                        lb_ne = std::max(lb_ne, tmp);
                        has_lb = true;
                    } else if (cty == CondType::LESS) {
                        lb_e = std::max(lb_e, tmp);
                        has_lb = true;
                    }
                }
                if (has_eq_val) {
                    for (int j = last; j <= i; j++) {
                        if (conds[j].type == CondType::NOT_EQUAL) {
                            auto tmp = std::get<std::string>(conds[j].val);
                            if (tmp == eq_val) {
                                return true;
                            }
                        }
                    }
                }
                std::string lower_bound;
                std::string upper_bound;
                bool has_ne = false;
                if (ub_ne <= ub_e) {
                    if (has_ub) {
                        new_conds.emplace_back(attrb, ub_ne, CondType::LESS);
                    }
                    upper_bound = ub_ne;
                    has_ne = true;
                } else {
                    if (has_ub) {
                        new_conds.emplace_back(attrb, ub_e, CondType::LESS_EQUAL);
                    }
                    upper_bound = ub_e;
                }
                if (lb_ne >= lb_e) {
                    if (has_lb) {
                        new_conds.emplace_back(attrb, lb_ne, CondType::GREAT);
                    }
                    lower_bound = lb_ne;
                    has_ne = true;
                } else {
                    if (has_lb) {
                        new_conds.emplace_back(attrb, lb_e, CondType::GREAT_EQUAL);
                    }
                    lower_bound = lb_e;
                }
                if (lower_bound > upper_bound ||
                        (lower_bound == upper_bound && has_ne)) {
                    return true;
                }
                if (has_eq_val) {
                    new_conds.pop_back();
                    int tn = new_conds.size() - 1;
                    new_conds[tn] = Condition(attrb, eq_val, CondType::EQUAL);
                }
                for (int j = last; j <= i; j++) {
                    if (conds[j].type == CondType::NOT_EQUAL) {
                        auto tmp = std::get<std::string>(conds[j].val);
                        if (tmp >= lower_bound && tmp <= upper_bound) {
                            new_conds.emplace_back(attrb, tmp,
                                CondType::NOT_EQUAL);
                        }
                    }
                }
            }
            last = i + 1;
        }
    }
    conds.swap(new_conds);
    return false;
}

MiniSQL::MiniSQL() {
    RM::bm = new BufferManager;
    cat_mgr.Load();
    // TODO
}
MiniSQL::~MiniSQL() {
    cat_mgr.Save();
    delete RM::bm;
    // TODO
}

void MiniSQL::CreateTable(const CreateTableStmt &stmt) {
    if (cat_mgr.CheckName(stmt.name)) {
        throw SQLExecError("duplicated table name '" + stmt.name + "'");
    }
    cat_mgr.NewTable(stmt.name, stmt.attrbs);

    auto &table = cat_mgr.GetTable(stmt.name);
    RM::CreateTable(table);
}

void MiniSQL::CreateIndex(const CreateIndexStmt &stmt) {
    // TODO
    if (cat_mgr.CheckName(stmt.name)) {
        throw SQLExecError("duplicated index name '" + stmt.name + "'");
    }
    if (!cat_mgr.CheckTable(stmt.table_name)) {
        throw SQLExecError("no table named '" + stmt.table_name + "'");
    }
    auto &table = cat_mgr.GetTable(stmt.table_name);
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

void MiniSQL::DropTable(const DropTableStmt &stmt) {
    if (!cat_mgr.CheckTable(stmt.name)) {
        throw SQLExecError("no table named '" + stmt.name + "'");
    }

    auto &table = cat_mgr.GetTable(stmt.name);
    RM::DropTable(table);

    cat_mgr.DropTable(stmt.name);
}

void MiniSQL::DropIndex(const DropIndexStmt &stmt) {
    // TODO
    if (!cat_mgr.CheckIndex(stmt.name)) {
        throw SQLExecError("no index named '" + stmt.name + "'");
    }
    cat_mgr.DropIndex(stmt.name);
}

void MiniSQL::Insert(const InsertStmt &stmt) {
    if (!cat_mgr.CheckTable(stmt.name)) {
        throw SQLExecError("no table named '" + stmt.name + "'");
    }
    auto &table = cat_mgr.GetTable(stmt.name);
    if (stmt.vals.size() != table.attrbs.size()) {
        throw SQLExecError("table '" + stmt.name + "' has " +
            std::to_string(table.attrbs.size()) + " attributes, but " +
            std::to_string(stmt.vals.size()) + " values are given");
    }
    int N = stmt.vals.size();
    std::vector<Value> vals;
    for (int i = 0; i < N; i++) {
        auto attrb = table.attrbs[i];
        auto val = stmt.vals[i];
        if (!CheckType(attrb, val)) {
            throw SQLExecError("value type of the " + OrderStr(i + 1) +
                " attribute doesn't match");
        }
        if (attrb.type == AttrbType::FLOAT) {
            if (auto p = std::get_if<int>(&val)) {
                vals.emplace_back(double(*p));
            } else {
                vals.emplace_back(std::get<double>(val));
            }
        } else {
            vals.emplace_back(val);
        }
    }

    RM::InsertRecord(table, vals);
}

void MiniSQL::Delete(const DeleteStmt &stmt) {
    if (!cat_mgr.CheckTable(stmt.table_name)) {
        throw SQLExecError("no table named '" + stmt.table_name + "'");
    }
    auto &table = cat_mgr.GetTable(stmt.table_name);
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

    auto conds = stmt.conds;
    if (MergeConditions(table, conds)) {
        return;
    }

    RM::DeleteRecord(table, conds);
}

void MiniSQL::Select(const SelectStmt &stmt) {
    if (!cat_mgr.CheckTable(stmt.table_name)) {
        throw SQLExecError("no table named '" + stmt.table_name + "'");
    }
    auto &table = cat_mgr.GetTable(stmt.table_name);
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
    if (!stmt.all) {
        for (const auto &attrb : stmt.attrb_names) {
            if (!table.HasAttrb(attrb)) {
                throw SQLExecError("table '" + stmt.table_name +
                    "' doesn't have an attribute named '" + attrb + "'");
            }
        }
    }

    auto conds = stmt.conds;
    if (MergeConditions(table, conds)) {
        PrintTable(table, {}, {});
        return;
    }
    // PrintConds(conds);

    const auto &datas = RM::SelectRecord(table, conds);
    if (stmt.all) {
        PrintTable(table, datas, {});
    } else {
        PrintTable(table, datas, stmt.attrb_names);
    }
}

void MiniSQL::Execfile(const ExecfileStmt &stmt) {
    std::ifstream fin(stmt.file_name);
    if (!fin) {
        throw SQLExecError("can't open '" + stmt.file_name + "'");
    }
    Interpreter interpreter;
    auto stmts = interpreter.ParseFile(stmt.file_name);

    for (const auto &stmt : stmts) {
        Execute(stmt);
    }
}

void MiniSQL::Quit(const QuitStmt &stmt) {
    should_quit = true;
}

void MiniSQL::Print(const SQLStatement &stmt) {
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
            std::cout << Val2Str(val, true) << "  ";
        }
        std::cout << ")" << std::endl;
    } else if (auto p = std::get_if<DeleteStmt>(&stmt)) {
        std::cout << "delete from " << p->table_name << std::endl;
        PrintConds(p->conds);
    } else if (auto p = std::get_if<SelectStmt>(&stmt)) {
        std::string attrbs = "* ";
        if (!p->all) {
            for (const auto &str : p->attrb_names) {
                attrbs += str + " ";
            }
        }
        std::cout << "select " << attrbs << "from " << p->table_name << std::endl;
        PrintConds(p->conds);
    } else if (auto p = std::get_if<QuitStmt>(&stmt)) {
        std::cout << "quit" << std::endl;
    } else if (auto p = std::get_if<ExecfileStmt>(&stmt)) {
        std::cout << "execfile " << p->file_name << std::endl;
    }
}

std::chrono::duration<double> MiniSQL::Execute(const SQLStatement &stmt) {
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
    } else if (auto p = std::get_if<QuitStmt>(&stmt)) {
        Quit(*p);
    }
    auto end = std::chrono::high_resolution_clock::now();
    return end - begin;
}

static std::string GetStmtsString() {
    std::string str;
    std::cout << "minisql> ";
    while (true) {
        std::string line;
        std::getline(std::cin, line);
        str += line;
        while (!line.empty() && isspace(line.back())) line.pop_back();
        if (line.back() == ';') {
            return str;
        }
        std::cout << "       > ";
    }
}

void MiniSQL::MainLoop() {
    while (!should_quit) {
        try {
            std::string str = GetStmtsString();
            auto stmts = inter.Parse(str);
            int i = 0;
            for (const auto &stmt : stmts) {
                // Print(stmt);
                ++i;
                std::cout << "statement " << i << ": ";
                auto time = Execute(stmt);
                std::cout << "executed successfully, time used: " <<
                    time.count() << "s" << std::endl;
            }
        } catch (const SQLExecError &e) {
            std::cout << e.what() << std::endl;
        } catch (const RM::RecordError &e) {
            std::cout << e.what() << std::endl;
        }
    }
}