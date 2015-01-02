
#include <Windows.h>
#include <stdio.h>
#include <intrin.h>

#ifdef _M_IX86
#pragma intrinsic(__readfsdword)
#define _get_process_id() __readfsdword(0x20)
#define _get_thread_id() __readfsdword(0x24)
#elif defined(_M_X64)
#pragma intrinsic(__readgsdword)
#define _get_process_id() __readgsdword(0x40)
#define _get_thread_id() __readgsdword(0x48)
#endif

int
main(int argc, char **argv)
{
    DWORD pid1, pid2, tid1, tid2 = 0;

    pid1 = GetCurrentProcessId();
    tid1 = GetCurrentThreadId();
    printf("PID1: %d, TID1: %d\n", pid1, tid1);
    pid2 = _get_process_id();
    tid2 = _get_thread_id();
    printf("PID2: %d, TID2: %d\n", pid2, tid2);

    return 0;
}
