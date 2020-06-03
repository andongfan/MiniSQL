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
                double tmp;
                if (auto p = std::get_if<double>(&v[i])) tmp = *p;
                else tmp = std::get<int>(v[i]);
                double val = tmp;
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

    static int pair2id(const Table &t, std::pair<int, int> p)
    {
        return p.first * PIECE_CAPACITY + p.second / PIECE_SIZE;
    }

    static std::pair<int,int> id2pair(const Table &t, int id) {
        return std::make_pair(id / PIECE_CAPACITY, id % PIECE_CAPACITY * PIECE_SIZE);
    }

    template <typename T>
    struct is_double
    {
        operator bool()
        {
            return false;
        }
    };

    template <>
    struct is_double<double>
    {
        operator bool()
        {
            return true;
        }
    };

    template<class T>
    static T myget(Value v) {
        if (is_double<T>()){
            T tmp;
            if (auto p = std::get_if<double>(&v))
                tmp = *p;
            else
                tmp = std::get<int>(v);
            return tmp;
        }
        return std::get<T>(v);
    }

    template<class T>
    static PieceVec IndexSelectTemp(Table &t, const Attribute &a, const Condition &c)
    {
        int len;
        if (a.type == AttrbType::CHAR)
        {
            len = a.Size() - 1;
        }
        else
        {
            len = a.Size();
        }
        int id;
        for (int i = 0; i < t.attrbs.size(); ++i) {
            if (t.attrbs[i].name == a.name) {
                id = i;
                break;
            }
        }
        PieceVec res;
        IndexManager<T> im(a.index, t.name, a.name, len);
        if (c.type == CondType::EQUAL) {
            std::pair<int, int> p = id2pair(t, im.findSingleRecordWithKey(myget<T>(c.val)));
            res.push_back(p);
        } else {
            Value lb, rb;
            if (c.type == CondType::GREAT || c.type == CondType::GREAT_EQUAL) {
                if (c.type == CondType::GREAT) {
                    if (a.type == AttrbType::CHAR) {
                        std::string s = std::get<string>(c.val);
                        s[s.size() - 1] = '\0';
                        lb = s;
                    } else if (a.type == AttrbType::FLOAT)
                    {
                        double tmp;
                        if (auto p = std::get_if<double>(&c.val))
                            tmp = *p;
                        else
                            tmp = std::get<int>(c.val);
                        double d = tmp - 1;
                        
                        lb = d;
                    }
                    else
                    {
                        int i = std::get<int>(c.val) - 1;
                        lb = i;
                    }
                } else {
                    lb = c.val;
                }
                if (a.type == AttrbType::CHAR) {
                    rb = std::string(a.char_len, '\xff');
                } else if (a.type == AttrbType::FLOAT) {
                    rb = std::numeric_limits<double>::max();
                } else {
                    rb = std::numeric_limits<int>::max();
                }
            } else {
                if (c.type == CondType::LESS)
                {
                    if (a.type == AttrbType::CHAR)
                    {
                        std::string s = std::get<string>(c.val);
                        s[s.size() - 1] = '\xff';
                        rb = s;
                    }
                    else if (a.type == AttrbType::FLOAT)
                    {
                        double tmp;
                        if (auto p = std::get_if<double>(&c.val))
                            tmp = *p;
                        else
                            tmp = std::get<int>(c.val);
                        double d = tmp + 1;
                        rb = d;
                    }
                    else
                    {
                        int i = std::get<int>(c.val) + 1;
                        rb = i;
                    }
                }
                else
                {
                    rb = c.val;
                }
                if (a.type == AttrbType::CHAR)
                {
                    rb = std::string(a.char_len, '\0');
                }
                else if (a.type == AttrbType::FLOAT)
                {
                    rb = std::numeric_limits<double>::min();
                }
                else
                {
                    rb = std::numeric_limits<int>::min();
                }
            }
            std::vector<int> recs;
            
            recs  = im.findRecordsWithRange(myget<T>(lb), myget<T>(rb));
            for (int r : recs) {
                res.push_back(id2pair(t, r));
            }
        }
        return res;
    }

    static PieceVec IndexSelect(Table &t, const Attribute &a, const Condition &c)
    {
        if (a.type == AttrbType::CHAR) {
            return IndexSelectTemp<std::string>(t, a, c);
        } else if (a.type == AttrbType::FLOAT) {
            return IndexSelectTemp<double>(t, a, c);
        } else {
            return IndexSelectTemp<int>(t, a, c);
        }
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
                    // std::cerr << std::get<std::string>(vals[id]) << ' ' << q<std::string>(v) << std::endl;
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

        std::pair<int, int> rec;
        bool inserted = false;
        for (int offset = 0; offset < BLOCK_SIZE; offset += PIECE_SIZE)
        {
            if (p[offset] == 0) // found empty piece
            {
                PutTuple(t, vals, p + offset);
                rec = std::make_pair(pageid, offset);
                inserted = true;
                break;
            }
        }

        if (not inserted) {
            // need to create a new page
            t.n_blocks += 1;
            int newpageid = bm->getPageId(t.name, blockcount);
            char *np = bm->getPageAddress(newpageid);
            PutTuple(t, vals, np);
            rec = std::make_pair(newpageid, 0);
        }

        for (int i = 0; i < t.attrbs.size(); ++i)
        {
            if (t.attrbs[i].index != "")
            {
                if (t.attrbs[i].type == AttrbType::CHAR)
                {
                    IndexManager<std::string> im(t.attrbs[i].index, t.name, t.attrbs[i].name, t.attrbs[i].char_len);
                    im.insertRecordWithKey(std::get<std::string>(vals[i]), pair2id(t, rec));
                } else if(t.attrbs[i].type == AttrbType::FLOAT) {
                    IndexManager<double> im(t.attrbs[i].index, t.name, t.attrbs[i].name, t.attrbs[i].Size());
                    double tmp;
                    if (auto p = std::get_if<double>(&vals[i]))
                        tmp = *p;
                    else
                        tmp = std::get<int>(vals[i]);
                    im.insertRecordWithKey(tmp, pair2id(t, rec));
                } else {
                    IndexManager<int> im(t.attrbs[i].index, t.name, t.attrbs[i].name, t.attrbs[i].Size());
                    im.insertRecordWithKey(std::get<int>(vals[i]), pair2id(t, rec));
                }
            }
        }

        return rec;
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
            if (c.type == CondType::NOT_EQUAL) { // cannot use index to optimiza unequality
                continue;
            }
            if (t.GetAttrb(c.attrb).index != "")
            {
                if (not flag)
                {
                    // std::cerr << "INDEX INVOLVED" << std::endl;
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
        std::vector<std::vector<Value>> vals;
        for (auto piece : v)
        {
            int pageid = bm->getPageId(t.name, piece.first);
            char *p = bm->getPageAddress(pageid);
            vals.push_back(GetTuple(t, p + piece.second));
            p[piece.second] = 0;
        }
        for (int i = 0; i < t.attrbs.size(); ++i)
        {
            if (t.attrbs[i].index != "")
            {
                if (t.attrbs[i].type == AttrbType::CHAR)
                {
                    IndexManager<std::string> im(t.attrbs[i].index, t.name, t.attrbs[i].name, t.attrbs[i].char_len);
                    for (auto val : vals)
                        im.deleteRecordByKey(std::get<std::string>(val[i]));
                }
                else if (t.attrbs[i].type == AttrbType::FLOAT)
                {
                    IndexManager<double> im(t.attrbs[i].index, t.name, t.attrbs[i].name, t.attrbs[i].Size());
                    for (auto val : vals) {
                        double tmp;
                        if (auto p = std::get_if<double>(&val[i]))
                            tmp = *p;
                        else
                            tmp = std::get<int>(val[i]);
                        im.deleteRecordByKey(tmp);
                    }
                }
                else
                {
                    IndexManager<int> im(t.attrbs[i].index, t.name, t.attrbs[i].name, t.attrbs[i].Size());
                    for (auto val : vals)
                        im.deleteRecordByKey(std::get<int>(val[i]));
                }
            }
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

    template <class T>
    static void BuildIndex(const std::string idxName, Table &t, int id, int len)
    {
        // std::cerr << "BuildIndex" << std::endl;
        IndexManager<T> im(idxName, t.name, t.attrbs[id].name, len);
        im.createIndex();
        PieceVec v = SelectPos(t, std::vector<Condition>());
        // std::cerr << "Done Selecting all" << std::endl;
        for (auto piece : v)
        {
            int pageid = bm->getPageId(t.name, piece.first);
            char *p = bm->getPageAddress(pageid);
            std::vector<Value> vec = GetTuple(t, p + piece.second);
            im.insertRecordWithKey(myget<T>(vec[id]), pair2id(t, piece));
        }
    }

    void CreateIndex(const std::string &idxName, Table &t, const std::string &attrb)
    {
        // std::cerr << "CreateIndex" << std::endl;
        for (int i = 0; i < t.attrbs.size(); ++i) {
            if (t.attrbs[i].name == attrb) {
                if (t.attrbs[i].type == AttrbType::CHAR) {
                    BuildIndex<string>(idxName, t, i, t.attrbs[i].char_len);
                } else if (t.attrbs[i].type == AttrbType::FLOAT) {
                    BuildIndex<double>(idxName, t, i, t.attrbs[i].Size());
                } else {
                    BuildIndex<int>(idxName,t, i, t.attrbs[i].Size());
                }
                t.attrbs[i].index = idxName; // set index file name
                break;
            }
        }
    }
    void DropIndex(const std::string &idxName, Table &t, const std::string &attrb) {
        int id;
        for (int i = 0; i < t.attrbs.size(); ++i) {
            if (t.attrbs[i].name == attrb) {
                t.attrbs[i].index = "";
                id = i;
                break;
            }
        }
        FILE *fp = fopen((idxName + "_" + t.name + "_" + attrb + ".data").c_str(), "wb+");
        fclose(fp);
    }

} // namespace RM