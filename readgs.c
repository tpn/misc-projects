
#include <Windows.h>
#include <stdio.h>
#include <intrin.h>

#ifdef _M_IX86
#error This file is for x64 only.
#endif

int
main(int argc, char **argv)
{
    DWORD pid1, tid1, tid2 = 0;
    LONG base = 0;
    LONG off1, off2, off4, off8 = 0;
    BYTE b;
    WORD w;
    DWORD dw;
    __int64 qw;
    int i;

    pid1 = GetCurrentProcessId();
    tid1 = GetCurrentThreadId();
    printf("PID: %d, TID: %d\n", pid1, tid1);

    base = FIELD_OFFSET(NT_TIB, Self);
    printf("base: %d (0x%x)\n", base, base);

    for (i = 0; i <= 10; i++) {
        printf("[%d]\n", i);
        off1 = base + i;
        off2 = base + (i * 2);
        off4 = base + (i * 4);
        off8 = base + (i * 8);

        b  =  __readgsbyte(off1);
        w  =  __readgsword(off2);
        dw = __readgsdword(off4);
        qw = __readgsqword(off8);
        printf("  1/%d/0x%x: c:  %c\n", off1, off1, b);
        printf("  2/%d/0x%x: d:  %d, x: 0x%x\n", off2, off2, w, w);
        printf("  4/%d/0x%x: dw: %d, x: 0x%x\n", off4, off4, dw, dw);
        printf("  8/%d/0x%x: qw: %lld, llx: 0x%llx\n", off8, off8, qw, qw);
    }

    return 0;
}
