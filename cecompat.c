/* cecompat.c - Implementation of Windows CE 2.11 Compatibility Layer */

#ifdef _WIN32_WCE

#include "cecompat.h"
#include <stdarg.h>

/* Global references to menu and command bar */
HMENU g_hMainMenu = NULL;
HWND  g_hwndCB    = NULL;

/* CE Time emulation */
time_t sim_time(time_t *timer) {
    time_t t = (time_t)(GetTickCount() / 1000);
    if (timer) {
        *timer = t;
    }
    return t;
}

/* CE strrchr implementation */
char *strrchr(const char *s, int c) {
    const char *last = NULL;
    do {
        if (*s == (char)c) last = s;
    } while (*s++);
    return (char *)last;
}

/* GetMenu shim */
HMENU GetMenu_CE(HWND hWnd) {
    if (g_hMainMenu) {
        return g_hMainMenu;
    }
    if (g_hwndCB) {
        return CommandBar_GetMenu(g_hwndCB, 0);
    }
    return NULL;
}

/* GetMenuItemCount shim */
int GetMenuItemCount_CE(HMENU hMenu) {
    MENUITEMINFOW mii;
    int count = 0;
    if (!hMenu) return -1;
    
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    while (GetMenuItemInfoW(hMenu, count, TRUE, &mii)) {
        count++;
    }
    return count;
}

/* GetMenuState shim */
UINT GetMenuState_CE(HMENU hMenu, UINT uId, UINT uFlags) {
    MENUITEMINFOW mii;
    BOOL byPos = (uFlags & MF_BYPOSITION) ? TRUE : FALSE;
    if (!hMenu) return (UINT)-1;

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STATE | MIIM_SUBMENU;
    if (!GetMenuItemInfoW(hMenu, uId, byPos, &mii)) {
        return (UINT)-1;
    }
    return mii.fState;
}

/* GetMenuStringA shim */
int GetMenuStringA_CE(HMENU hMenu, UINT uIDItem, LPSTR lpString, int nMaxCount, UINT uFlag) {
    MENUITEMINFOW mii;
    WCHAR wsz[256];
    BOOL byPos = (uFlag & MF_BYPOSITION) ? TRUE : FALSE;
    if (!hMenu || !lpString || nMaxCount <= 0) return 0;

    lpString[0] = '\0';
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.dwTypeData = wsz;
    mii.cch = 255;
    if (!GetMenuItemInfoW(hMenu, uIDItem, byPos, &mii)) {
        return 0;
    }
    return WideCharToMultiByte(CP_ACP, 0, wsz, -1, lpString, nMaxCount, NULL, NULL);
}

/* CreateWindowExA shim */
HWND CreateWindowExA_CE(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                        DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    WCHAR wszClass[128];
    WCHAR wszName[256];
    LPCWSTR pClassW;
    LPCWSTR pNameW = NULL;

    if (((DWORD)lpClassName & 0xFFFF0000) == 0) {
        /* Atom / resource ID */
        pClassW = (LPCWSTR)lpClassName;
    } else {
        MultiByteToWideChar(CP_ACP, 0, lpClassName, -1, wszClass, 128);
        pClassW = wszClass;
    }

    if (lpWindowName) {
        MultiByteToWideChar(CP_ACP, 0, lpWindowName, -1, wszName, 256);
        pNameW = wszName;
    }

    /* On Windows CE, top-level windows cannot have an HMENU in CreateWindow */
    if (!(dwStyle & WS_CHILD)) {
        hMenu = NULL;
    }

    return CreateWindowExW(dwExStyle, pClassW, pNameW, dwStyle, X, Y, nWidth, nHeight,
                           hWndParent, hMenu, hInstance, lpParam);
}

/* RegisterClassA shim */
ATOM RegisterClassA_CE(const WNDCLASSA *lpwcA) {
    WNDCLASSW wcW;
    static WCHAR wszClassStore[8][64];
    static int classIdx = 0;
    WCHAR *pClassTarget;

    if (!lpwcA) return 0;

    ZeroMemory(&wcW, sizeof(wcW));
    wcW.style         = lpwcA->style;
    wcW.lpfnWndProc   = lpwcA->lpfnWndProc;
    wcW.cbClsExtra    = lpwcA->cbClsExtra;
    wcW.cbWndExtra    = lpwcA->cbWndExtra;
    wcW.hInstance     = lpwcA->hInstance;
    wcW.hIcon         = lpwcA->hIcon;
    wcW.hCursor       = lpwcA->hCursor;
    wcW.hbrBackground = lpwcA->hbrBackground;
    wcW.lpszMenuName  = NULL; /* CE does not use window menus */

    pClassTarget = wszClassStore[classIdx % 8];
    classIdx++;
    MultiByteToWideChar(CP_ACP, 0, lpwcA->lpszClassName, -1, pClassTarget, 64);
    wcW.lpszClassName = pClassTarget;

    return RegisterClassW(&wcW);
}

