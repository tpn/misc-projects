// hooktest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>

void _declspec(dllimport) InstallHook(void);

int _tmain(int argc, _TCHAR* argv[])
{
    if ((argc == 2) && argv[1] != NULL)
    {
        HWND hWnd = NULL;

        sscanf(argv[1], "%x", & hWnd);

        TCHAR message[MAX_PATH];

        wsprintf(message, "Hook window %x ?", hWnd);

        if (MessageBox(NULL, message, "HookTest", MB_OK) == IDOK)
        {
            InstallHook();

            SendMessage(hWnd, WM_USER, 0, 130869856);

            MessageBox(NULL, "Check your clipboard\r\nClose your hooked application first.", "HookTest", MB_OK);

            return 0;
        }
    }

    printf("Usage HookTest <hwnd_id_in_hex>");

    return -1;
}

