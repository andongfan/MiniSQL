#include <iostream>

#include "Interpreter.h"
#include "API.h"
#include "SQLExecError.h"

int main() {
    // TODO

    Interpreter inter;
    auto stmts = inter.ParseFile("../test.sql");
    
    try {
        for (const auto &stmt : stmts) {
            minisql::Print(stmt);
            auto time = minisql::Execute(stmt);
            std::cout << "executed successfully, time used: " <<
                time.count() << "s" << std::endl;
        }
    } catch (const SQLExecError &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
