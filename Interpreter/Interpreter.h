#pragma once

#include "SQLStatement.h"
#include "SQLStmtError.h"

class Interpreter {
  public:
    SQLStatement ParseStmt(const std::string &stmt) const;
    std::vector<SQLStatement> ParseFile(const std::string &file_name) const;
    std::vector<SQLStatement> Parse(const std::string &str) const;

  private:
    CreateTableStmt ParseCreateTable(const std::string &stmt, int begin) const;
    CreateIndexStmt ParseCreateIndex(const std::string &stmt, int begin) const;
    DropTableStmt ParseDropTable(const std::string &stmt, int begin) const;
    DropIndexStmt ParseDropIndex(const std::string &stmt, int begin) const;
    InsertStmt ParseInsert(const std::string &stmt, int begin) const;
    DeleteStmt ParseDelete(const std::string &stmt, int begin) const;
    SelectStmt ParseSelect(const std::string &stmt, int begin) const;
    ExecfileStmt ParseExecfile(const std::string &stmt, int begin) const;
    QuitStmt ParseQuit(const std::string &stmt, int begin) const;

    std::vector<Condition> ParseWhere(const std::string &stmt, int begin) const;
};