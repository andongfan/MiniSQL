#pragma once

#include <chrono>

#include "SQLStatement.h"

namespace minisql {

void Print(const SQLStatement &stmt);

std::chrono::duration<double> Execute(const SQLStatement &stmt);

}