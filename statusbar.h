/* statusbar.h - Docked single-row status bar for WiNTown
 * Displays Funds, Go-To button, Notice ticker, Date, and RCI Demand Meter
 */

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <windows.h>

#define STATUSBAR_ROW_HEIGHT 20
#define STATUSBAR_NUM_ROWS   2
#define STATUSBAR_HEIGHT     (STATUSBAR_ROW_HEIGHT * STATUSBAR_NUM_ROWS)

/* Initialize and create status bar window */
HWND CreateStatusBar(HWND hwndParent, int x, int y, int width, int height);

/* Get handle to status bar window */
HWND GetStatusBarWindow(void);

/* Request status bar repaint / update */
void UpdateStatusBar(void);

/* Clean up status bar resources */
void CleanupStatusBar(void);

#endif /* STATUSBAR_H */
