/* Original URL: http://www.microsoft.com/msj/archive/s2cea.htm#fig1 */
//==========================================================================
// File: SHOWTIB.CPP
// Author: Matt Pietrek
// To Build:
//  CL /MT SHOWTIB.CPP USER32.LIB (Visual C++)
//  BCC32 -tWM SHOWTIB.CPP (Borland C++, TASM32 required)
//==========================================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#pragma hdrstop
#include "showtib.h"

#define SHOWTIB_MAX_THREADS 64

CRITICAL_SECTION gDisplayTIB_CritSect;

void
test(void *p)
{
    printf("  test: 0x%08X\n", p);
}

typedef void (*PTEST)(void *p);

void DisplayTIB2(PSTR pszThreadName)
{
    PTIB pTIB;
    PTIB pTIB2;
    WORD fsSel;
    PTEST p1 = test;
    PTEST p2 = NULL;
    ULONG pfiberdata = NULL;
    void *pstacktop = NULL;
    void **x1 = NULL;
    void **x2 = NULL;

    EnterCriticalSection( &gDisplayTIB_CritSect );

    __asm
    {
        mov     EAX, FS:[0x18]
        mov     [pTIB], EAX
        mov     [fsSel], FS

        mov     EAX, FS:[0x10]
        mov     [pfiberdata], EAX

        mov     EAX, FS:[0x04]
        mov     [pstacktop], EAX
    }

    x1 = (void **)(pTIB);
    x2 = (void **)((ULONG)x1+16);
    printf("  TIB %04X (Address: 0x%x)\n", fsSel, pTIB );
    printf("  FiberData: 0x%x\n", &pTIB->TIB_UNION1.WINNT.FiberData );
    printf("  x1: 0x%x\n", x1);
    printf("  x1+0x10: 0x%x\n", x1+0x10);
    printf("  x1+16: 0x%x\n", x1+16);
    printf("  (ulong)x1+16: 0x%x\n", (ULONG)x1+16);
    printf("  x2: 0x%x\n", x2);
    printf("  *x2: 0x%x\n", *x2);
    printf("  (ulong)p1: 0x%x\n", (ULONG)p1);

    *x2 = (void *)p1;

    __asm
    {
        mov     EAX, FS:[0x18]
        mov     [pTIB], EAX
    }
    printf("  FiberData: 0x%x\n", &pTIB->TIB_UNION1.WINNT.FiberData );
    printf("  (ULONG)FiberData: 0x%x\n", (ULONG)pTIB->TIB_UNION1.WINNT.FiberData );
    printf("  x2: 0x%x\n", x2);
    printf("  *x2: 0x%x\n", *x2);

    printf("calling (*p2)");
    p2 = (PTEST)*x2;
    (*p2)(p2);



    LeaveCriticalSection( &gDisplayTIB_CritSect );
    return;



    /*printf(" addr of test: 0x%08X\n", pt);*/
    printf("Contents of thread %s (0x%08X)\n", pszThreadName, pszThreadName);

    printf( "  TIB %04X (Address: 0x%08X)\n", fsSel, pTIB );
    printf( "  SEH chain: 0x%08X\n", pTIB->pvExcept );
    printf( "  Stack top: 0x%08X\n", pTIB->pvStackUserTop );
    printf( "  Stack top 2: 0x%08X\n", pstacktop);
    printf( "  Stack base: 0x%08X\n", pTIB->pvStackUserBase );
    printf( "  pvArbitray: 0x%08X\n", pTIB->pvArbitrary );
    printf( "  TLS array *: 0x%08X\n", pTIB->pvTLSArray );

    printf("  SubSystem TIB: 0x%08X\n", pTIB->TIB_UNION1.WINNT.SubSystemTib);
    printf("  FiberData: 0x%08X\n", &pTIB->TIB_UNION1.WINNT.FiberData );
    printf("  FiberData2: 0x%08X\n", (void *)pfiberdata);
    printf("  FiberData3: 0x%08X\n", pTIB2);
    printf("  x1: 0x%08X\n", x1);
    printf("  &x1: 0x%08X\n", &x1);
    printf("  *x1: 0x%08X\n", *x1);
    printf("  unknown1: 0x%08X\n", pTIB->TIB_UNION2.WINNT.unknown1);
    printf("  process ID: 0x%08X\n", pTIB->TIB_UNION2.WINNT.processID);
    printf("  thread ID: 0x%08X\n", pTIB->TIB_UNION2.WINNT.threadID);
    printf("  unknown2: 0x%08X\n", pTIB->TIB_UNION2.WINNT.unknown2);

    printf( "\n" );

    LeaveCriticalSection( &gDisplayTIB_CritSect );
}

void SetFiberData(PSTR pszThreadName)
{
    PTIB pTIB;
    WORD fsSel;
    PTEST p1 = test;
    PTEST p2 = NULL;
    void *pfiberdata = NULL;
    void *pstacktop = NULL;

    EnterCriticalSection( &gDisplayTIB_CritSect );

    __asm
    {
        mov     EAX, [p1]
        mov     FS:[0x10], EAX
    }

    /*printf(" addr of test: 0x%08X\n", pt);*/
    printf("Contents of thread %s (0x%08X)\n", pszThreadName, pszThreadName);
    printf("  FiberData: 0x%08X\n", pTIB->TIB_UNION1.WINNT.FiberData );
    printf("  FiberData2: 0x%08X\n", pfiberdata);

    printf( "\n" );

    LeaveCriticalSection( &gDisplayTIB_CritSect );
}


