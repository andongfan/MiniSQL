#ifndef PAGE_HEADER
#define PAGE_HEADER

#include <ctime>
#include <string>

const int BLOCK_SIZE = 4096;

class Page // use the term "page" when it's in memory
{
    char data[BLOCK_SIZE];
    bool isPinned; // pinned pages won't be thrown out
    int isDirty;   // dirty pages are being written and cannot be thrown out
    // difference between pinned and dirty pages:
    // a page is pinned intentionally to increase efficiency
    // a page is set to dirty when DBMS tries to modfiy it
    time_t lastModifiedTime; // the latest timestamp during which the page is modified
    std::string filename;
    int offset;

public:
    Page();
    void setPin(const bool &);
    void setDirty(const int &);
    bool getPin();
    int getDirty();
    char *getMemoryAddress();
    clock_t getLastModifiedTime() const;
    std::string getFilename();
    int getOffset();
    bool loadFile(const std::string &, int x);
    void storeFile();
    // returns whether the loading is successful
};

#endif