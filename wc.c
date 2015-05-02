#include <wchar.h>
#include <stdio.h>

int main(void)
{
    /*
    fputwc(0x107, stdout);
    putc(10, stdout);
    */
    wchar_t wc = getwchar();
    printf("%d\n", (int) wc);
    return 0;
}
