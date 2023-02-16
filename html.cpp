#include <cctype>
#include <cstdlib>
using namespace std;

#include "htmlconv.h"

string MakeLower(string s)
{
    string t;
    size_t i = 0;
    while(i != s.size())
    {
        t += tolower(s.at(i));
        i++;
    }
    return t;
}

bool ParseHtmlAttrs(const string& content, ATTRMAP& attrmap)
{
    size_t i, j;
    string key, value;

    i = 0;
    while(content[i] != '\0')
    {
        // skip space
        while(isspace(content[i])) i++;

        j = i;

        // skip alpha or '-'
        while(isalpha(content[i]) || content[i] == '-') i++;

        if (i == j)
            break;

        // key
        key = MakeLower(content.substr(j, i - j));

        // skip space
        while(isspace(content[i])) i++;

        if (content[i] == '=' && content.size() != i)
        {
            i++;
            // skip space
            while(isspace(content[i])) i++;

            // found quote?
            if (content[i] == '"')
            {
                i++;
                j = i;
                // go to next quote
                while(content.size() != i && content[i] != '"') i++;
            }
            else
            {
                j = i;
                // skip non-space
                while(content.size() != i && !isspace(content[i])) i++;
            }
            value = content.substr(j, i - j);
            if (content[i] == '"') i++;
        }
        else
        {
            value.clear();
        }
        attrmap.insert(pair<string, string>(key, value));
    }
    return true;
}

bool ParseHtmlElement(const string& content, string& tagname, ATTRMAP& attrmap)
{
    size_t i, j, k;

    attrmap.clear();
    i = content.find_first_not_of(" \t\n\r\f\v");
    if (i == string::npos)
    {
        tagname.clear();
        return false;
    }
    j = content.find_first_of(" \t\n\r\f\v", i);
    if (j == string::npos)
    {
        tagname = MakeLower(content.substr(i));
        return true;
    }

    tagname = MakeLower(content.substr(i, j - i));
    if (tagname.size() >= 3 && tagname.substr(0, 3) == "!--")
    {
        tagname = "!--";
        return true;
    }

    return ParseHtmlAttrs(content.substr(j), attrmap);
}

void HtmlElement(string& result, const string& tagname, const ATTRMAP& attrmap, bool closed)
{
    ATTRMAP::const_iterator it, end;
    result.clear();
    result += "<";
    result += tagname;
    end = attrmap.end();
    for(it = attrmap.begin(); it != end; it++)
    {
        result += " ";
        result += it->first;
        result += "=\"";
        result += it->second;
        result += "\"";
    }
    if (closed)
        result += " />";
    else
        result += ">";
}

wstring str_replace(wstring s, WCHAR chOld, WCHAR chNew)
{
    wstring t;
    for(size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == chOld)
            t += chNew;
        else
            t += s[i];
    }
    return t;
}

void convert(wstring& s)
{
    INT i;
 
    i = 0;
    while((i = s.find(L"&quot;", i)) != -1)
    {
        s.replace(i, 6, L"\"");
        i++;
    }
    i = 0;
    while((i = s.find(L"&lt;", i)) != -1)
    {
        s.replace(i, 4, L"<");
        i++;
    }
    i = 0;
    while((i = s.find(L"&gt;", i)) != -1)
    {
        s.replace(i, 4, L">");
        i++;
    }
    i = 0;
    while((i = s.find(L"&amp;", i)) != -1)
    {
        s.replace(i, 5, L"&");
        i++;
    }
    i = 0;
    while((i = s.find(L"%", i)) != -1)
    {
        if (isxdigit(s[i + 1]) && isxdigit(s[i + 2]))
        {
            int n = 0;
            if (L'0' <= s[i + 1] && s[i + 1] <= L'9')
                n = s[i + 1] - L'0';
            else if (L'A' <= s[i + 1] && s[i + 1] <= L'F')
                n = s[i + 1] - L'A' + 10;
            else if (L'a' <= s[i + 1] && s[i + 1] <= L'f')
                n = s[i + 1] - L'a' + 10;
            n *= 16;
            if (L'0' <= s[i + 2] && s[i + 2] <= L'9')
                n += s[i + 2] - L'0';
            else if (L'A' <= s[i + 2] && s[i + 2] <= L'F')
                n += s[i + 2] - L'A' + 10;
            else if (L'a' <= s[i + 2] && s[i + 2] <= L'f')
                n += s[i + 2] - L'a' + 10;
            wstring t;
            t += (WCHAR)n;
            s.replace(i, 3, t);
            i += 3;
        }
        else if(s[i + 1] == L'%')
        {
            s.replace(i, 2, L"%");
            i++;
        }
    }
    i = 0;
    while((i = s.find(L"&#", i)) != -1)
    {
        LPWSTR endptr;
        WCHAR wch;
        if ((s[i + 2] == L'X' || s[i + 2] == L'x'))
        {
            wch = (WCHAR)wcstol(&s[i + 3], &endptr, 16);
            if (endptr != NULL && *endptr == L';')
            {
                wstring t;
                t += wch;
                s.replace(i, endptr -  &(s.c_str()[i]) + 1, t);
                i++;
            }
        }
        else
        {
            wch = (WCHAR)wcstol(&s[i], &endptr, 10);
            if (endptr != NULL && *endptr == L';')
            {
                wstring t;
                t += wch;
                s.replace(i, endptr -  &(s.c_str()[i]) + 1, t);
                i++;
            }
        }
    }
}

