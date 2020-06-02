#pragma once

#include <chrono>

#include "SQLStatement.h"
#include "CatalogManager.h"
#include "Interpreter.h"
#include "RecordManager.h"

class MiniSQL {
  public:
    MiniSQL();
    ~MiniSQL();

    void Print(const SQLStatement &stmt);
    std::chrono::duration<double> Execute(const SQLStatement &stmt);

    void MainLoop();

  private:
    void CreateTable(const CreateTableStmt &stmt);
    void CreateIndex(const CreateIndexStmt &stmt);
    void DropTable(const DropTableStmt &stmt);
    void DropIndex(const DropIndexStmt &stmt);
    void Insert(const InsertStmt &stmt);
    void Delete(const DeleteStmt &stmt);
    void Select(const SelectStmt &stmt);
    void Quit(const QuitStmt &stmt);
    void Execfile(const ExecfileStmt &stmt);

    Interpreter inter;
    CatalogManager cat_mgr;
    bool should_quit = false;
};