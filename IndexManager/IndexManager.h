#include <string>
#include <vector>
#include "BPlusTree.h"
using std::vector;
using std::string;

template<class T>
class IndexManager {
public:
    string fileName;
    int typeLen;
    IndexManager(string indexName, string tableName, string attrName, int typeLen);
    int findSingleRecordWithKey(const T& key);
    vector<int> findRecordsWithRange(const T& st, const T& ed);
    bool insertRecordWithKey(const T& key, int recordID);
    bool deleteRecordByKey(const T& key);
    bool createIndex();
    bool dropIndex();

    // debug
    void printTree();
};

template<class T>
void IndexManager<T>::printTree() {
    BPTree<T> bPlusTree(fileName);
    bPlusTree.print();
    cout << "-------------\n";
}

template<class T>
IndexManager<T>::IndexManager(string indexName, string tableName, string attrName, int _typeLen) {
    fileName = indexName + "_" + tableName + "_" + attrName + ".data";
    if constexpr (std::is_same<T, std::string>()) {
        typeLen = _typeLen + 1;
    } else {
        typeLen = _typeLen;
    }
}

template<class T>
int IndexManager<T>::findSingleRecordWithKey(const T& key) {
    BPTree<T> bPlusTree(fileName);
    return bPlusTree.findRecordWithKey(key);
}


template<class T>
vector<int> IndexManager<T>::findRecordsWithRange(const T& st, const T& ed) {
    BPTree<T> bPlusTree(fileName);
    return bPlusTree.findRecordWithRange(st, ed);
}

template<class T>
bool IndexManager<T>::insertRecordWithKey(const T& key, int recordID) {
    BPTree<T> bPlusTree(fileName);
    return bPlusTree.insertRecordWithKey(key, recordID);
}

template<class T>
bool IndexManager<T>::deleteRecordByKey(const T& key) {
    BPTree<T> bPlusTree(fileName);
    return bPlusTree.deleteRecordWithKey(key);
}

template<class T>
bool IndexManager<T>::createIndex() {
    int order = (BLOCK_SIZE - 20) / (typeLen + 4) + 1;
    // int order = 4;
    int page = buf_mgr.getPageId(fileName, 0);
    buf_mgr.pinPage(page);
    int count = 0;
    int root = -1;
    int firstEmpty = -1;
    char* data = buf_mgr.getPageAddress(page);
    memcpy(data, &order, 4);
    memcpy(data + 4, &typeLen, 4);
    memcpy(data + 8, &count, 4);
    memcpy(data + 12, &root, 4);
    memcpy(data + 16, &firstEmpty, 4);
    buf_mgr.dirtPage(page);
    buf_mgr.unpinPage(page);
}

template<class T>
bool IndexManager<T>::dropIndex() {
    // delete the file
}