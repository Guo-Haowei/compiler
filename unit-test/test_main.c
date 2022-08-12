#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

int array_test();
int fcache_test();
int list_test();

int main()
{

    array_test();
    fcache_test();
    list_test();

#ifdef _MSC_VER
    _CrtDumpMemoryLeaks();
#endif
    return 0;
}
