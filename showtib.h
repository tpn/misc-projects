/* Original URL: http://www.microsoft.com/msj/archive/s2cea.htm#fig1 */
//===========================================================
// File: TIB.H
// Author: Matt Pietrek
// From: Microsoft Systems Journal "Under the Hood", May 1996
//===========================================================
#pragma pack(1)

typedef struct _EXCEPTION_REGISTRATION_RECORD
{
    struct _EXCEPTION_REGISTRATION_RECORD * pNext;
    FARPROC                                 pfnHandler;
} EXCEPTION_REGISTRATION_RECORD, *PEXCEPTION_REGISTRATION_RECORD;

typedef struct _TIB
{
PEXCEPTION_REGISTRATION_RECORD pvExcept; // 00h Head of exception record list
PVOID   pvStackUserTop;     // 04h Top of user stack
PVOID   pvStackUserBase;    // 08h Base of user stack

union                       // 0Ch (NT/Win95 differences)
{
    struct  // Win95 fields
    {
        WORD    pvTDB;         // 0Ch TDB
        WORD    pvThunkSS;     // 0Eh SS selector used for thunking to 16 bits
        DWORD   unknown1;      // 10h
    } WIN95;

    struct  // WinNT fields
    {
        PVOID SubSystemTib;     // 0Ch
        ULONG FiberData;        // 10h
    } WINNT;
} TIB_UNION1;

PVOID   pvArbitrary;        // 14h Available for application use
struct _tib *ptibSelf;      // 18h Linear address of TIB structure

union                       // 1Ch (NT/Win95 differences)
{
    struct  // Win95 fields
    {
        WORD    TIBFlags;           // 1Ch
        WORD    Win16MutexCount;    // 1Eh
        DWORD   DebugContext;       // 20h
        DWORD   pCurrentPriority;   // 24h
        DWORD   pvQueue;            // 28h Message Queue selector
    } WIN95;

    struct  // WinNT fields
    {
        DWORD unknown1;             // 1Ch
        DWORD processID;            // 20h
        DWORD threadID;             // 24h
        DWORD unknown2;             // 28h
    } WINNT;
} TIB_UNION2;

PVOID*  pvTLSArray;         // 2Ch Thread Local Storage array

union                       // 30h (NT/Win95 differences)
{
    struct  // Win95 fields
    {
        PVOID*  pProcess;     // 30h Pointer to owning process database
    } WIN95;
} TIB_UNION3;

} TIB, *PTIB;
#pragma pack()

/* vim:set ts=8 sw=4 sts=4 tw=78 et: */
