#include <stdio.h>
#include <stdarg.h>

extern "C" void _puts(int num, ...) {
    va_list valist;

    va_start(valist, num);

    for (int i = 0; i < num; i++) {
        int n = va_arg(valist, int);

        printf("%d ", n);
    }
    printf("\n");

    va_end(valist);
}