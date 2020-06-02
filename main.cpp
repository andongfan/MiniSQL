#include <iostream>

#include "Interpreter.h"
#include "API.h"
#include "SQLExecError.h"

std::string GetStmtsString() {
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

int main() {
    minisql::Initialize();

    Interpreter inter;

    while (true) {
        try {
            std::string str = GetStmtsString();
            auto stmts = inter.Parse(str);
            bool quit = false;
            int i = 0;
            for (const auto &stmt : stmts) {
                // minisql::Print(stmt);
                if (auto p = std::get_if<QuitStmt>(&stmt)) {
                    quit = true;
                    break;
                }
                ++i;
                std::cout << "statement " << i << ": ";
                auto time = minisql::Execute(stmt);
                std::cout << "executed successfully, time used: " <<
                    time.count() << "s" << std::endl;
            }
            if (quit) {
                break;
            }
        } catch (const SQLExecError &e) {
            std::cout << e.what() << std::endl;
        }
    }

    minisql::Finalize();

    return 0;
}
