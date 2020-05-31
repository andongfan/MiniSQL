#include <vector>
#include <cmath>
#include <iostream>
#define TEST_N 3
#define NOT_VALID -1
using std::cin;
using std::cout;

#define DEBUG

template <class T>
class BPTreeNode {
private:
    BPTreeNode* parent;
    #ifndef DEBUG
    std::vector <T> keys;
    std::vector <int> records;
    std::vector <BPTreeNode*> children;
    #endif
    BPTreeNode* rightNode;
    BPTreeNode* leftNode;
    bool isLeaf;
    int N;
    BPTreeNode* splitLeaf();
    BPTreeNode* splitNonLeaf();

    // void merge(BPTreeNode* left, BPTreeNode* right);
    // void redistribute(BPTreeNode* left, BPTreeNode* right);

public:
    template <class U>
    friend void deleteNonLeaf(int pos, BPTreeNode<U>* node);

    BPTreeNode(BPTreeNode* _parent, int _N, bool _isLeaf);

    #ifdef DEBUG
    std::vector <T> keys;
    std::vector <int> records;
    std::vector <BPTreeNode*> children;
    #endif

    bool isRoot();
    bool isLeafNode();
    BPTreeNode* getParent();
    bool isOverLoad(int size = -1);
    bool isHungry(int size = -1);
    int keyNum();

    int findRecordWithKey(const T& key);
    bool addKeyIntoNode(const T& key, int recordID);
    int deleteKeyFromNode(const T& key);
    BPTreeNode* getChildWithKey(const T& key);

    void pushKeyAndRecord(const T& key, int recordID);
    BPTreeNode* split();

    bool tryReplaceKey(const T& oldKey, const T& newKey);
    void inflateLeaf();
};

template <class T>
BPTreeNode<T>::BPTreeNode(BPTreeNode* _parent, int _N, bool _isLeaf) : parent(_parent), N(_N), isLeaf(_isLeaf) {
    leftNode = rightNode = NULL;
}
template <class T>
inline int BPTreeNode<T>::keyNum() {
    return keys.size();
}

template <class T>
inline bool BPTreeNode<T>::isRoot() {
    return parent == NULL;
}

template <class T>
inline bool BPTreeNode<T>::isLeafNode() {
    return isLeaf;
}

template <class T>
inline bool BPTreeNode<T>::isOverLoad(int size) {
    if (size == -1) size = keys.size();
    // return isLeaf ? size > N : size > N - 1;
    return size > N - 1;
}

template <class T>
inline bool BPTreeNode<T>::isHungry(int size) {
    if (parent == NULL) return false;
    return isLeaf ? size < std::ceil((N - 1) / 2.0) : size < std::ceil(N / 2.0) - 1;
}

template <class T>
inline BPTreeNode<T>* BPTreeNode<T>::getParent() {
    return parent;
}

template <class T>
int BPTreeNode<T>::findRecordWithKey(const T& key) {
    auto it = std::find(keys.begin(), keys.end(), key);
    if (it == keys.end()) return NOT_VALID;
    return records[it - keys.begin()];
}

template <class T>
bool BPTreeNode<T>::addKeyIntoNode(const T& key, int recordID) {
    auto lb = std::lower_bound(keys.begin(), keys.end(), key);
    if (lb != keys.begin() && *lb == key) return false;
    records.insert(lb - keys.begin() + records.begin(), recordID);
    keys.insert(lb, key);
    return true;
}

// take care of type
template <class T>
int BPTreeNode<T>::deleteKeyFromNode(const T& key) {
    auto it = std::find(keys.begin(), keys.end(), key);
    if (it == keys.end()) {
        return NOT_VALID;
    }
    records.erase(it - keys.begin() + records.begin());
    keys.erase(it);
    return keys.front();
}

template <class T>
BPTreeNode<T>* BPTreeNode<T>::getChildWithKey(const T& key) {
    if (isLeaf) return NULL;
    auto lb = std::lower_bound(keys.begin(), keys.end(), key);
    if (lb == keys.end()) return children.back();
    return (key < *lb) ? children[lb - keys.begin()] : children[lb - keys.begin() + 1];
}

template <class T>
void BPTreeNode<T>::pushKeyAndRecord(const T& key, int recordID) {
    keys.push_back(key);
    records.push_back(recordID);
}

