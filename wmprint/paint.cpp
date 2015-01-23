#include "stdafx.h"
#include <assert.h>

#include "resource.h"
#include "hookpaint.h"

////////////////////////////////////////////////////////

void PrintWindow(HWND hWnd)
{
    HDC hDCMem = CreateCompatibleDC(NULL);

    RECT rect;

    GetWindowRect(hWnd, & rect);

    HBITMAP hBmp = NULL;

    {
        HDC hDC = GetDC(hWnd);
        hBmp = CreateCompatibleBitmap(hDC, rect.right - rect.left, rect.bottom - rect.top);
        ReleaseDC(hWnd, hDC);
    }

    HGDIOBJ hOld = SelectObject(hDCMem, hBmp);
    SendMessage(hWnd, WM_PRINT, (WPARAM) hDCMem, PRF_CHILDREN | PRF_CLIENT | PRF_ERASEBKGND | PRF_NONCLIENT | PRF_OWNED);

    SelectObject(hDCMem, hOld);
    DeleteObject(hDCMem);

    OpenClipboard(hWnd);
 
    EmptyClipboard(); 
    SetClipboardData(CF_BITMAP, hBmp);
    CloseClipboard();
}


void OnDraw(HWND hWnd, HDC hDC)
{
	RECT rt;
	GetClientRect(hWnd, &rt);

    SelectObject(hDC, GetSysColorBrush(COLOR_INFOBK));
    Ellipse(hDC, rt.left+5, rt.top+5, rt.right-3, rt.bottom-5);
}

void OnPaint(HWND hWnd)
{
   	PAINTSTRUCT ps;
	
    HDC hDC = BeginPaint(hWnd, & ps);

    OnDraw(hWnd, hDC);

    EndPaint(hWnd, & ps);
}


void OnPaint(HWND hWnd, WPARAM wParam)
{
   	PAINTSTRUCT ps;
    HDC         hDC;

    if ( wParam==0 )
        hDC = BeginPaint(hWnd, & ps);
    else
        hDC = (HDC) wParam;

    OnDraw(hWnd, hDC);

    if ( wParam==0 )
        EndPaint(hWnd, & ps);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message) 
	{
		case WM_COMMAND:
			switch ( LOWORD(wParam) )
			{
				case IDM_EXIT:
				    DestroyWindow(hWnd);
				    break;

                case ID_FILE_PRINT:
                    PrintWindow(hWnd);
                    break;

				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_PAINT:
            OnPaint(hWnd);
			break;
		
        case WM_DESTROY:
			PostQuitMessage(0);
			break;

        default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}


LRESULT CALLBACK WndProc0(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_COMMAND:
			switch ( LOWORD(wParam) )
			{
				case IDM_EXIT:
				    DestroyWindow(hWnd);
				    break;

                case ID_FILE_PRINT:
                    PrintWindow(hWnd);
                    break;

				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

        case WM_PRINTCLIENT:
            SendMessage(hWnd, WM_PAINT, wParam, lParam);
            break;

		case WM_PAINT:
            OnPaint(hWnd, wParam);
			break;
		
        case WM_DESTROY:
			PostQuitMessage(0);
			break;

        default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    WNDCLASSEX wcex;

    memset(&wcex, 0, sizeof(wcex));
	
    wcex.cbSize         = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC) WndProc;
	wcex.hInstance		= hInstance;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR) IDC_PAINT;
	wcex.lpszClassName	= "Class";

    RegisterClassEx(&wcex);

    HWND hWnd = CreateWindow("Class", "WM_PRINT", WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    assert(hWnd);

    CPaintHook hook;

    hook.SubClass(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
   	
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}
