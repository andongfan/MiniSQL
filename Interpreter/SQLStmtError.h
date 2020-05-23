#pragma

#include <exception>
#include <string>

class SQLStmtError : public std::exception {
  public:
    SQLStmtError(const std::string &stmt, const std::string &info);

    const char *what() const noexcept override;

  private:
    std::string stmt;
    std::string info;
    std::string output_str;
};