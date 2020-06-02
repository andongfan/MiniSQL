#include "Page.hpp"
#include <string>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <iostream>

Page::Page()
{
    isPinned = isDirty = 0;
    offset = 0;
}

void Page::setPin(const bool &b)
{
    isPinned = b;
}

void Page::setDirty(const int &b)
{
    isDirty = std::max(0, b);
}

bool Page::getPin()
{
    return isPinned;
}

int Page::getDirty()
{
    return isDirty;
}

std::string Page::getFilename()
{
    return filename;
}

char *Page::getMemoryAddress()
{
    isDirty += 1;
    lastModifiedTime = clock();
    return (char *)data;
}

clock_t Page::getLastModifiedTime() const
{
    return lastModifiedTime;
}

int Page::getOffset()
{
    return offset;
}

bool Page::loadFile(const std::string &s, int x)
{
    // std::cerr << s << ' ' << x << std::endl;
    // std::cerr << "loading " << s << std::endl;
    // need to guarantee that the 1st to (x-1)th page was created before
    FILE *fp = fopen((s + ".data").c_str(), "rb+");
    if (fp == nullptr)
    {
        fp = fopen((s + ".data").c_str(), "wb+");
    }
    filename = s;
    offset = x;
    fseek(fp, offset * BLOCK_SIZE, 0);
    int t = fread(data, BLOCK_SIZE, 1, fp);
    if (t != 1 && ferror(fp))
    {
        std::cerr << "reading file " << s << " error" << std::endl;
        return false;
    }
    if (t != 1 && feof(fp))
    {
        fwrite("\0", 1, 1, fp);
    }
    fclose(fp);
    return true;
}

void Page::storeFile()
{
    FILE *fp = fopen((filename + ".data").c_str(), "rb+");
    if (fp == nullptr)
    {
        fp = fopen((filename + ".data").c_str(), "wb+");
        return;
    }
    fseek(fp, offset * BLOCK_SIZE, 0);
    fwrite(data, BLOCK_SIZE, 1, fp);
    fclose(fp);
}