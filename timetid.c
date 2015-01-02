
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

#define COUNT 100000000

__int64
tid_api(int c)
{
    int i = 0;
    __int64 t = 0;
    DWORD pid = 0;
    DWORD tid = 0;

    for (i = 0; i <= c; i++) {
        pid = GetCurrentProcessId();
        tid = GetCurrentThreadId();
        t += pid + tid;
    }

    return t;
}

__int64
tid_native(int c)
{
    int i = 0;
    __int64 t = 0;
    DWORD pid = 0;
    DWORD tid = 0;

    for (i = 0; i <= c; i++) {
        pid = _get_process_id();
        tid = _get_thread_id();
        t += pid + tid;
    }

    return t;
}

void
assert(int i)
{
    if (i == 0) {
        printf("err");
        exit(1);
    }
}

double
tid_timeit(__int64 (*fn)(int), double freq, int c, int round)
{
    double t = 0.0;
    __int64 r = 0;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    Sleep(0);
    assert(QueryPerformanceCounter(&start));
    r = (*fn)(c);
    assert(QueryPerformanceCounter(&end));
    t = ((double)end.QuadPart-start.QuadPart)/freq;

    printf("round %d: %.3f [%lld]\n", round, t, r);
    return t;
}

int
main(int argc, char **argv)
{
    double r1, r2, pi, freq = 0.0;

    LARGE_INTEGER i;
    assert(QueryPerformanceFrequency(&i));
    freq = ((double)i.QuadPart)/1000.0;

    r1 = tid_timeit(&tid_api, freq, COUNT, 1);
    r2 = tid_timeit(&tid_native, freq, COUNT, 2);
    pi = ((r1-r2)/r1)*100.0;
    printf("improvement: %.2f\n", pi);
    return 0;
}

/*
int
old(int argc, char **argv)
{
    DWORD pid1, pid2, tid1, tid2 = 0;
    int i = 0;
    __int64 start = 0;
    __int64 end = 0;
    __int64 t = 0;

    Sleep(0);
    start = __rdtsc();
    for (i = 0; i <= COUNT; i++) {
        pid1 = GetCurrentProcessId();
        tid1 = GetCurrentThreadId();
        t += pid1 + tid1;
    }
    end = __rdtsc();
    printf("round 1: %d [%lld]\n", end - start, t);

    start = 0;
    end = 0;
    t = 0;

    Sleep(0);
    start = __rdtsc();
    for (i = 0; i <= COUNT; i++) {
        pid2 = _get_process_id();
        tid2 = _get_thread_id();
        t += pid2 + tid2;
    }
    end = __rdtsc();
    printf("round 2: %d [%lld]\n", end - start, t);

    return 0;
}
*/
