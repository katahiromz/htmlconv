#include <windows.h>
#include <cstdlib>
#include <cstring>
#include <tchar.h>
#include <mbstring.h>
#include <new>

#include "htmlconv.h"

LPSTR file_get_contents(LPCTSTR pszFileName, DWORD *pcb)
{
    LPSTR psz;
    HANDLE hFile;
    DWORD cbDone;
    g_error = IDS_SUCCESS;
    hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        *pcb = GetFileSize(hFile, NULL);
        if (*pcb != 0xFFFFFFFF)
        {
            psz = new CHAR[*pcb + 1];
            if (psz != NULL)
            {
                if (ReadFile(hFile, psz, *pcb, &cbDone, NULL))
                {
                    psz[*pcb] = '\0';
                    CloseHandle(hFile);
                    return psz;
                }
                delete[] psz;
            }
            else
                g_error = IDS_OUTOFMEMORY;
        }
        else
            g_error = IDS_CANNOT_OPEN;
        CloseHandle(hFile);
    }
    else
        g_error = IDS_CANNOT_OPEN;
    return NULL;
}

BOOL file_set_contents(LPCTSTR pszFileName, LPCSTR psz, DWORD cb)
{
    HANDLE hFile;
    DWORD cbDone;
    BOOL f = FALSE;
    g_error = IDS_SUCCESS;
    hFile = CreateFile(pszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (WriteFile(hFile, psz, cb, &cbDone, NULL))
            f = TRUE;
        f = (f && CloseHandle(hFile));
        if (!f)
        {
            DeleteFile(pszFileName);
            g_error = IDS_CANNOT_WRITE;
        }
    }
    else
        g_error = IDS_CANNOT_WRITE;
    return f;
}

BOOL MakeBackup(LPCTSTR pszFileName)
{
    TCHAR szBakFile[MAX_PATH];
    lstrcpy(szBakFile, pszFileName);
    lstrcat(szBakFile, TEXT(".bak"));
    if (GetFileAttributes(szBakFile) != 0xFFFFFFFF)
    {
        if (g_fBackup && !MakeBackup(szBakFile))
            return FALSE;
    }
    return CopyFile(pszFileName, szBakFile, FALSE);
}

bool ConvertTextFile(LPCSTR pszFileName, int iCode)
{
    DWORD cb;
    LPSTR pszContent;

    g_error = IDS_SUCCESS;
    try
    {
        string content;
        pszContent = file_get_contents(pszFileName, &cb);
        if (cb >= 2)
        {
            if ((BYTE)pszContent[0] == 0xFF && 
                (BYTE)pszContent[1] == 0xFE)
            {
                // UTF-16 little endian
                content = text_ansi_from_wide((LPWSTR)&pszContent[2]);
            }
            else if (pszContent[1] == 0x00)
            {
                // UTF-16 little endian
                content = css_ansi_from_wide((LPWSTR)pszContent);
            }
            else if ((BYTE)pszContent[0] == 0xFE && 
                     (BYTE)pszContent[1] == 0xFF)
            {
                // UTF-16 big endian
                for(DWORD i = 0; i < cb - 1; i += 2)
                {
                    CHAR ch = pszContent[i];
                    pszContent[i] = pszContent[i + 1];
                    pszContent[i + 1] = ch;
                }
                content = text_ansi_from_wide((LPWSTR)&pszContent[2]);
            }
            else if (pszContent[0] == 0x00)
            {
                // UTF-16 big endian
                for(DWORD i = 0; i < cb - 1; i += 2)
                {
                    CHAR ch = pszContent[i];
                    pszContent[i] = pszContent[i + 1];
                    pszContent[i + 1] = ch;
                }
                content = text_ansi_from_wide((LPWSTR)pszContent);
            }
        }
        if (content.empty())
            content = pszContent;
        delete[] pszContent;

        if (ConvertText(content, iCode))
        {
            if (!g_fBackup || MakeBackup(pszFileName))
                file_set_contents(pszFileName, content.c_str(), content.size());
            else
            {
                g_error = IDS_CANNOT_CREATE_BACKUP;
                return false;
            }
        }
    }
    catch(bad_alloc)
    {
        g_error = IDS_OUTOFMEMORY;
        return false;
    }

    return true;
}