/* GetClassInfoA shim */
BOOL GetClassInfoA_CE(HINSTANCE hInstance, LPCSTR lpClassName, LPWNDCLASSA lpWndClass) {
    WNDCLASSW wcW;
    WCHAR wszClass[128];
    LPCWSTR pClassW;
    BOOL ret;

    if (!lpClassName) return FALSE;

    if (((DWORD)lpClassName & 0xFFFF0000) == 0) {
        pClassW = (LPCWSTR)lpClassName;
    } else {
        MultiByteToWideChar(CP_ACP, 0, lpClassName, -1, wszClass, 128);
        pClassW = wszClass;
    }

    ret = GetClassInfoW(hInstance, pClassW, &wcW);
    if (ret && lpWndClass) {
        lpWndClass->style         = wcW.style;
        lpWndClass->lpfnWndProc   = wcW.lpfnWndProc;
        lpWndClass->cbClsExtra    = wcW.cbClsExtra;
        lpWndClass->cbWndExtra    = wcW.cbWndExtra;
        lpWndClass->hInstance     = wcW.hInstance;
        lpWndClass->hIcon         = wcW.hIcon;
        lpWndClass->hCursor       = wcW.hCursor;
        lpWndClass->hbrBackground = wcW.hbrBackground;
        lpWndClass->lpszMenuName  = NULL;
        lpWndClass->lpszClassName = lpClassName;
    }
    return ret;
}

