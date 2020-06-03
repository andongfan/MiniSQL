#include "CatalogManager.h"

#include <algorithm>
#include <fstream>

void CatalogManager::NewTable(const std::string &name,
        const std::vector<Attribute> &attrbs) {
    tables[name] = Table { name, attrbs };
}

void CatalogManager::NewIndex(const std::string &name,
        const std::string &table_name, const std::string &attrb_name) {
    indices[name] = { name, table_name, attrb_name };
    auto &table = tables[table_name];
    for (auto &attrb : table.attrbs) {
        if (attrb.name == attrb_name) {
            attrb.index = name;
            break;
        }
    }
}

void CatalogManager::DropTable(const std::string &name) {
    auto table = tables[name];
    tables.erase(name);
    for (auto attrb : table.attrbs) {
        if (attrb.index != "") {
            indices.erase(attrb.index);
        }
    }
}

void CatalogManager::DropIndex(const std::string &name) {
    auto index = indices[name];
    indices.erase(name);
    auto table = tables[index.table_name];
    for (auto &attrb : table.attrbs) {
        if (attrb.name == index.attrb_name) {
            attrb.index = "";
            break;
        }
    }
}

bool CatalogManager::CheckTable(const std::string &name) const {
    return tables.count(name) != 0;
}

bool CatalogManager::CheckIndex(const std::string &name) const {
    return indices.count(name) != 0;
}

bool CatalogManager::CheckName(const std::string &name) const {
    return CheckTable(name) || CheckIndex(name);
}

const Table &CatalogManager::GetTable(const std::string &name) const {
    return tables.at(name);
}
Table &CatalogManager::GetTable(const std::string &name) {
    return tables[name];
}

Index CatalogManager::GetIndex(const std::string &name) const {
    return indices.at(name);
}

static std::string GetStringFromBinFile(std::ifstream &fin) {
    int len;
    fin.read(reinterpret_cast<char *>(&len), sizeof(int));
    char *cstr = new char[len + 1];
    fin.read(cstr, len);
    cstr[len] = 0;
    std::string str;
    std::copy_n(cstr, len, std::back_inserter(str));
    delete[] cstr;
    return str;
}

void CatalogManager::Load() {
    std::ifstream fin(cat_mgr_file, std::ios::binary);
    if (!fin) {
        return;
    }

    while (fin) {
        auto ch = fin.get();
        if (fin.eof()) {
            break;
        }
        bool is_table = ch;
        
        if (is_table) {
            std::string str = GetStringFromBinFile(fin);
            const char *p = str.data();
            Table table = Table::Parse(p);
            tables[table.name] = table;
        } else {
            std::string name = GetStringFromBinFile(fin);
            std::string table_name = GetStringFromBinFile(fin);
            std::string attrb_name = GetStringFromBinFile(fin);
            indices[name] = { name, table_name, attrb_name };
        }
    }
}

void CatalogManager::Save() {
    std::ofstream fout(cat_mgr_file, std::ios::binary);
    
    std::string str;
    for (const auto &[name, table] : tables) {
        str += char(1);
        auto table_s = table.Serialize();
        int len = table_s.size();
        std::copy_n(reinterpret_cast<char *>(&len), sizeof(int),
            std::back_inserter(str));
        str += table_s;
    }

    for (const auto &[name, index] : indices) {
        str += char(0);

        int len = index.name.size();
        std::copy_n(reinterpret_cast<char *>(&len), sizeof(int),
            std::back_inserter(str));
        std::copy_n(index.name.c_str(), len, std::back_inserter(str));
        len = index.table_name.size();
        std::copy_n(reinterpret_cast<char *>(&len), sizeof(int),
            std::back_inserter(str));
        std::copy_n(index.table_name.c_str(), len, std::back_inserter(str));
        len = index.attrb_name.size();
        std::copy_n(reinterpret_cast<char *>(&len), sizeof(int),
            std::back_inserter(str));
        std::copy_n(index.attrb_name.c_str(), len, std::back_inserter(str));
    }

    fout.write(str.c_str(), str.size());
}