bool ConvertCSSFile(LPCTSTR pszFileName, int iCode)
{
    DWORD cb;
    LPSTR pszContent;

    g_error = IDS_SUCCESS;
    try
    {
        string content;
        pszContent = file_get_contents(pszFileName, &cb);
        if (cb >= 2)
        {
            if ((BYTE)pszContent[0] == 0xFF && 
                (BYTE)pszContent[1] == 0xFE)
            {
                // UTF-16 little endian
                content = css_ansi_from_wide((LPWSTR)&pszContent[2]);
            }
            else if (pszContent[1] == 0x00)
            {
                // UTF-16 little endian
                content = css_ansi_from_wide((LPWSTR)pszContent);
            }
            else if ((BYTE)pszContent[0] == 0xFE && 
                     (BYTE)pszContent[1] == 0xFF)
            {
                // UTF-16 big endian
                for(DWORD i = 0; i < cb - 1; i += 2)
                {
                    CHAR ch = pszContent[i];
                    pszContent[i] = pszContent[i + 1];
                    pszContent[i + 1] = ch;
                }
                content = css_ansi_from_wide((LPWSTR)&pszContent[2]);
            }
            else if (pszContent[0] == 0x00)
            {
                // UTF-16 big endian
                for(DWORD i = 0; i < cb - 1; i += 2)
                {
                    CHAR ch = pszContent[i];
                    pszContent[i] = pszContent[i + 1];
                    pszContent[i + 1] = ch;
                }
                content = css_ansi_from_wide((LPWSTR)pszContent);
            }
        }

        if (content.empty())
            content = pszContent;
        delete[] pszContent;

        if (ConvertCSS(content, iCode))
        {
            if (!g_fBackup || MakeBackup(pszFileName))
                file_set_contents(pszFileName, content.c_str(), content.size());
            else
            {
                g_error = IDS_CANNOT_CREATE_BACKUP;
                return FALSE;
            }
        }
    }
    catch(bad_alloc)
    {
        g_error = IDS_OUTOFMEMORY;
        return FALSE;
    }

    return TRUE;
}

bool ConvertJSFile(LPCTSTR pszFileName, int iCode)
{
    DWORD cb;
    LPSTR pszContent;

    g_error = IDS_SUCCESS;
    try
    {
        string content;
        pszContent = file_get_contents(pszFileName, &cb);
        if (cb >= 2)
        {
            if ((BYTE)pszContent[0] == 0xFF && 
                (BYTE)pszContent[1] == 0xFE)
            {
                // UTF-16 little endian
                content = js_ansi_from_wide((LPWSTR)&pszContent[2]);
            }
            else if (pszContent[1] == 0x00)
            {
                // UTF-16 little endian
                content = js_ansi_from_wide((LPWSTR)pszContent);
            }
            else if ((BYTE)pszContent[0] == 0xFE && 
                     (BYTE)pszContent[1] == 0xFF)
            {
                // UTF-16 big endian
                for(DWORD i = 0; i < cb - 1; i += 2)
                {
                    CHAR ch = pszContent[i];
                    pszContent[i] = pszContent[i + 1];
                    pszContent[i + 1] = ch;
                }
                content = js_ansi_from_wide((LPWSTR)&pszContent[2]);
            }
            else if (pszContent[0] == 0x00)
            {
                // UTF-16 big endian
                for(DWORD i = 0; i < cb - 1; i += 2)
                {
                    CHAR ch = pszContent[i];
                    pszContent[i] = pszContent[i + 1];
                    pszContent[i + 1] = ch;
                }
                content = js_ansi_from_wide((LPWSTR)pszContent);
            }
        }

        if (content.empty())
            content = pszContent;
        delete[] pszContent;
        if (ConvertJS(content, iCode))
        {
            if (!g_fBackup || MakeBackup(pszFileName))
                file_set_contents(pszFileName, content.c_str(), content.size());
            else
            {
                g_error = IDS_CANNOT_CREATE_BACKUP;
                return FALSE;
            }
        }
    }
    catch(bad_alloc)
    {
        g_error = IDS_OUTOFMEMORY;
        return FALSE;
    }

    return TRUE;
}

