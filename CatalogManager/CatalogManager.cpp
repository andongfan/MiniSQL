#include "CatalogManager.h"

void CatalogManager::NewTable(const std::string &name,
        const std::vector<Attribute> &attrbs) {
    tables[name] = Table { name, attrbs };
}

void CatalogManager::NewIndex(const std::string &name,
        const std::string &table_name, const std::string &attrb_name) {
    indices[name] = { name, table_name, attrb_name };
    auto table = tables[table_name];
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
        if (attrb.name == index.attbr_name) {
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

Table CatalogManager::GetTable(const std::string &name) const {
    return tables.at(name);
}

Index CatalogManager::GetIndex(const std::string &name) const {
    return indices.at(name);
}

CatalogManager cat_mgr;