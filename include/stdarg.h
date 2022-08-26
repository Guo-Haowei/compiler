#ifndef __STDARG_H__
#define __STDARG_H__

typedef struct {
    int gp_offset;
    int fp_offset;
    void* overflow_arg_area;
    void* reg_save_area;
} __va_elem;

typedef __va_elem va_list[1];
#define va_start(ap, last)            \
    *(ap) = *(__va_elem*)__va_area__; \
    ((void)0)
#define va_end(ap)

static void* __va_arg_gp(__va_elem* ap)
{
    void* r = (char*)ap->reg_save_area + ap->gp_offset;
    // void* r = (char*)ap->reg_save_area + (ap->gp_offset += 8);
    ap->gp_offset += 8;
    return r;
}

static void* __va_arg_mem(__va_elem* ap)
{
    *(int*)(0) = 1;
}

#define va_arg(AP, TYPE) \
    (*(TYPE*)(__builtin_reg_class(TYPE) == 0 ? __va_arg_gp(AP) : __va_arg_mem(AP)))

typedef va_list __builtin_va_list;

#endif