bool ConvertHtmlFile(LPCTSTR pszFileName, int iCode,
    set<wstring>& stylesheets, set<wstring>& scripts)
{
    TCHAR szPath[MAX_PATH];
    LPTSTR pch;
    DWORD cb;
    LPSTR pszContent;

    GetFullPathName(pszFileName, MAX_PATH, szPath, &pch);
    pch = _tcsrchr(szPath, _T('\\'));
    if (pch != NULL)
        *pch = _T('\0');
    SetCurrentDirectory(szPath);

    g_error = IDS_SUCCESS;
    try
    {
        string content;
        pszContent = file_get_contents(pszFileName, &cb);
        if (cb >= 2)
        {
            if ((BYTE)pszContent[0] == 0xFF && 
                (BYTE)pszContent[1] == 0xFE)
            {
                // UTF-16 little endian
                content = html_ansi_from_wide((LPWSTR)&pszContent[2]);
            }
            else if (pszContent[1] == 0x00)
            {
                // UTF-16 little endian
                content = html_ansi_from_wide((LPWSTR)pszContent);
            }
            else if ((BYTE)pszContent[0] == 0xFE && 
                     (BYTE)pszContent[1] == 0xFF)
            {
                // UTF-16 big endian
                for(DWORD i = 0; i < cb - 1; i += 2)
                {
                    CHAR ch = pszContent[i];
                    pszContent[i] = pszContent[i + 1];
                    pszContent[i + 1] = ch;
                }
                content = html_ansi_from_wide((LPWSTR)&pszContent[2]);
            }
            else if (pszContent[0] == 0x00)
            {
                // UTF-16 big endian
                for(DWORD i = 0; i < cb - 1; i += 2)
                {
                    CHAR ch = pszContent[i];
                    pszContent[i] = pszContent[i + 1];
                    pszContent[i + 1] = ch;
                }
                content = html_ansi_from_wide((LPWSTR)pszContent);
            }
        }
        if (content.empty())
            content = pszContent;
        delete[] pszContent;
        if (ConvertHtml(content, iCode, stylesheets, scripts))
        {
            if (!g_fBackup || MakeBackup(pszFileName))
                file_set_contents(pszFileName, content.c_str(), content.size());
            else
            {
                g_error = IDS_CANNOT_CREATE_BACKUP;
                return false;
            }
        }
    }
    catch(bad_alloc)
    {
        g_error = IDS_OUTOFMEMORY;
        return false;
    }

    return true;
}

INT GetFileType(LPCTSTR pszFileName)
{
    LPCTSTR pch = (LPCTSTR)_tcsrchr(pszFileName, _T('\\'));
    if (pch == NULL)
        pch = pszFileName;
    pch = (LPCTSTR)_tcsrchr(pch, _T('.'));
    if (pch != NULL)
    {
        pch++;
        if (lstrcmpi(pch, TEXT("html")) == 0 ||
            lstrcmpi(pch, TEXT("htm")) == 0)
        {
            return FILETYPE_HTML;
        }
        else if (lstrcmpi(pch, TEXT("js")) == 0)
        {
            return FILETYPE_JS;
        }
        else if (lstrcmpi(pch, TEXT("css")) == 0)
        {
            return FILETYPE_CSS;
        }
        else if (lstrcmpi(pch, TEXT("vbs")) == 0)
        {
            return FILETYPE_VBS;
        }
        else if (
            lstrcmpi(pch, TEXT("jpg")) == 0 ||
            lstrcmpi(pch, TEXT("jpeg")) == 0 ||
            lstrcmpi(pch, TEXT("jpe")) == 0 ||
            lstrcmpi(pch, TEXT("jfif")) == 0 ||
            lstrcmpi(pch, TEXT("gif")) == 0 ||
            lstrcmpi(pch, TEXT("png")) == 0 ||
            lstrcmpi(pch, TEXT("tiff")) == 0 ||
            lstrcmpi(pch, TEXT("tif")) == 0 ||
            lstrcmpi(pch, TEXT("bmp")) == 0 ||
            lstrcmpi(pch, TEXT("dib")) == 0)
        {
            return FILETYPE_IMG;
        }
    }
    return FILETYPE_TEXT;
}
