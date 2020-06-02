#include "Interpreter.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unordered_set>

static char EscapeChar(char ch) {
    if (ch == '0') return '\0';
    if (ch == '\'') return '\'';
    if (ch == '"') return '"';
    if (ch == 'b') return '\b';
    if (ch == 'n') return '\n';
    if (ch == 'r') return '\r';
    if (ch == 't') return '\t';
    if (ch == '\\') return '\\';
    return ch;
}

static std::string GetString(const std::string &str, int &begin,
        bool fir_num = false) {
    int cnt = 0;
    bool fir = true;
    for (; begin + cnt < str.size(); cnt++) {
        char ch = str[begin + cnt];
        if (fir && !fir_num) {
            if (!isalpha(ch) && ch != '_') {
                break;
            }
            fir = false;
        } else {
            if (!isalnum(ch) && ch != '_') {
                break;
            }
        }
    }
    int temp = begin;
    begin += cnt;
    return str.substr(temp, cnt);
}
static Value GetValue(const std::string &str, int &begin) {
    if (begin >= str.size()) {
        throw SQLStmtError(str, "expected a string/integer/float");
    }
    if (str[begin] == '\'' || str[begin] == '"') {
        char delim = str[begin];
        std::string str_v;
        bool slash = false;
        for (begin++; begin < str.size(); begin++) {
            if (slash) {
                str_v += EscapeChar(str[begin]);
                slash = false;
            } else if (str[begin] == '\\') {
                slash = true;
            } else if (str[begin] == '\'' && delim == '\'') {
                ++begin;
                return str_v;
            } else if (str[begin] == '"' && delim == '"') {
                ++begin;
                return str_v;
            } else {
                str_v += str[begin];
            }
        }
        throw SQLStmtError(str, "missing another quotation mark");
    } else if (isdigit(str[begin])) {
        int cnt = 0;
        bool has_point = false;
        for (int i = begin; i < str.size(); i++) {
            if (isdigit(str[i])) {
                ++cnt;
            } else if (str[i] == '.' && !has_point) {
                ++cnt;
                has_point = true;
            } else {
                break;
            }
        }
        auto num_str = str.substr(begin, cnt);
        begin += cnt;
        if (has_point) {
            double float_v = std::atof(num_str.c_str());
            return float_v;
        } else {
            int int_v = std::atoi(num_str.c_str());
            return int_v;
        }
    }
    throw SQLStmtError(str, "expected a string/integer/float");
}
static CondType GetCondType(const std::string &str, int &begin) {
    if (begin >= str.size()) {
        throw SQLStmtError(str, "expected comparison operator");
    }

    if (str[begin] == '=') {
        ++begin;
        return CondType::EQUAL;
    } else if (str[begin] == '<') {
        ++begin;
        if (begin < str.size() && str[begin] == '=') {
            ++begin;
            return CondType::LESS_EQUAL;
        } else if (begin < str.size() && str[begin] == '>') {
            ++begin;
            return CondType::NOT_EQUAL;
        }
        return CondType::LESS;
    } else if (str[begin] == '>') {
        ++begin;
        if (begin < str.size() && str[begin] == '=') {
            ++begin;
            return CondType::GREAT_EQUAL;
        }
        return CondType::GREAT;
    }
    throw SQLStmtError(str, "expected comparison operator");
}

static void Forward(const std::string &str, int &i) {
    while (isspace(str[i]) && i < str.size()) ++i;
}

static bool IsNumberStr(const std::string &str) {
    for (auto ch : str) {
        if (!isdigit(ch)) {
            return false;
        }
    }
    return true;
}
static bool IsIdentifierStr(const std::string &str) {
    return str.size() > 0 && !isdigit(str[0]);
}

static std::string ExpFndStr(const std::string &exp, const std::string &fnd) {
    return "expected '" + exp + "', found '" + fnd + "'";
}
static std::string ExpFndStr(const std::string &exp, char fnd) {
    return "expected '" + exp + "', found '" + fnd + "'";
}

static std::string
NoAttrbStr(const std::string &attrb_name, const std::string &table_name) {
    return "no attribute named '" + attrb_name + "' in table '" +
        table_name + "'";
}

