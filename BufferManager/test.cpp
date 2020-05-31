#include "BufferManager.hpp"

int main()
{
    BufferManager bm;
    for (int i = 0; i < 300; ++i)
    {
        int id = bm.getPageId("db.data", i);
        printf("%d -- %d\n", i, id);
        char *p = bm.getPageAddress(id);
        sprintf(p, "test data %d", i);
    }
    for (int i = 0; i < 300; ++i)
    {
        int id = bm.getPageId("db.data", i);
        printf("%d ", id);
        char *p = bm.getPageAddress(id);
        printf("%s\n", p);
    }
    return 0;
}