// send ceil((n-1)/2) up to the parent and pointers from (ceil((n-1)/2) + 1) to ... send to splitNode
template <class T>
BPTreeNode<T>* BPTreeNode<T>::splitNonLeaf() {
    const auto upIndex = std::ceil((N - 1.0) / 2);
    const auto keyLen = keys.size();
    const auto upKey = keys[upIndex];
    cout << upIndex;
    BPTreeNode<T>* splitNode = new BPTreeNode<T>(parent, N, isLeaf);
    for (int i = upIndex + 1; i < keyLen; i++) {
        splitNode -> keys.push_back(keys[upIndex + 1]);
        splitNode -> children.push_back(children[upIndex + 1]);
        children[upIndex + 1] -> parent = splitNode;
        keys.erase(upIndex + 1 + keys.begin());
        children.erase(upIndex + 1 + children.begin());
    }
    splitNode -> children.push_back(children[upIndex + 1]);
    children[upIndex + 1] -> parent = splitNode;
    children.erase(upIndex + 1 + children.begin());
    keys.erase(upIndex + keys.begin());

    splitNode -> rightNode = rightNode;
    splitNode -> leftNode = this;
    if (rightNode != NULL) {
        rightNode -> leftNode = splitNode;
    }
    rightNode = splitNode;

    if (parent == NULL) {
        BPTreeNode<T>* newRoot = new BPTreeNode<T>(NULL, N, false);
        newRoot -> keys.push_back(upKey);
        newRoot -> children.push_back(this);
        newRoot -> children.push_back(splitNode);
        parent = newRoot;
        splitNode -> parent = newRoot;
        return newRoot;
    }
    auto& parentKeys = parent -> keys;
    auto& parentChild = parent -> children;
    auto it = std::lower_bound(parentKeys.begin(), parentKeys.end(), upKey);
    parentChild.insert(it - parentKeys.begin() + parentChild.begin() + 1, splitNode);
    parentKeys.insert(it, upKey);
    return parent;
}

// split overload node and insert smallest key of the new node into parent
template <class T>
BPTreeNode<T>* BPTreeNode<T>::splitLeaf() {
    BPTreeNode<T>* splitNode = new BPTreeNode<T>(parent, N, isLeaf);
    const int len = keys.size();
    const int leftLen = std::ceil(len / 2.0);
    for (int i = leftLen; i < len; i++) {
        splitNode -> pushKeyAndRecord(keys[leftLen], records[leftLen]);
        keys.erase(keys.begin() + leftLen);
        records.erase(records.begin() + leftLen);
    }

    splitNode -> rightNode = rightNode;
    splitNode -> leftNode = this;
    if (rightNode != NULL) {
        rightNode -> leftNode = splitNode;
    }
    rightNode = splitNode;

    if (parent == NULL) {
        BPTreeNode<T>* newRoot = new BPTreeNode<T>(NULL, N, false);
        newRoot -> pushKeyAndRecord(splitNode -> keys[0], splitNode -> records[0]);
        newRoot -> children.push_back(this);
        newRoot -> children.push_back(splitNode);
        parent = newRoot;
        splitNode -> parent = newRoot;
        return newRoot;
    }
    auto& parentKeys = parent -> keys;
    auto& parentRecord = parent -> records;
    auto& parentChild = parent -> children;
    auto it = std::lower_bound(parentKeys.begin(), parentKeys.end(), splitNode -> keys[0]);
    if (it == parentKeys.end()) {
        parentRecord.push_back(splitNode -> records[0]);
        parentChild.push_back(splitNode);
        parentKeys.push_back(splitNode -> keys[0]);
        return parent;
    }
    parentRecord.insert(it - parentKeys.begin() + parentRecord.begin(), splitNode -> records[0]);
    parentChild.insert(it - parentKeys.begin() + parentChild.begin() + 1, splitNode);
    parentKeys.insert(it, splitNode -> keys[0]);
    return parent;
}

template <class T>
BPTreeNode<T>* BPTreeNode<T>::split() {
    return isLeaf ? splitLeaf() : splitNonLeaf();
}

template <class T>
bool BPTreeNode<T>::tryReplaceKey(const T& oldKey, const T& newKey) {
    auto it = std::lower_bound(keys.begin(), keys.end(), oldKey);
    if (it != keys.end() && *it == oldKey) {
        keys[it - keys.begin()] = newKey;
        return true;
    }
    return false;
}

// template <class T>
// void BPTreeNode<T>::redistribute(BPTreeNode<T>* left, BPTreeNode<T>* right) {
//     if (left -> keyNum() < right -> keyNum()) {

//     } else {

//     }
// }

