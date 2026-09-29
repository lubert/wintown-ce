/* cecompat.h - Windows CE 2.11 MIPS Compatibility Layer for WinTown
 * Provides ANSI wrappers and defines missing Win32 symbols on Windows CE
 */

#ifndef _CECOMPAT_H
#define _CECOMPAT_H

#ifdef _WIN32_WCE

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>

/* Missing window styles and class styles */
#ifndef WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW (WS_BORDER | WS_CAPTION | WS_SYSMENU)
#endif

#ifndef WS_THICKFRAME
#define WS_THICKFRAME 0
#endif

#ifndef CS_BYTEALIGNWINDOW
#define CS_BYTEALIGNWINDOW 0
#endif

/* Missing TrackPopupMenu flags */
#ifndef TPM_RIGHTBUTTON
#define TPM_RIGHTBUTTON 0x0000L
#endif

/* Missing pen styles */
#ifndef PS_DOT
#define PS_DOT PS_SOLID
#endif

/* Missing stretch modes */
#ifndef COLORONCOLOR
#define COLORONCOLOR 3
#endif

/* Missing LoadImage flags */
#ifndef LR_LOADFROMFILE
#define LR_LOADFROMFILE 0x0010
#endif

#ifndef LR_CREATEDIBSECTION
#define LR_CREATEDIBSECTION 0x2000
#endif

#ifndef LR_DEFAULTCOLOR
#define LR_DEFAULTCOLOR 0x0000
#endif

#ifndef MF_BITMAP
#define MF_BITMAP 0x00000004L
#endif

#ifndef ZeroMemory
#define ZeroMemory(Destination,Length) memset((Destination),0,(Length))
#endif

#ifndef CharNextA
#define CharNextA(s) (*(s) ? (s) + 1 : (s))
#endif

#ifndef UpdateColors
#define UpdateColors(hdc) 0
#endif

#ifndef InvalidateRgn
#define InvalidateRgn(hwnd, hrgn, erase) InvalidateRect((hwnd), NULL, (erase))
#endif

#ifndef CheckDlgButton
#define CheckDlgButton(hDlg, id, uCheck) SendDlgItemMessage((hDlg), (id), BM_SETCHECK, (WPARAM)(uCheck), 0)
#endif

#ifndef IsDlgButtonChecked
#define IsDlgButtonChecked(hDlg, id) SendDlgItemMessage((hDlg), (id), BM_GETCHECK, 0, 0)
#endif

/* Time compatibility */
#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif
time_t sim_time(time_t *timer);
#define time(t) sim_time(t)

/* String compatibility for CE */
char *strrchr(const char *s, int c);
#undef lstrcpy
#define lstrcpy strcpy
#undef lstrcat
#define lstrcat strcat
#undef lstrlen
#define lstrlen strlen
#undef lstrcmp
#define lstrcmp strcmp

#undef OPENFILENAME
#define OPENFILENAME OPENFILENAMEA
#undef LPOPENFILENAME
#define LPOPENFILENAME LPOPENFILENAMEA

/* Global handles for CE CommandBar and Menu tracking */
extern HMENU g_hMainMenu;
extern HWND  g_hwndCB;

HMENU GetMenu_CE(HWND hWnd);
#undef GetMenu
#define GetMenu GetMenu_CE

/* Menu functions missing from Windows CE */
int  GetMenuItemCount_CE(HMENU hMenu);
#undef GetMenuItemCount
#define GetMenuItemCount GetMenuItemCount_CE

UINT GetMenuState_CE(HMENU hMenu, UINT uId, UINT uFlags);
#undef GetMenuState
#define GetMenuState GetMenuState_CE

int  GetMenuStringA_CE(HMENU hMenu, UINT uIDItem, LPSTR lpString, int nMaxCount, UINT uFlag);
#undef GetMenuString
#define GetMenuString GetMenuStringA_CE

