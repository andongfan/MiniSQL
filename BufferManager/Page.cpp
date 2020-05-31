#include "Page.hpp"
#include <string>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <iostream>

Page::Page()
{
    isPinned = isDirty = false;
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
    FILE *fp = fopen(s.c_str(), "rb");
    filename = s;
    if (fp == nullptr)
    {
        std::cerr << "opening file " << s << " error" << std::endl;
        return false;
    }
    offset = x;
    fseek(fp, offset * BLOCK_SIZE, 0);
    int t = fread(data, BLOCK_SIZE, 1, fp);
    if (t != 1 && ferror(fp))
    {
        std::cerr << "reading file " << s << " error" << std::endl;
        return false;
    }
    fclose(fp);
    return true;
}

void Page::storeFile()
{
    FILE *fp = fopen(filename.c_str(), "wb");
    if (fp == nullptr)
    {
        std::cerr << "creating file " << filename << " error" << std::endl;
        return;
    }
    fseek(fp, offset * BLOCK_SIZE, 0);
    fwrite(data, BLOCK_SIZE, 1, fp);
    fclose(fp);
}