template <class T>
void deleteNonLeaf(int pos, BPTreeNode<T>* node) {
    auto& nodeKeys = node -> keys;
    auto& nodeChild = node -> children;
    nodeKeys.erase(nodeKeys.begin() + pos);
    delete nodeChild[pos + 1];
    nodeChild.erase(nodeChild.begin() + pos + 1);
    BPTreeNode<T>* parent = node -> parent;
    if (node -> getParent() == NULL || (node -> keyNum() >= std::floor((node -> N + 1.0) / 2) - 1)) {
        return;
    }
    if (node -> leftNode && node -> leftNode -> parent == node -> parent && node -> leftNode -> keyNum() >= std::floor((node -> N + 1.0) / 2)) {
        int pos;
        for (pos = 0; pos < parent -> children.size(); pos++) {
            if (parent -> children[pos] == node -> leftNode) {
                break;
            }
        }
        nodeKeys.insert(nodeKeys.begin(), parent -> keys[pos]);
        parent -> keys[pos] = node -> leftNode -> keys.back();
        node -> leftNode -> keys.pop_back();
        nodeChild.insert(nodeChild.begin(), node -> leftNode -> children.back());
        node -> leftNode -> children.pop_back();
        nodeChild[0] -> parent = node;
    } else if (node -> rightNode && node -> rightNode -> parent == node -> parent && node -> rightNode -> keyNum() >= std::floor((node -> N + 1.0) / 2)) {
        int pos;
        for (pos = 0; pos < parent -> children.size(); pos++) {
            if (parent -> children[pos] == node) {
                break;
            }
        }
        nodeKeys.push_back(parent -> keys[pos]);
        parent -> keys[pos] = node -> rightNode -> keys.front();
        node -> rightNode -> keys.erase(node -> rightNode -> keys.begin());
        nodeChild.push_back(node -> rightNode -> children.front());
        nodeChild.back() -> parent = node;
        node -> rightNode -> children.erase(node -> rightNode -> children.begin());
    } else if (node -> leftNode && node -> leftNode -> parent == node -> parent) {
        int pos;
        for (pos = 0; pos < parent -> children.size(); pos++) {
            if (parent -> children[pos] == node -> leftNode) {
                break;
            }
        }
        node -> leftNode -> keys.push_back(parent -> keys[pos]);
        for (int i = 0; i < nodeKeys.size(); i++) {
            node -> leftNode -> keys.push_back(node -> keys.front());
            node -> children.front() -> parent = node -> leftNode;
            node -> leftNode -> children.push_back(node -> children.front());
            node -> keys.erase(node -> keys.begin());
            node -> children.erase(node -> children.begin());
        }
        node -> children.front() -> parent = node -> leftNode;
        node -> leftNode -> children.push_back(node -> children[0]);
        node -> children.erase(node -> children.begin());
        deleteNonLeaf<T>(pos, parent);
    } else if (node -> rightNode && node -> rightNode -> parent == node -> parent) {
        int pos;
        for (pos = 0; pos < parent -> children.size(); pos++) {
            if (parent -> children[pos] == node) {
                break;
            }
        }
        node -> keys.push_back(parent -> keys[pos]);
        for (int i = 0; i < node -> rightNode -> keys.size(); i++) {
            node -> keys.push_back(node -> rightNode -> keys.front());
            node -> children.push_back(node -> rightNode -> children.front());
            node -> children.back() -> parent = node;
            node -> rightNode -> keys.erase(node -> rightNode -> keys.begin());
            node -> rightNode -> children.erase(node -> rightNode -> children.begin());
        }
        node -> children.push_back(node -> rightNode -> children.front());
        node -> children.back() -> parent = node;
        node -> rightNode -> children.erase(node -> rightNode -> children.begin());
        deleteNonLeaf<T>(pos, parent);
    }
}