/* ANSI wrappers for Unicode-only WinCE APIs */
HWND CreateWindowExA_CE(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                        DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
                        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
#undef CreateWindowEx
#define CreateWindowEx CreateWindowExA_CE
#undef CreateWindow
#define CreateWindow(cls, name, style, x, y, w, h, parent, menu, inst, param) \
    CreateWindowExA_CE(0, cls, name, style, x, y, w, h, parent, menu, inst, param)

/* Window class registration */
#undef WNDCLASS
#define WNDCLASS WNDCLASSA

ATOM RegisterClassA_CE(const WNDCLASSA *lpWndClass);
#undef RegisterClass
#define RegisterClass RegisterClassA_CE

BOOL GetClassInfoA_CE(HINSTANCE hInstance, LPCSTR lpClassName, LPWNDCLASSA lpWndClass);
#undef GetClassInfo
#define GetClassInfo GetClassInfoA_CE

LRESULT DefWindowProcA_CE(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#undef DefWindowProc
#define DefWindowProc DefWindowProcA_CE

int MessageBoxA_CE(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
#undef MessageBox
#define MessageBox MessageBoxA_CE

BOOL SetWindowTextA_CE(HWND hWnd, LPCSTR lpString);
#undef SetWindowText
#define SetWindowText SetWindowTextA_CE

int GetWindowTextA_CE(HWND hWnd, LPSTR lpString, int nMaxCount);
#undef GetWindowText
#define GetWindowText GetWindowTextA_CE

BOOL TextOutA_CE(HDC hdc, int x, int y, LPCSTR lpString, int nCount);
#undef TextOut
#define TextOut TextOutA_CE

int DrawTextA_CE(HDC hdc, LPCSTR lpchText, int nCount, LPRECT lpRect, UINT uFormat);
#undef DrawText
#define DrawText DrawTextA_CE

BOOL MoveToEx_CE(HDC hdc, int x, int y, LPPOINT lpPoint);
#undef MoveToEx
#define MoveToEx MoveToEx_CE

BOOL LineTo_CE(HDC hdc, int x, int y);
#undef LineTo
#define LineTo LineTo_CE

int FrameRect_CE(HDC hdc, CONST RECT *lprc, HBRUSH hbr);
#undef FrameRect
#define FrameRect FrameRect_CE

int GetScrollPos_CE(HWND hWnd, int nBar);
#undef GetScrollPos
#define GetScrollPos GetScrollPos_CE

BOOL GetScrollRange_CE(HWND hWnd, int nBar, LPINT lpMinPos, LPINT lpMaxPos);
#undef GetScrollRange
#define GetScrollRange GetScrollRange_CE

BOOL AppendMenuA_CE(HMENU hMenu, UINT uFlags, UINT uIDNewItem, LPCSTR lpNewItem);
#undef AppendMenu
#define AppendMenu AppendMenuA_CE

BOOL InsertMenuA_CE(HMENU hMenu, UINT uPosition, UINT uFlags, UINT uIDNewItem, LPCSTR lpNewItem);
#undef InsertMenu
#define InsertMenu InsertMenuA_CE

BOOL ModifyMenuA_CE(HMENU hMnu, UINT uPosition, UINT uFlags, UINT uIDNewItem, LPCSTR lpItem);
#undef ModifyMenu
#define ModifyMenu ModifyMenuA_CE

void OutputDebugStringA_CE(LPCSTR lpOutputString);
#undef OutputDebugString
#define OutputDebugString OutputDebugStringA_CE

HWND FindWindowA_CE(LPCSTR lpClassName, LPCSTR lpWindowName);
#undef FindWindow
#define FindWindow FindWindowA_CE

#undef wvsprintf
#define wvsprintf vsprintf

int wsprintfA_CE(LPSTR buffer, LPCSTR format, ...);
#undef wsprintf
#define wsprintf wsprintfA_CE

DWORD GetModuleFileNameA_CE(HMODULE hModule, LPSTR lpFilename, DWORD nSize);
#undef GetModuleFileName
#define GetModuleFileName GetModuleFileNameA_CE

DWORD GetTempPathA_CE(DWORD nBufferLength, LPSTR lpBuffer);
#undef GetTempPath
#define GetTempPath GetTempPathA_CE

UINT GetTempFileNameA_CE(LPCSTR lpPathName, LPCSTR lpPrefixString, UINT uUnique, LPSTR lpTempFileName);
#undef GetTempFileName
#define GetTempFileName GetTempFileNameA_CE

BOOL DeleteFileA_CE(LPCSTR lpFileName);
#undef DeleteFile
#define DeleteFile DeleteFileA_CE

HFONT CreateFontA_CE(int cHeight, int cWidth, int cEscapement, int cOrientation,
                     int cWeight, DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut,
                     DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision,
                     DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName);
#undef CreateFont
#define CreateFont CreateFontA_CE

UINT GetDlgItemTextA_CE(HWND hDlg, int nIDDlgItem, LPSTR lpString, int cchMax);
#undef GetDlgItemText
#define GetDlgItemText GetDlgItemTextA_CE

BOOL SetDlgItemTextA_CE(HWND hDlg, int nIDDlgItem, LPCSTR lpString);
#undef SetDlgItemText
#define SetDlgItemText SetDlgItemTextA_CE

BOOL GetOpenFileNameA_CE(LPOPENFILENAMEA lpofn);
#undef GetOpenFileName
#define GetOpenFileName GetOpenFileNameA_CE

BOOL GetSaveFileNameA_CE(LPOPENFILENAMEA lpofn);
#undef GetSaveFileName
#define GetSaveFileName GetSaveFileNameA_CE

#endif /* _WIN32_WCE */

#endif /* _CECOMPAT_H */