SQLStatement Interpreter::ParseStmt(const std::string &stmt) const {
    for (int i = 0; i < stmt.size(); i++) {
        if (!isspace(stmt[i])) {
            auto str = GetString(stmt, i);
            if (str == "create") {
                Forward(stmt, i);
                auto str2 = GetString(stmt, i);
                if (str2 == "table") {
                    return ParseCreateTable(stmt, i);
                } else if (str2 == "index") {
                    return ParseCreateIndex(stmt, i);
                } else {
                    throw SQLStmtError(stmt,
                        ExpFndStr("table' or 'index", str2));
                }
            } else if (str == "drop") {
                Forward(stmt, i);
                auto str2 = GetString(stmt, i);
                if (str2 == "table") {
                    return ParseDropTable(stmt, i);
                } else if (str2 == "index") {
                    return ParseDropIndex(stmt, i);
                } else {
                    throw SQLStmtError(stmt,
                        ExpFndStr("table' or 'index", str2));
                }
            } else if (str == "insert") {
                return ParseInsert(stmt, i);
            } else if (str == "delete") {
                return ParseDelete(stmt, i);
            } else if (str == "select") {
                return ParseSelect(stmt, i);
            } else if (str == "quit" || str == "exit") {
                return ParseQuit(stmt, i);
            } else if (str == "execfile") {
                return ParseExecfile(stmt, i);
            } else {
                throw SQLStmtError(stmt, "unknown statement");
            }
        }
    }
    throw SQLStmtError(stmt, "unknown statement");
}

std::vector<SQLStatement>
Interpreter::ParseFile(const std::string &file_name) const {
    std::ifstream fin(file_name);
    if (!fin) {
        return {};
    }

    std::string line, str;
    while (std::getline(fin, line)) {
        str += line;
    }

    return Parse(str);
}

std::vector<SQLStatement> Interpreter::Parse(const std::string &str) const {
    std::vector<SQLStatement> stmts;

    int last = 0;
    bool has_error = false;
    for (int i = 0; i < str.size(); i++) {
        if (str[i] == ';') {
            try {
                int begin = last;
                last = i + 1;
                auto stmt = ParseStmt(str.substr(begin, last - begin));
                stmts.push_back(stmt);
            } catch(const SQLStmtError &e) {
                std::cout << e.what() << std::endl;
                has_error = true;
            }
        }
    }
    if (last + 1 < str.size()) {
        try {
            auto stmt = ParseStmt(str.substr(last, str.size() - last));
            stmts.push_back(stmt);
        } catch (const SQLStmtError &e) {
            std::cout << e.what() << std::endl;
            has_error = true;
        }
    }

    if (has_error) {
        stmts.clear();
    }
    return stmts;
}

