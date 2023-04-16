#ifndef _UTEST_H
#define _UTEST_H

extern void utest_add(char *desc, void (*fp)());
extern void utest_run(void);
extern void utest_parse_args(int argc, char *argv[], char *target_arg, void (*fp)());

#define UTEST_ADD(fn) \
    extern void fn(); \
    utest_add(#fn, fn)
#endif
