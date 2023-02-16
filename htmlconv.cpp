#include <windows.h>
#include <commctrl.h>

#include "htmlconv.h"

HINSTANCE g_hInstance;
HWND g_hMainWnd;
HWND g_hConvertingDlg;

int g_error;
int g_nl;
BOOL g_fCSS;
BOOL g_fJS;
BOOL g_fBackup;
BOOL g_fTerminated;
BOOL g_fCancelling;
HANDLE g_hThread;
OSVERSIONINFOA g_osver;
INT g_nSort1 = 0;
INT g_nSort2 = 0;
INT g_nSort3 = 0;

LPWSTR *mywargv;

INT WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     pszCmdLine,
    INT       nCmdShow)
{
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_LISTVIEW_CLASSES | ICC_USEREX_CLASSES;
    InitCommonControlsEx(&iccx);

    g_hInstance = hInstance;
    g_osver.dwOSVersionInfoSize = sizeof(g_osver);
    GetVersionExA(&g_osver);

    WNDCLASS wc;
    WNDCLASSEX wcx;
    if (!GetClassInfoA(hInstance, WC_COMBOBOXEXA, &wc) &&
        !GetClassInfoExA(hInstance, WC_COMBOBOXEXA, &wcx))
    {
        MessageBoxA(NULL, "ÉvÉçÉOÉâÉÄÇÕÇ±ÇÃOSÇ…ëŒâûÇµÇƒÇ¢Ç‹ÇπÇÒÅB", NULL, MB_ICONERROR);
        return 1;
    }

    if (g_osver.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        INT argc;
        mywargv = CommandLineToArgvW(GetCommandLineW(), &argc);
        DialogBoxW(hInstance, MAKEINTRESOURCEW(1), NULL, DialogProcW);
        GlobalFree(mywargv);
    }
    else
        DialogBoxA(hInstance, MAKEINTRESOURCEA(1), NULL, DialogProcA);
    return 0;
}