CreateTableStmt
Interpreter::ParseCreateTable(const std::string &stmt, int begin) const {
    CreateTableStmt sql_stmt;

    Forward(stmt, begin);
    auto table_name = GetString(stmt, begin);
    if (table_name.size() == 0) {
        throw SQLStmtError(stmt, "expected table name");
    }
    if (!IsIdentifierStr(table_name)) {
        throw SQLStmtError(stmt, "invalid table name '" + table_name + "'");
    }
    sql_stmt.name = table_name;

    Forward(stmt, begin);
    if (begin >= stmt.size()) {
        throw SQLStmtError(stmt, "expected '('");
    } if (stmt[begin] != '(') {
        throw SQLStmtError(stmt, ExpFndStr("(", stmt[begin]));
    }
    ++begin;

    bool has_pk = false, suffix_pk = false;
    while (true) {
        Attribute attrb;

        Forward(stmt, begin);
        auto attrb_name = GetString(stmt, begin);
        if (attrb_name.size() == 0) {
            throw SQLStmtError(stmt,
                "expected attribute name or 'primary key'");
        }
        if (!IsIdentifierStr(attrb_name)) {
            throw SQLStmtError(stmt,
                "invalid attribute name '" + attrb_name + "'");
        }
        if (has_pk && !suffix_pk) {
            throw SQLStmtError(stmt,
                "no attribute defination is allowed after 'primary key (xxx)'");
        }
        attrb.name = attrb_name;

        Forward(stmt, begin);
        auto type_str = GetString(stmt, begin);
        bool is_pk = false;
        if (type_str == "int") {
            attrb.type = AttrbType::INT;
        } else if (type_str == "float") {
            attrb.type = AttrbType::FLOAT;
        } else if (type_str == "char") {
            Forward(stmt, begin);
            if (begin >= stmt.size()) {
                throw SQLStmtError(stmt, "expected '('");
            } if (stmt[begin] != '(') {
                throw SQLStmtError(stmt, ExpFndStr("(", stmt[begin]));
            }
            ++begin;

            Forward(stmt, begin);
            auto num_str = GetString(stmt, begin, true);
            if (!IsNumberStr(num_str)) {
                std::string info("expected integer 1~255, found '");
                info += num_str;
                info += "'";
                throw SQLStmtError(stmt, info);
            }

            Forward(stmt, begin);
            if (begin >= stmt.size()) {
                throw SQLStmtError(stmt, "expected ')'");
            } if (stmt[begin] != ')') {
                throw SQLStmtError(stmt, ExpFndStr(")", stmt[begin]));
            }
            ++begin;

            attrb.type = AttrbType::CHAR;
            attrb.char_len = std::atoi(num_str.c_str());
            if (attrb.char_len < 1 || attrb.char_len > 255) {
                std::string info("expected integer 1~255, found '");
                info += num_str;
                info += "'";
                throw SQLStmtError(stmt, info);
            }
        } else if (attrb_name == "primary" && type_str == "key") {
            if (has_pk) {
                throw SQLStmtError(stmt,
                    "duplicated definiation of primary key");
            }

            Forward(stmt, begin);
            if (begin >= stmt.size()) {
                throw SQLStmtError(stmt, "expected '('");
            } if (stmt[begin] != '(') {
                throw SQLStmtError(stmt, ExpFndStr("(", stmt[begin]));
            }
            ++begin;

            Forward(stmt, begin);
            auto pk_name = GetString(stmt, begin);

            Forward(stmt, begin);
            if (begin >= stmt.size()) {
                throw SQLStmtError(stmt, "expected ')'");
            } if (stmt[begin] != ')') {
                throw SQLStmtError(stmt, ExpFndStr(")", stmt[begin]));
            }
            ++begin;

            bool found = false;
            for (auto &attrb : sql_stmt.attrbs) {
                if (attrb.name == pk_name) {
                    attrb.is_primary = true;
                    attrb.is_unique = true;
                    found = true;   
                }
            }
            if (!found) {
                throw SQLStmtError(stmt, NoAttrbStr(pk_name, sql_stmt.name));
            }

            has_pk = true;
            is_pk = true;
        } else {
            throw SQLStmtError(stmt, "unknown attribute type");
        }

        // suffix 'primary key' / 'unique'
        if (!is_pk) {
            Forward(stmt, begin);
            auto extra_str = GetString(stmt, begin);
            if (extra_str == "unique") {
                attrb.is_unique = true;
            } else if (extra_str == "primary") {
                Forward(stmt, begin);
                extra_str = GetString(stmt, begin);
                if (extra_str == "key") {
                    if (has_pk) {
                        throw SQLStmtError(stmt,
                            "duplicated definiation of primary key");
                    }
                    attrb.is_primary = true;
                    attrb.is_unique = true;
                    has_pk = true;
                    suffix_pk = true;
                } else {
                    throw SQLStmtError(stmt, ExpFndStr("key", extra_str));
                }
            } else if (extra_str.size() > 0) {
                throw SQLStmtError(stmt,
                    ExpFndStr("unique' or 'primary key", extra_str));
            }

            sql_stmt.attrbs.push_back(attrb);
        }

        Forward(stmt, begin);
        if (begin < stmt.size() && stmt[begin] == ')') {
            ++begin;
            break;
        } else if (begin >= stmt.size()) {
            throw SQLStmtError(stmt, "expected ','");
        } if (stmt[begin] != ',') {
            throw SQLStmtError(stmt, ExpFndStr(",", stmt[begin]));
        }
        ++begin;
    }

    Forward(stmt, begin);
    if (begin >= stmt.size()) {
        throw SQLStmtError(stmt, "expected ';'");
    } if (stmt[begin] != ';') {
        throw SQLStmtError(stmt, ExpFndStr(";", stmt[begin]));
    }

    if (!has_pk) {
        throw SQLStmtError(stmt, "no primary key is defined");
    }

    std::unordered_set<std::string> attrb_names;
    for (const auto &attrb : sql_stmt.attrbs) {
        if (attrb_names.count(attrb.name) != 0) {
            throw SQLStmtError(stmt,
                "duplicated attribute name '" + attrb.name + "'");
        }
        attrb_names.insert(attrb.name);
    }

    return sql_stmt;
}

