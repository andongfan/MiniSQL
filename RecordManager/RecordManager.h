#pragma once

#include <string>
#include <vector>
#include <utility>
#include "SQLStatement.h"
#include "BufferManager.hpp"
#include "IndexManager.h"
#include "Table.h"

namespace RM
{
    class RecordError : public std::exception
    {
        std::string msg;
    public:
        RecordError(const std::string info)
        {
            msg = info;
        }
        const char *what() const noexcept override
        {
            return msg.c_str();
        }
    };

    inline BufferManager *bm;
    // static int GetBlockCount(const Table &t);
    // static std::vector<Value> GetTuple(const Table &t, char *p);
    // static void PutTuple(const Table &t, const std::vector<Value> v, char *p);
    // static bool CheckUniqueness(const Table &t, const Attribute &a, const Value &v);
    void CreateTable(Table &t);
    void DropTable(Table &t);
    std::pair<int, int> InsertRecord(Table &t, const std::vector<Value> &vals);
    void DeleteRecord(Table &t, const std::vector<Condition> conds);
    std::vector<std::vector<Value>> SelectRecord(Table &t, const std::vector<Condition> conds);
    void UpdateRecord(Table &t, const std::vector<Condition> &conds, const std::vector<std::pair<std::string, Value>> &values);
    void CreateIndex(const std::string &idxName, Table &t, const std::string &attrb);
    void DropIndex(const std::string &idxName, Table &t, const std::string &attrb);
} // namespace RM