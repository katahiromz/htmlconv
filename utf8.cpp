#include <windows.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
using namespace std;

BOOL APIENTRY IsTextUTF8(CONST VOID *p, INT cb)
{
    INT cchTrail;
    BOOL f;
    CONST BYTE *pb = (CONST BYTE*)p;

    cchTrail = 0;
    f = FALSE;
    while(cb > 0)
    {
        if (*pb & 0x80)
        {
            if (cchTrail == 0)
            {
                if ((*pb & 0xE0) == 0xC0)       // 110?????(2)
                    cchTrail = 1;
                else if ((*pb & 0xF0) == 0xE0)  // 1110????(2)
                    cchTrail = 2;
                else if ((*pb & 0xF8) == 0xF0)  // 11110???(2)
                    cchTrail = 3;
                else if ((*pb & 0xFC) == 0xF8)  // 111110??(2)
                    cchTrail = 4;
                else if ((*pb & 0xFE) == 0xFC)  // 1111110?(2)
                    cchTrail = 5;
                else
                    return FALSE;
                f = TRUE;
            }
            else
            {
                if ((*pb & 0xC0) != 0x80)
                    return FALSE;
                cchTrail--;
            }
        }
        else
        {
            if (cchTrail != 0)
                return FALSE;
        }
        cb--;
        pb++;
    }
    return f;
}

LPWSTR APIENTRY ConvertUTF8ToUnicode(LPCSTR pszUTF8, INT cch, BOOL *pfCESU8)
{
    LPWSTR pszWide, pch;
    UINT wch, x;
    BYTE b0, b1;
    CONST BYTE *pb = (CONST BYTE *)pszUTF8;

    *pfCESU8 = FALSE;
    pch = pszWide = new WCHAR[cch + 1];
    while(cch > 0)
    {
        if ((*pb & 0x80) == 0)
        {
            *pch++ = (WCHAR)*pb;
            pb++;
            cch--;
        }
        else if (cch >= 2 && 
                 (pb[0] & 0xE0) == 0xC0 && 
                 (pb[1] & 0xC0) == 0x80)
        {
            b0 = (BYTE)((pb[0] << 6) | (pb[1] & 0x3F));
            b1 = (BYTE)((pb[0] & 0x1C) >> 2);
            *pch++ = MAKEWORD(b0, b1);
            pb++;
            pb++;
            cch--;
            cch--;
        }
        else if (cch >= 3 && 
                 (pb[0] & 0xF0) == 0xE0 && 
                 (pb[1] & 0xC0) == 0x80 &&
                 (pb[2] & 0xC0) == 0x80)
        {
            b0 = (BYTE)((pb[2] & 0x3F) | ((INT)(pb[1] & 0x03) << 6));
            b1 = (BYTE)(((INT)(pb[0] & 0x0F) << 4) | ((pb[1] & 0x3C) >> 2));
            *pch = MAKEWORD(b0, b1);
            if (0xD800 <= *pch && *pch <= 0xDBFF)
                *pfCESU8 = TRUE;
            pch++;
            pb += 3;
            cch -= 3;
        }
        else if (cch >= 4 &&
                 (pb[0] & 0xF8) == 0xF0 && 
                 (pb[1] & 0xC0) == 0x80 &&
                 (pb[2] & 0xC0) == 0x80 &&
                 (pb[3] & 0xC0) == 0x80)
        {
            wch = ((UINT)(pb[0] & 0x03) << 18) |
                  ((UINT)(pb[1] & 0x3F) << 12) |
                  ((UINT)(pb[2] & 0x3F) << 6) |
                   (UINT)(pb[3] & 0x3F);
            x = wch - 0x10000;
            *pch++ = (WORD)(x / 0x400 + 0xD800);
            *pch++ = (WORD)(x % 0x400 + 0xDC00);
            pb += 4;
            cch -= 4;
        }
        else if (cch >= 5 &&
                 (pb[0] & 0xFC) == 0xF8 && 
                 (pb[1] & 0xC0) == 0x80 &&
                 (pb[2] & 0xC0) == 0x80 &&
                 (pb[3] & 0xC0) == 0x80 &&
                 (pb[4] & 0xC0) == 0x80)
        {
            *pch++ = '?';
            pb += 5;
            cch -= 5;
        }
        else if (cch >= 6 &&
                 (pb[0] & 0xFE) == 0xFC && 
                 (pb[1] & 0xC0) == 0x80 &&
                 (pb[2] & 0xC0) == 0x80 &&
                 (pb[3] & 0xC0) == 0x80 &&
                 (pb[4] & 0xC0) == 0x80 &&
                 (pb[5] & 0xC0) == 0x80)
        {
            *pch++ = '?';
            pb += 6;
            cch -= 6;
        }
        else
        {
            *pch++ = '?';
            pb++;
            cch--;
            while(cch > 0 && (*pb & 0xC0) == 0x80)
            {
                pb++;
                cch--;
            }
        }
    }
    *pch = 0;
    return pszWide;
}