template <class T>
void BPTreeNode<T>::inflateLeaf() {
    int thisSize = keys.size();
    if (leftNode && leftNode -> parent == parent && leftNode -> keyNum() >= floor((N + 1.0) / 2) + 1) {
        keys.insert(keys.begin(), leftNode -> keys.back());
        records.insert(records.begin(), leftNode -> records.back());
        leftNode -> keys.pop_back();
        leftNode -> records.pop_back();
        int pos;
        for (pos = 0; pos < parent -> children.size(); pos++) {
            if (parent -> children[pos] == leftNode) {
                break;
            }
        }
        parent -> keys[pos] = keys.front();
    } else if (rightNode && rightNode -> parent == parent && rightNode -> keyNum() >= floor((N + 1.0) / 2) + 1) {
        keys.push_back(rightNode -> keys.front());
        records.push_back(rightNode -> records.front());
        rightNode -> keys.erase(rightNode -> keys.begin());
        rightNode -> children.erase(rightNode -> children.begin());
        int pos;
        for (pos = 0; pos < parent -> children.size(); pos++) {
            if (parent -> children[pos] == this) {
                break;
            }
        }
        parent -> keys[pos] = rightNode -> keys.front();
    } else if (leftNode && leftNode -> parent == parent) { // BUG HERE!!!!!! sibling is not linked list
        for (int i = 0; i < thisSize; i++) {
            leftNode -> keys.push_back(keys.front());
            leftNode -> records.push_back(records.front());
            keys.erase(keys.begin());
            records.erase(records.begin());
        }
        leftNode -> rightNode = rightNode;
        if (rightNode) rightNode -> leftNode = leftNode;
        int pos;
        for (pos = 0; pos < parent -> children.size(); pos++) {
            if (parent -> children[pos] == leftNode) {
                break;
            }
        }
        deleteNonLeaf<T>(pos, parent);
    } else if (rightNode && rightNode -> parent == parent) {
        thisSize = rightNode -> keyNum();
        for (int i = 0; i < thisSize; i++) {
            keys.push_back(rightNode -> keys[i]);
            records.push_back(rightNode -> records[i]);
            rightNode -> keys.erase(rightNode -> keys.begin());
            rightNode -> records.erase(rightNode -> records.begin());
        }
        if (rightNode -> rightNode) rightNode -> rightNode -> leftNode = this;
        rightNode = rightNode -> rightNode;
        int pos;
        for (pos = 0; pos < parent -> children.size(); pos++) {
            if (parent -> children[pos] == this) {
                break;
            }
        }
        deleteNonLeaf<T>(pos, parent);
    }
    // root and leaf
}

// B+ Tree
template <class T>
class BPTree {
    using BPNodePtr = BPTreeNode<T>*;
private:
    BPNodePtr rt;
    BPNodePtr findNodeWithKey(const T& key);       // returns b+ node pointer and position in node
    const int N;            

public:
    BPTree(int _N);
    BPNodePtr root() {
        return rt;
    }
    int findRecordWithKey(const T& key);                     // get the record for key
    bool insertRecordWithKey(const T& key, int recordID);    // insert key-record pair into b+
    bool deleteRecordWithKey(const T& key);                  // delete key-record pair from b+

    void print();                                            // debug
    void DFS(BPNodePtr x, int d);
};

template <class T>
BPTree<T>::BPTree(int _N) : N(_N){
    rt = new BPTreeNode<T>(NULL, _N, true);
};

template <class T>
BPTreeNode<T>* BPTree<T>::findNodeWithKey(const T& key) {
    BPNodePtr curNode = rt;
    BPNodePtr nextNode = curNode -> getChildWithKey(key);
    while(nextNode) {
        curNode = nextNode;
        nextNode = curNode -> getChildWithKey(key);
    }
    return curNode;
}

template <class T>
int BPTree<T>::findRecordWithKey(const T& key) {
    BPNodePtr node = findNodeWithKey(key);
    if (!node) return NOT_VALID;
    return node -> findRecordWithKey(key);
}

// take care that the overload/hungry check is handled by BPTree
template <class T>
bool BPTree<T>::insertRecordWithKey(const T& key, int recordID) {
    BPNodePtr toInsert = findNodeWithKey(key);
    if (!toInsert -> addKeyIntoNode(key, recordID)) return false;
    BPNodePtr current = toInsert;
    while(current -> isOverLoad()) {
        current = current -> split();
        if (current -> getParent() == NULL) {
            rt = current;
        }
    }
    return true;
}

template <class T>
bool BPTree<T>::deleteRecordWithKey(const T& key) {
    BPNodePtr toDelete = findNodeWithKey(key);
    if (!toDelete) return false;
    T newFront = toDelete -> deleteKeyFromNode(key);
    if (newFront == NOT_VALID) return false;
    // if (!toDelete -> isHungry()) return true;
    // TODO: merge/redistribute and propagate
    if (toDelete -> keyNum() >= std::floor((N + 1.0) / 2)) {
        return true;
    }
    toDelete -> inflateLeaf();
    if (rt -> children.size() == 1) {
        rt = rt -> children[0];
    }
    return true;
}

// debug
template <class T>
void BPTree<T>::print() {
    DFS(rt, 0);
}

template <class T>
void BPTree<T>::DFS(BPNodePtr x, int d) {
    cout << "depth: " << d << " | ";
    for (auto el : x -> keys) {
        cout << el << ' ';
    }
    cout << std::endl;
    for (auto ptr : x -> children) {
        DFS(ptr, d + 1);
    }
}

// 0 1 0 2 0 3 0 4 0 6 0 7 0 8 0 9 1 6