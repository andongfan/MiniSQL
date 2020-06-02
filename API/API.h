#pragma once

#include <chrono>

#include "SQLStatement.h"

namespace minisql {

void Initialize();

void Finalize();

void Print(const SQLStatement &stmt);

std::chrono::duration<double> Execute(const SQLStatement &stmt);

}