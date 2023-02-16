#include <windows.h>
#include <commctrl.h>
#include <dlgs.h>
#include <string.h>
#include <tchar.h>
#include <stdlib.h>
#include <process.h>

#include "htmlconv.h"

static tstring g_log;

static VOID AddFile(HWND hDlg, LPCTSTR pszFileName)
{
    TCHAR szFullPath[MAX_PATH];
    LPTSTR pch, pchExt;
    LV_ITEM lvi;
    ITEM* pItem;

    GetFullPathName(pszFileName, MAX_PATH, szFullPath, &pch);
    pchExt = _tcsrchr(pch, '.');
    if (pchExt == NULL)
        pchExt = TEXT("(なし)");
    else
        pchExt++;

    INT nType = GetFileType(pszFileName);
    if (nType == FILETYPE_HTML || 
        nType == FILETYPE_JS ||
        nType == FILETYPE_CSS)
    {
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = ListView_GetItemCount(GetDlgItem(hDlg, ctl1));
        lvi.iSubItem = 0;
        lvi.pszText = pch;
        pItem = new ITEM(pch, pchExt, szFullPath);
        lvi.lParam = (LPARAM)pItem;
        INT i = ListView_InsertItem(GetDlgItem(hDlg, ctl1), &lvi);
        if (i != -1)
        {
            ListView_SetItemText(GetDlgItem(hDlg, ctl1), i, 1, pchExt);
            ListView_SetItemText(GetDlgItem(hDlg, ctl1), i, 2, szFullPath);
        }
    }
}

static VOID AddDir(HWND hDlg, LPCTSTR pszDir)
{
    set<tstring> files;
    set<tstring>::iterator it, end;
    TCHAR szWildCard[128] = TEXT("*.htm;*.html;*.js;*.css");
    TCHAR szCurDir[MAX_PATH];
    TCHAR szFullPath[MAX_PATH];
    LPTSTR pch;

    GetCurrentDirectory(MAX_PATH, szCurDir);
    if (SetCurrentDirectory(pszDir))
    {
        HANDLE hFind;
        WIN32_FIND_DATA find;
        LPTSTR pch;
        pch = _tcstok(szWildCard, _T(";"));
        while(pch != NULL)
        {
            hFind = FindFirstFile(szWildCard, &find);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (lstrcmp(find.cFileName, TEXT(".")) != 0 &&
                        lstrcmp(find.cFileName, TEXT("..")) != 0)
                    {
                        if (!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                        {
                            GetFullPathName(find.cFileName, MAX_PATH, szFullPath, &pch);
                            files.insert(szFullPath);
                        }
                    }
                } while(FindNextFile(hFind, &find));
                FindClose(hFind);
            }
            pch = _tcstok(NULL, _T(";"));
        }
        SetCurrentDirectory(szCurDir);

        end = files.end();
        for(it = files.begin(); it != end; it++)
        {
            AddFile(hDlg, it->c_str());
        }

        if (SetCurrentDirectory(pszDir))
        {
            hFind = FindFirstFile(TEXT("*"), &find);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (lstrcmp(find.cFileName, TEXT(".")) != 0 &&
                        lstrcmp(find.cFileName, TEXT("..")) != 0)
                    {
                        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        {
                            AddDir(hDlg, find.cFileName);
                        }
                    }
                } while(FindNextFile(hFind, &find));
                FindClose(hFind);
            }
            SetCurrentDirectory(szCurDir);
        }
    }
}

static VOID OnDropFiles(HWND hDlg, HDROP hDrop)
{
    UINT i, c;
    TCHAR szFile[MAX_PATH];
    DWORD attrs;

    c = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
    for(i = 0; i < c; i++)
    {
        DragQueryFile(hDrop, i, szFile, MAX_PATH);
        attrs = GetFileAttributes(szFile);
        if (attrs != 0xFFFFFFFF)
        {
            if (attrs & FILE_ATTRIBUTE_DIRECTORY)
                AddDir(hDlg, szFile);
            else
                AddFile(hDlg, szFile);
        }
    }
    DragFinish(hDrop);
}