bool ParseUrl(wstring& fname, const string& href)
{
    wstring s, t;
    if (href.size() > 5 && href.substr(0, 5) == "http:")
    {
        return false;
    }
    if (href.size() > 4 && href.substr(0, 4) == "ftp:")
    {
        return false;
    }
    if (href.size() > 7 && href.substr(0, 7) == "file://")
    {
        s = text_wide_from_ansi(href.substr(7));
    }
    else if (href.size() > 5 && href.substr(0, 5) == "file:")
    {
        s = text_wide_from_ansi(href.substr(5));
    }
    else
    {
        s = text_wide_from_ansi(href);
    }
    s = str_replace(s, L'|', L':');
    s = str_replace(s, L'/', L'\\');
    convert(s);
    fname = s;
    return true;
}

bool Parse1Html(const string& content, PARSE1INFO& info)
{
    string element_content;
    string tagname;
    ATTRMAP attrmap;
    size_t i, j, k, l;
    bool closed;

    info.is_html = false;
    info.has_head = false;
    info.has_title = false;
    info.has_meta_content_type = false;
    info.meta_content_type_closed = true;
    info.content_script_type = "text/javascript";

    for(j = 0; ; j++)
    {
        i = content.find('<', j);
        if (i == string::npos)
            break;
        j = content.find('>', i);
        if (j == string::npos)
            break;

        element_content = content.substr(i + 1, j - i - 1);
        if (element_content[element_content.size() - 1] == '/')
        {
            element_content.resize(element_content.size() - 1);
            closed = true;
        }
        else
            closed = false;

        if (ParseHtmlElement(element_content, tagname, attrmap))
        {
            if (tagname == "!--")
            {
                j = content.find("-->", i);
                if (j == string::npos)
                    break;
            }
            else if (tagname == "html")
            {
                info.is_html = true;
            }
            else if (tagname == "head")
            {
                info.has_head = true;
                info.head_begin = i;
                info.head_inner_begin = j + 1;
            }
            else if (tagname == "/head")
            {
                info.head_inner_end = i;
                info.head_end = j + 1;
            }
            else if (tagname == "title")
            {
                info.has_title = true;
                info.title_begin = i;
            }
            else if (tagname == "/title")
            {
                info.title_end = j + 1;
            }
            else if (tagname == "script")
            {
                if (!closed)
                {
                    k = content.find("</SCRIPT>", j);
                    l = content.find("</script>", j);
                    if (k != string::npos && l != string::npos)
                    {
                        if (k < l)
                            j = k + 9;
                        else
                            j = l + 9;
                    }
                    else if (k != string::npos)
                        j = k + 9;
                    else if (l != string::npos)
                        j = l + 9;
                }
                else
                    j++;
            }
            else if (tagname == "meta")
            {
                ATTRMAP::iterator it, end = attrmap.end();
                it = attrmap.find("http-equiv");
                if (it != end && MakeLower(it->second) == "content-type")
                {
                    it = attrmap.find("content");
                    if (it != end)
                    {
                        size_t i = it->second.find("charset=");
                        if (i != string::npos)
                        {
                            i += 8;
                            info.charset = MakeLower(it->second.substr(i));
                        }
                    }
                    info.has_meta_content_type = true;
                    info.meta_content_type_begin = i;
                    info.meta_content_type_end = j + 1;
                    info.meta_content_type_closed = closed;
                }
                else if (it != end && MakeLower(it->second) == "content-script-type")
                {
                    it = attrmap.find("content");
                    if (it != end)
                    {
                        info.content_script_type = MakeLower(it->second);
                    }
                }
            }
            else if (tagname == "body")
            {
                break;
            }
        }
    }
    return info.is_html;
}

