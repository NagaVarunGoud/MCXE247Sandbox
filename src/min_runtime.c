#include <stdint.h>

/* Prototypes */
int main(void);
void _start(void);
void __assert_func(const char *file,
                   int line,
                   const char *func,
                   const char *failedexpr);

/* Minimal entry point */
void _start(void)
{
    main();

    while (1)
    {
    }
}

/* Minimal assert handler */
void __assert_func(const char *file,
                   int line,
                   const char *func,
                   const char *failedexpr)
{
    (void)file;
    (void)line;
    (void)func;
    (void)failedexpr;

    while (1)
    {
    }
}
