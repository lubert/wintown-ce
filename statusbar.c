/* statusbar.c - Docked dual-row status bar for WiNTown
 * Row 1: Funds, Go-To button, Notice ticker, Date
 * Row 2: Active Tool/Cost, Bookmarks [1..5], City Class/Population
 * Far Right (spans rows 1 & 2): RCI Demand Meter
 */

#include "statusbar.h"
#include "sim.h"
#include "notify.h"
#include "tools.h"
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
extern void GetCurrentCenterTile(int *tileX, int *tileY);
extern void ShowBudgetWindow(HWND hwndParent);
extern void ShowEvaluationWindow(HWND hwndParent);
#define IDM_VIEW_CHARTSWINDOW 4106

#define STATUSBAR_CLASS "WiNTownStatusBar"

#ifndef DEFAULT_GUI_FONT
#define DEFAULT_GUI_FONT SYSTEM_FONT
#endif

static HWND hwndStatusBar = NULL;
static BOOL isGoToPressed = FALSE;
static HFONT hFontRCI = NULL;

/* Bookmarks: 5 map location slots */
typedef struct {
    int x;
    int y;
} BookmarkLocation;

static BookmarkLocation cityBookmarks[5] = {
    {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}
};
static int pressedBookmark = -1;

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

static const char *cityClassNames[6] = {
    "Village", "Town", "City", "Capital", "Metropolis", "Megalopolis"
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

/* Helper to calculate all panel rectangles across the 2 rows */
static void GetPanelRects(int barWidth, int barHeight,
                          RECT *prcFunds, RECT *prcGoTo, RECT *prcNotice, RECT *prcDate,
                          RECT *prcTool, RECT *prcBookmarkLabel, RECT prcBookmarkBtn[5],
                          RECT *prcInfo, RECT *prcRCI) {
    int row1Top = 2;
    int row1Bottom = 19;
    int row2Top = 21;
    int row2Bottom = barHeight - 2;

    int rciW = 44;
    int fundsW = 95;
    int goToW = 45;
    int dateW = 75;

    int toolW = 145;
    int bmLabelW = 60;
    int btnW = 16;
    int btnGap = 2;
    int i;

    /* Far Right: RCI Meter spans BOTH rows */
    prcRCI->right = barWidth - 2;
    prcRCI->left = prcRCI->right - rciW;
    prcRCI->top = 2;
    prcRCI->bottom = barHeight - 2;

    /* --- Row 1 --- */
    /* Panel 1: Funds */
    prcFunds->left = 2;
    prcFunds->right = prcFunds->left + fundsW;
    prcFunds->top = row1Top;
    prcFunds->bottom = row1Bottom;

    /* Panel 2: Go-To button */
    prcGoTo->left = prcFunds->right + 3;
    prcGoTo->right = prcGoTo->left + goToW;
    prcGoTo->top = row1Top;
    prcGoTo->bottom = row1Bottom;

    /* Panel 4: Date (anchored right of Row 1, left of RCI) */
    prcDate->right = prcRCI->left - 3;
    prcDate->left = prcDate->right - dateW;
    prcDate->top = row1Top;
    prcDate->bottom = row1Bottom;

    /* Panel 3: Notice Ticker (fills middle of Row 1) */
    prcNotice->left = prcGoTo->right + 3;
    prcNotice->right = prcDate->left - 3;
    prcNotice->top = row1Top;
    prcNotice->bottom = row1Bottom;
    if (prcNotice->right < prcNotice->left) {
        prcNotice->right = prcNotice->left;
    }

    /* --- Row 2 --- */
    /* Panel 5: Active Tool name and cost */
    prcTool->left = 2;
    prcTool->right = prcTool->left + toolW;
    prcTool->top = row2Top;
    prcTool->bottom = row2Bottom;

    /* Panel 6: Bookmark Label */
    prcBookmarkLabel->left = prcTool->right + 3;
    prcBookmarkLabel->right = prcBookmarkLabel->left + bmLabelW;
    prcBookmarkLabel->top = row2Top;
    prcBookmarkLabel->bottom = row2Bottom;

    /* Panels 7-11: Bookmark Buttons 1..5 */
    for (i = 0; i < 5; i++) {
        prcBookmarkBtn[i].left = prcBookmarkLabel->right + 2 + i * (btnW + btnGap);
        prcBookmarkBtn[i].right = prcBookmarkBtn[i].left + btnW;
        prcBookmarkBtn[i].top = row2Top;
        prcBookmarkBtn[i].bottom = row2Bottom;
    }

    /* Panel 12: City Class and Population (fills rest of Row 2) */
    prcInfo->left = prcBookmarkBtn[4].right + 3;
    prcInfo->right = prcRCI->left - 3;
    prcInfo->top = row2Top;
    prcInfo->bottom = row2Bottom;
    if (prcInfo->right < prcInfo->left) {
        prcInfo->right = prcInfo->left;
    }
}

/* Paint 2-Row Tall RCI Demand Meter with center labels */
static void DrawRCIMeter(HDC hdc, const RECT *prc) {
    int midY;
    int textTop, textBottom;
    int colW = 10;
    int gap = 3;
    int startX;
    int i;
    RECT rcBar, rcText;
    HBRUSH hbr;
    HPEN hPenDark, hOldPen;
    short valves[3];
    COLORREF colors[3];
    int maxUpH, maxDownH;
    static const char *labels[3] = { "R", "C", "I" };

    HFONT hOldFont = NULL;

    /* Fill background and bevel */
    FillRect(hdc, prc, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    Draw3DBevel(hdc, prc->left, prc->top, prc->right, prc->bottom, TRUE);

    midY = (prc->top + prc->bottom) / 2;
    textTop = midY - 5;
    textBottom = midY + 5;

    valves[0] = RValve;
    valves[1] = CValve;
    valves[2] = IValve;

    colors[0] = RGB(0, 255, 0);     /* R: Primary Green */
    colors[1] = RGB(0, 0, 255);     /* C: Primary Blue */
    colors[2] = RGB(255, 255, 0);   /* I: Primary Yellow */

    startX = prc->left + (prc->right - prc->left - (3 * colW + 2 * gap)) / 2;

    /* Draw center labels R, C, I with matching primary colors */
    if (!hFontRCI) {
        hFontRCI = CreateFont(9, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
    }
    if (hFontRCI) {
        hOldFont = (HFONT)SelectObject(hdc, hFontRCI);
    }

    SetBkMode(hdc, TRANSPARENT);
    for (i = 0; i < 3; i++) {
        rcText.left = startX + i * (colW + gap);
        rcText.right = rcText.left + colW;
        rcText.top = textTop;
        rcText.bottom = textBottom;
        SetTextColor(hdc, colors[i]);
        DrawText(hdc, labels[i], 1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    if (hOldFont) {
        SelectObject(hdc, hOldFont);
    }

    maxUpH = textTop - (prc->top + 2);
    maxDownH = (prc->bottom - 2) - textBottom;
    if (maxUpH < 3) maxUpH = 3;
    if (maxDownH < 3) maxDownH = 3;

    /* Draw positive (above) and negative (below) demand bars */
    for (i = 0; i < 3; i++) {
        int v = valves[i];
        int bx = startX + i * (colW + gap);
        rcBar.left = bx;
        rcBar.right = bx + colW;

        if (v > 2000) v = 2000;
        if (v < -2000) v = -2000;

        if (v > 0) {
            int barH = (v * maxUpH) / 2000;
            if (barH < 1 && v > 50) barH = 1;
            rcBar.top = textTop - barH;
            rcBar.bottom = textTop;
            hbr = CreateSolidBrush(colors[i]);
            FillRect(hdc, &rcBar, hbr);
            DeleteObject(hbr);
        } else if (v < 0) {
            int barH = ((-v) * maxDownH) / 2000;
            if (barH < 1 && v < -50) barH = 1;
            rcBar.top = textBottom;
            rcBar.bottom = textBottom + barH;
            hbr = CreateSolidBrush(colors[i]);
            FillRect(hdc, &rcBar, hbr);
            DeleteObject(hbr);
        }
    }
}

/* Paint entire status bar */
static void PaintStatusBar(HWND hwnd, HDC hdc) {
    RECT rcClient;
    RECT rcTop, rcRowDiv;
    RECT rcFunds, rcGoTo, rcNotice, rcDate;
    RECT rcTool, rcBookmarkLabel, rcBookmarkBtn[5], rcInfo, rcRCI;
    RECT rcText;
    int locX, locY;
    BOOL hasLocation;
    char textBuf[128];
    char numBuf[32];
    char toolBuf[128];
    char btnStr[8];
    const char *noticeMsg;
    HFONT hFont, hOldFont;
    int i, cClass;

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
    GetPanelRects(rcClient.right, rcClient.bottom,
                  &rcFunds, &rcGoTo, &rcNotice, &rcDate,
                  &rcTool, &rcBookmarkLabel, rcBookmarkBtn,
                  &rcInfo, &rcRCI);

    /* Subtle row divider line between row 1 and row 2 (up to RCI meter) */
    rcRowDiv.left = 0; rcRowDiv.top = 20; rcRowDiv.right = rcRCI.left - 1; rcRowDiv.bottom = 21;
    FillRect(hdcMem, &rcRowDiv, (HBRUSH)GetStockObject(WHITE_BRUSH));

    /* Use default GUI font */
    hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    hOldFont = (HFONT)SelectObject(hdcMem, hFont);
    SetBkMode(hdcMem, TRANSPARENT);

    /* ================= ROW 1 ================= */

    /* 1. Panel: Funds */
    FillRect(hdcMem, &rcFunds, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    Draw3DBevel(hdcMem, rcFunds.left, rcFunds.top, rcFunds.right, rcFunds.bottom, TRUE);
    FormatNumber(TotalFunds, numBuf);
    wsprintf(textBuf, "Funds: $%s", numBuf);
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

    /* ================= ROW 2 ================= */

    /* 5. Panel: Active Tool and Cost */
    FillRect(hdcMem, &rcTool, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    Draw3DBevel(hdcMem, rcTool.left, rcTool.top, rcTool.right, rcTool.bottom, TRUE);
    GetCurrentToolInfo(toolBuf, sizeof(toolBuf));
    rcText = rcTool;
    rcText.left += 5;
    rcText.right -= 4;
    rcText.top += 1;
    SetTextColor(hdcMem, RGB(0, 0, 0));
    DrawText(hdcMem, toolBuf, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    /* 6. Panel: Bookmark Label */
    FillRect(hdcMem, &rcBookmarkLabel, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    Draw3DBevel(hdcMem, rcBookmarkLabel.left, rcBookmarkLabel.top, rcBookmarkLabel.right, rcBookmarkLabel.bottom, TRUE);
    rcText = rcBookmarkLabel;
    rcText.top += 1;
    SetTextColor(hdcMem, RGB(0, 0, 0));
    DrawText(hdcMem, "Bookmark", -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    /* 7-11. Panels: Bookmark Buttons 1..5 */
    for (i = 0; i < 5; i++) {
        BOOL isSet = (cityBookmarks[i].x >= 0);
        BOOL isPressed = (pressedBookmark == i);

        FillRect(hdcMem, &rcBookmarkBtn[i], (HBRUSH)GetStockObject(LTGRAY_BRUSH));
        Draw3DBevel(hdcMem, rcBookmarkBtn[i].left, rcBookmarkBtn[i].top,
                    rcBookmarkBtn[i].right, rcBookmarkBtn[i].bottom,
                    isPressed ? TRUE : !isSet);

        rcText = rcBookmarkBtn[i];
        if (isPressed) {
            rcText.left += 1;
            rcText.top += 1;
        }
        wsprintf(btnStr, "%d", i + 1);
        SetTextColor(hdcMem, isSet ? RGB(0, 0, 0) : RGB(130, 130, 130));
        DrawText(hdcMem, btnStr, 1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    /* 12. Panel: City Class & Population */
    FillRect(hdcMem, &rcInfo, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    Draw3DBevel(hdcMem, rcInfo.left, rcInfo.top, rcInfo.right, rcInfo.bottom, TRUE);
    cClass = CityClass;
    if (cClass < 0) cClass = 0;
    if (cClass > 5) cClass = 5;
    FormatNumber(CityPop, numBuf);
    wsprintf(textBuf, "%s - Pop: %s", cityClassNames[cClass], numBuf);
    SetTextColor(hdcMem, RGB(0, 0, 0));
    rcText = rcInfo;
    rcText.top += 1;
    DrawText(hdcMem, textBuf, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    /* ================= FAR RIGHT: RCI METER ================= */
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
        RECT rcFunds, rcGoTo, rcNotice, rcDate;
        RECT rcTool, rcBookmarkLabel, rcBookmarkBtn[5], rcInfo, rcRCI;
        POINT pt;
        int i;

        GetClientRect(hwnd, &rcClient);
        GetPanelRects(rcClient.right, rcClient.bottom,
                      &rcFunds, &rcGoTo, &rcNotice, &rcDate,
                      &rcTool, &rcBookmarkLabel, rcBookmarkBtn,
                      &rcInfo, &rcRCI);

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

        if (PtInRect(&rcDate, pt) || PtInRect(&rcInfo, pt)) {
            /* Click on Date or City Info: open Evaluation / Newspaper window */
            ShowEvaluationWindow(hwndMain);
            return 0;
        }

        if (PtInRect(&rcRCI, pt)) {
            /* Click on RCI Meter: toggle Charts window */
            SendMessage(hwndMain, WM_COMMAND, IDM_VIEW_CHARTSWINDOW, 0);
            return 0;
        }

        for (i = 0; i < 5; i++) {
            if (PtInRect(&rcBookmarkBtn[i], pt)) {
                pressedBookmark = i;
                SetCapture(hwnd);
                InvalidateRect(hwnd, &rcBookmarkBtn[i], FALSE);
                return 0;
            }
        }

        return 0;
    }

    case WM_LBUTTONUP: {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);
        RECT rcClient;
        RECT rcFunds, rcGoTo, rcNotice, rcDate;
        RECT rcTool, rcBookmarkLabel, rcBookmarkBtn[5], rcInfo, rcRCI;
        POINT pt;
        int lx, ly;

        pt.x = xPos;
        pt.y = yPos;

        GetClientRect(hwnd, &rcClient);
        GetPanelRects(rcClient.right, rcClient.bottom,
                      &rcFunds, &rcGoTo, &rcNotice, &rcDate,
                      &rcTool, &rcBookmarkLabel, rcBookmarkBtn,
                      &rcInfo, &rcRCI);

        if (isGoToPressed) {
            isGoToPressed = FALSE;
            ReleaseCapture();
            InvalidateRect(hwnd, &rcGoTo, FALSE);

            if (PtInRect(&rcGoTo, pt) && GetNoticeLocation(&lx, &ly)) {
                CenterMapOnTile(lx, ly);
            }
            return 0;
        }

        if (pressedBookmark >= 0) {
            int btn = pressedBookmark;
            pressedBookmark = -1;
            ReleaseCapture();
            InvalidateRect(hwnd, NULL, FALSE);

            if (PtInRect(&rcBookmarkBtn[btn], pt)) {
                if (cityBookmarks[btn].x >= 0) {
                    /* Jump to saved bookmark */
                    CenterMapOnTile(cityBookmarks[btn].x, cityBookmarks[btn].y);
                } else {
                    /* Set bookmark to current map center */
                    int cx = 0, cy = 0;
                    char msgBuf[64];
                    GetCurrentCenterTile(&cx, &cy);
                    cityBookmarks[btn].x = cx;
                    cityBookmarks[btn].y = cy;
                    wsprintf(msgBuf, "Bookmark %d set at (%d, %d)", btn + 1, cx, cy);
                    SetCurrentNoticeMessage(msgBuf);
                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            return 0;
        }

        return 0;
    }

    case WM_RBUTTONDOWN: {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);
        RECT rcClient;
        RECT rcFunds, rcGoTo, rcNotice, rcDate;
        RECT rcTool, rcBookmarkLabel, rcBookmarkBtn[5], rcInfo, rcRCI;
        POINT pt;
        int i;

        pt.x = xPos;
        pt.y = yPos;

        GetClientRect(hwnd, &rcClient);
        GetPanelRects(rcClient.right, rcClient.bottom,
                      &rcFunds, &rcGoTo, &rcNotice, &rcDate,
                      &rcTool, &rcBookmarkLabel, rcBookmarkBtn,
                      &rcInfo, &rcRCI);

        /* Right-click on a bookmark button re-sets it to current location */
        for (i = 0; i < 5; i++) {
            if (PtInRect(&rcBookmarkBtn[i], pt)) {
                int cx = 0, cy = 0;
                char msgBuf[64];
                GetCurrentCenterTile(&cx, &cy);
                cityBookmarks[i].x = cx;
                cityBookmarks[i].y = cy;
                wsprintf(msgBuf, "Bookmark %d updated to (%d, %d)", i + 1, cx, cy);
                SetCurrentNoticeMessage(msgBuf);
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
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
    if (hFontRCI) {
        DeleteObject(hFontRCI);
        hFontRCI = NULL;
    }
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
