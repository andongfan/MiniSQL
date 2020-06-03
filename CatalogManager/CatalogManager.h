#pragma once

#include <unordered_map>

#include "Table.h"
#include "Index.h"

class CatalogManager {
  public:
    void NewTable(const std::string &name, const std::vector<Attribute> &attrbs);
    void NewIndex(const std::string &name, const std::string &table_name,
        const std::string &attrb_name);

    void DropTable(const std::string &name);
    void DropIndex(const std::string &name);

    bool CheckTable(const std::string &name) const;
    bool CheckIndex(const std::string &name) const;
    bool CheckName(const std::string &name) const;
    
    const Table &GetTable(const std::string &name) const;
    Table &GetTable(const std::string &name);
    Index GetIndex(const std::string &name) const;

    void Load();
    void Save();

  private:
    std::unordered_map<std::string, Table> tables;
    std::unordered_map<std::string, Index> indices;

    inline static const std::string cat_mgr_file = "cat_mgr.data";
};