CreateIndexStmt
Interpreter::ParseCreateIndex(const std::string &stmt, int begin) const {
    CreateIndexStmt sql_stmt;

    Forward(stmt, begin);
    auto index_name = GetString(stmt, begin);
    if (index_name.size() == 0) {
        throw SQLStmtError(stmt, "expected index name");
    }
    if (!IsIdentifierStr(index_name)) {
        throw SQLStmtError(stmt, "invalid index name '" + index_name + "'");
    }
    sql_stmt.name = index_name;

    Forward(stmt, begin);
    auto on_str = GetString(stmt, begin);
    if (on_str != "on") {
        throw SQLStmtError(stmt, ExpFndStr("on", on_str));
    }

    Forward(stmt, begin);
    auto table_name = GetString(stmt, begin);
    if (table_name.size() == 0) {
        throw SQLStmtError(stmt, "expected table name");
    }
    sql_stmt.table_name = table_name;

    Forward(stmt, begin);
    if (begin >= stmt.size()) {
        throw SQLStmtError(stmt, "expected '('");
    } if (stmt[begin] != '(') {
        throw SQLStmtError(stmt, ExpFndStr("(", stmt[begin]));
    }
    ++begin;

    Forward(stmt, begin);
    auto attrb_name = GetString(stmt, begin);
    if (attrb_name.size() == 0) {
        throw SQLStmtError(stmt, "expected attribute name");
    }
    sql_stmt.attrb_name = attrb_name;

    Forward(stmt, begin);
    if (begin >= stmt.size()) {
        throw SQLStmtError(stmt, "expected ')'");
    } if (stmt[begin] != ')') {
        throw SQLStmtError(stmt, ExpFndStr(")", stmt[begin]));
    }
    ++begin;

    Forward(stmt, begin);
    if (begin >= stmt.size()) {
        throw SQLStmtError(stmt, "expected ';'");
    } if (stmt[begin] != ';') {
        throw SQLStmtError(stmt, ExpFndStr(";", stmt[begin]));
    }

    return sql_stmt;
}

DropTableStmt
Interpreter::ParseDropTable(const std::string &stmt, int begin) const {
    DropTableStmt sql_stmt;

    Forward(stmt, begin);
    auto table_name = GetString(stmt, begin);
    if (table_name.size() == 0) {
        throw SQLStmtError(stmt, "expected table name");
    }
    sql_stmt.name = table_name;

    Forward(stmt, begin);
    if (begin >= stmt.size()) {
        throw SQLStmtError(stmt, "expected ';'");
    } if (stmt[begin] != ';') {
        throw SQLStmtError(stmt, ExpFndStr(";", stmt[begin]));
    }

    return sql_stmt;
}

DropIndexStmt
Interpreter::ParseDropIndex(const std::string &stmt, int begin) const {
    DropIndexStmt sql_stmt;

    Forward(stmt, begin);
    auto index_name = GetString(stmt, begin);
    if (index_name.size() == 0) {
        throw SQLStmtError(stmt, "expected index name");
    }
    sql_stmt.name = index_name;

    Forward(stmt, begin);
    if (begin >= stmt.size()) {
        throw SQLStmtError(stmt, "expected ';'");
    } if (stmt[begin] != ';') {
        throw SQLStmtError(stmt, ExpFndStr(";", stmt[begin]));
    }

    return sql_stmt;
}

InsertStmt Interpreter::ParseInsert(const std::string &stmt, int begin) const {
    InsertStmt sql_stmt;

    Forward(stmt, begin);
    auto into_str = GetString(stmt, begin);
    if (into_str != "into") {
        throw SQLStmtError(stmt, ExpFndStr("into", into_str));
    }

    Forward(stmt, begin);
    auto table_name = GetString(stmt, begin);
    if (table_name.size() == 0) {
        throw SQLStmtError(stmt, "expected table name");
    }
    sql_stmt.name = table_name;

    Forward(stmt, begin);
    auto values_str = GetString(stmt, begin);
    if (values_str != "values") {
        throw SQLStmtError(stmt, ExpFndStr("values", values_str));
    }

    Forward(stmt, begin);
    if (begin >= stmt.size()) {
        throw SQLStmtError(stmt, "expected '('");
    } if (stmt[begin] != '(') {
        throw SQLStmtError(stmt, ExpFndStr("(", stmt[begin]));
    }
    ++begin;

    while (true) {
        Forward(stmt, begin);
        auto val = GetValue(stmt, begin);
        sql_stmt.vals.push_back(val);

        Forward(stmt, begin);
        if (begin < stmt.size() && stmt[begin] == ')') {
            ++begin;
            break;
        } else if (begin >= stmt.size()) {
            throw SQLStmtError(stmt, "expected ','");
        } if (stmt[begin] != ',') {
            throw SQLStmtError(stmt, ExpFndStr(",", stmt[begin]));
        }
        ++begin;
    }

    Forward(stmt, begin);
    if (begin >= stmt.size()) {
        throw SQLStmtError(stmt, "expected ';'");
    } if (stmt[begin] != ';') {
        throw SQLStmtError(stmt, ExpFndStr(";", stmt[begin]));
    }

    return sql_stmt;
}