/* DefWindowProcA shim */
LRESULT DefWindowProcA_CE(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

/* MessageBoxA shim */
int MessageBoxA_CE(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    WCHAR wszText[512];
    WCHAR wszCaption[128];

    wszText[0] = 0;
    wszCaption[0] = 0;

    if (lpText) {
        MultiByteToWideChar(CP_ACP, 0, lpText, -1, wszText, 512);
    }
    if (lpCaption) {
        MultiByteToWideChar(CP_ACP, 0, lpCaption, -1, wszCaption, 128);
    }

    return MessageBoxW(hWnd, wszText, wszCaption, uType);
}

/* SetWindowTextA shim */
BOOL SetWindowTextA_CE(HWND hWnd, LPCSTR lpString) {
    WCHAR wsz[256];
    if (!lpString) return FALSE;
    MultiByteToWideChar(CP_ACP, 0, lpString, -1, wsz, 256);
    return SetWindowTextW(hWnd, wsz);
}

/* GetWindowTextA shim */
int GetWindowTextA_CE(HWND hWnd, LPSTR lpString, int nMaxCount) {
    WCHAR wsz[256];
    int len;
    if (!lpString || nMaxCount <= 0) return 0;
    len = GetWindowTextW(hWnd, wsz, 256);
    if (len <= 0) {
        lpString[0] = '\0';
        return 0;
    }
    return WideCharToMultiByte(CP_ACP, 0, wsz, -1, lpString, nMaxCount, NULL, NULL);
}

/* TextOutA shim */
BOOL TextOutA_CE(HDC hdc, int x, int y, LPCSTR lpString, int nCount) {
    WCHAR wsz[512];
    int wlen;
    if (!lpString || nCount <= 0) return FALSE;
    wlen = MultiByteToWideChar(CP_ACP, 0, lpString, nCount, wsz, 512);
    return ExtTextOutW(hdc, x, y, 0, NULL, wsz, wlen, NULL);
}

/* DrawTextA shim */
int DrawTextA_CE(HDC hdc, LPCSTR lpchText, int nCount, LPRECT lpRect, UINT uFormat) {
    WCHAR wsz[1024];
    int wlen;
    if (!lpchText) return 0;
    wlen = MultiByteToWideChar(CP_ACP, 0, lpchText, nCount, wsz, 1024);
    return DrawTextW(hdc, wsz, wlen, lpRect, uFormat);
}

static POINT s_curPt = {0, 0};

/* MoveToEx shim */
BOOL MoveToEx_CE(HDC hdc, int x, int y, LPPOINT lpPoint) {
    if (lpPoint) {
        *lpPoint = s_curPt;
    }
    s_curPt.x = x;
    s_curPt.y = y;
    return TRUE;
}

/* LineTo shim */
BOOL LineTo_CE(HDC hdc, int x, int y) {
    POINT pts[2];
    pts[0] = s_curPt;
    pts[1].x = x;
    pts[1].y = y;
    s_curPt = pts[1];
    return Polyline(hdc, pts, 2);
}

/* FrameRect shim */
int FrameRect_CE(HDC hdc, CONST RECT *lprc, HBRUSH hbr) {
    RECT rc;
    if (!lprc || !hbr) return 0;
    SetRect(&rc, lprc->left, lprc->top, lprc->right, lprc->top + 1);
    FillRect(hdc, &rc, hbr);
    SetRect(&rc, lprc->left, lprc->bottom - 1, lprc->right, lprc->bottom);
    FillRect(hdc, &rc, hbr);
    SetRect(&rc, lprc->left, lprc->top + 1, lprc->left + 1, lprc->bottom - 1);
    FillRect(hdc, &rc, hbr);
    SetRect(&rc, lprc->right - 1, lprc->top + 1, lprc->right, lprc->bottom - 1);
    FillRect(hdc, &rc, hbr);
    return 1;
}

/* GetScrollPos shim */
int GetScrollPos_CE(HWND hWnd, int nBar) {
    SCROLLINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    if (GetScrollInfo(hWnd, nBar, &si)) {
        return si.nPos;
    }
    return 0;
}

/* GetScrollRange shim */
BOOL GetScrollRange_CE(HWND hWnd, int nBar, LPINT lpMinPos, LPINT lpMaxPos) {
    SCROLLINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE;
    if (GetScrollInfo(hWnd, nBar, &si)) {
        if (lpMinPos) *lpMinPos = si.nMin;
        if (lpMaxPos) *lpMaxPos = si.nMax;
        return TRUE;
    }
    return FALSE;
}

/* AppendMenuA shim */
BOOL AppendMenuA_CE(HMENU hMenu, UINT uFlags, UINT uIDNewItem, LPCSTR lpNewItem) {
    WCHAR wsz[256];
    if (lpNewItem && !(uFlags & (MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))) {
        MultiByteToWideChar(CP_ACP, 0, lpNewItem, -1, wsz, 256);
        return AppendMenuW(hMenu, uFlags, uIDNewItem, wsz);
    }
    return AppendMenuW(hMenu, uFlags, uIDNewItem, (LPCWSTR)lpNewItem);
}

/* InsertMenuA shim */
BOOL InsertMenuA_CE(HMENU hMenu, UINT uPosition, UINT uFlags, UINT uIDNewItem, LPCSTR lpNewItem) {
    WCHAR wsz[256];
    if (lpNewItem && !(uFlags & (MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))) {
        MultiByteToWideChar(CP_ACP, 0, lpNewItem, -1, wsz, 256);
        return InsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, wsz);
    }
    return InsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, (LPCWSTR)lpNewItem);
}

/* ModifyMenuA shim */
BOOL ModifyMenuA_CE(HMENU hMnu, UINT uPosition, UINT uFlags, UINT uIDNewItem, LPCSTR lpItem) {
    WCHAR wsz[256];
    RemoveMenu(hMnu, uPosition, uFlags & (MF_BYCOMMAND | MF_BYPOSITION));
    if (lpItem && !(uFlags & (MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))) {
        MultiByteToWideChar(CP_ACP, 0, lpItem, -1, wsz, 256);
        return InsertMenuW(hMnu, uPosition, uFlags, uIDNewItem, wsz);
    }
    return InsertMenuW(hMnu, uPosition, uFlags, uIDNewItem, (LPCWSTR)lpItem);
}

/* FindWindowA shim */
HWND FindWindowA_CE(LPCSTR lpClassName, LPCSTR lpWindowName) {
    WCHAR wszClass[128];
    WCHAR wszName[128];
    LPCWSTR pClass = NULL;
    LPCWSTR pName = NULL;
    if (lpClassName) {
        MultiByteToWideChar(CP_ACP, 0, lpClassName, -1, wszClass, 128);
        pClass = wszClass;
    }
    if (lpWindowName) {
        MultiByteToWideChar(CP_ACP, 0, lpWindowName, -1, wszName, 128);
        pName = wszName;
    }
    return FindWindowW(pClass, pName);
}

