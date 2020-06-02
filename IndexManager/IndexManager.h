#include <string>

template<class T>
class IndexManager {
    using string = std::string;
    int findRecordWithKey(string indexName, const T& key);
    bool insertRecordWithKey(string indexName, const T& key, int recordID);
    bool deleteRecordByKey(string indexName, const T& key);
    bool createIndex(string IndexName, string tableName, string attrName);
    bool dropIndex(string IndexName);
};

