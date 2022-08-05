#include "minic.h"

#include <stdio.h>

void gen(Function const* prog)
{
    (void)prog;
    printf("define i32 @main() #0 {\n");
    printf("ret i32 0\n");
    printf("}\n");
    return;
}