static VOID UpdateLog(VOID)
{
    SetDlgItemText(g_hConvertingDlg, edt1, g_log.c_str());
    SendDlgItemMessage(g_hConvertingDlg, edt1, EM_SETSEL,
        g_log.size(), g_log.size());
    SendDlgItemMessage(g_hConvertingDlg, edt1, EM_SCROLLCARET, 0, 0);
}

static VOID WriteLog(INT ids, LPCTSTR pszFile)
{
    TCHAR szFmt[256];
    TCHAR sz[1024];
    LoadString(g_hInstance, ids, szFmt, 256);
    wsprintf(sz, szFmt, pszFile);
    g_log += sz;
}

static unsigned __stdcall convert_proc(void *)
{
    HWND hDlg;
    set<tstring> htmls, stylesheets, scripts;
    set<wstring> stylesheetsW, scriptsW;
    set<tstring>::iterator it, end;
    set<wstring>::iterator itW, endW;
    size_t i, c;
    INT type, iCode;
    LPTSTR pszFile;
    INT cSuccess, cError;
    TCHAR szFmt[256], sz[256];

    hDlg = g_hMainWnd;
    switch((INT)SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL, 0, 0))
    {
    case 0:
        iCode = SJIS;
        break;

    case 1:
        iCode = EUC;
        break;

    case 2:
        iCode = UTF8;
    	break;

    default:
        return 0;
    }

    g_nl = (INT)SendDlgItemMessage(hDlg, cmb2, CB_GETCURSEL, 0, 0);
    g_fCSS = (IsDlgButtonChecked(hDlg, chx1) == BST_CHECKED);
    g_fJS = (IsDlgButtonChecked(hDlg, chx2) == BST_CHECKED);
    g_fBackup = (IsDlgButtonChecked(hDlg, chx3) == BST_CHECKED);

    TCHAR szPath[MAX_PATH];
    HWND hCtl1 = GetDlgItem(hDlg, ctl1);
    c = ListView_GetItemCount(hCtl1);
    for(i = 0; i < c; i++)
    {
        ListView_GetItemText(hCtl1, i, 2, szPath, MAX_PATH);
        type = GetFileType(szPath);
        switch(type)
        {
        case FILETYPE_HTML:
            htmls.insert(szPath);
            break;

        case FILETYPE_JS:
            scripts.insert(szPath);
            break;

        case FILETYPE_CSS:
            stylesheets.insert(szPath);
            break;
        }
    }

    cSuccess = cError = 0;
    LoadString(g_hInstance, IDS_START, sz, 256);
    g_log = sz;
    UpdateLog();

    if (!g_fCancelling)
    {
        end = htmls.end();
        for(it = htmls.begin(); it != end; it++)
        {
            if (g_fCancelling)
                break;
            if (ConvertHtmlFile(it->c_str(), iCode, stylesheetsW, scriptsW))
            {
                WriteLog(IDS_SUCCESS, it->c_str());
                cSuccess++;
            }
            else
            {
                WriteLog(g_error, it->c_str());
                cError++;
            }
            UpdateLog();
        }
    }

    if (!g_fCancelling)
    {
        end = stylesheets.end();
        for(it = stylesheets.begin(); it != end; it++)
        {
            if (g_fCancelling)
                break;
            if (ConvertCSSFile(it->c_str(), iCode))
            {
                WriteLog(IDS_SUCCESS, it->c_str());
                cSuccess++;
            }
            else
            {
                WriteLog(g_error, it->c_str());
                cError++;
            }
            UpdateLog();
        }
    }

    if (!g_fCancelling)
    {
        end = scripts.end();
        for(it = scripts.begin(); it != end; it++)
        {
            if (g_fCancelling)
                break;
            if (ConvertJSFile(it->c_str(), iCode))
            {
                WriteLog(IDS_SUCCESS, it->c_str());
                cSuccess++;
            }
            else
            {
                WriteLog(g_error, it->c_str());
                cError++;
            }
            UpdateLog();
        }
    }

    if (!g_fCancelling)
    {
        endW = stylesheetsW.end();
        for(itW = stylesheetsW.begin(); itW != endW; itW++)
        {
            if (g_fCancelling)
                break;
            if (ConvertCSSFile(itW->c_str(), iCode))
            {
#ifdef UNICODE
                WriteLog(IDS_SUCCESS, itW->c_str());
#else
                WriteLog(IDS_SUCCESS, text_ansi_from_wide(*itW).c_str());
#endif
                cSuccess++;
            }
            else
            {
#ifdef UNICODE
                WriteLog(g_error, itW->c_str());
#else
                WriteLog(g_error, text_ansi_from_wide(*itW).c_str());
#endif
                cError++;
            }
            UpdateLog();
        }
    }

    if (!g_fCancelling)
    {
        endW = scriptsW.end();
        for(itW = scriptsW.begin(); itW != endW; itW++)
        {
            if (g_fCancelling)
                break;
            if (ConvertJSFile(itW->c_str(), iCode))
            {
#ifdef UNICODE
                WriteLog(IDS_SUCCESS, itW->c_str());
#else
                WriteLog(IDS_SUCCESS, text_ansi_from_wide(*itW).c_str());
#endif
                cSuccess++;
            }
            else
            {
#ifdef UNICODE
                WriteLog(g_error, itW->c_str());
#else
                WriteLog(g_error, text_ansi_from_wide(*itW).c_str());
#endif
                cError++;
            }
            UpdateLog();
        }
    }

    if (g_fCancelling)
    {
        LoadString(g_hInstance, IDS_CANCELLED, sz, 256);
        g_log += sz;
        UpdateLog();
    }
    else
    {
        LoadString(g_hInstance, IDS_STAT, szFmt, 256);
        wsprintf(sz, szFmt, cSuccess, cError);
        g_log += sz;
        UpdateLog();
    }

    SetDlgItemText(g_hConvertingDlg, IDCANCEL, TEXT("閉じる(&C)"));
    SetWindowText(g_hConvertingDlg, TEXT("変換結果"));
    g_fTerminated = TRUE;
    return cError == 0;
}

