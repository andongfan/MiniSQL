#pragma once

#include <exception>
#include <string>

class SQLExecError : public std::exception {
  public:
    SQLExecError(const std::string &info);

    const char *what() const noexcept override;

  private:
    std::string info;
};