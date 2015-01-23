// hook.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "hookpaint.h"


// Shared Data

#pragma data_seg("Shared")

HHOOK   h_WndProcHook  = NULL;

#pragma data_seg()

HINSTANCE hInstance;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    hInstance = (HINSTANCE) hModule;

    return TRUE;
}


LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    CWPRETSTRUCT * pInfo = (CWPRETSTRUCT *) (lParam - 4);

    if ((pInfo != NULL) && (pInfo->message == WM_USER) && (pInfo->lParam == 130869856))
    {
        CaptureWindow(pInfo->hwnd);
    }
    
    if (h_WndProcHook)
    {
        return CallNextHookEx(h_WndProcHook, nCode, wParam, lParam);
    }
    else
    {
        return 0;
    }
}


void _declspec(dllexport) InstallHook(void)
{
   	h_WndProcHook  = SetWindowsHookEx(WH_CALLWNDPROC, (HOOKPROC) CallWndProc, hInstance, 0);
}