static BOOL Converting_OnInitDialog(HWND hDlg, LPARAM lParam)
{
    g_hConvertingDlg = hDlg;
    g_fCancelling = FALSE;
    g_fTerminated = FALSE;
    g_hThread = (HANDLE)_beginthreadex(NULL, 0, convert_proc, NULL, 0, NULL);
    return TRUE;
}

static VOID Converting_OnCancel(HWND hDlg)
{
    if (!g_fTerminated)
    {
        g_fCancelling = TRUE;
        g_hThread = NULL;
        SetDlgItemText(hDlg, IDCANCEL, TEXT("閉じる(&C)"));
        SetWindowText(g_hConvertingDlg, TEXT("変換結果"));
        g_fTerminated = TRUE;
    }
    else
    {
        EndDialog(hDlg, IDCANCEL);
        CloseHandle(g_hThread);
    }
}

static INT_PTR CALLBACK ConvertingDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        return Converting_OnInitDialog(hDlg, lParam);

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDCANCEL:
            Converting_OnCancel(hDlg);
            break;
        }
    }
    return FALSE;
}

static VOID Convert(HWND hDlg)
{
    EnableWindow(GetDlgItem(hDlg, psh1), FALSE);
    EnableWindow(GetDlgItem(hDlg, psh2), FALSE);
    EnableWindow(GetDlgItem(hDlg, psh3), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);

    BOOL f = FALSE;
    g_log.clear();
    DialogBoxParam(g_hInstance, MAKEINTRESOURCE(2), hDlg, ConvertingDlgProc, (LPARAM)&f);
    if (f)
    {
        TCHAR sz[256];
        LoadString(g_hInstance, IDS_CANNOT_START_THREAD, sz, 256);
        MessageBox(hDlg, sz, NULL, MB_ICONERROR);
    }

    EnableWindow(GetDlgItem(hDlg, psh1), TRUE);
    EnableWindow(GetDlgItem(hDlg, psh2), TRUE);
    EnableWindow(GetDlgItem(hDlg, psh3), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDCANCEL), TRUE);
}