bool Parse2Html(const string& content, PARSE2INFO& info)
{
    string element_content;
    string tagname;
    ATTRMAP attrmap;
    ELEMENT element;
    ATTRMAP::iterator it, end;
    size_t i, j, k, l;
    bool closed;

    for(j = 0; ; j++)
    {
        i = content.find('<', j);
        if (i == string::npos)
            break;
        j = content.find('>', i);
        if (j == string::npos)
            break;

        element_content = content.substr(i + 1, j - i - 1);
        if (element_content[element_content.size() - 1] == '/')
        {
            element_content.resize(element_content.size() - 1);
            closed = true;
        }
        else
            closed = false;

        if (ParseHtmlElement(element_content, tagname, attrmap))
        {
            if (tagname == "!--")
            {
                j = content.find("-->", i);
                if (j == string::npos)
                    break;
            }
            else if (tagname == "link")
            {
                ATTRMAP::iterator it;
                it = attrmap.find("rel");
                if (it != attrmap.end() &&
                    MakeLower(it->second) == "stylesheet")
                {
                    element.tagname = tagname;
                    element.begin = i;
                    element.end = j + 1;
                    element.attrmap = attrmap;
                    element.closed = closed;
                    info.elements.push_back(element);
                }
            }
            else if (tagname == "script")
            {
                ATTRMAP::iterator it;
                it = attrmap.find("type");
                if ((it == attrmap.end() && info.content_script_type == "text/javascript") ||
                    MakeLower(it->second) == "text/javascript")
                {
                    it = attrmap.find("src");
                    if (it != attrmap.end())
                    {
                        element.tagname = tagname;
                        element.begin = i;
                        element.end = j + 1;
                        element.attrmap = attrmap;
                        element.closed = closed;
                        info.elements.push_back(element);
                    }
                    else
                    {
                        if (!closed)
                        {
                            k = content.find("</SCRIPT>", j);
                            l = content.find("</script>", j);
                            if (k != string::npos && l != string::npos)
                            {
                                if (k < l)
                                    j = k + 9;
                                else
                                    j = l + 9;
                            }
                            else if (k != string::npos)
                                j = k + 9;
                            else if (l != string::npos)
                                j = l + 9;
                        }
                        else
                            j++;
                    }
                }
                else
                {
                    if (!closed)
                    {
                        k = content.find("</SCRIPT>", j);
                        l = content.find("</script>", j);
                        if (k != string::npos && l != string::npos)
                        {
                            if (k < l)
                                j = k + 9;
                            else
                                j = l + 9;
                        }
                        else if (k != string::npos)
                            j = k + 9;
                        else if (l != string::npos)
                            j = l + 9;
                    }
                    else
                        j++;
                }
            }
        }
    }
    return true;
}

string html_ansi_from_wide(wstring w)
{
    string s;
    CHAR sz[32];
    BOOL fUsedDefaultChar;
    LPCWSTR pwsz = w.c_str();
    INT c = w.size();
 
    for(INT i = 0; i < c; i++)
    {
        ZeroMemory(sz, 3);
        WideCharToMultiByte(CP_SHIFTJIS, 0, &pwsz[i], 1, sz, 3, NULL, &fUsedDefaultChar);
        if (fUsedDefaultChar)
            wsprintfA(sz, "&#%d;", pwsz[i]);
        s += sz;
    }
    return s;
}

string css_ansi_from_wide(wstring w)
{
    string s;
    CHAR sz[32];
    BOOL fUsedDefaultChar;
    LPCWSTR pwsz = w.c_str();
    INT c = w.size();
 
    for(INT i = 0; i < c; i++)
    {
        ZeroMemory(sz, 3);
        WideCharToMultiByte(CP_SHIFTJIS, 0, &pwsz[i], 1, sz, 3, NULL, &fUsedDefaultChar);
        if (fUsedDefaultChar)
            wsprintfA(sz, "\\%X;", pwsz[i]);
        s += sz;
    }
    return s;
}