void DisplayTIB( PSTR pszThreadName )
{
    PTIB pTIB;
    WORD fsSel;
    PTEST p1 = test;
    PTEST p2 = NULL;

    EnterCriticalSection( &gDisplayTIB_CritSect );

    __asm
    {
        mov     EAX, FS:[18h]
        mov     [pTIB], EAX
        mov     [fsSel], FS
    }

    printf(" addr of test: 0x%08X\n", p1);
    printf("Contents of thread %s (0x%08X)\n", pszThreadName, pszThreadName);

    printf( "  TIB %04X (Address: 0x%08X)\n", fsSel, pTIB );
    printf( "  SEH chain: 0x%08X\n", pTIB->pvExcept );
    printf( "  Stack top: 0x%08X\n", pTIB->pvStackUserTop );
    printf( "  Stack base: 0x%08X\n", pTIB->pvStackUserBase );
    printf( "  pvArbitray: 0x%08X\n", pTIB->pvArbitrary );
    printf( "  TLS array *: 0x%08X\n", pTIB->pvTLSArray );

    printf( "  ----OS Specific fields----\n" );
    if ( 0xC0000000 == (GetVersion() & 0xC0000000) )    // Is this Win95 ?
    {
        printf( "  TDB: %04X\n", pTIB->TIB_UNION1.WIN95.pvTDB );
        printf( "  Thunk SS: %04X\n", pTIB->TIB_UNION1.WIN95.pvThunkSS );
        printf( "  TIB flags: %04X\n", pTIB->TIB_UNION2.WIN95.TIBFlags );
        printf( "  Win16Mutex count: %04X\n",
                    pTIB->TIB_UNION2.WIN95.Win16MutexCount );
        printf( "  DebugContext: 0x%08X\n", pTIB->TIB_UNION2.WIN95.DebugContext );
        printf( "  Current Priority *: 0x%08X (%u)\n",
                    pTIB->TIB_UNION2.WIN95.pCurrentPriority,
                    *(PDWORD)(pTIB->TIB_UNION2.WIN95.pCurrentPriority) );
        printf( "  Queue: %04X\n", pTIB->TIB_UNION2.WIN95.pvQueue );
        printf( "  Process *: 0x%08X\n", pTIB->TIB_UNION3.WIN95.pProcess );
    }
    else if ( 0 == (GetVersion() & 0xC0000000) )    // Is this WinNT?
    {
        printf("  SubSystem TIB: 0x%08X\n", pTIB->TIB_UNION1.WINNT.SubSystemTib);
        printf("  FiberData: 0x%08X\n", pTIB->TIB_UNION1.WINNT.FiberData );
        printf("  unknown1: 0x%08X\n", pTIB->TIB_UNION2.WINNT.unknown1);
        printf("  process ID: 0x%08X\n", pTIB->TIB_UNION2.WINNT.processID);
        printf("  thread ID: 0x%08X\n", pTIB->TIB_UNION2.WINNT.threadID);
        printf("  unknown2: 0x%08X\n", pTIB->TIB_UNION2.WINNT.unknown2);
    }
    else
    {
        printf("  Unsupported Win32 implementation\n" );
    }

    printf( "\n" );

    LeaveCriticalSection( &gDisplayTIB_CritSect );
}

void MyThreadFunction( void * threadParam )
{
    char szThreadName[128];

    wsprintf( szThreadName, "%u", threadParam );    // Give the thread a name

    // If multiple threads are specified, give'em different priorities
    if ( (DWORD)threadParam & 1 )
        SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST );

    //DisplayTIB( szThreadName );     // Display the thread's TIB
    DisplayTIB2(szThreadName);
    //SetFiberData(szThreadName);

    // Let other threads execute while this thread is still alive.  The idea
    // here is to try and prevent memory region and selector reuse.
    Sleep( 1000 );
}

int main( int argc, char *argv[] )
{
    int i = 0;
    if ( argc < 2 )
    {
        printf( "Syntax: SHOWTIB [# of threads]\n" );
        return 1;
    }

    InitializeCriticalSection( &gDisplayTIB_CritSect );

    unsigned cThreads = atoi( argv[1] );

    if ( (cThreads < 1) || (cThreads > SHOWTIB_MAX_THREADS) )
    {
        printf( "thread count must be > 1 and < %u\n", SHOWTIB_MAX_THREADS );
    }
    else
    {
        // Allocate an array to hold the thread handles
        HANDLE threadHandles[ SHOWTIB_MAX_THREADS ];

        // Create the specified number of threads
        for (i = 0; i < cThreads; i++ )
            threadHandles[i] = (HANDLE)
                _beginthread(MyThreadFunction,0,(PVOID)i);

        // Wait for all the threads to finish before we exit the program
        WaitForMultipleObjects( cThreads, threadHandles, TRUE, INFINITE );

        // We don't need the thread handles anymore.  Close'em!
        for ( i = 0; i < cThreads; i++ )
            CloseHandle( threadHandles[i] );
    }

    DeleteCriticalSection( &gDisplayTIB_CritSect );

    return 0;
}

/* vim:set ts=8 sw=4 sts=4 tw=78 et: */