LPSTR APIENTRY ConvertUnicodeToUTF8(LPCWSTR pszWide, INT cch, BOOL fCESU8)
{
    INT i, c;
    LPSTR pszUTF8;
    BYTE *pb;
    UINT wch;

    c = 0;
    for(i = 0; i < cch; i++)
    {
        if (pszWide[i] <= 0x007F)
            c += 1;
        else if (0x0080 <= pszWide[i] && pszWide[i] <= 0x07FF)
            c += 2;
        else if (0x0800 <= pszWide[i])
        {
            if (!fCESU8 && i + 1 < cch &&
                0xD800 <= pszWide[i] && pszWide[i] <= 0xDBFF &&
                0xDC00 <= pszWide[i + 1] && pszWide[i + 1] <= 0xDFFF)
            {
                c += 4;
                i++;
            }
            else
                c += 3;
        }
    }

    pszUTF8 = new CHAR[c + 1];
    c = 0;
    for(i = 0; i < cch; i++)
    {
        pb = (BYTE *)&pszWide[i];
        if (pszWide[i] <= 0x007F)
        {
            pszUTF8[c] = pb[0];
            c++;
        }
        else if (0x0080 <= pszWide[i] && pszWide[i] <= 0x07FF)
        {
            pszUTF8[c] = (BYTE)(((pb[0] & 0xC0) >> 6) | 
                                ((pb[1] & 0x07) << 2) | 0xC0);
            pszUTF8[c + 1] = (BYTE)((pb[0] & 0x3F) | 0x80);
            c++;
            c++;
        }
        else if (0x0800 <= pszWide[i])
        {
            if (!fCESU8 && i + 1 < cch &&
                0xD800 <= pszWide[i] && pszWide[i] <= 0xDBFF &&
                0xDC00 <= pszWide[i + 1] && pszWide[i + 1] <= 0xDFFF)
            {
                wch = (pszWide[i] - 0xD800) * 0x400 + 
                      (pszWide[i + 1] - 0xDC00) + 0x10000;
                pszUTF8[c] =     (BYTE)(((wch & 0x001C0000) >> 18) | 0xF0);
                pszUTF8[c + 1] = (BYTE)(((wch & 0x0003F000) >> 12) | 0x80);
                pszUTF8[c + 2] = (BYTE)(((wch & 0x00000FC0) >> 6) | 0x80);
                pszUTF8[c + 3] = (BYTE)( (wch & 0x0000003F) | 0x80);
                c += 4;
                i++;
            }
            else
            {
                pszUTF8[c] = (BYTE)(((pb[1] & 0xF0) >> 4) | 0xE0);
                pszUTF8[c + 1] = (BYTE)(((pb[0] & 0xC0) >> 6) | 
                                        ((pb[1] & 0x0F) << 2) | 0x80);
                pszUTF8[c + 2] = (BYTE)((pb[0] & 0x3F) | 0x80);
                c += 3;
            }
        }
    }
    pszUTF8[c] = 0;
    return pszUTF8;
}
