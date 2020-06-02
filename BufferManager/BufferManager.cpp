#include "BufferManager.hpp"
#include "Page.hpp"
#include <iostream>

const int FRAME_SIZE = 256 * 64;
BufferManager::BufferManager()
{
    pages = new Page[FRAME_SIZE];
}

BufferManager::~BufferManager()
{
    for (int i = 0; i < FRAME_SIZE; ++i)
    {
        if (pages[i].getDirty())
        {
            pages[i].storeFile();
        }
    }
    delete[] pages;
}

// returns -1 if no empty page is found
// returns page id if one empty page is found
int BufferManager::getEmptyPageId()
{
    for (int i = 0; i < FRAME_SIZE; ++i)
    {
        if (pages[i].getDirty())
        {
            pages[i].storeFile();
            pages[i].setDirty(0);
        }
    }
    int ret = -1; // returns -1 if there's no page available
    for (int i = 0; i < FRAME_SIZE; ++i)
    {
        if (not pages[i].getPin())
        {
            if (ret == -1 ||
                pages[i].getLastModifiedTime() < pages[ret].getLastModifiedTime())
            {
                ret = i;
            }
        }
    }
    return ret;
}

int BufferManager::getPageId(const std::string &filename, int x)
{
    for (int i = 0; i < FRAME_SIZE; ++i)
    {
        if (pages[i].getFilename() == filename && pages[i].getOffset() == x)
        { // block already in buffer, returns the head address
            return i;
        }
    }
    // block not in buffer, need to load
    int id = getEmptyPageId();
    if (id == -1)
    {
        std::cerr << "Buffer full!" << std::endl;
        return -1; // cannot load file into buffer
    }
    pages[id].loadFile(filename, x);
    return id;
}

char *BufferManager::getPageAddress(int pageId)
{
    return pages[pageId].getMemoryAddress();
}

void BufferManager::pinPage(int pageId)
{
    pages[pageId].setPin(true);
}

void BufferManager::dirtPage(int pageId)
{
    pages[pageId].setDirty(pages[pageId].getDirty() + 1);
}

void BufferManager::unpinPage(int pageId)
{
    pages[pageId].setPin(false);
}

<<<<<<< HEAD
BufferManager buf_mgr;
=======
void BufferManager::deletePageWithName(const std::string &filename)
{
    for (int i = 0; i < FRAME_SIZE; ++i)
    {
        if (pages[i].getFilename() == filename)
        {
            pages[i].setDirty(0);
            pages[i].setPin(0);
        }
    }
}
>>>>>>> master
