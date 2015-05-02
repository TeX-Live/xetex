#include <stdio.h>

int main(void)
{
    putc(82, stdout);
    putc(10, stdout);
    putc(0x107, stdout);
    putc(10, stdout);
    return 0;
}
