#include "RecordManager.h"
#include <exception>
#include <iostream>
#include <algorithm>
#include <cstdio>
#define PIECE_SIZE (t.Size() + 1)
#define PIECE_CAPACITY (BLOCK_SIZE / PIECE_SIZE)
namespace RM
{
    typedef std::vector<std::pair<int, int>> PieceVec;

    static int GetBlockCount(Table &t)
    {
        return t.n_blocks;
    }

    void CreateTable(Table &t)
    {
        int pageid = bm->getPageId(t.name, 0);
        char *p = bm->getPageAddress(pageid);
        *p = 0; // clear the first piece
        t.n_blocks = 1;
    }

    void DropTable(Table &t)
    {
        bm->deletePageWithName(t.name);
        remove((t.name + ".data").c_str());
    }

    static std::vector<Value> GetTuple(Table &t, char *p)
    {
        const std::vector<Attribute> &a = t.attrbs;
        std::vector<Value> ret;
        ++p; // skip the first sign byte
        for (int offset = 0, i = 0; i < a.size(); ++i)
        {
            if (a[i].type == AttrbType::FLOAT)
            {
                double val = *(double *)(p + offset);
                // std::cerr << "DVAL " << val << std::endl;
                ret.push_back(val);
            }
            else if (a[i].type == AttrbType::INT)
            {
                int val = *(int *)(p + offset);
                // std::cerr << "IVAL " << val << std::endl;
                ret.push_back(val);
            }
            else // string
            {
                std::string val = std::string((char *)(p + offset));
                ret.push_back(val);
                // std::cerr << "SVAL " << val << std::endl;
            }
            offset += a[i].Size();
        }
        return ret;
    }

    static void PutTuple(Table &t, const std::vector<Value> v, char *p)
    {
        // assuming that values in v matches the attribute types of table t
        *p = 1; // no longer empty
        ++p;
        const std::vector<Attribute> &a = t.attrbs;
        for (int i = 0, offset = 0; i < a.size(); ++i)
        {
            if (a[i].type == AttrbType::FLOAT)
            {
                double val = std::get<double>(v[i]);
                memcpy(p + offset, &val, a[i].Size());
            }
            else if (a[i].type == AttrbType::INT)
            {
                int val = std::get<int>(v[i]);
                memcpy(p + offset, &val, a[i].Size());
            }
            else // string
            {
                std::string val = std::get<std::string>(v[i]);
                memcpy(p + offset, val.c_str(), a[i].Size());
            }
            offset += a[i].Size();
        }
    }

    static PieceVec IndexSelect(Table &t, const Attribute &a, const Condition &c)
    {
        // TODO
    }