/* OutputDebugStringA shim */
void OutputDebugStringA_CE(LPCSTR lpOutputString) {
    WCHAR wsz[512];
    if (!lpOutputString) return;
    MultiByteToWideChar(CP_ACP, 0, lpOutputString, -1, wsz, 512);
    OutputDebugStringW(wsz);
}

/* wsprintfA shim */
int wsprintfA_CE(LPSTR buffer, LPCSTR format, ...) {
    int ret;
    va_list args;
    va_start(args, format);
    ret = vsprintf(buffer, format, args);
    va_end(args);
    return ret;
}

/* GetModuleFileNameA shim */
DWORD GetModuleFileNameA_CE(HMODULE hModule, LPSTR lpFilename, DWORD nSize) {
    WCHAR wsz[MAX_PATH];
    DWORD ret = GetModuleFileNameW(hModule, wsz, MAX_PATH);
    if (ret == 0) return 0;
    return (DWORD)WideCharToMultiByte(CP_ACP, 0, wsz, -1, lpFilename, nSize, NULL, NULL);
}

/* GetTempPathA shim */
DWORD GetTempPathA_CE(DWORD nBufferLength, LPSTR lpBuffer) {
    WCHAR wsz[MAX_PATH];
    DWORD ret = GetTempPathW(MAX_PATH, wsz);
    if (ret == 0) {
        /* Default fallback to root on CE */
        strcpy(lpBuffer, "\\Temp");
        return 5;
    }
    return (DWORD)WideCharToMultiByte(CP_ACP, 0, wsz, -1, lpBuffer, nBufferLength, NULL, NULL);
}

/* GetTempFileNameA shim */
UINT GetTempFileNameA_CE(LPCSTR lpPathName, LPCSTR lpPrefixString, UINT uUnique, LPSTR lpTempFileName) {
    static UINT s_uCount = 1;
    UINT num = uUnique ? uUnique : (s_uCount++);
    if (lpPathName && lpPrefixString && lpTempFileName) {
        sprintf(lpTempFileName, "%s\\%s%04X.tmp", lpPathName, lpPrefixString, num & 0xFFFF);
        return num;
    }
    return 0;
}

/* DeleteFileA shim */
BOOL DeleteFileA_CE(LPCSTR lpFileName) {
    WCHAR wsz[MAX_PATH];
    if (!lpFileName) return FALSE;
    MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, wsz, MAX_PATH);
    return DeleteFileW(wsz);
}

/* CreateFontA shim */
HFONT CreateFontA_CE(int cHeight, int cWidth, int cEscapement, int cOrientation,
                     int cWeight, DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut,
                     DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision,
                     DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName) {
    LOGFONTW lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = cHeight;
    lf.lfWidth = cWidth;
    lf.lfEscapement = cEscapement;
    lf.lfOrientation = cOrientation;
    lf.lfWeight = cWeight;
    lf.lfItalic = (BYTE)bItalic;
    lf.lfUnderline = (BYTE)bUnderline;
    lf.lfStrikeOut = (BYTE)bStrikeOut;
    lf.lfCharSet = (BYTE)iCharSet;
    lf.lfOutPrecision = (BYTE)iOutPrecision;
    lf.lfClipPrecision = (BYTE)iClipPrecision;
    lf.lfQuality = (BYTE)iQuality;
    lf.lfPitchAndFamily = (BYTE)iPitchAndFamily;
    if (pszFaceName) {
        MultiByteToWideChar(CP_ACP, 0, pszFaceName, -1, lf.lfFaceName, LF_FACESIZE);
    }
    return CreateFontIndirectW(&lf);
}

/* GetDlgItemTextA shim */
UINT GetDlgItemTextA_CE(HWND hDlg, int nIDDlgItem, LPSTR lpString, int cchMax) {
    WCHAR wsz[256];
    UINT ret;
    if (!lpString || cchMax <= 0) return 0;
    ret = GetDlgItemTextW(hDlg, nIDDlgItem, wsz, 256);
    if (ret > 0) {
        WideCharToMultiByte(CP_ACP, 0, wsz, -1, lpString, cchMax, NULL, NULL);
    } else {
        lpString[0] = '\0';
    }
    return ret;
}

