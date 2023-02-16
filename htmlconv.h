#ifndef HTML_H
#define HTML_H

#include <windows.h>
#include <map>
#include <string>
#include <list>
#include <set>
using namespace std;

#ifdef UNICODE
	#define tstring wstring
#else
	#define tstring string
#endif

#define CP_SHIFTJIS 932
#define CP_LATIN1   1252

#define FILETYPE_TEXT 0
#define FILETYPE_HTML 1
#define FILETYPE_JS   2
#define FILETYPE_CSS  3
#define FILETYPE_VBS  4
#define FILETYPE_IMG  5

#define NL_NOCONVERT 0
#define NL_CRLF      1
#define NL_LF        2
#define NL_CR        3

typedef map<string, string> ATTRMAP;

typedef enum
{
	SJIS = 0,
	EUC = 1,
	JIS = 2,
	UTF8 = 3
} ICODE;

typedef struct tagITEM
{
	tstring first;
	tstring second;
	tstring third;
	tagITEM(LPCTSTR f, LPCTSTR s, LPCTSTR t)
	: first(f), second(s), third(t)
	{
	}
	tagITEM(tstring f, tstring s, tstring t)
	: first(f), second(s), third(t)
	{
	}
} ITEM;

typedef struct
{
	string charset;
	string content_script_type;
	bool is_html;
	bool has_head;
	size_t head_begin;
	size_t head_inner_begin;
	size_t head_inner_end;
	size_t head_end;
	bool has_meta_content_type;
	size_t meta_content_type_begin;
	size_t meta_content_type_end;
	bool meta_content_type_closed;
	bool has_title;
	size_t title_begin;
	size_t title_end;
} PARSE1INFO;

typedef struct
{
	string tagname;
	size_t begin;
	size_t end;
	ATTRMAP attrmap;
	bool closed;
} ELEMENT;

typedef struct
{
	size_t begin;
	size_t end;
} BLOCK;

typedef struct
{
	string content_script_type;
	list<ELEMENT> elements;
} PARSE2INFO;

// html.cpp
bool ParseHtmlAttrs(const string& content, ATTRMAP& attrmap);
bool ParseHtmlElement(const string& content, string& tagname, ATTRMAP& attrmap);
void HtmlElement(string& result, const string& tagname, const ATTRMAP& attrmap, bool closed);
bool ParseUrl(string& fname, const string& href);
bool Parse1Html(const string& content, PARSE1INFO& info);
bool Parse2Html(const string& content, PARSE2INFO& info);
string html_ansi_from_wide(wstring w);
string css_ansi_from_wide(wstring w);
string js_ansi_from_wide(wstring w);
string text_ansi_from_wide(wstring w);
wstring text_wide_from_ansi(string a);
bool ConvertText(string& content, int iCode);
bool ConvertCSS(string& content, int iCode);
bool ConvertJS(string& content, int iCode);
bool ConvertHtml(string& content, int iCode, set<wstring>& stylesheets, set<wstring>& scripts);

// file.cpp (ANSI)
LPSTR file_get_contents(LPCSTR pszFileName, DWORD *pcb);
BOOL file_set_contents(LPCSTR pszFileName, LPCSTR psz, DWORD cb);
BOOL MakeBackup(LPCSTR pszFileName);
bool ConvertTextFile(LPCSTR pszFileName, int iCode);
bool ConvertCSSFile(LPCSTR pszFileName, int iCode);
bool ConvertJSFile(LPCSTR pszFileName, int iCode);
bool ConvertHtmlFile(LPCSTR pszFileName, int iCode,
    set<wstring>& stylesheets, set<wstring>& scripts);
INT GetFileType(LPCSTR pszFileName);
// file.cpp (Wide)
LPSTR file_get_contents(LPCWSTR pszFileName, DWORD *pcb);
BOOL file_set_contents(LPCWSTR pszFileName, LPCSTR psz, DWORD cb);
BOOL MakeBackup(LPCWSTR pszFileName);
bool ConvertTextFile(LPCWSTR pszFileName, int iCode);
bool ConvertCSSFile(LPCWSTR pszFileName, int iCode);
bool ConvertJSFile(LPCWSTR pszFileName, int iCode);
bool ConvertHtmlFile(LPCWSTR pszFileName, int iCode,
    set<wstring>& stylesheets, set<wstring>& scripts);
INT GetFileType(LPCWSTR pszFileName);

// utf8.cpp
BOOL APIENTRY IsTextUTF8(CONST VOID *p, INT cb);
LPWSTR APIENTRY ConvertUTF8ToUnicode(LPCSTR pszUTF8, INT cch, BOOL *pfCESU8);
LPSTR APIENTRY ConvertUnicodeToUTF8(LPCWSTR pszWide, INT cch, BOOL fCESU8);

// ui.cpp
#ifdef UNICODE
	#define DialogProc DialogProcW
#else
	#define DialogProc DialogProcA
#endif
INT_PTR CALLBACK DialogProcA(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DialogProcW(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// htmlconv.cpp
extern HINSTANCE g_hInstance;
extern HWND g_hMainWnd;
extern HWND g_hConvertingDlg;

extern int g_error;
extern int g_nl;
extern BOOL g_fCSS;
extern BOOL g_fJS;
extern BOOL g_fBackup;
extern BOOL g_fTerminated;
extern BOOL g_fCancelling;
extern HANDLE g_hThread;
extern OSVERSIONINFOA g_osver;
extern INT g_nSort1;
extern INT g_nSort2;
extern INT g_nSort3;

extern LPWSTR *mywargv;

#include "resource.h"

#endif  // ndef HTML_H
