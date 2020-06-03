#ifndef BUFFER_MANAGER_HEADER
#define BUFFER_MANAGER_HEADER

#include "Page.hpp"
#include <string>

class BufferManager
{
private:
    Page *pages;

    int getEmptyPageId();

public:
    BufferManager();
    ~BufferManager();
    // find an empty page
    // if no such page exists
    // throw one in buffer out
    int getPageId(const std::string &filename, int x);
    char *getPageAddress(int pageId);
    void pinPage(int pageId);
    void dirtPage(int pageId);
    void unpinPage(int pageId);
    void undirtPage(int pageId);
    void deletePageWithName(const std::string &filename);
};

extern BufferManager buf_mgr;
#endif