/* SetDlgItemTextA shim */
BOOL SetDlgItemTextA_CE(HWND hDlg, int nIDDlgItem, LPCSTR lpString) {
    WCHAR wsz[256];
    if (!lpString) return FALSE;
    MultiByteToWideChar(CP_ACP, 0, lpString, -1, wsz, 256);
    return SetDlgItemTextW(hDlg, nIDDlgItem, wsz);
}

/* Helper to convert double-null filter string to wide */
static void ConvertFilterToW(LPCSTR lpFilterA, LPWSTR lpFilterW, int maxChars) {
    int i = 0;
    if (!lpFilterA) {
        lpFilterW[0] = 0;
        lpFilterW[1] = 0;
        return;
    }
    while (lpFilterA[i] || lpFilterA[i+1]) {
        lpFilterW[i] = (WCHAR)(unsigned char)lpFilterA[i];
        i++;
        if (i >= maxChars - 2) break;
    }
    lpFilterW[i++] = 0;
    lpFilterW[i++] = 0;
}

/* GetOpenFileNameA shim */
BOOL GetOpenFileNameA_CE(LPOPENFILENAMEA lpofn) {
    OPENFILENAMEW ofnW;
    WCHAR wszFile[MAX_PATH];
    WCHAR wszFilter[512];
    WCHAR wszTitle[128];
    BOOL ret;

    if (!lpofn) return FALSE;

    ZeroMemory(&ofnW, sizeof(ofnW));
    ofnW.lStructSize = sizeof(ofnW);
    ofnW.hwndOwner = lpofn->hwndOwner;
    ofnW.hInstance = lpofn->hInstance;

    ConvertFilterToW(lpofn->lpstrFilter, wszFilter, 512);
    ofnW.lpstrFilter = wszFilter;

    wszFile[0] = 0;
    if (lpofn->lpstrFile && lpofn->lpstrFile[0]) {
        MultiByteToWideChar(CP_ACP, 0, lpofn->lpstrFile, -1, wszFile, MAX_PATH);
    }
    ofnW.lpstrFile = wszFile;
    ofnW.nMaxFile = MAX_PATH;

    if (lpofn->lpstrTitle) {
        MultiByteToWideChar(CP_ACP, 0, lpofn->lpstrTitle, -1, wszTitle, 128);
        ofnW.lpstrTitle = wszTitle;
    }

    ofnW.Flags = lpofn->Flags;

    ret = GetOpenFileNameW(&ofnW);
    if (ret && lpofn->lpstrFile && lpofn->nMaxFile > 0) {
        WideCharToMultiByte(CP_ACP, 0, wszFile, -1, lpofn->lpstrFile, lpofn->nMaxFile, NULL, NULL);
    }
    return ret;
}

/* GetSaveFileNameA shim */
BOOL GetSaveFileNameA_CE(LPOPENFILENAMEA lpofn) {
    OPENFILENAMEW ofnW;
    WCHAR wszFile[MAX_PATH];
    WCHAR wszFilter[512];
    WCHAR wszTitle[128];
    BOOL ret;

    if (!lpofn) return FALSE;

    ZeroMemory(&ofnW, sizeof(ofnW));
    ofnW.lStructSize = sizeof(ofnW);
    ofnW.hwndOwner = lpofn->hwndOwner;
    ofnW.hInstance = lpofn->hInstance;

    ConvertFilterToW(lpofn->lpstrFilter, wszFilter, 512);
    ofnW.lpstrFilter = wszFilter;

    wszFile[0] = 0;
    if (lpofn->lpstrFile && lpofn->lpstrFile[0]) {
        MultiByteToWideChar(CP_ACP, 0, lpofn->lpstrFile, -1, wszFile, MAX_PATH);
    }
    ofnW.lpstrFile = wszFile;
    ofnW.nMaxFile = MAX_PATH;

    if (lpofn->lpstrTitle) {
        MultiByteToWideChar(CP_ACP, 0, lpofn->lpstrTitle, -1, wszTitle, 128);
        ofnW.lpstrTitle = wszTitle;
    }

    ofnW.Flags = lpofn->Flags;

    ret = GetSaveFileNameW(&ofnW);
    if (ret && lpofn->lpstrFile && lpofn->nMaxFile > 0) {
        WideCharToMultiByte(CP_ACP, 0, wszFile, -1, lpofn->lpstrFile, lpofn->nMaxFile, NULL, NULL);
    }
    return ret;
}

#endif /* _WIN32_WCE */
