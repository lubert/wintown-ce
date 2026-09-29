/* statusbar.c - Docked single-row status bar for WiNTown
 * Displays Funds, Go-To button, Notice ticker, Date, and RCI Demand Meter
 */

#include "statusbar.h"
#include "sim.h"
#include "notify.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
#ifdef _WIN32_WCE
#include "cecompat.h"
#endif

/* External data and functions */
extern HWND hwndMain;
extern char *FormatNumber(long n, char *buf);
extern void CenterMapOnTile(int tileX, int tileY);
extern void ShowBudgetWindow(HWND hwndParent);
extern void ShowEvaluationWindow(HWND hwndParent);

#define STATUSBAR_CLASS "WiNTownStatusBar"

#ifndef DEFAULT_GUI_FONT
#define DEFAULT_GUI_FONT SYSTEM_FONT
#endif

static HWND hwndStatusBar = NULL;
static BOOL isGoToPressed = FALSE;

/* Offscreen backbuffer for flicker-free painting */
static HDC hdcMem = NULL;
static HBITMAP hbmMem = NULL;
static HBITMAP hbmOld = NULL;
static int memWidth = 0;
static int memHeight = 0;

static const char *monthNames[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/* Draw 3D panel borders: sunken (field) or raised (button) */
static void Draw3DBevel(HDC hdc, int left, int top, int right, int bottom, BOOL sunken) {
    HBRUSH hbrTopLeft;
    HBRUSH hbrBottomRight;
    RECT rc;

    hbrTopLeft = (HBRUSH)GetStockObject(sunken ? DKGRAY_BRUSH : WHITE_BRUSH);
    hbrBottomRight = (HBRUSH)GetStockObject(sunken ? WHITE_BRUSH : DKGRAY_BRUSH);

    /* Top edge */
    rc.left = left; rc.top = top; rc.right = right; rc.bottom = top + 1;
    FillRect(hdc, &rc, hbrTopLeft);

    /* Left edge */
    rc.left = left; rc.top = top; rc.right = left + 1; rc.bottom = bottom;
    FillRect(hdc, &rc, hbrTopLeft);

    /* Bottom edge */
    rc.left = left; rc.top = bottom - 1; rc.right = right; rc.bottom = bottom;
    FillRect(hdc, &rc, hbrBottomRight);

    /* Right edge */
    rc.left = right - 1; rc.top = top; rc.right = right; rc.bottom = bottom;
    FillRect(hdc, &rc, hbrBottomRight);
}

/* Helper to get panel rectangles */
static void GetPanelRects(int barWidth, int barHeight,
                          RECT *prcFunds, RECT *prcGoTo,
                          RECT *prcNotice, RECT *prcDate, RECT *prcRCI) {
    int fundsW = 85;
    int goToW = 45;
    int rciW = 42;
    int dateW = 75;

    /* Panel 1: Funds */
    prcFunds->left = 2;
    prcFunds->top = 2;
    prcFunds->right = prcFunds->left + fundsW;
    prcFunds->bottom = barHeight - 2;

    /* Panel 2: Go-To button */
    prcGoTo->left = prcFunds->right + 3;
    prcGoTo->top = 2;
    prcGoTo->right = prcGoTo->left + goToW;
    prcGoTo->bottom = barHeight - 2;

    /* Panel 5: RCI Meter (right-anchored) */
    prcRCI->right = barWidth - 2;
    prcRCI->left = prcRCI->right - rciW;
    prcRCI->top = 2;
    prcRCI->bottom = barHeight - 2;

    /* Panel 4: Date (left of RCI) */
    prcDate->right = prcRCI->left - 3;
    prcDate->left = prcDate->right - dateW;
    prcDate->top = 2;
    prcDate->bottom = barHeight - 2;

    /* Panel 3: Notice Ticker (fills remaining space) */
    prcNotice->left = prcGoTo->right + 3;
    prcNotice->right = prcDate->left - 3;
    prcNotice->top = 2;
    prcNotice->bottom = barHeight - 2;
    if (prcNotice->right < prcNotice->left) {
        prcNotice->right = prcNotice->left;
    }
}

/* Paint RCI Demand Meter */
static void DrawRCIMeter(HDC hdc, const RECT *prc) {
    int midY;
    int maxBarH;
    int colW = 9;
    int gap = 3;
    int startX;
    int i;
    RECT rcBar;
    HBRUSH hbr;
    HPEN hPenDark, hOldPen;
    short valves[3];
    COLORREF colors[3];

    /* Fill background and bevel */
    FillRect(hdc, prc, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    Draw3DBevel(hdc, prc->left, prc->top, prc->right, prc->bottom, TRUE);

    midY = (prc->top + prc->bottom) / 2;
    maxBarH = (midY - prc->top - 2);
    if (maxBarH < 4) maxBarH = 4;

    valves[0] = RValve;
    valves[1] = CValve;
    valves[2] = IValve;

    colors[0] = RGB(0, 180, 0);     /* R: Green */
    colors[1] = RGB(0, 100, 230);   /* C: Blue */
    colors[2] = RGB(220, 170, 0);   /* I: Amber / Yellow */

    startX = prc->left + (prc->right - prc->left - (3 * colW + 2 * gap)) / 2;

    for (i = 0; i < 3; i++) {
        int v = valves[i];
        int barH;
        int bx = startX + i * (colW + gap);

        if (v > 2000) v = 2000;
        if (v < -2000) v = -2000;
        barH = (v * maxBarH) / 2000;

        rcBar.left = bx;
        rcBar.right = bx + colW;

        if (barH > 0) {
            rcBar.top = midY - barH;
            rcBar.bottom = midY;
            hbr = CreateSolidBrush(colors[i]);
            FillRect(hdc, &rcBar, hbr);
            DeleteObject(hbr);
        } else if (barH < 0) {
            rcBar.top = midY;
            rcBar.bottom = midY - barH;
            hbr = CreateSolidBrush(colors[i]);
            FillRect(hdc, &rcBar, hbr);
            DeleteObject(hbr);
        }
    }

    /* Draw center baseline across the columns */
    hPenDark = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
    hOldPen = (HPEN)SelectObject(hdc, hPenDark);
    MoveToEx(hdc, startX - 1, midY, NULL);
    LineTo(hdc, startX + 3 * colW + 2 * gap, midY);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPenDark);
}

/* Paint entire status bar */
static void PaintStatusBar(HWND hwnd, HDC hdc) {
    RECT rcClient;
    RECT rcTop;
    RECT rcFunds, rcGoTo, rcNotice, rcDate, rcRCI;
    RECT rcText;
    int locX, locY;
    BOOL hasLocation;
    char textBuf[128];
    char numBuf[32];
    const char *noticeMsg;
    HFONT hFont, hOldFont;

    GetClientRect(hwnd, &rcClient);
    if (rcClient.right <= rcClient.left || rcClient.bottom <= rcClient.top)
        return;

    /* Setup offscreen memory buffer for double-buffering */
    if (hdcMem == NULL || memWidth != rcClient.right || memHeight != rcClient.bottom) {
        if (hbmMem) {
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
        }
        if (hdcMem) {
            DeleteDC(hdcMem);
        }
        hdcMem = CreateCompatibleDC(hdc);
        hbmMem = CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
        hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        memWidth = rcClient.right;
        memHeight = rcClient.bottom;
    }

    /* Fill background */
    FillRect(hdcMem, &rcClient, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

    /* Top border line separating from CommandBar / menu */
    rcTop.left = 0; rcTop.top = 0; rcTop.right = rcClient.right; rcTop.bottom = 1;
    FillRect(hdcMem, &rcTop, (HBRUSH)GetStockObject(WHITE_BRUSH));

    /* Calculate panel positions */
    GetPanelRects(rcClient.right, rcClient.bottom, &rcFunds, &rcGoTo, &rcNotice, &rcDate, &rcRCI);

    /* Use default GUI font */
    hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    hOldFont = (HFONT)SelectObject(hdcMem, hFont);
    SetBkMode(hdcMem, TRANSPARENT);

    /* 1. Panel: Funds */
    FillRect(hdcMem, &rcFunds, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    Draw3DBevel(hdcMem, rcFunds.left, rcFunds.top, rcFunds.right, rcFunds.bottom, TRUE);
    FormatNumber(TotalFunds, numBuf);
    wsprintf(textBuf, "$%s", numBuf);
    SetTextColor(hdcMem, (TotalFunds < 0) ? RGB(180, 0, 0) : RGB(0, 0, 0));
    rcText = rcFunds;
    rcText.top += 1;
    DrawText(hdcMem, textBuf, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    /* 2. Panel: Go-To Button */
    hasLocation = GetNoticeLocation(&locX, &locY);
    FillRect(hdcMem, &rcGoTo, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    Draw3DBevel(hdcMem, rcGoTo.left, rcGoTo.top, rcGoTo.right, rcGoTo.bottom,
                hasLocation ? isGoToPressed : TRUE);
    rcText = rcGoTo;
    if (hasLocation && isGoToPressed) {
        rcText.left += 1;
        rcText.top += 1;
    }
    SetTextColor(hdcMem, hasLocation ? RGB(0, 0, 0) : RGB(128, 128, 128));
    DrawText(hdcMem, "Go-To", -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    /* 3. Panel: Notice Ticker */
    FillRect(hdcMem, &rcNotice, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    Draw3DBevel(hdcMem, rcNotice.left, rcNotice.top, rcNotice.right, rcNotice.bottom, TRUE);
    noticeMsg = GetCurrentNoticeMessage();
    rcText = rcNotice;
    rcText.left += 4;
    rcText.right -= 4;
    rcText.top += 1;
    SetTextColor(hdcMem, hasLocation ? RGB(160, 0, 0) : RGB(0, 0, 0));
    DrawText(hdcMem, noticeMsg, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    /* 4. Panel: Date */
    FillRect(hdcMem, &rcDate, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    Draw3DBevel(hdcMem, rcDate.left, rcDate.top, rcDate.right, rcDate.bottom, TRUE);
    wsprintf(textBuf, "%s %d", monthNames[CityMonth % 12], CityYear);
    SetTextColor(hdcMem, RGB(0, 0, 0));
    rcText = rcDate;
    rcText.top += 1;
    DrawText(hdcMem, textBuf, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    /* 5. Panel: RCI Demand Meter */
    DrawRCIMeter(hdcMem, &rcRCI);

    /* Blit offscreen buffer to window DC */
    BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldFont);
}

/* Status bar window procedure */
static LRESULT CALLBACK StatusBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintStatusBar(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; /* Suppress erase background to prevent flicker */

    case WM_LBUTTONDOWN: {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);
        RECT rcClient;
        RECT rcFunds, rcGoTo, rcNotice, rcDate, rcRCI;
        POINT pt;

        GetClientRect(hwnd, &rcClient);
        GetPanelRects(rcClient.right, rcClient.bottom, &rcFunds, &rcGoTo, &rcNotice, &rcDate, &rcRCI);

        pt.x = xPos;
        pt.y = yPos;

        if (PtInRect(&rcFunds, pt)) {
            /* Click on Funds: open City Budget dialog */
            ShowBudgetWindow(hwndMain);
            return 0;
        }

        if (PtInRect(&rcGoTo, pt)) {
            int lx, ly;
            if (GetNoticeLocation(&lx, &ly)) {
                isGoToPressed = TRUE;
                SetCapture(hwnd);
                InvalidateRect(hwnd, &rcGoTo, FALSE);
            }
            return 0;
        }

        if (PtInRect(&rcNotice, pt)) {
            int lx, ly;
            if (GetNoticeLocation(&lx, &ly)) {
                CenterMapOnTile(lx, ly);
            }
            return 0;
        }

        if (PtInRect(&rcDate, pt)) {
            /* Click on Date: open Evaluation / Newspaper window */
            ShowEvaluationWindow(hwndMain);
            return 0;
        }

        return 0;
    }

    case WM_LBUTTONUP: {
        int xPos;
        int yPos;
        RECT rcClient;
        RECT rcFunds, rcGoTo, rcNotice, rcDate, rcRCI;
        POINT pt;
        int lx, ly;

        if (isGoToPressed) {
            xPos = LOWORD(lParam);
            yPos = HIWORD(lParam);

            isGoToPressed = FALSE;
            ReleaseCapture();

            GetClientRect(hwnd, &rcClient);
            GetPanelRects(rcClient.right, rcClient.bottom, &rcFunds, &rcGoTo, &rcNotice, &rcDate, &rcRCI);

            pt.x = xPos;
            pt.y = yPos;

            InvalidateRect(hwnd, &rcGoTo, FALSE);

            if (PtInRect(&rcGoTo, pt) && GetNoticeLocation(&lx, &ly)) {
                CenterMapOnTile(lx, ly);
            }
            return 0;
        }
        return 0;
    }

    case WM_DESTROY:
        CleanupStatusBar();
        hwndStatusBar = NULL;
        return 0;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

/* Create status bar window */
HWND CreateStatusBar(HWND hwndParent, int x, int y, int width, int height) {
    static BOOL registered = FALSE;
    WNDCLASS wc;
    HINSTANCE hInst;

    hInst = GetModuleHandle(NULL);

    if (!registered) {
        ZeroMemory(&wc, sizeof(wc));
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = StatusBarProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
        wc.lpszClassName = STATUSBAR_CLASS;

        RegisterClass(&wc);
        registered = TRUE;
    }

    hwndStatusBar = CreateWindow(
        STATUSBAR_CLASS, NULL,
        WS_CHILD | WS_VISIBLE,
        x, y, width, height,
        hwndParent, NULL, hInst, NULL);

    return hwndStatusBar;
}

HWND GetStatusBarWindow(void) {
    return hwndStatusBar;
}

void UpdateStatusBar(void) {
    if (hwndStatusBar && IsWindow(hwndStatusBar)) {
        InvalidateRect(hwndStatusBar, NULL, FALSE);
    }
}

void CleanupStatusBar(void) {
    if (hdcMem) {
        if (hbmMem) {
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            hbmMem = NULL;
        }
        DeleteDC(hdcMem);
        hdcMem = NULL;
    }
}
