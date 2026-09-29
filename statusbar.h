/* statusbar.h - Docked single-row status bar for WiNTown
 * Displays Funds, Go-To button, Notice ticker, Date, and RCI Demand Meter
 */

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <windows.h>

#define STATUSBAR_HEIGHT 20

/* Initialize and create status bar window */
HWND CreateStatusBar(HWND hwndParent, int x, int y, int width, int height);

/* Get handle to status bar window */
HWND GetStatusBarWindow(void);

/* Request status bar repaint / update */
void UpdateStatusBar(void);

/* Clean up status bar resources */
void CleanupStatusBar(void);

#endif /* STATUSBAR_H */