    static bool CheckUniqueness(Table &t, const int id, const Value &v)
    {
        // std::cerr << "CheckUniqueness " << id << ' ' << std::get<std::string>(v) << std::endl;
        if (t.attrbs[id].index != "")
        { // use index to check uniqueness
            PieceVec vec = IndexSelect(t, t.attrbs[id], Condition(t.attrbs[id].name, v, CondType::EQUAL));
            return not vec.empty();
        }
        // std::cerr << "QWQ" << std::endl;
        int blockcount = GetBlockCount(t);
        for (int i = 0; i < blockcount; ++i)
        {
            int pageid = bm->getPageId(t.name, i);
            char *p = bm->getPageAddress(pageid);
            for (int offset = 0; offset < BLOCK_SIZE; offset += PIECE_SIZE)
            {
                if (p[offset] == 1)
                {
                    std::vector<Value> vals = GetTuple(t, p + offset);
                    // std::cerr << vals.size() << std::endl;
                    // std::cerr << std::get<std::string>(vals[id]) << ' ' << std::get<std::string>(v) << std::endl;
                    if (vals[id] == v)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    std::pair<int, int> InsertRecord(Table &t, const std::vector<Value> &vals)
    {
        for (int i = 0; i < t.attrbs.size(); ++i)
        {
            if (t.attrbs[i].is_unique && not CheckUniqueness(t, i, vals[i]))
            {
                throw RecordError("attribute " + t.attrbs[i].name + " not unique!");
            }
        }
        int blockcount = GetBlockCount(t);
        int pageid = bm->getPageId(t.name, blockcount - 1); // try to insert record into the last page
        char *p = bm->getPageAddress(pageid);

        for (int offset = 0; offset < BLOCK_SIZE; offset += PIECE_SIZE)
        {
            if (p[offset] == 0) // found empty piece
            {
                PutTuple(t, vals, p + offset);
                return std::make_pair(pageid, offset);
            }
        }

        // need to create a new page
        t.n_blocks += 1;
        int newpageid = bm->getPageId(t.name, blockcount);
        char *np = bm->getPageAddress(newpageid);
        PutTuple(t, vals, np);
        return std::make_pair(newpageid, 0);
    }

    static PieceVec Intersect(PieceVec p, PieceVec q)
    {
        std::sort(p.begin(), p.end());
        std::sort(q.begin(), q.end());
        PieceVec::iterator ip = p.begin(), iq = q.begin();
        PieceVec res;
        while (ip != p.end() && iq != q.end())
        {
            while (ip != p.end() && *ip < *iq)
            {
                ++ip;
            }
            while (iq != q.end() && *iq < *ip)
            {
                ++iq;
            }
            while (ip != p.end() && iq != q.end() && *ip == *iq)
            {
                res.push_back(*ip);
                ++ip;
                ++iq;
            }
        }
        return res;
    }

    static PieceVec SelectPos(Table &t, const std::vector<Condition> conds)
    {
        PieceVec v;
        bool flag = false;
        for (Condition c : conds)
        {
            if (t.GetAttrb(c.attrb).index != "")
            {
                if (not flag)
                {
                    flag = true;
                    v = IndexSelect(t, t.GetAttrb(c.attrb), c);
                }
                else
                {
                    v = Intersect(v, IndexSelect(t, t.GetAttrb(c.attrb), c));
                }
            }
        }
        if (not flag)
        { // no index involved
            int blockcount = GetBlockCount(t);
            for (int i = 0; i < blockcount; ++i)
            {
                int pageid = bm->getPageId(t.name, i);
                char *p = bm->getPageAddress(pageid);
                for (int offset = 0; offset < BLOCK_SIZE; offset += PIECE_SIZE)
                {
                    if (p[offset] == 1)
                    {
                        std::vector<Value> tup = GetTuple(t, p + offset);
                        bool good = true;
                        for (Condition c : conds)
                        {
                            for (int j = 0; j < t.attrbs.size(); ++j)
                            {
                                if (c.attrb == t.attrbs[j].name && not c.IsTrue(tup[j]))
                                {
                                    good = false;
                                    break;
                                }
                            }
                            if (not good)
                            {
                                break;
                            }
                        }
                        if (good)
                        {
                            v.push_back(std::make_pair(i, offset));
                        }
                    }
                }
            }
        }
        return v;
    }

    void DeleteRecord(Table &t, const std::vector<Condition> conds)
    {
        PieceVec v = SelectPos(t, conds);
        for (auto piece : v)
        {
            int pageid = bm->getPageId(t.name, piece.first);
            char *p = bm->getPageAddress(pageid);
            p[piece.second] = 0;
        }
    }

    std::vector<std::vector<Value>> SelectRecord(Table &t, const std::vector<Condition> conds)
    {
        std::vector<std::vector<Value>> res;
        PieceVec v = SelectPos(t, conds);
        for (auto piece : v)
        {
            int pageid = bm->getPageId(t.name, piece.first);
            char *p = bm->getPageAddress(pageid);
            res.push_back(GetTuple(t, p + piece.second));
        }
        return res;
    }
} // namespace RM