static BOOL OnInitDialog(HWND hDlg)
{
    g_hMainWnd = hDlg;
    DragAcceptFiles(hDlg, TRUE);

    HICON hIcon, hIconSm;
    hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(1));
    hIconSm = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(1),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        0);
    SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);

#if 1
    COMBOBOXEXITEM cbex = { 0 };
    cbex.mask = CBEIF_TEXT;
    cbex.iItem = -1;
    cbex.pszText = TEXT("Shift_JIS");
    SendDlgItemMessage(hDlg, cmb1, CBEM_INSERTITEM, 0, (LPARAM)&cbex);
    cbex.pszText = TEXT("EUC-JP");
    SendDlgItemMessage(hDlg, cmb1, CBEM_INSERTITEM, 0, (LPARAM)&cbex);
    cbex.pszText = TEXT("UTF-8");
    SendDlgItemMessage(hDlg, cmb1, CBEM_INSERTITEM, 0, (LPARAM)&cbex);

    cbex.pszText = TEXT("変更なし");
    SendDlgItemMessage(hDlg, cmb2, CBEM_INSERTITEM, 0, (LPARAM)&cbex);
    cbex.pszText = TEXT("CR+LF (Windows)");
    SendDlgItemMessage(hDlg, cmb2, CBEM_INSERTITEM, 0, (LPARAM)&cbex);
    cbex.pszText = TEXT("LF (UNIX)");
    SendDlgItemMessage(hDlg, cmb2, CBEM_INSERTITEM, 0, (LPARAM)&cbex);
    cbex.pszText = TEXT("CR (Mac)");
    SendDlgItemMessage(hDlg, cmb2, CBEM_INSERTITEM, 0, (LPARAM)&cbex);
#else
    SendDlgItemMessage(hDlg, cmb1, CB_ADDSTRING, 0, (LPARAM)TEXT("Shift_JIS"));
    SendDlgItemMessage(hDlg, cmb1, CB_ADDSTRING, 0, (LPARAM)TEXT("EUC-JP"));
    SendDlgItemMessage(hDlg, cmb1, CB_ADDSTRING, 0, (LPARAM)TEXT("ISO-2022-JP (JIS)"));
    SendDlgItemMessage(hDlg, cmb1, CB_ADDSTRING, 0, (LPARAM)TEXT("UTF-8"));

    SendDlgItemMessage(hDlg, cmb2, CB_ADDSTRING, 0, (LPARAM)TEXT("変更なし"));
    SendDlgItemMessage(hDlg, cmb2, CB_ADDSTRING, 0, (LPARAM)TEXT("CR+LF (Windows)"));
    SendDlgItemMessage(hDlg, cmb2, CB_ADDSTRING, 0, (LPARAM)TEXT("LF (UNIX)"));
    SendDlgItemMessage(hDlg, cmb2, CB_ADDSTRING, 0, (LPARAM)TEXT("CR (Mac)"));
#endif

    SendDlgItemMessage(hDlg, cmb1, CB_SETCURSEL, 0, 0);
    SendDlgItemMessage(hDlg, cmb2, CB_SETCURSEL, 0, 0);

    CheckDlgButton(hDlg, chx1, BST_CHECKED);
    CheckDlgButton(hDlg, chx2, BST_CHECKED);
    CheckDlgButton(hDlg, chx3, BST_CHECKED);

    LV_COLUMN col;
    col.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    col.cx = 130;
    col.fmt = LVCFMT_LEFT;
    col.pszText = TEXT("ファイル名");
    ListView_InsertColumn(GetDlgItem(hDlg, ctl1), 0, &col);
    col.cx = 60;
    col.fmt = LVCFMT_LEFT;
    col.pszText = TEXT("拡張子");
    ListView_InsertColumn(GetDlgItem(hDlg, ctl1), 1, &col);
    col.cx = 800;
    col.fmt = LVCFMT_LEFT;
    col.pszText = TEXT("フルパス");
    ListView_InsertColumn(GetDlgItem(hDlg, ctl1), 2, &col);

    ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, ctl1),
        LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	for(INT i = 1; i < __argc; i++)
	{
#ifdef UNICODE
		DWORD attrs = GetFileAttributes(mywargv[i]);
		if (attrs != 0xFFFFFFFF)
		{
			if (attrs & FILE_ATTRIBUTE_DIRECTORY)
				AddDir(hDlg, mywargv[i]);
			else
				AddFile(hDlg, mywargv[i]);
		}
#else
		DWORD attrs = GetFileAttributes(_argv[i]);
		if (attrs != 0xFFFFFFFF)
		{
			if (attrs & FILE_ATTRIBUTE_DIRECTORY)
				AddDir(hDlg, _argv[i]);
			else
				AddFile(hDlg, _argv[i]);
		}
#endif
	}

    return TRUE;
}