string js_ansi_from_wide(wstring w)
{
    string s;
    CHAR sz[32];
    BOOL fUsedDefaultChar;
    LPCWSTR pwsz = w.c_str();
    INT c = w.size();
 
    for(INT i = 0; i < c; i++)
    {
        ZeroMemory(sz, 3);
        WideCharToMultiByte(CP_SHIFTJIS, 0, &pwsz[i], 1, sz, 3, NULL, &fUsedDefaultChar);
        if (fUsedDefaultChar)
            wsprintfA(sz, "\\u%4X;", pwsz[i]);
        s += sz;
    }
    return s;
}

string text_ansi_from_wide(wstring w)
{
    DWORD cch;
    cch = WideCharToMultiByte(CP_SHIFTJIS, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    LPSTR psz = new CHAR[cch + 1];
    WideCharToMultiByte(CP_SHIFTJIS, 0, w.c_str(), -1, psz, cch + 1, NULL, NULL);
    string a(psz);
    delete[] psz;
    return a;
}

wstring text_wide_from_ansi(string a)
{
    DWORD cch;
    cch = MultiByteToWideChar(CP_SHIFTJIS, 0, a.c_str(), -1, NULL, 0);
    LPWSTR psz = new WCHAR[cch + 1];
    MultiByteToWideChar(CP_SHIFTJIS, 0, a.c_str(), -1, psz, cch + 1);
    wstring w(psz);
    delete[] psz;
    return w;
}

#include <nkf32.h>

string SetOption(int iCode)
{
    string charset;

    switch(iCode)
    {
    case SJIS:
        switch(g_nl)
        {
        case NL_CRLF:
            SetNkfOption("-s -x -Lw -c");
            break;

        case NL_LF:
            SetNkfOption("-s -x -Lu -d");
            break;

        case NL_CR:
            SetNkfOption("-s -x -Lm");
            break;

        default:
            SetNkfOption("-s -x");
        }
        charset = "Shift_JIS";
        break;

    case EUC:
        switch(g_nl)
        {
        case NL_CRLF:
            SetNkfOption("-e -x -Lw -c");
            break;

        case NL_LF:
            SetNkfOption("-e -x -Lu -d");
            break;

        case NL_CR:
            SetNkfOption("-e -x -Lm");
            break;

        default:
            SetNkfOption("-e -x");
        }
        charset = "EUC-JP";
        break;

    case JIS:
        switch(g_nl)
        {
        case NL_CRLF:
            SetNkfOption("-j -x -Lw -c");
            break;

        case NL_LF:
            SetNkfOption("-j -x -Lu -d");
            break;

        case NL_CR:
            SetNkfOption("-j -x -Lm");
            break;

        default:
            SetNkfOption("-j -x");
        }
        charset = "ISO-2022-JP";
        break;

    case UTF8:
        switch(g_nl)
        {
        case NL_CRLF:
            SetNkfOption("-w -w80 -x -Lw -c");
            break;

        case NL_LF:
            SetNkfOption("-w -w80 -x -Lu -d");
            break;

        case NL_CR:
            SetNkfOption("-w -w80 -x -Lm");
            break;

        default:
            SetNkfOption("-w -w80 -x");
        }
        charset = "UTF-8";
        break;
    }

    return charset;
}

bool ConvertCSS(string& content, int iCode)
{
    string charset, charset_old;

    g_error = IDS_SUCCESS;

    charset = SetOption(iCode);

    try
    {
        if (IsTextUTF8(content.c_str(), content.size()))
        {
            BOOL f;
            LPWSTR pszWide = ConvertUTF8ToUnicode(content.c_str(), content.size(), &f);
            if (pszWide[0] == 0xFEFF)
                content = css_ansi_from_wide(pszWide + 1);
            else
                content = css_ansi_from_wide(pszWide);
            delete[] pszWide;
        }
    }
    catch(bad_alloc)
    {
        g_error = IDS_OUTOFMEMORY;
        return false;
    }

    string statement;
    statement += "@charset \"";
    statement += MakeLower(charset);
    {
        size_t i;
        switch(g_nl)
        {
        case NL_CRLF:
            statement += "\";\r\n";
            break;

        case NL_LF:
            statement += "\";\n";
            break;

        case NL_CR:
            statement += "\";\r";
            break;

        case NL_NOCONVERT:
            i = content.find("\r\n");
            if (i == string::npos)
            {
                i = content.find("\n");
                if (i == string::npos)
                {
                    i = content.find("\r");
                    if (i == string::npos)
                        statement += "\";\r\n";
                    else
                        statement += "\";\r";
                }
                else
                    statement += "\";\n";
            }
            else
                statement += "\";\r\n";
        }
    }

    size_t charset_begin, charset_end;
    size_t charset_inner_begin, charset_inner_end;
    bool charset_found = false;
    size_t i = content.find("@charset");
    if (i != string::npos)
    {
        charset_begin = i;
        i += 8;
        while(isspace(content[i])) i++;
        if (content[i] == '"')
        {
            i++;
            charset_inner_begin = i;
            while(content[i] != '\0' && content[i] != '\"') i++;
            if (content[i] == '\"')
            {
                charset_inner_end = i;
                i++;
                while(isspace(content[i])) i++;
                if (content[i] == ';')
                {
                    i++;
                    charset_end = i;
                    charset_found = true;
                }
            }
        }
    }
    if (charset_found)
    {
        charset_old = content.substr(charset_inner_begin, charset_inner_end - charset_inner_begin);
        content.replace(charset_begin, charset_end - charset_begin, statement);
    }
    else
    {
        content.replace((size_t)0, (size_t)0, statement);
    }

    try
    {
        if (MakeLower(charset_old) == "latin1" ||
            MakeLower(charset_old) == "iso-8859-1")
        {
            LPWSTR pszWide;
            INT cchWide;
            cchWide = MultiByteToWideChar(CP_LATIN1, 0, content.c_str(), -1, NULL, 0);
            pszWide = new WCHAR[cchWide + 1];
            MultiByteToWideChar(CP_LATIN1, 0, content.c_str(), -1, pszWide, cchWide + 1);
            content = css_ansi_from_wide(pszWide);
            delete[] pszWide;
        }

        DWORD cch = content.size() * 4 / 3 + 1;
        LPSTR psz = (LPSTR)malloc(cch);
        if (psz == NULL)
        {
            g_error = IDS_OUTOFMEMORY;
            return false;
        }
        DWORD cchDone;
        for(;;)
        {
            NkfConvertSafe(psz, cch, &cchDone, content.c_str(), content.size());
            if (cchDone < cch)
                break;
            cch *= 2;
            psz = (LPSTR)realloc(psz, cch);
            if (psz == NULL)
            {
                g_error = IDS_OUTOFMEMORY;
                return false;
            }
        }
        content = psz;
        free(psz);
        return true;
    }
    catch(bad_alloc)
    {
        g_error = IDS_OUTOFMEMORY;
    }
    return false;
}

bool ConvertText(string& content, int iCode)
{
    string charset;

    g_error = IDS_SUCCESS;
    charset = SetOption(iCode);

    try
    {
        DWORD cch = content.size() * 4 / 3 + 1;
        LPSTR psz = (LPSTR)malloc(cch);
        if (psz == NULL)
        {
            g_error = IDS_OUTOFMEMORY;
            return false;
        }
        DWORD cchDone;
        for(;;)
        {
            NkfConvertSafe(psz, cch, &cchDone, content.c_str(), content.size());
            if (cchDone < cch)
                break;
            cch *= 2;
            psz = (LPSTR)realloc(psz, cch);
            if (psz == NULL)
            {
                g_error = IDS_OUTOFMEMORY;
                return false;
            }
        }
        content = psz;
        free(psz);
        return true;
    }
    catch(bad_alloc)
    {
        g_error = IDS_OUTOFMEMORY;
    }
    return false;
}

bool ConvertJS(string& content, int iCode)
{
    string charset;

    g_error = IDS_SUCCESS;
    charset = SetOption(iCode);

    try
    {
        if (IsTextUTF8(content.c_str(), content.size()))
        {
            BOOL f;
            LPWSTR pszWide = ConvertUTF8ToUnicode(content.c_str(), content.size(), &f);
            if (pszWide[0] == 0xFEFF)
                content = js_ansi_from_wide(pszWide + 1);
            else
                content = js_ansi_from_wide(pszWide);
            delete[] pszWide;
        }
    }
    catch(bad_alloc)
    {
        g_error = IDS_OUTOFMEMORY;
        return false;
    }

    try
    {
        DWORD cch = content.size() * 4 / 3 + 1;
        LPSTR psz = (LPSTR)malloc(cch);
        if (psz == NULL)
        {
            g_error = IDS_OUTOFMEMORY;
            return false;
        }
        DWORD cchDone;
        for(;;)
        {
            NkfConvertSafe(psz, cch, &cchDone, content.c_str(), content.size());
            if (cchDone < cch)
                break;
            cch *= 2;
            psz = (LPSTR)realloc(psz, cch);
            if (psz == NULL)
            {
                g_error = IDS_OUTOFMEMORY;
                return false;
            }
        }
        content = psz;
        free(psz);
        return true;
    }
    catch(bad_alloc)
    {
        g_error = IDS_OUTOFMEMORY;
    }
    return false;
}

bool ConvertHtml(string& content, int iCode, set<wstring>& stylesheets, set<wstring>& scripts)
{
    string charset;

    g_error = IDS_SUCCESS;
    charset = SetOption(iCode);

    try
    {
        if (IsTextUTF8(content.c_str(), content.size()))
        {
            BOOL f;
            LPWSTR pszWide = ConvertUTF8ToUnicode(content.c_str(), content.size(), &f);
            if (pszWide[0] == 0xFEFF)
                content = html_ansi_from_wide(pszWide + 1);
            else
                content = html_ansi_from_wide(pszWide);
            delete[] pszWide;
        }

        string head_inner;
        string title;
        PARSE1INFO info1;

        if (Parse1Html(content, info1))
        {
            if (MakeLower(info1.charset) == "latin1" || MakeLower(info1.charset) == "iso-8859-1")
            {
                LPWSTR pszWide;
                INT cchWide;
                cchWide = MultiByteToWideChar(CP_LATIN1, MB_COMPOSITE, content.c_str(), -1, NULL, 0);
                pszWide = new WCHAR[cchWide + 1];
                MultiByteToWideChar(CP_LATIN1, MB_COMPOSITE, content.c_str(), -1, pszWide, cchWide + 1);
                content = html_ansi_from_wide(pszWide);
                delete[] pszWide;
            }
        }

        {
            DWORD cch = content.size() * 4 / 3 + 1;
            LPSTR psz = (LPSTR)malloc(cch);
            if (psz == NULL)
            {
                g_error = IDS_OUTOFMEMORY;
                return false;
            }
            DWORD cchDone;
            for(;;)
            {
                NkfConvertSafe(psz, cch, &cchDone, content.c_str(), content.size());
                if (cchDone < cch)
                    break;
                cch *= 2;
                psz = (LPSTR)realloc(psz, cch);
                if (psz == NULL)
                {
                    g_error = IDS_OUTOFMEMORY;
                    return false;
                }
            }
            content = psz;
            free(psz);
        }

        if (Parse1Html(content, info1))
        {
            string meta_content_type("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=");
            meta_content_type += charset;
            if (info1.meta_content_type_closed)
                meta_content_type += string("\" />");
            else
                meta_content_type += string("\">");

            if (info1.has_meta_content_type)
            {
                if (info1.head_begin < info1.meta_content_type_begin)
                {
                    if (info1.has_title)
                    {
                        if (info1.meta_content_type_begin < info1.title_begin)
                        {
                            content.replace(info1.meta_content_type_begin,
                                info1.meta_content_type_end - info1.meta_content_type_begin,
                                meta_content_type);
                        }
                        else
                        {
                            string title_block;
                            title_block = content.substr(info1.title_begin,
                                info1.title_end - info1.title_begin);
                            content.replace(info1.meta_content_type_begin,
                                info1.meta_content_type_end - info1.meta_content_type_begin,
                                "");
                            content.replace(info1.title_begin,
                                info1.title_end - info1.title_begin, "");
                            content.replace(info1.title_begin, 0, title_block);
                            content.replace(info1.title_begin, 0, meta_content_type);
                        }
                    }
                    else
                    {
                        content.replace(info1.meta_content_type_begin,
                            info1.meta_content_type_end - info1.meta_content_type_begin,
                            meta_content_type);
                    }
                }
                else
                {
                    content.replace(info1.meta_content_type_begin,
                        info1.meta_content_type_end - info1.meta_content_type_begin,
                        meta_content_type);
                }
            }
            else if (info1.has_head)
            {
                content.replace(info1.head_inner_begin, 0, meta_content_type);
            }
            else
            {
                ;
            }
        }

        PARSE2INFO info2;
        info2.content_script_type = info1.content_script_type;
        Parse2Html(content, info2);

        {
            list<ELEMENT>::reverse_iterator it, end = info2.elements.rend();
            wstring fname;
            bool changed;
            for(it = info2.elements.rbegin(); it != end; it++)
            {
                changed = false;
                if (it->tagname == "link")
                {
                    ATTRMAP::iterator it2, end2 = it->attrmap.end();
                    it2 = it->attrmap.find("href");
                    if (it2 != end2 && ParseUrl(fname, it2->second))
                    {
                        if (g_osver.dwPlatformId == VER_PLATFORM_WIN32_NT)
                        {
                            WCHAR szFullPath[MAX_PATH];
                            LPWSTR pch;
                            if (GetFullPathNameW(fname.c_str(), MAX_PATH, szFullPath, &pch) &&
                                GetFileAttributesW(fname.c_str()) != 0xFFFFFFFF)
                            {
                                if (g_fCSS)
                                    stylesheets.insert(szFullPath);
                                it2 = it->attrmap.find("charset");
                                if (it2 != end2)
                                {
                                    it2->second = charset;
                                }
                                else
                                {
                                    it->attrmap.insert(pair<string, string>("charset", charset));
                                }
                                changed = true;
                            }
                        }
                        else
                        {
                            CHAR szFileName[MAX_PATH];
                            CHAR szFullPath[MAX_PATH];
                            LPSTR pch;
                            WideCharToMultiByte(CP_SHIFTJIS, 0, fname.c_str(), -1, szFileName, MAX_PATH, NULL, NULL);
                            if (GetFullPathNameA(szFileName, -1, szFullPath, &pch) &&
                                GetFileAttributesA(szFileName) != 0xFFFFFFFF)
                            {
                                if (g_fCSS)
                                    stylesheets.insert(text_wide_from_ansi(szFullPath));
                                it2 = it->attrmap.find("charset");
                                if (it2 != end2)
                                {
                                    it2->second = charset;
                                }
                                else
                                {
                                    it->attrmap.insert(pair<string, string>("charset", charset));
                                }
                                changed = true;
                            }
                        }
                    }
                }
                else if (it->tagname == "script")
                {
                    ATTRMAP::iterator it2, end2 = it->attrmap.end();
                    it2 = it->attrmap.find("type");
                    if (it2 != end2 && MakeLower(it2->second) != "text/javascript")
                    {
                        continue;
                    }
                    if (it2 == end2 && info2.content_script_type != "text/javascript")
                    {
                        continue;
                    }
                    it2 = it->attrmap.find("src");
                    if (it2 != end2 && ParseUrl(fname, it2->second))
                    {
                        if (g_osver.dwPlatformId == VER_PLATFORM_WIN32_NT)
                        {
                            WCHAR szFullPath[MAX_PATH];
                            LPWSTR pch;
                            if (GetFullPathNameW(fname.c_str(), MAX_PATH, szFullPath, &pch) &&
                                GetFileAttributesW(fname.c_str()) != 0xFFFFFFFF)
                            {
                                if (g_fJS)
                                    scripts.insert(szFullPath);
                                it2 = it->attrmap.find("charset");
                                if (it2 != end2)
                                {
                                    it2->second = charset;
                                }
                                else
                                {
                                    it->attrmap.insert(pair<string, string>("charset", charset));
                                }
                                changed = true;
                            }
                        }
                        else
                        {
                            CHAR szFileName[MAX_PATH];
                            CHAR szFullPath[MAX_PATH];
                            LPSTR pch;
                            WideCharToMultiByte(CP_SHIFTJIS, 0, fname.c_str(), -1, szFileName, MAX_PATH, NULL, NULL);
                            if (GetFullPathNameA(szFileName, -1, szFullPath, &pch) &&
                                GetFileAttributesA(szFileName) != 0xFFFFFFFF)
                            {
                                if (g_fJS)
                                    scripts.insert(text_wide_from_ansi(szFullPath));
                                it2 = it->attrmap.find("charset");
                                if (it2 != end2)
                                {
                                    it2->second = charset;
                                }
                                else
                                {
                                    it->attrmap.insert(pair<string, string>("charset", charset));
                                }
                                changed = true;
                            }
                        }
                    }
                }
                if (changed)
                {
                    string tag;
                    HtmlElement(tag, it->tagname, it->attrmap, it->closed);
                    content.replace(it->begin, it->end - it->begin, tag);
                }
            }
        }

        return true;
    }
    catch(bad_alloc)
    {
        g_error = IDS_OUTOFMEMORY;
    }
    return false;
}
