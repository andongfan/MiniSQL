#include "RecordManager.h"
#include "CatalogManager.h"
#include "BufferManager.hpp"
#include "SQLStatement.h"
#include <vector>
#include <iostream>

Table stu;

std::pair<int, int> newStudent(const std::string &id, const int age, const double gpa)
{
    std::cout << "Inserting " << id << ' ' << age << ' ' << gpa << std::endl;
    std::vector<Value> v;
    v.push_back(id);
    v.push_back(age);
    v.push_back(gpa);
    std::pair<int, int> res;
    try
    {
        res = RM::InsertRecord(stu, v);
    }
    catch (RM::RecordError &e)
    {
        std::cerr << e.what() << std::endl;
    }
    return res;
}

int main()
{
    CatalogManager cat_mgr;
    std::vector<Attribute> v;
    Attribute id;
    id.name = "id";
    id.type = AttrbType::CHAR;
    id.is_primary = false;
    id.is_unique = true;
    id.char_len = 4;
    v.push_back(id);
    Attribute age;
    age.name = "age";
    age.type = AttrbType::INT;
    age.is_unique = false;
    age.is_primary = false;
    v.push_back(age);
    Attribute GPA;
    GPA.name = "GPA";
    GPA.type = AttrbType::FLOAT;
    GPA.is_unique = false;
    GPA.is_primary = false;
    v.push_back(GPA);
    cat_mgr.NewTable("student", v);
    stu = cat_mgr.GetTable("student");

    RM::bm = new BufferManager;
    RM::CreateTable(stu);

    newStudent("0001", 10, 5.0);
    newStudent("0001", 15, 1.5);
    newStudent("0002", 11, 2);
    std::pair<int, int> p = newStudent("0003", 12, 3);
    std::pair<int, int> q = newStudent("0004", 13, 4);
    newStudent("0005", 14, 4.2);
    std::vector<Condition> conds;
    conds.push_back(Condition("GPA", 3, CondType::GREAT));
    std::vector<std::vector<Value>> res = RM::SelectRecord(stu, conds);
    for (auto v : res)
    {
        for (int i = 0; i < stu.attrbs.size(); ++i)
        {
            if (stu.attrbs[i].type == AttrbType::INT)
            {
                std::cout << std::get<int>(v[i]) << ',';
            }
            if (stu.attrbs[i].type == AttrbType::FLOAT)
            {
                std::cout << std::get<double>(v[i]) << ',';
            }
            if (stu.attrbs[i].type == AttrbType::CHAR)
            {
                std::cout << std::get<std::string>(v[i]) << ',';
            }
        }
        std::cout << std::endl;
    }
    delete RM::bm;
    return 0;
}