static VOID OnPsh1(HWND hDlg)
{
    INT i, c;
    c = (INT)SendDlgItemMessage(hDlg, ctl1, LVM_GETSELECTEDCOUNT, 0, 0);
    for(i = c - 1; i >= 0; i--)
    {
        if (SendDlgItemMessage(hDlg, ctl1, LVM_GETITEMSTATE, i, LVIS_SELECTED) & LVIS_SELECTED)
        {
            SendDlgItemMessage(hDlg, ctl1, LVM_DELETEITEM, i, 0);
        }
    }
}

static VOID OnPsh2(HWND hDlg)
{
    SendDlgItemMessage(hDlg, ctl1, LVM_DELETEALLITEMS, 0, 0);
}

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    ITEM *pItem1 = (ITEM *)lParam1;
    ITEM *pItem2 = (ITEM *)lParam2;
    if (g_nSort1)
    {
        if (pItem1->first == pItem2->first)
            return 0;
        else if (pItem1->first < pItem2->first)
            return g_nSort1;
        else
            return -g_nSort1;
    }
    if (g_nSort2)
    {
        if (pItem1->second == pItem2->second)
            return 0;
        else if (pItem1->second < pItem2->second)
            return g_nSort2;
        else
            return -g_nSort2;
    }
    if (g_nSort3)
    {
        if (pItem1->third == pItem2->third)
            return 0;
        else if (pItem1->third < pItem2->third)
            return g_nSort3;
        else
            return -g_nSort3;
    }
    return 0;
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR pnmh;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        return OnInitDialog(hDlg);

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case psh1:
            OnPsh1(hDlg);
            break;

        case psh2:
            OnPsh2(hDlg);
            break;

        case psh3:
            Convert(hDlg);
            break;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        break;

    case WM_NOTIFY:
        pnmh = (LPNMHDR)lParam;
        if (pnmh->code == LVN_COLUMNCLICK)
        {
            NM_LISTVIEW *pnmv = (NM_LISTVIEW *)lParam;
            if (pnmv->iSubItem == 2)
            {
                if (g_nSort3)
                    g_nSort3 = -g_nSort3;
                else
                    g_nSort3 = -1;
                g_nSort1 = 0;
                g_nSort2 = 0;
            }
            else if (pnmv->iSubItem == 1)
            {
                if (g_nSort2)
                    g_nSort2 = -g_nSort2;
                else
                    g_nSort2 = -1;
                g_nSort3 = 0;
                g_nSort1 = 0;
            }
            else
            {
                if (g_nSort1)
                    g_nSort1 = -g_nSort1;
                else
                    g_nSort1 = -1;
                g_nSort2 = 0;
                g_nSort3 = 0;
            }
            ListView_SortItems(GetDlgItem(hDlg, ctl1), CompareFunc, 0);
        }
        else if (pnmh->code == LVN_DELETEITEM)
        {
            NM_LISTVIEW *pnmv = (NM_LISTVIEW *)lParam;
            ITEM *pItem = (ITEM *)pnmv->lParam;
            delete pItem;
        }
        break;

    case WM_DROPFILES:
        OnDropFiles(hDlg, (HDROP)wParam);
        break;
    }
    return FALSE;
}