DeleteStmt Interpreter::ParseDelete(const std::string &stmt, int begin) const {
    DeleteStmt sql_stmt;

    Forward(stmt, begin);
    auto from_str = GetString(stmt, begin);
    if (from_str != "from") {
        throw SQLStmtError(stmt, ExpFndStr("from", from_str));
    }

    Forward(stmt, begin);
    auto table_name = GetString(stmt, begin);
    if (table_name.size() == 0) {
        throw SQLStmtError(stmt, "expected table name");
    }
    sql_stmt.table_name = table_name;

    Forward(stmt, begin);
    if (begin < stmt.size() && stmt[begin] == ';') {
        return sql_stmt;
    }
    auto where_str = GetString(stmt, begin);
    if (where_str != "where") {
        throw SQLStmtError(stmt, ExpFndStr("where", where_str));
    }
    sql_stmt.conds = ParseWhere(stmt, begin);

    return sql_stmt;
}

SelectStmt Interpreter::ParseSelect(const std::string &stmt, int begin) const {
    SelectStmt sql_stmt;

    Forward(stmt, begin);
    if (begin >= stmt.size()) {
        throw SQLStmtError(stmt, "expected * or attribute name list");
    }
    if (stmt[begin] == '*') {
        sql_stmt.all = true;
        ++begin;
    } else {
        while (true) {
            Forward(stmt, begin);
            auto str = GetString(stmt, begin);
            if (str.size() == 0) {
                throw SQLStmtError(stmt, "expected attribute name");
            }
            sql_stmt.attrb_names.emplace_back(str);
            Forward(stmt, begin);
            if (begin >= stmt.size()) {
                throw SQLStmtError(stmt, "expected ',' or from");
            } else if (stmt[begin] == ',') {
                ++begin;
                continue;
            } else {
                break;
            }
        }
    }

    Forward(stmt, begin);
    auto from_str = GetString(stmt, begin);
    if (from_str != "from") {
        throw SQLStmtError(stmt, ExpFndStr("from", from_str));
    }

    Forward(stmt, begin);
    auto table_name = GetString(stmt, begin);
    if (table_name.size() == 0) {
        throw SQLStmtError(stmt, "expected table name");
    }
    sql_stmt.table_name = table_name;

    Forward(stmt, begin);
    if (begin < stmt.size() && stmt[begin] == ';') {
        return sql_stmt;
    }
    auto where_str = GetString(stmt, begin);
    if (where_str != "where") {
        throw SQLStmtError(stmt, ExpFndStr("where", where_str));
    }
    sql_stmt.conds = ParseWhere(stmt, begin);

    return sql_stmt;
}

ExecfileStmt
Interpreter::ParseExecfile(const std::string &stmt, int begin) const {
    ExecfileStmt sql_stmt;

    Forward(stmt, begin);
    int cnt = 0;
    for (; begin + cnt < stmt.size(); cnt++) {
        if (isspace(stmt[begin + cnt]) || stmt[begin + cnt] == ';') {
            break;
        }
    }
    sql_stmt.file_name = stmt.substr(begin, cnt);
    begin += cnt;

    Forward(stmt, begin);
    if (begin >= stmt.size() || stmt[begin] != ';') {
        throw SQLStmtError(stmt, ExpFndStr(";", stmt[begin]));
    }

    return sql_stmt;
}

QuitStmt Interpreter::ParseQuit(const std::string &stmt, int begin) const {
    Forward(stmt, begin);
    if (begin >= stmt.size() || stmt[begin] != ';') {
        throw SQLStmtError(stmt, ExpFndStr(";", stmt[begin]));
    }

    return QuitStmt {};
}

std::vector<Condition>
Interpreter::ParseWhere(const std::string &stmt, int begin) const {
    std::vector<Condition> conds;

    while (true) {
        Forward(stmt, begin);
        auto attrb_name = GetString(stmt, begin);
        if (attrb_name.size() == 0) {
            throw SQLStmtError(stmt, "expected attribute name");
        }

        Forward(stmt, begin);
        CondType type = GetCondType(stmt, begin);

        Forward(stmt, begin);
        auto val = GetValue(stmt, begin);

        conds.emplace_back(attrb_name, val, type);

        Forward(stmt, begin);
        if (begin < stmt.size() && stmt[begin] == ';') {
            break;
        }
        auto and_str = GetString(stmt, begin);
        if (and_str != "and") {
            throw SQLStmtError(stmt, ExpFndStr(stmt, and_str));
        }
    }

    return conds;
}