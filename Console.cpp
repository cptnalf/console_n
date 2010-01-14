/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2001-2004 by Marko Bozikovic
// All rights reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Send bug reports, bug fixes, enhancements, requests, flames, etc., and
// I'll try to keep a version up to date.  I can be reached as follows:
//    marko.bozikovic@alterbox.net
//    bozho@kset.org
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Console.cpp - console class implementation

#include "stdafx.h"
#include <atlbase.h>
#include <msxml.h>
#include <shellapi.h>
#include <commctrl.h>
#include <memory>

#include "resource.h"
#include "FileStream.h"
#include "ComBSTROut.h"
#include "ComVariantOut.h"
#include "Cursors.h"
#include "Dialogs.h"
#include "Console.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Console class


/////////////////////////////////////////////////////////////////////////////
// Console window class names
const TCHAR Console::m_szConsoleClass[] = _T("Console Main Command Window");
const TCHAR Console::m_szHiddenParentClass[] = _T("Console Invisible Parent Window");


/////////////////////////////////////////////////////////////////////////////
// win console window title
const tstring Console::m_strWinConsoleTitle(_T("Console Command Window"));


/////////////////////////////////////////////////////////////////////////////
// ctor/dtor

Console::Console(LPCTSTR pszConfigFile, LPCTSTR pszShellCmdLine, LPCTSTR pszConsoleTitle, LPCTSTR pszReloadNewConfig)
: m_hWnd(NULL)
, m_bInitializing(TRUE)
, m_bReloading(FALSE)
, m_strConfigEditor(_T("notepad.exe"))
, m_strConfigEditorParams(_T(""))
, m_strShell(_T(""))
, m_strShellCmdLine(pszShellCmdLine)
, m_hwndInvisParent(NULL)
, m_hdcConsole(NULL)
, m_hdcWindow(NULL)
, m_hbmpConsole(NULL)
, m_hbmpConsoleOld(NULL)
, m_hbmpWindow(NULL)
, m_hbmpWindowOld(NULL)
, m_hBkBrush(NULL)
, m_dwMasterRepaintInt(500)
, m_dwChangeRepaintInt(50)
, m_strIconFilename(_T(""))
, m_hSmallIcon(NULL)
, m_hBigIcon(NULL)
, m_hPopupMenu(NULL)
, m_hSysMenu(NULL)
, m_hConfigFilesMenu(NULL)
, m_bPopupMenuDisabled(FALSE)
, m_strWindowTitleDefault(_tcslen(pszConsoleTitle) == 0 ? _T("console") : pszConsoleTitle)
, m_strWindowTitle(m_strWindowTitleDefault)
, m_strWindowTitleCurrent(m_strWindowTitleDefault)
, m_strFontName(_T("Lucida Console"))
, m_dwFontSize(8)
, m_bBold(FALSE)
, m_bItalic(FALSE)
, m_bUseFontColor(FALSE)
, m_crFontColor(RGB(0, 0, 0))
, m_hFont(NULL)
, m_hFontOld(NULL)
	, m_shadowDistance(0)
	, m_shadowColor(0)
, m_nX(0)
, m_nY(0)
, m_nInsideBorder(0)
, m_nWindowWidth(0)
, m_nWindowHeight(0)
, m_nXBorderSize(0)
, m_nYBorderSize(0)
, m_nCaptionSize(0)
, m_nClientWidth(0)
, m_nClientHeight(0)
, m_nCharHeight(0)
, m_nCharWidth(0)
, m_dwWindowBorder(BORDER_NONE)
, m_bShowScrollbar(FALSE)
, m_nScrollbarStyle(FSB_REGULAR_MODE)
, m_crScrollbarColor(::GetSysColor(COLOR_3DHILIGHT))
, m_nScrollbarWidth(::GetSystemMetrics(SM_CXVSCROLL))
, m_nScrollbarButtonHeight(::GetSystemMetrics(SM_CYVSCROLL))
, m_nScrollbarThunmbHeight(::GetSystemMetrics(SM_CYVTHUMB))
, m_dwTaskbarButton(TASKBAR_BUTTON_NORMAL)
, m_bMouseDragable(TRUE)
, m_nSnapDst(10)
, m_dwDocked(DOCK_NONE)
, m_dwOriginalZOrder(Z_ORDER_REGULAR)
, m_dwCurrentZOrder(Z_ORDER_REGULAR)
, m_dwTransparency(TRANSPARENCY_NONE)
, m_byAlpha(150)
, m_byInactiveAlpha(150)
, m_bBkColorSet(FALSE)
, m_crBackground(RGB(0, 0, 0))
, m_bTintSet(FALSE)
, m_byTintOpacity(50)
, m_byTintR(0)
, m_byTintG(0)
, m_byTintB(0)
, m_bBitmapBackground(FALSE)
, m_strBackgroundFile(_T(""))
, m_hdcBackground(NULL)
, m_hbmpBackground(NULL)
, m_hbmpBackgroundOld(NULL)
, m_dwBackgroundStyle(BACKGROUND_STYLE_RESIZE)
, m_bRelativeBackground(FALSE)
, m_bExtendBackground(FALSE)
, m_nBackgroundOffsetX(0)
, m_nBackgroundOffsetY(0)
, m_bHideWindow(FALSE)
, m_bHideConsole(TRUE)
, m_dwHideConsoleTimeout(0)
, m_bStartMinimized(FALSE)
, m_dwCursorStyle(CURSOR_STYLE_CONSOLE)
, m_crCursorColor(RGB(255, 255, 255))
, m_bCursorVisible(FALSE)
, m_pCursor(NULL)
, m_hWndConsole(NULL)
, m_hStdOut(NULL)
, m_hStdOutFresh(NULL)
, m_hQuitEvent(NULL)
, m_hConsoleProcess(NULL)
, m_hMonitorThread(NULL)
, m_dwRows(25)
, m_dwColumns(80)
, m_dwBufferRows(25)
, m_bUseTextBuffer(FALSE)
, m_nTextSelection(TEXT_SELECTION_NONE)
, m_bCopyOnSelect(FALSE)
, m_bInverseShift(FALSE)
, m_hdcSelection(NULL)
, m_hbmpSelection(NULL)
, m_hbmpSelectionOld(NULL)
, m_hbrushSelection(::CreateSolidBrush(RGB(0xff, 0xff, 0xff)))
, m_pScreenBuffer(NULL)
, m_pScreenBufferNew(NULL)
{

	m_strConfigFile = GetFullFilename(pszConfigFile);

	if (!_tcsicmp(pszReloadNewConfig, _T("yes"))) 
		{
			m_dwReloadNewConfigDefault = RELOAD_NEW_CONFIG_YES;
		} 
	else if (!_tcsicmp(pszReloadNewConfig, _T("no"))) 
		{
			m_dwReloadNewConfigDefault = RELOAD_NEW_CONFIG_NO;
		} 
	else 
		{
			m_dwReloadNewConfigDefault = RELOAD_NEW_CONFIG_PROMPT;
		}
	
	m_dwReloadNewConfig = m_dwReloadNewConfigDefault;
		
	m_mouseCursorOffset.x = 0;
	m_mouseCursorOffset.y = 0;

	m_coordSelOrigin.X = 0;
	m_coordSelOrigin.Y = 0;

	::ZeroMemory(&m_rectSelection, sizeof(RECT));

	// get Console.exe directory
	TCHAR szPathName[MAX_PATH];
	::ZeroMemory(szPathName, sizeof(szPathName));
	::GetModuleFileName(g_hInstance, szPathName, MAX_PATH);

	tstring	strExeDir(szPathName);

	strExeDir = strExeDir.substr(0, strExeDir.rfind(_T("\\")));
	strExeDir += TCHAR('\\');

	// if no config file is given, get console.xml from the startup directory
	if (m_strConfigFile.length() == 0) 
		{
			
			m_strConfigFile = strExeDir + tstring(_T("console.xml"));
		}
	
	// get readme filename
	m_strReadmeFile = strExeDir + tstring(_T("Readme.txt"));
	
	::ZeroMemory(&m_csbiCursor, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
	::ZeroMemory(&m_csbiConsole, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
	
	SetDefaultConsoleColors();
}

Console::~Console() 
{
	
	DEL_ARR(m_pScreenBuffer);
	DEL_ARR(m_pScreenBufferNew);
	
	if (m_hSmallIcon) ::DestroyIcon(m_hSmallIcon);
	if (m_hBigIcon) ::DestroyIcon(m_hBigIcon);
	
	::CloseHandle(m_hConsoleProcess);
	::CloseHandle(m_hQuitEvent);
	
	// shutdown console process
	::CloseHandle(m_hStdOut);
	::CloseHandle(m_hStdOutFresh);
	if (m_hWndConsole) ::SendMessage(m_hWndConsole, WM_CLOSE, 0, 0);	
	::FreeConsole();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Public methods

/////////////////////////////////////////////////////////////////////////////
// creates and shows Console window

BOOL Console::Create(TCHAR* pszConfigPath) 
{
	if (!GetOptions()) return FALSE;
	
	if (!RegisterWindowClasses()) return FALSE;
	
	if (!CreateConsoleWindow()) return FALSE;

	// create window DC
	HDC hdcDesktop = ::GetDCEx(m_hWnd, NULL, 0);
	m_hdcConsole = ::CreateCompatibleDC(hdcDesktop);
	m_hdcWindow = ::CreateCompatibleDC(hdcDesktop);
	::ReleaseDC(m_hWnd, hdcDesktop);
	
	// create selection DC
	m_hdcSelection = ::CreateCompatibleDC(m_hdcWindow);
	
	if (!SetupMenus()) return FALSE;

	CreateNewFont();
	CreateNewBrush();
	
	SetWindowTransparency();
	SetWindowIcons();

	if (!StartShellProcess()) ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
	
	CreateCursor();
	
	// now we can start the monitor thread
	::ResumeThread(m_hMonitorThread);
	
	// set the long repaint timer
	if (m_dwMasterRepaintInt) ::SetTimer(m_hWnd, TIMER_REPAINT_MASTER, m_dwMasterRepaintInt, NULL);
	
	m_bInitializing = FALSE;
	
	// register for files/folders drag-n-drop
	::DragAcceptFiles(m_hWnd, TRUE);

	RefreshScreenBuffer();
	::CopyMemory(m_pScreenBuffer, m_pScreenBufferNew, sizeof(CHAR_INFO) * m_dwRows * m_dwColumns);
	RepaintWindow();
	::UpdateWindow(m_hWnd);

	if (m_bStartMinimized) 
		{
			if (m_dwTaskbarButton > TASKBAR_BUTTON_NORMAL) 
				{
					m_bHideWindow = TRUE;
				} 
			else
				{
					::ShowWindow(m_hWnd, SW_MINIMIZE);
				}
		} 
	else 
		{
			::ShowWindow(m_hWnd, SW_SHOW);
		}
	
	::SetForegroundWindow(m_hWnd);
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Message handlers

/////////////////////////////////////////////////////////////////////////////

void Console::OnDestroy() 
{
	
	if (!m_bInitializing) 
		{
			::SetEvent(m_hQuitEvent);
			::WaitForSingleObject(m_hMonitorThread, 2000);
			::CloseHandle(m_hMonitorThread);
			
			// kill timers
			::KillTimer(m_hWnd, TIMER_REPAINT_CHANGE);
			if (m_dwMasterRepaintInt) ::KillTimer(m_hWnd, TIMER_REPAINT_MASTER);
			
			DestroyCursor();
			
			// cleanup graphics objects
			if (m_hFontOld) ::SelectObject(m_hdcConsole, m_hFontOld);
			if (m_hFont) ::DeleteObject(m_hFont);
			
			if (m_hBkBrush) ::DeleteObject(m_hBkBrush);
			
			if (m_hbmpConsoleOld) ::SelectObject(m_hdcConsole, m_hbmpConsoleOld);
			if (m_hbmpConsole) ::DeleteObject(m_hbmpConsole);
		
			if (m_hbmpWindowOld) ::SelectObject(m_hdcWindow, m_hbmpWindowOld);
			if (m_hbmpWindow) ::DeleteObject(m_hbmpWindow);
		
			if (m_hbmpBackgroundOld) ::SelectObject(m_hdcBackground, m_hbmpBackgroundOld);
			if (m_hbmpBackground) ::DeleteObject(m_hbmpBackground);
			if (m_hdcBackground) ::DeleteDC(m_hdcBackground);
		
			if (m_hbmpSelectionOld) ::SelectObject(m_hdcSelection, m_hbmpSelectionOld);
			if (m_hbmpSelection)::DeleteObject(m_hbmpSelection);
			if (m_hdcSelection) ::DeleteDC(m_hdcSelection);
		
			if (m_hbrushSelection) ::DeleteObject(m_hbrushSelection);
		
			::DestroyMenu(m_hConfigFilesMenu);
			::DestroyMenu(m_hPopupMenu);
		
			SetTrayIcon(NIM_DELETE);
		
			::DeleteDC(m_hdcConsole);
			::DeleteDC(m_hdcWindow);
			::PostQuitMessage(0);
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnNcDestroy() {
	if (!m_bInitializing) delete this;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnPaint() 
{
	
	PAINTSTRUCT	paintStruct;
	HDC hdc = ::BeginPaint(m_hWnd, &paintStruct);
	
	if (!m_nTextSelection) 
		{
			// if there's no selection, just blit console image
			::BitBlt(
							 hdc, 
							 paintStruct.rcPaint.left, 
							 paintStruct.rcPaint.top, 
							 paintStruct.rcPaint.right - paintStruct.rcPaint.left + 1, 
							 paintStruct.rcPaint.bottom - paintStruct.rcPaint.top + 1, 
							 m_hdcConsole, 
							 paintStruct.rcPaint.left, 
							 paintStruct.rcPaint.top, 
							 SRCCOPY);
		} 
	else
		{
			// there's a text selection, first compose the image in an off-screen
			// buffer, and then blit it
			::BitBlt(
							 m_hdcWindow, 
							 paintStruct.rcPaint.left, 
							 paintStruct.rcPaint.top, 
							 paintStruct.rcPaint.right - paintStruct.rcPaint.left + 1, 
							 paintStruct.rcPaint.bottom - paintStruct.rcPaint.top + 1, 
							 m_hdcConsole, 
							 paintStruct.rcPaint.left, 
							 paintStruct.rcPaint.top, 
							 SRCCOPY);
			
			::BitBlt(
							 m_hdcWindow, 
							 m_rectSelection.left, 
							 m_rectSelection.top, 
							 m_rectSelection.right - m_rectSelection.left, 
							 m_rectSelection.bottom - m_rectSelection.top, 
							 m_hdcSelection, 
							 0, 
							 0, 
							 SRCINVERT);
			::BitBlt(
							 hdc, 
							 paintStruct.rcPaint.left, 
							 paintStruct.rcPaint.top, 
							 paintStruct.rcPaint.right - paintStruct.rcPaint.left + 1, 
							 paintStruct.rcPaint.bottom - paintStruct.rcPaint.top + 1, 
							 m_hdcWindow, 
							 paintStruct.rcPaint.left, 
							 paintStruct.rcPaint.top, 
							 SRCCOPY);
		}
	
	::EndPaint(m_hWnd, &paintStruct);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnPaintTimer() {
	
	::KillTimer(m_hWnd, TIMER_REPAINT_CHANGE);
	RefreshStdOut();
	RefreshScreenBuffer();

	if (GetChangeRate() > 15) 
		{
			::CopyMemory(m_pScreenBuffer, m_pScreenBufferNew, sizeof(CHAR_INFO) * m_dwRows * m_dwColumns);
			RepaintWindow();
		} 
	else
		{
			RepaintWindowChanges();
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnCursorTimer() 
{
	if (m_pCursor) 
		{
			m_pCursor->PrepareNext();
			DrawCursor();
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnWindowPosChanging(WINDOWPOS* lpWndPos) 
{
	
	if (!(lpWndPos->flags & SWP_NOMOVE)) 
		{
			if (m_nSnapDst >= 0) 
				{
					// we'll snap Console window to the desktop edges
					RECT rectDesktop;
					
					if (g_bWin2000) 
						{
							POINT pt;
							if (::GetCursorPos(&pt)) 
								{
									MONITORINFO monitorInfo;
									monitorInfo.cbSize = sizeof(MONITORINFO);
									if (g_pfnGetMonitorInfo(g_pfnMonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), 
																					&monitorInfo))
										{
											rectDesktop = monitorInfo.rcWork;
										}
									
								} 
							else 
								{
									rectDesktop.left	= ::GetSystemMetrics(SM_XVIRTUALSCREEN);
									rectDesktop.top		= ::GetSystemMetrics(SM_YVIRTUALSCREEN);
									rectDesktop.right	= rectDesktop.left + ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
									rectDesktop.bottom	= rectDesktop.top + ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
								}
							
						} 
					else
						{
							// we keep this for WinNT compatibility
							
							rectDesktop.left= 0;
							rectDesktop.top	= 0;
							rectDesktop.right	= ::GetSystemMetrics(SM_CXSCREEN);
							rectDesktop.bottom	= ::GetSystemMetrics(SM_CYSCREEN);
							
							RECT rectTaskbar = {0, 0, 0, 0};
							HWND hWndTaskbar = ::FindWindow(_T("Shell_TrayWnd"), _T(""));
							
							
							if (hWndTaskbar) 
								{
									
									::GetWindowRect(hWndTaskbar, &rectTaskbar);
									
									if ((rectTaskbar.top <= rectDesktop.top) 
											&& (rectTaskbar.left <= rectDesktop.left) 
											&& (rectTaskbar.right >= rectDesktop.right)) 
										{
											// top taskbar
											rectDesktop.top += rectTaskbar.bottom;
											
										} 
									else if ((rectTaskbar.top > rectDesktop.top) 
													 && (rectTaskbar.left <= rectDesktop.left)) 
										{
											// bottom taskbar
											rectDesktop.bottom = rectTaskbar.top;
											
										} 
									else if ((rectTaskbar.top <= rectDesktop.top) 
													 && (rectTaskbar.left > rectDesktop.left)) 
										{
											// right taskbar
											rectDesktop.right = rectTaskbar.left;
											
										} 
									else 
										{
											// left taskbar
											rectDesktop.left += rectTaskbar.right;
										}
								}
						}
					
					m_dwDocked = DOCK_NONE;
					DWORD	dwLeftRight = 0;
					DWORD	dwTopBottom = 0;
					
					// now, see if we're close to the edges
					if (lpWndPos->x <= rectDesktop.left + m_nSnapDst) 
						{
							lpWndPos->x = rectDesktop.left;
							dwLeftRight = 1;
						}
					
					if (lpWndPos->x >= rectDesktop.right - m_nWindowWidth - m_nSnapDst) 
						{
							lpWndPos->x = rectDesktop.right - m_nWindowWidth;
							dwLeftRight = 2;
						}
					
					if (lpWndPos->y <= rectDesktop.top + m_nSnapDst) 
						{
							lpWndPos->y = rectDesktop.top;
							dwTopBottom = 1;
						}
					
					if (lpWndPos->y >= rectDesktop.bottom - m_nWindowHeight - m_nSnapDst) 
						{
							lpWndPos->y = rectDesktop.bottom - m_nWindowHeight;
							dwTopBottom = 2;
						}
					
					// now, see if the window is docked
					if (dwLeftRight == 1) 
						{
							// left edge
							if (dwTopBottom == 1) 
								{
									// top left
									m_dwDocked = DOCK_TOP_LEFT;
								} 
							else if (dwTopBottom == 2) 
								{
									// bottom left
									m_dwDocked = DOCK_BOTTOM_LEFT;
								}
						} 
					else if (dwLeftRight == 2) 
						{
							// right edge
							if (dwTopBottom == 1) 
								{
									// top right
									m_dwDocked = DOCK_TOP_RIGHT;
								} 
							else if (dwTopBottom == 2) 
								{
									// bottom right
									m_dwDocked = DOCK_BOTTOM_RIGHT;
								}
						}
				}
			
			m_nX = lpWndPos->x;
			m_nY = lpWndPos->y;
			
			//		TRACE(_T("Win pos: %ix%i\n"), m_nX, m_nY);
			
			// we need to repaint for relative backgrounds
			if (m_bRelativeBackground && !m_bInitializing) RepaintWindow();
		}
	
	if (m_dwCurrentZOrder == Z_ORDER_ONBOTTOM) lpWndPos->hwndInsertAfter = HWND_BOTTOM;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnActivateApp(BOOL bActivate, DWORD dwFlags) {
	
	if (m_pCursor) 
		{
			m_pCursor->SetState(bActivate);
			DrawCursor();
		}
	
	if ((m_dwTransparency == TRANSPARENCY_ALPHA) && (m_byInactiveAlpha > 0)) 
		{
			if (bActivate) 
				{
					g_pfnSetLayeredWndAttr(m_hWnd, m_crBackground, m_byAlpha, LWA_ALPHA);
				} 
			else
				{
					g_pfnSetLayeredWndAttr(m_hWnd, m_crBackground, m_byInactiveAlpha, LWA_ALPHA);
				}
			
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnVScroll(WPARAM wParam) 
{
	
	TRACE(_T("VScroll\n"));
	
	int nCurrentPos = ::GetScrollPos(m_hWnd, SB_VERT);
	int nDelta = 0;
	
	switch(LOWORD (wParam)) 
		{ 
			
		case SB_PAGEUP: 
			nDelta = -5; 
			break; 
			
		case SB_PAGEDOWN: 
			nDelta = 5; 
			break; 
			
		case SB_LINEUP: 
			nDelta = -1; 
			break; 
			
		case SB_LINEDOWN: 
			nDelta = 1; 
			break; 
			
		case SB_THUMBTRACK: 
			nDelta = HIWORD(wParam) - nCurrentPos; 
			break;
		
		case SB_ENDSCROLL:
		
			return;
		
		default: 
			return;
		}
	
	if (nDelta = max(-nCurrentPos, min(nDelta, (int)(m_dwBufferRows-m_dwRows) - nCurrentPos))) 
		{
			
			nCurrentPos += nDelta; 
			
			SMALL_RECT sr;
			sr.Top = nDelta;
			sr.Bottom = nDelta;
			sr.Left = sr.Right = 0;
			::SetConsoleWindowInfo(m_hStdOutFresh, FALSE, &sr);
		
			SCROLLINFO si;
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_POS; 
			si.nPos   = nCurrentPos; 
			::FlatSB_SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
		
			// this seems to work/look much better than direct repainting...
			::SetTimer(m_hWnd, TIMER_REPAINT_CHANGE, m_dwChangeRepaintInt, NULL);
		}
	
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnInputLangChangeRequest(WPARAM wParam, LPARAM lParam) 
{
	::PostMessage(m_hWndConsole, WM_INPUTLANGCHANGEREQUEST, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnLButtonDown(UINT uiFlags, POINTS points) 
{
	
	RECT windowRect;
	::GetCursorPos(&m_mouseCursorOffset);
	::GetWindowRect(m_hWnd, &windowRect);
	m_mouseCursorOffset.x -= windowRect.left;
	m_mouseCursorOffset.y -= windowRect.top;
	
	if (!m_bMouseDragable ||
			(m_bInverseShift == !(uiFlags & MK_SHIFT))) 
		{
			
			if (m_nCharWidth) 
				{
					
					if (m_nTextSelection == TEXT_SELECTION_SELECTED) return;
					
					// fixed-width characters
					// start copy text selection
					::SetCapture(m_hWnd);
					
					if (!m_nTextSelection) 
						{
							RECT rect;
							rect.left = 0;
							rect.top = 0;
							rect.right = m_nClientWidth;
							rect.bottom = m_nClientHeight;
							::FillRect(m_hdcSelection, &rect, m_hbrushSelection);
						}
					
					m_nTextSelection = TEXT_SELECTION_SELECTING;
					
					m_coordSelOrigin.X = min(max(points.x - m_nInsideBorder, 0) / m_nCharWidth, (short)(m_dwColumns-1));
					m_coordSelOrigin.Y = min(max(points.y - m_nInsideBorder, 0) / m_nCharHeight, (short)(m_dwRows-1));
					
					m_rectSelection.left = m_rectSelection.right = 
						m_coordSelOrigin.X * m_nCharWidth + m_nInsideBorder;
					m_rectSelection.top = m_rectSelection.bottom = 
						m_coordSelOrigin.Y * m_nCharHeight + m_nInsideBorder;
					
					TRACE(_T("Starting point: %ix%i\n"), m_coordSelOrigin.X, m_coordSelOrigin.Y);
				}
			
		} 
	else
		{
			if (m_nTextSelection) 
				{
					return;
				} 
			else if (m_bMouseDragable) 
				{
					// start to drag window
					::SetCapture(m_hWnd);
				}
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnLButtonUp(UINT uiFlags, POINTS points) 
{
	
	if ((m_nTextSelection == TEXT_SELECTION_SELECTED) ||
			// X Windows select/copy style
			((m_nTextSelection == TEXT_SELECTION_SELECTING) && (m_bCopyOnSelect))) 
		{
			
			// if the user clicked inside the selection rectangle, copy data
			if ((points.x >= m_rectSelection.left) && 
					(points.x <= m_rectSelection.right) && 
					(points.y >= m_rectSelection.top) && 
					(points.y <= m_rectSelection.bottom)) 
				{
					
					CopyTextToClipboard();
				}
			
			if (m_bCopyOnSelect) ::ReleaseCapture();
			ClearSelection();
			
		}
	else if (m_nTextSelection == TEXT_SELECTION_SELECTING) 
		{
			
			if ((m_rectSelection.left == m_rectSelection.right) &&
					(m_rectSelection.top == m_rectSelection.bottom)) 
				{
					
					m_nTextSelection = TEXT_SELECTION_NONE;
					
				} 
			else
				{
					m_nTextSelection = TEXT_SELECTION_SELECTED;
				}
			::ReleaseCapture();
			
		} 
	else if (m_bMouseDragable) 
		{
			// end window drag
			::ReleaseCapture();
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnLButtonDblClick(UINT uiFlags, POINTS points) 
{
	
	ToggleWindowOnTop();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnRButtonUp(UINT uiFlags, POINTS points) {
	
	if (uiFlags & MK_SHIFT) 
		{
			PasteClipoardText();
		}
	else 
		{
			
			if (m_bPopupMenuDisabled) return;
			
			POINT	point;
			point.x = points.x;
			point.y = points.y;
			::ClientToScreen(m_hWnd, &point);
			
			HMENU	hPopup = ::GetSubMenu(m_hPopupMenu, 0);
			
			// show popup menu
			::TrackPopupMenu(hPopup, 0, point.x, point.y, 0, m_hWnd, NULL);
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnMButtonDown(UINT uiFlags, POINTS points) {

	PasteClipoardText();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnMouseMove(UINT uiFlags, POINTS points) {

	RECT	windowRect;
	int		deltaX, deltaY;
	POINT	point;

	if (uiFlags & MK_LBUTTON) 
		{
			
			::GetWindowRect(m_hWnd, &windowRect);
			
			point.x = points.x;
			point.y = points.y;
			
			::ClientToScreen(m_hWnd, &point);
			
			deltaX = point.x - windowRect.left - m_mouseCursorOffset.x;
			deltaY = point.y - windowRect.top - m_mouseCursorOffset.y;
			
			if (deltaX | deltaY) 
				{
					
					TRACE(_T("m_nTextSelection: %i, Delta X: %i Delta Y: %i\n"), 
								m_nTextSelection, deltaX, deltaY);
					
					if (m_nTextSelection) 
						{ 
							if ((!m_bMouseDragable) || (m_bInverseShift == !(uiFlags & MK_SHIFT))) 
								{
									
									// some text has been selected, just return
									if (m_nTextSelection == TEXT_SELECTION_SELECTED) return;
									
									// selecting text for copy/paste
									COORD coordSel;
									
									::InvalidateRect(m_hWnd, &m_rectSelection, FALSE);
									
									coordSel.X = min(max(points.x - m_nInsideBorder, 0) / m_nCharWidth, (short)(m_dwColumns-1));
									coordSel.Y = min(max(points.y - m_nInsideBorder, 0) / m_nCharHeight, (short)(m_dwRows-1));
									
									TRACE(_T("End point: %ix%i\n"), coordSel.X, coordSel.Y);
									
									if (coordSel.X >= m_coordSelOrigin.X) 
										{
											m_rectSelection.left = m_coordSelOrigin.X * m_nCharWidth + m_nInsideBorder;
											m_rectSelection.right = (coordSel.X + 1) * m_nCharWidth + m_nInsideBorder;
										} 
									else
										{
											m_rectSelection.left = coordSel.X * m_nCharWidth + m_nInsideBorder;
											m_rectSelection.right = (m_coordSelOrigin.X + 1) * m_nCharWidth + m_nInsideBorder;
										}
									
									if (coordSel.Y >= m_coordSelOrigin.Y) 
										{
											m_rectSelection.top = m_coordSelOrigin.Y * m_nCharHeight + m_nInsideBorder;
											m_rectSelection.bottom = (coordSel.Y + 1) * m_nCharHeight + m_nInsideBorder;
										} 
									else 
										{
											m_rectSelection.top = coordSel.Y * m_nCharHeight + m_nInsideBorder;
											m_rectSelection.bottom = (m_coordSelOrigin.Y + 1) * m_nCharHeight + m_nInsideBorder;
										}
									
									TRACE(_T("Selection rect: %i,%i x %i,%i\n"), 
												m_rectSelection.left, m_rectSelection.top, 
												m_rectSelection.right, m_rectSelection.bottom);
									
									::InvalidateRect(m_hWnd, &m_rectSelection, FALSE);
								}
							
						} 
					else if (m_bMouseDragable) 
						{
							
							// moving the window
							HWND hwndZ;
							switch (m_dwCurrentZOrder)
								{
								case Z_ORDER_REGULAR	: hwndZ = HWND_NOTOPMOST; break;
								case Z_ORDER_ONTOP		: hwndZ = HWND_TOPMOST; break;
								case Z_ORDER_ONBOTTOM	: hwndZ = HWND_BOTTOM; break;
								}
							
							::SetWindowPos(
														 m_hWnd, 
														 hwndZ, 
														 windowRect.left + deltaX, 
														 windowRect.top + deltaY, 
														 0, 
														 0,
														 SWP_NOSIZE);
							
							::PostMessage(m_hWnd, WM_PAINT, 0, 0);
						}
				}
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnSetCursor(WORD wHitTest, WORD wMouseMessage) 
{
	
	if (wHitTest == HTBORDER) 
		{
			if (wMouseMessage == WM_LBUTTONDOWN) 
				{
					if (m_bMouseDragable) 
						{
							// start to drag window
							RECT windowRect;
							::GetCursorPos(&m_mouseCursorOffset);
							::GetWindowRect(m_hWnd, &windowRect);
							m_mouseCursorOffset.x -= windowRect.left;
							m_mouseCursorOffset.y -= windowRect.top;
							::SetCapture(m_hWnd);
						}
				} 
			else if (wMouseMessage == WM_LBUTTONUP) 
				{
					if (m_bMouseDragable) 
						{
							// end window drag
							::ReleaseCapture();
						}
				}
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnInitMenuPopup(HMENU hMenu, UINT uiPos, BOOL bSysMenu) 
{
	
	if ((hMenu != ::GetSubMenu(m_hPopupMenu, 0)) && (hMenu != m_hSysMenu)) return;
	
	// update configuration files submenu
	UpdateConfigFilesSubmenu();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnDropFiles(HDROP hDrop) 
{
	
	UINT	uiFilesCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	tstring	strFilenames(_T(""));
	
	// concatenate all filenames
	for (UINT i = 0; i < uiFilesCount; ++i) 
		{
			TCHAR	szFilename[MAX_PATH];
			::ZeroMemory(szFilename, sizeof(szFilename));
			
			::DragQueryFile(hDrop, i, szFilename, MAX_PATH);
			
			tstring strFilename(szFilename);
			
			// if there are spaces in the filename, put quotes around it
			if (strFilename.find(_T(" ")) != tstring::npos) 
				{ strFilename = tstring(_T("\"")) + strFilename + tstring(_T("\"")); }
			
			if (i > 0) strFilenames += _T(" ");
			strFilenames += strFilename;
			
		}
	::DragFinish(hDrop);
	
	SendTextToConsole(strFilenames.c_str());
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL Console::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (HIWORD(wParam) == 0) 
		{
			// popup menu
			return HandleMenuCommand(LOWORD(wParam));
		}
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL Console::OnSysCommand(WPARAM wParam, LPARAM lParam) 
{
	
	return HandleMenuCommand(wParam);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnTrayNotify(WPARAM wParam, LPARAM lParam) 
{
	
	switch (lParam) 
		{
		case WM_RBUTTONUP: 
			{
				
				if (m_bPopupMenuDisabled) return;
				
				POINT	posCursor;
				
				::GetCursorPos(&posCursor);
				// show popup menu
				::SetForegroundWindow(m_hWnd);
				::TrackPopupMenu(::GetSubMenu(m_hPopupMenu, 0), 0, posCursor.x, posCursor.y, 0, m_hWnd, NULL);
				::PostMessage(m_hWnd, WM_NULL, 0, 0);
				
				return;
			}
			
		case WM_LBUTTONDOWN: 
			m_bHideWindow = false;
			ShowHideWindow();
			::SetForegroundWindow(m_hWnd);
			return;
			
		case WM_LBUTTONDBLCLK:
			m_bHideWindow = !m_bHideWindow;
			ShowHideWindow();
			::SetForegroundWindow(m_hWnd);
			return;
			
		default : return;
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnWallpaperChanged(const TCHAR* pszFilename) 
{
	
	if (m_dwTransparency == TRANSPARENCY_FAKE) 
		{
		SetWindowTransparency();
		CreateBackgroundBitmap();
		RepaintWindow();
	}
	
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL Console::RegisterWindowClasses() 
{
	WNDCLASSEX wcx; 

	// register the Console window class
	::ZeroMemory(&wcx, sizeof(WNDCLASSEX));
	wcx.cbSize			= sizeof(wcx);
	wcx.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wcx.lpfnWndProc		= Console::WindowProc;
	wcx.cbClsExtra		= 0;
	wcx.cbWndExtra		= 0;
	wcx.hInstance		= g_hInstance;
	wcx.hIcon			= ::LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON));
	wcx.hCursor			= ::LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground	= ::CreateSolidBrush(RGB(255, 255, 255));
	wcx.lpszMenuName	=  NULL;
	wcx.lpszClassName	= Console::m_szConsoleClass;
	wcx.hIconSm			= NULL;
	
	if ((::RegisterClassEx(&wcx) == 0) && (::GetLastError() != ERROR_CLASS_ALREADY_EXISTS)) return FALSE;

	// register the hidden parent window class
	::ZeroMemory(&wcx, sizeof(WNDCLASSEX));
	wcx.cbSize			= sizeof(wcx);
	wcx.style			= 0;
	wcx.lpfnWndProc		= ::DefWindowProc;
	wcx.cbClsExtra		= 0;
	wcx.cbWndExtra		= 0;
	wcx.hInstance		= g_hInstance;
	wcx.hIcon			= NULL;
	wcx.hCursor			= ::LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground	= NULL;
	wcx.lpszMenuName	= NULL;
	wcx.lpszClassName	= Console::m_szHiddenParentClass;
	wcx.hIconSm			= NULL;
	
	if ((::RegisterClassEx(&wcx) == 0) && (::GetLastError() != ERROR_CLASS_ALREADY_EXISTS)) return FALSE;
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL Console::SetupMenus() 
{
	
	MENUITEMINFO	mii;
	
	// setup popup menu
	if ((m_hPopupMenu = ::LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_POPUP_MENU))) == NULL) return FALSE;
	if ((m_hConfigFilesMenu = ::CreateMenu()) == NULL) return FALSE;
	
	// add stuff to the system menu
	TCHAR*			arrMenuItems[] = {
		_T("Read&me"), 
		_T("&About Console"), 
		_T(""), 
		_T("&Copy"), 
		_T("&Paste"), 
		_T(""), 
		_T("Always on &top"), 
		_T("&Hide console"), 
		_T(""), 
		_T("&Select configuration file"), 
		_T("&Edit configuration file"), 
		_T("&Reload settings"), 
		_T("")};
	
	DWORD			arrMenuTypes[] = {
		MFT_STRING, 
		MFT_STRING, 
		MFT_SEPARATOR, 
		MFT_STRING, 
		MFT_STRING, 
		MFT_SEPARATOR, 
		MFT_STRING, 
		MFT_STRING, 
		MFT_SEPARATOR, 
		MFT_STRING, 
		MFT_STRING, 
		MFT_STRING, 
		MFT_SEPARATOR};
	DWORD			arrMenuIDs[] = {
		ID_SHOW_README_FILE, 
		ID_ABOUT, 
		0, 
		ID_COPY, 
		ID_PASTE, 
		0, 
		ID_TOGGLE_ONTOP, 
		ID_HIDE_CONSOLE, 
		0, 
		ID_SEL_CONFIG_FILE, 
		ID_EDIT_CONFIG_FILE, 
		ID_RELOAD_SETTINGS, 
		0};
	
	m_hSysMenu = ::GetSystemMenu(m_hWnd, FALSE);
	
	for (int i = 0; i < sizeof(arrMenuIDs)/sizeof(arrMenuIDs[0]); ++i) 
		{
			::ZeroMemory(&mii, sizeof(MENUITEMINFO));
			mii.cbSize		= sizeof(MENUITEMINFO);
			mii.fMask		= MIIM_TYPE | MIIM_ID;
			mii.fType		= arrMenuTypes[i];
			if (mii.fType == MFT_STRING) 
				{
					mii.wID			= arrMenuIDs[i];
					mii.dwTypeData	= arrMenuItems[i];
					mii.cch			= _tcslen(arrMenuItems[i]);
				} 
			else 
				{
					mii.wID			= 0;
				}
			::InsertMenuItem(m_hSysMenu, SC_CLOSE, FALSE, &mii);
		}
	
	// set config files menu as a submenu of "Select configuration file" item
	// in the popup and system menus
	::ZeroMemory(&mii, sizeof(MENUITEMINFO));
	mii.cbSize		= sizeof(MENUITEMINFO);
	mii.fMask		= MIIM_SUBMENU;
	mii.hSubMenu	= m_hConfigFilesMenu;
	
	::SetMenuItemInfo(m_hPopupMenu, ID_SEL_CONFIG_FILE, FALSE, &mii);
	::SetMenuItemInfo(m_hSysMenu, ID_SEL_CONFIG_FILE, FALSE, &mii);
	
	// load the initial list (so the list can be used by external programs)
	UpdateConfigFilesSubmenu();
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL Console::CreateConsoleWindow() 
{
	// create the window
	DWORD dwWindowStyle = WS_VSCROLL;
	switch (m_dwWindowBorder) 
		{
		case BORDER_NONE : 
			dwWindowStyle |= WS_POPUPWINDOW & ~WS_BORDER;
			break;
		case BORDER_REGULAR :
			dwWindowStyle |= WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
			break;
			
		case BORDER_THIN :
			dwWindowStyle |= WS_POPUPWINDOW;
			break;
			
		default: dwWindowStyle |= WS_POPUPWINDOW & ~WS_BORDER;
		}
	
	// If the taskbar style is set to hide or tray, we create an invisible window
	// to parent the main window to. This hides the taskbar button while allowing
	// us to use the normal title bar.
	if (m_dwTaskbarButton > TASKBAR_BUTTON_NORMAL) 
		{
			if ((m_hwndInvisParent = ::CreateWindowEx(
																								WS_EX_TOOLWINDOW,
																								Console::m_szHiddenParentClass,
																								_T(""),
																								WS_POPUP,
																								0,
																								0,
																								0,
																								0,
																								NULL,
																								NULL,
																								g_hInstance,
																								NULL)) == NULL) 
				{
					return FALSE;
				}
			
			// don't let the user minimize it if there's no way to restore it
			if (m_dwTaskbarButton == TASKBAR_BUTTON_HIDE) dwWindowStyle &= ~WS_MINIMIZEBOX;
		}
	
	
	if ((m_hWnd = ::CreateWindowEx(
																 0, //m_dwTaskbarButton > TASKBAR_BUTTON_NORMAL ? WS_EX_TOOLWINDOW : 0,
																 Console::m_szConsoleClass, 
																 m_strWindowTitle.c_str(),
																 dwWindowStyle,
																 ((m_nX == -1) || (m_nY == -1)) ? CW_USEDEFAULT : 0,
																 0,
																 0,
																 0,
																 m_hwndInvisParent,
																 NULL,
																 g_hInstance,
																 NULL)) == NULL) 
		{
			return FALSE;
		}
	
	if ((m_nX == -1) || (m_nY == -1)) 
		{
			RECT rectWindow;
			
			::GetWindowRect(m_hWnd, &rectWindow);
			m_nX = (int)rectWindow.left;
			m_nY = (int)rectWindow.top;
		}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::CreateNewFont() 
{
	// create a new font
	if (m_hFontOld) ::SelectObject(m_hdcConsole, m_hFontOld);
	if (m_hFont) ::DeleteObject(m_hFont);
	
	m_hFont = ::CreateFont(
												 -MulDiv(m_dwFontSize, ::GetDeviceCaps(m_hdcConsole, LOGPIXELSY), 72),
												 0,
												 0,
												 0,
												 m_bBold ? FW_BOLD : 0,
												 m_bItalic,
												 FALSE,
												 FALSE,
												 DEFAULT_CHARSET,						
												 OUT_DEFAULT_PRECIS,
												 CLIP_DEFAULT_PRECIS,
												 DEFAULT_QUALITY,
												 DEFAULT_PITCH,
												 m_strFontName.c_str());
	
	m_hFontOld = (HFONT)::SelectObject(m_hdcConsole, m_hFont);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::CreateNewBrush() 
{
	// create a new background brush
	if (m_hBkBrush) ::DeleteObject(m_hBkBrush);
	m_hBkBrush = ::CreateSolidBrush(m_crBackground);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::CreateCursor() 
{
	
	switch (m_dwCursorStyle) 
		{
		case CURSOR_STYLE_XTERM :
			m_pCursor = (Cursor*)new XTermCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_BLOCK :
			m_pCursor = (Cursor*)new BlockCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_NBBLOCK :
			m_pCursor = (Cursor*)new NBBlockCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_PULSEBLOCK :
			m_pCursor = (Cursor*)new PulseBlockCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_BAR :
			m_pCursor = (Cursor*)new BarCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_CONSOLE :
			m_pCursor = (Cursor*)new ConsoleCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_NBHLINE :
			m_pCursor = (Cursor*)new NBHLineCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_HLINE :
			m_pCursor = (Cursor*)new HLineCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_VLINE :
			m_pCursor = (Cursor*)new VLineCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_RECT :
			m_pCursor = (Cursor*)new RectCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_NBRECT :
			m_pCursor = (Cursor*)new NBRectCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_PULSERECT :
			m_pCursor = (Cursor*)new PulseRectCursor(m_hWnd, m_hdcConsole, m_crCursorColor);
			break;
		case CURSOR_STYLE_FADEBLOCK :
			m_pCursor = (Cursor*)new FadeBlockCursor(m_hWnd, m_hdcConsole, m_crCursorColor, m_crBackground);
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::CalcWindowSize() 
{
	
	TEXTMETRIC	textMetric;
	::GetTextMetrics(m_hdcConsole, &textMetric);
	
	m_nCharHeight	= textMetric.tmHeight;
	m_nClientHeight	= m_dwRows * m_nCharHeight + 2*m_nInsideBorder;
	
	if (!(textMetric.tmPitchAndFamily & TMPF_FIXED_PITCH)) 
		{
			// fixed pitch font (TMPF_FIXED_PITCH is cleared!!!)
			m_nCharWidth= textMetric.tmAveCharWidth;
			m_nClientWidth	= m_dwColumns * m_nCharWidth + 2*m_nInsideBorder;
		} 
	else 
		{
			// variable pitch font
			int nCharWidth;
			::GetCharWidth32(m_hdcConsole, _TCHAR('m'), _TCHAR('m'), &nCharWidth);
			m_nClientWidth	= m_dwColumns * nCharWidth;
		}
	
	WINDOWINFO wndInfo;
	TITLEBARINFO titlebarInfo;
	
	::ZeroMemory(&wndInfo, sizeof(WINDOWINFO));
	::ZeroMemory(&titlebarInfo, sizeof(TITLEBARINFO));
	
	wndInfo.cbSize		= sizeof(WINDOWINFO);
	titlebarInfo.cbSize = sizeof(TITLEBARINFO);
	
	::GetWindowInfo(m_hWnd, &wndInfo);
	::GetTitleBarInfo(m_hWnd, &titlebarInfo);
	
	m_nXBorderSize = wndInfo.cxWindowBorders;
	m_nYBorderSize = wndInfo.cyWindowBorders;
	m_nCaptionSize = (titlebarInfo.rgstate[0] & STATE_SYSTEM_INVISIBLE) ? 
		0 : titlebarInfo.rcTitleBar.bottom - titlebarInfo.rcTitleBar.top;
	
	switch (m_dwWindowBorder) 
		{
		case BORDER_NONE :
			m_nWindowHeight	= m_nClientHeight;
			m_nWindowWidth	= m_nClientWidth;
			break;
			
		case BORDER_REGULAR : 
			{
				
				TRACE(_T("m_nXBorderSize: %i\n"), m_nXBorderSize);
				TRACE(_T("m_nYBorderSize: %i\n"), m_nYBorderSize);
				TRACE(_T("m_nCaptionSize: %i\n"), m_nCaptionSize);
				
				m_nWindowHeight	= m_nClientHeight + m_nCaptionSize + 2*m_nYBorderSize;
				m_nWindowWidth	= m_nClientWidth + 2 * m_nXBorderSize;
				
				break;
			}
			
		case BORDER_THIN :
			
			m_nWindowHeight	= m_nClientHeight + 2*m_nYBorderSize;
			m_nWindowWidth	= m_nClientWidth + 2*m_nXBorderSize;
		}
	
	if (m_bShowScrollbar) m_nWindowWidth += m_nScrollbarWidth;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::SetWindowTransparency() 
{
	
	// set alpha transparency (Win2000 and later only!)
	if (g_bWin2000 && ((m_dwTransparency == TRANSPARENCY_ALPHA) 
										 || (m_dwTransparency == TRANSPARENCY_COLORKEY))) 
		{
			
			::SetWindowLong(m_hWnd, GWL_EXSTYLE, ::GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
			g_pfnSetLayeredWndAttr(m_hWnd, m_crBackground, m_byAlpha, 
														 m_dwTransparency == TRANSPARENCY_ALPHA ? LWA_ALPHA : LWA_COLORKEY);
			
		} 
	else if (m_dwTransparency == TRANSPARENCY_FAKE) 
		{
			// get wallpaper settings
			HKEY hkeyDesktop;
			if (::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Desktop"), 0, 
												 KEY_READ, &hkeyDesktop) == ERROR_SUCCESS) 
				{
					TCHAR	szData[MAX_PATH];
					DWORD	dwDataSize = MAX_PATH;
					
					DWORD	dwWallpaperStyle;
					DWORD	dwTileWallpaper;
					
					::ZeroMemory(szData, sizeof(szData));
					::RegQueryValueEx(hkeyDesktop, _T("Wallpaper"), NULL, NULL, (BYTE*)szData, &dwDataSize);
			
					if (_tcslen(szData) > 0) 
						{
							m_bBitmapBackground = TRUE;
							m_strBackgroundFile		= szData;
							m_bRelativeBackground	= TRUE;
							m_bExtendBackground		= FALSE;
							
							// get wallpaper style and tile flag
							dwDataSize = MAX_PATH;
							::ZeroMemory(szData, sizeof(szData));
							::RegQueryValueEx(hkeyDesktop, _T("WallpaperStyle"), NULL, NULL, 
																(BYTE*)szData, &dwDataSize);
							
							dwWallpaperStyle = _ttoi(szData);
							
							dwDataSize = MAX_PATH;
							::ZeroMemory(szData, sizeof(szData));
							::RegQueryValueEx(hkeyDesktop, _T("TileWallpaper"), NULL, NULL, 
																(BYTE*)szData, &dwDataSize);
							
							dwTileWallpaper = _ttoi(szData);
							
							if (dwTileWallpaper == 1) 
								{
									m_dwBackgroundStyle = BACKGROUND_STYLE_TILE;
								}
							else
								{
									
									if (dwWallpaperStyle == 0) 
										{
											m_dwBackgroundStyle = BACKGROUND_STYLE_CENTER;
										} 
									else 
										{
											m_dwBackgroundStyle = BACKGROUND_STYLE_RESIZE;
										}
								}
						}
					
					::RegCloseKey(hkeyDesktop);
				}
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::SetScrollbarStuff()
{
	
	if (m_bUseTextBuffer)
		{
			m_bShowScrollbar = m_dwBufferRows > m_dwRows;
		} 
	else 
		{
			m_bShowScrollbar = FALSE;
		}
	
	// we don't call InitializeFlatSB due to some problems on Win2k, Windowblinds and new-style scrollbars
	if (m_nScrollbarStyle != FSB_REGULAR_MODE) ::InitializeFlatSB(m_hWnd);
	::FlatSB_ShowScrollBar(m_hWnd, SB_VERT, m_bShowScrollbar);
	::FlatSB_SetScrollRange(m_hWnd, SB_VERT, 0, m_dwBufferRows-m_dwRows, FALSE);
	
	// set scrollbar properties
	::FlatSB_SetScrollProp(m_hWnd, WSB_PROP_VSTYLE, m_nScrollbarStyle, FALSE);
	::FlatSB_SetScrollProp(m_hWnd, WSB_PROP_VBKGCOLOR, m_crScrollbarColor, FALSE);
	::FlatSB_SetScrollProp(m_hWnd, WSB_PROP_CXVSCROLL , m_nScrollbarWidth, FALSE);
	::FlatSB_SetScrollProp(m_hWnd, WSB_PROP_CYVSCROLL, m_nScrollbarButtonHeight, FALSE);
	::FlatSB_SetScrollProp(m_hWnd, WSB_PROP_CYVTHUMB, m_nScrollbarThunmbHeight, TRUE);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::SetDefaultConsoleColors() 
{
	
	m_arrConsoleColors[0]	= 0x000000;
	m_arrConsoleColors[1]	= 0x800000;
	m_arrConsoleColors[2]	= 0x008000;
	m_arrConsoleColors[3]	= 0x808000;
	m_arrConsoleColors[4]	= 0x000080;
	m_arrConsoleColors[5]	= 0x800080;
	m_arrConsoleColors[6]	= 0x008080;
	m_arrConsoleColors[7]	= 0xC0C0C0;
	m_arrConsoleColors[8]	= 0x808080;
	m_arrConsoleColors[9]	= 0xFF0000;
	m_arrConsoleColors[10]	= 0x00FF00;
	m_arrConsoleColors[11]	= 0xFFFF00;
	m_arrConsoleColors[12]	= 0x0000FF;
	m_arrConsoleColors[13]	= 0xFF00FF;
	m_arrConsoleColors[14]	= 0x00FFFF;
	m_arrConsoleColors[15]	= 0xFFFFFF;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::SetWindowSizeAndPosition() 
{
	
	// set window position
	DWORD   dwScreenWidth	= ::GetSystemMetrics(g_bWin2000 ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
	DWORD   dwScreenHeight	= ::GetSystemMetrics(g_bWin2000 ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
	DWORD   dwTop			= ::GetSystemMetrics(g_bWin2000 ? SM_YVIRTUALSCREEN : 0);
	DWORD   dwLeft			= ::GetSystemMetrics(g_bWin2000 ? SM_XVIRTUALSCREEN : 0);
	
	switch (m_dwDocked) 
		{
		case DOCK_TOP_LEFT:
			// top left
			m_nX = dwLeft;
			m_nY = dwTop;
			break;
			
		case DOCK_TOP_RIGHT:
			// top right
			m_nX = dwScreenWidth - m_nWindowWidth;
			m_nY = dwTop;
			break;
			
		case DOCK_BOTTOM_RIGHT:
			// bottom right
			m_nX = dwScreenWidth - m_nWindowWidth;
			m_nY = dwScreenHeight - m_nWindowHeight;
			break;
			
		case DOCK_BOTTOM_LEFT:
			// bottom left
			m_nX = dwLeft;
			m_nY = dwScreenHeight - m_nWindowHeight;
			break;
		}
	
	HWND hwndZ;
	switch (m_dwCurrentZOrder) 
		{
		case Z_ORDER_REGULAR	: hwndZ = HWND_NOTOPMOST; break;
		case Z_ORDER_ONTOP		: hwndZ = HWND_TOPMOST; break;
		case Z_ORDER_ONBOTTOM	: hwndZ = HWND_BOTTOM; break;
		}
	
	::SetWindowPos(
								 m_hWnd, 
								 hwndZ, 
								 m_nX, 
								 m_nY, 
								 m_nWindowWidth, 
								 m_nWindowHeight, 
								 0);
	
	UpdateOnTopMenuItem();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::DestroyCursor() 
{
	if (m_pCursor) DEL_OBJ(m_pCursor);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL Console::StartShellProcess() 
{
	
	if (m_strShell.length() == 0) 
		{
			TCHAR	szComspec[MAX_PATH];
		
			if (::GetEnvironmentVariable(_T("COMSPEC"), szComspec, MAX_PATH) > 0) 
				{
					m_strShell = szComspec;		
				} 
			else
				{
					m_strShell = _T("cmd.exe");
				}
		}

	tstring	strShellCmdLine(m_strShell);
	if (m_strShellCmdLine.length() > 0) 
		{
			strShellCmdLine += _T(" ");
			strShellCmdLine += m_strShellCmdLine;
		}

//	strShellCmdLine = "cmd.exe";
	
	// create the console window
	TCHAR	szConsoleTitle[MAX_PATH];
	::AllocConsole();
	// we use this to avoid possible problems with multiple console instances running
	_stprintf(szConsoleTitle, _T("%i"), ::GetCurrentThreadId());
	::SetConsoleTitle(szConsoleTitle);
	m_hStdOut	= ::GetStdHandle(STD_OUTPUT_HANDLE);
	while ((m_hWndConsole = ::FindWindow(NULL, szConsoleTitle)) == NULL) ::Sleep(50);
	::SetConsoleTitle(m_strWinConsoleTitle.c_str());
	
	// this is a little hack needed to support columns greater than standard 80
	RefreshStdOut();
	InitConsoleWndSize(80);
	ResizeConsoleWindow();
	
	::SetConsoleCtrlHandler(Console::CtrlHandler, TRUE);
	
	// setup the start up info struct
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	::ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	
	if (!::CreateProcess(
											 NULL, 
											 (TCHAR*)strShellCmdLine.c_str(), 
											 NULL, 
											 NULL, 
											 TRUE, 
											 0, 
											 NULL,
											 NULL,
											 &si,
											 &pi)) 
		{
			return FALSE;
		}
	
	if (m_dwHideConsoleTimeout > 0) 
		{
			::ShowWindow(m_hWndConsole, SW_MINIMIZE);
			::SetTimer(m_hWnd, TIMER_SHOW_HIDE_CONSOLE, m_dwHideConsoleTimeout, NULL);
		} 
	else 
		{
			ShowHideConsole();
		}
	
	// close main thread handle
	::CloseHandle(pi.hThread);
	
	// set handles
	m_hQuitEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hConsoleProcess = pi.hProcess;
	
	// create exit monitor thread
	DWORD dwThreadID;
	m_hMonitorThread = ::CreateThread(NULL, 0, Console::MonitorThreadStatic, this, 
																		CREATE_SUSPENDED, &dwThreadID);
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::RefreshStdOut() 
{
	if (m_hStdOutFresh) ::CloseHandle(m_hStdOutFresh);
	m_hStdOutFresh = ::CreateFile(_T("CONOUT$"), 
																GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
																NULL, OPEN_EXISTING, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::RefreshScreenBuffer() 
{
	// if we're initializing, do nothing
	if (m_bInitializing) return;
	
	::GetConsoleScreenBufferInfo(m_hStdOutFresh, &m_csbiConsole);

	SCROLLINFO si;
	si.cbSize = sizeof(si); 
	si.fMask  = SIF_POS; 
	si.nPos   = (int)m_csbiConsole.srWindow.Top;
	::FlatSB_SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
	
	if ((m_csbiConsole.srWindow.Right - m_csbiConsole.srWindow.Left + 1 != m_dwColumns) 
			|| (m_csbiConsole.srWindow.Bottom - m_csbiConsole.srWindow.Top + 1 != m_dwRows) 
			|| (m_csbiConsole.dwSize.Y != m_dwBufferRows)) 
		{
			m_dwColumns = m_csbiConsole.srWindow.Right - m_csbiConsole.srWindow.Left + 1;
			m_dwRows	= m_csbiConsole.srWindow.Bottom - m_csbiConsole.srWindow.Top + 1;
			ResizeConsoleWindow();
		}
	
	COORD		coordBufferSize;
	COORD		coordStart;
	SMALL_RECT	srRegion;
	
	coordStart.X		= 0;
	coordStart.Y		= 0;
	
	coordBufferSize.X	= (short)m_dwColumns;
	coordBufferSize.Y	= (short)m_dwRows;
	
	srRegion.Top		= m_csbiConsole.srWindow.Top;
	srRegion.Left		= 0;
	srRegion.Bottom		= (short)(m_csbiConsole.srWindow.Top + m_dwRows - 1);
	srRegion.Right		= (short)(m_dwColumns - 1);

	DEL_ARR(m_pScreenBufferNew);
	m_pScreenBufferNew = new CHAR_INFO[m_dwRows * m_dwColumns];
	
	::ReadConsoleOutput(m_hStdOutFresh, m_pScreenBufferNew, coordBufferSize, coordStart, &srRegion);
	
	// set console window title
	TCHAR szWinConsoleTitle[MAX_PATH+1];
	::GetConsoleTitle(szWinConsoleTitle, MAX_PATH);

	tstring strWinConsoleTitle(szWinConsoleTitle);
	tstring strConsoleTitle(_T(""));

	// Here we decide about updating Console window title.
	// There are 2 possibilities:

	if (m_strWinConsoleTitle.compare(0, m_strWinConsoleTitle.length(), 
																	 strWinConsoleTitle, 0, m_strWinConsoleTitle.length()) == 0) 
		{
			// 1. Windows console title starts with the original title, just see if
			//    windows titles differ, and if they do, update it.
			if ((m_strWindowTitle.length() == 0)  &&
					(strWinConsoleTitle[m_strWinConsoleTitle.length()] == ' ') &&
			(strWinConsoleTitle[m_strWinConsoleTitle.length()+1] == '-')) 
				{
					strConsoleTitle = strWinConsoleTitle.substr(m_strWinConsoleTitle.length()+3);
				} 
			else 
				{
					strConsoleTitle = 
						m_strWindowTitle + strWinConsoleTitle.substr(m_strWinConsoleTitle.length());
				}
			
		} 
	else 
		{
			// 2. Windows console title is completely changed. To set Console title, 
			//    we need to get Windows console title and concatenate it to our 
			//    original Console title (if it changed since the last update)
			if (m_strWindowTitle.length() == 0) 
				{
					strConsoleTitle = strWinConsoleTitle;
				} 
			else 
				{
					strConsoleTitle = m_strWindowTitle + tstring(_T(" - ")) + strWinConsoleTitle;
				}
		}
	
	if (m_strWindowTitleCurrent.compare(strConsoleTitle) != 0) 
		{
			m_strWindowTitleCurrent = strConsoleTitle;
			::SetWindowText(m_hWnd, m_strWindowTitleCurrent.c_str());
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::InitConsoleWndSize(DWORD dwColumns) 
{
	
	if (m_nTextSelection) ClearSelection();
	
	COORD coordConsoleSize;
	coordConsoleSize.X = (SHORT)dwColumns;
	coordConsoleSize.Y = (SHORT)m_dwBufferRows;
	
	SMALL_RECT	srConsoleRect;
	srConsoleRect.Top	= srConsoleRect.Left =0;
	srConsoleRect.Right	= (short)(dwColumns - 1);
	srConsoleRect.Bottom= (short)(m_dwRows - 1);
	
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	::GetConsoleScreenBufferInfo(m_hStdOutFresh, &csbi);
	
	if ((DWORD) csbi.dwSize.X * csbi.dwSize.Y > (DWORD) dwColumns * m_dwBufferRows) 
		{
			::SetConsoleWindowInfo(m_hStdOutFresh, TRUE, &srConsoleRect);
			::SetConsoleScreenBufferSize(m_hStdOutFresh, coordConsoleSize);
		} 
	else if (((DWORD)csbi.dwSize.X < dwColumns) 
					 || ((DWORD)csbi.dwSize.Y < m_dwBufferRows) 
					 || ((DWORD)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1) != m_dwRows)) 
		{
			::SetConsoleScreenBufferSize(m_hStdOutFresh, coordConsoleSize);
			::SetConsoleWindowInfo(m_hStdOutFresh, TRUE, &srConsoleRect);
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

inline void Console::GetCursorRect(RECT& rectCursor) 
{
	
	::ZeroMemory(&rectCursor, sizeof(RECT));
	
	if (m_nCharWidth > 0) 
		{
			// fixed pitch
			
			rectCursor.left = m_csbiCursor.dwCursorPosition.X * m_nCharWidth + m_nInsideBorder;
			rectCursor.top = m_csbiCursor.dwCursorPosition.Y * m_nCharHeight + m_nInsideBorder;
			rectCursor.right = 
				m_csbiCursor.dwCursorPosition.X * m_nCharWidth + m_nCharWidth + m_nInsideBorder;
			rectCursor.bottom = 
				m_csbiCursor.dwCursorPosition.Y * m_nCharHeight + m_nCharHeight + m_nInsideBorder;
			
		} 
	else
		{
			
			// variable pitch, we do a little joggling here :-)
			RECT			rectLine;
			int				nLastCharWidth;
			auto_ptr<TCHAR>	pszLine(new TCHAR[m_csbiCursor.dwCursorPosition.X + 2]);
			::ZeroMemory(pszLine.get(), (m_csbiCursor.dwCursorPosition.X + 2)*sizeof(TCHAR));
			
			for (short i = 0; i <= m_csbiCursor.dwCursorPosition.X; ++i) 
				{ 
					pszLine.get()[i] = 
						m_pScreenBuffer[m_csbiCursor.dwCursorPosition.Y * m_dwColumns + i].Char.UnicodeChar;
				}
			
			rectLine.left	= rectLine.right	= 0;
			rectLine.top	= rectLine.bottom	= m_csbiCursor.dwCursorPosition.Y * m_nCharHeight;
			
			::DrawText(
								 m_hdcConsole, 
								 pszLine.get(),
								 -1,
								 &rectLine,
								 DT_CALCRECT);
			
			if (!::GetCharWidth32(
														m_hdcConsole, 
														pszLine.get()[m_csbiCursor.dwCursorPosition.X], 
														pszLine.get()[m_csbiCursor.dwCursorPosition.X], 
														&nLastCharWidth)) 
				{
					
					return;
				}
			
			rectCursor.left		= rectLine.right - (DWORD)nLastCharWidth + m_nInsideBorder;
			rectCursor.top		= rectLine.top + m_nInsideBorder;
			rectCursor.right	= rectLine.right + m_nInsideBorder;
			rectCursor.bottom	= rectLine.bottom + m_nInsideBorder;
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::ClearSelection() 
{
	m_nTextSelection = TEXT_SELECTION_NONE;
	m_coordSelOrigin.X = 0;
	m_coordSelOrigin.Y = 0;
	::ZeroMemory(&m_rectSelection, sizeof(RECT));
	
	RepaintWindow();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

DWORD Console::GetChangeRate() 
{
	DWORD dwCount				= m_dwRows * m_dwColumns;
	DWORD dwChangedPositions	= 0;
	
	for (DWORD i = 0; i < dwCount; ++i) 
		{
			if (m_pScreenBuffer[i].Char.UnicodeChar != m_pScreenBufferNew[i].Char.UnicodeChar) 
				{ ++dwChangedPositions; }
		}
	
	return dwChangedPositions*100/dwCount;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::ShowHideWindow() 
{
	::ShowWindowAsync(m_hWnd, m_bHideWindow ? SW_HIDE : SW_SHOW);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::ShowHideConsole() 
{
	::ShowWindow(m_hWndConsole, m_bHideConsole ? SW_HIDE : SW_SHOWNORMAL);
	if (!m_bHideConsole) ::SetForegroundWindow(m_hWndConsole);
	UpdateHideConsoleMenuItem();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::ShowHideConsoleTimeout() 
{
	::KillTimer(m_hWnd, TIMER_SHOW_HIDE_CONSOLE);
	ShowHideConsole();
}
	
/////////////////////////////////////////////////////////////////////////////

	
/////////////////////////////////////////////////////////////////////////////

void Console::ToggleWindowOnTop() 
{
	if (m_dwCurrentZOrder == m_dwOriginalZOrder) 
		{
			m_dwCurrentZOrder	= Z_ORDER_ONTOP;
		} 
	else 
		{
			m_dwCurrentZOrder = m_dwOriginalZOrder;
		}
	
	HWND hwndZ;
	switch (m_dwCurrentZOrder) 
		{
		case Z_ORDER_REGULAR	: hwndZ = HWND_NOTOPMOST; break;
		case Z_ORDER_ONTOP		: hwndZ = HWND_TOPMOST; break;
		case Z_ORDER_ONBOTTOM	: hwndZ = HWND_BOTTOM; break;
		}
	
	::SetWindowPos(
								 m_hWnd, 
								 hwndZ, 
								 0, 
								 0, 
								 0, 
								 0, 
								 SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
	
	UpdateOnTopMenuItem();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL Console::HandleMenuCommand(unsigned int dwID) 
{
	
	// check if it's one of the main menu commands
	switch (dwID) 
		{
		case ID_SHOW_README_FILE:
			ShowReadmeFile();
			return FALSE;
			
		case ID_ABOUT:
			About();
			return FALSE;
			
		case ID_COPY:
			CopyTextToClipboard();
			return FALSE;
			
		case ID_PASTE:
			PasteClipoardText();
			return FALSE;
		
		case ID_HIDE_CONSOLE:
			m_bHideConsole = !m_bHideConsole;
			ShowHideConsole();
			return FALSE;
		
		case ID_EDIT_CONFIG_FILE:
			EditConfigFile();
			return FALSE;
		
		case ID_RELOAD_SETTINGS:
			ReloadSettings();
			return FALSE;
		
		case ID_TOGGLE_ONTOP:
			ToggleWindowOnTop();
			return FALSE;
		
		case ID_EXIT_CONSOLE:
			::SendMessage(m_hWnd, WM_CLOSE, 0, 0);
			return FALSE;
		
		case SC_MINIMIZE:
			if (m_dwTaskbarButton > TASKBAR_BUTTON_NORMAL) 
				{
					m_bHideWindow = !m_bHideWindow;
					ShowHideWindow();
					::SetForegroundWindow(m_hWnd);
					return FALSE;
				}
			return TRUE;
		}
	
	// check if it's one of config file submenu items
	if ((dwID >= ID_FIRST_XML_FILE) &&
			(dwID <= ID_LAST_XML_FILE)) 
		{
			TCHAR	szFilename[MAX_PATH];
			::ZeroMemory(szFilename, sizeof(szFilename));
			::GetMenuString(m_hConfigFilesMenu, dwID, szFilename, MAX_PATH, MF_BYCOMMAND);
			m_strConfigFile = tstring(szFilename);
			
			if (m_dwReloadNewConfig == RELOAD_NEW_CONFIG_PROMPT) 
				{
					if (::MessageBox(
													 m_hWndConsole, 
													 _T("Load new settings?"), 
													 _T("New configuration selected"), 
													 MB_YESNO|MB_ICONQUESTION) == IDYES) 
						{
							ReloadSettings();
						}
				} 
			else if (m_dwReloadNewConfig == RELOAD_NEW_CONFIG_YES) 
				{
					ReloadSettings();
				}
			
			return FALSE;
		}
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::UpdateOnTopMenuItem() 
{
	::CheckMenuItem(::GetSubMenu(m_hPopupMenu, 0), ID_TOGGLE_ONTOP, 
									MF_BYCOMMAND | ((m_dwCurrentZOrder == Z_ORDER_ONTOP) ? MF_CHECKED : MF_UNCHECKED));
	::CheckMenuItem(m_hSysMenu, ID_TOGGLE_ONTOP, 
									MF_BYCOMMAND | ((m_dwCurrentZOrder == Z_ORDER_ONTOP) ? MF_CHECKED : MF_UNCHECKED));
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::UpdateHideConsoleMenuItem() 
{
	::CheckMenuItem(::GetSubMenu(m_hPopupMenu, 0), ID_HIDE_CONSOLE, 
									MF_BYCOMMAND | (m_bHideConsole ? MF_CHECKED : MF_UNCHECKED));
	::CheckMenuItem(m_hSysMenu, ID_HIDE_CONSOLE, 
									MF_BYCOMMAND | (m_bHideConsole ? MF_CHECKED : MF_UNCHECKED));
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::UpdateConfigFilesSubmenu() 
{
	// populate m_hConfigFilesMenu
	
	// first, delete old items
	while (::GetMenuItemCount(m_hConfigFilesMenu) != 0)
		{  ::DeleteMenu(m_hConfigFilesMenu, 0, MF_BYPOSITION); }
	
	// then, enumerate the files
	WIN32_FIND_DATA	wfd;
	HANDLE			hWfd = NULL;
	BOOL			bMoreFiles = TRUE;
	DWORD			dwID = ID_FIRST_XML_FILE;
	
	::ZeroMemory(&wfd, sizeof(WIN32_FIND_DATA));
	
	// create the search mask...
	int		nBackslashPos = m_strConfigFile.rfind(_TCHAR('\\'));
	tstring	strConfigFileDir(m_strConfigFile.substr(0, nBackslashPos+1));
	tstring	strSearchFileMask(strConfigFileDir + tstring(_T("*.xml")));
	
	// ... and enumearate files
	hWfd = ::FindFirstFile(strSearchFileMask.c_str(), &wfd);
	while ((hWfd != INVALID_HANDLE_VALUE) && bMoreFiles) 
		{
			MENUITEMINFO	mii;
			TCHAR			szFilename[MAX_PATH];
		
			_sntprintf(szFilename, MAX_PATH, _T("%s"),
								 (strConfigFileDir + tstring(wfd.cFileName)).c_str());
		
			::ZeroMemory(&mii, sizeof(MENUITEMINFO));
			mii.cbSize		= sizeof(MENUITEMINFO);
			mii.fMask		= MIIM_TYPE | MIIM_ID | MIIM_STATE;
			mii.fType		= MFT_RADIOCHECK | MFT_STRING;
			mii.wID			= dwID++;
			mii.dwTypeData	= szFilename;
			mii.cch			= _tcslen(wfd.cFileName);
		
			if (_tcsicmp(szFilename, m_strConfigFile.c_str()) == 0) 
				{
					mii.fState	= MFS_CHECKED;
				} 
			else
				{
					mii.fState	= MFS_UNCHECKED;
				}
			
			::InsertMenuItem(m_hConfigFilesMenu, dwID-ID_FIRST_XML_FILE, TRUE, &mii);
			
			bMoreFiles = ::FindNextFile(hWfd, &wfd);
		}
	
	::FindClose(hWfd);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::CopyTextToClipboard() 
{
	int nSelX = (m_rectSelection.left - m_nInsideBorder)/m_nCharWidth;
	int nSelY = (m_rectSelection.top - m_nInsideBorder)/m_nCharHeight;
	int nSelColumns = (m_rectSelection.right-m_rectSelection.left)/m_nCharWidth;
	int nSelRows = (m_rectSelection.bottom-m_rectSelection.top)/m_nCharHeight;
	
	// nothing selected, just return
	if ((nSelColumns == 0) || (nSelRows == 0)) 
		{
			ClearSelection();
			return;
		}
	
	if (!::OpenClipboard(m_hWnd)) 
		{
			ClearSelection();
			return;
		}
	
	::EmptyClipboard();
	
	HGLOBAL hglbData = ::GlobalAlloc(GMEM_MOVEABLE, (nSelColumns+2)*nSelRows*sizeof(TCHAR)); 
	if (hglbData == NULL)
		{ 
			::CloseClipboard();
			ClearSelection();
			return;
		} 
	
	// lock the handle and copy the text to the buffer. 
	TCHAR* pszData = (TCHAR*)::GlobalLock(hglbData); 
	int i =0;
	for (i = 0; i < nSelRows; ++i)
		{
			for (int j = 0; j < nSelColumns; ++j)
				{
					pszData[i*(nSelColumns+2) + j] = 
						m_pScreenBuffer[(nSelY+i)*m_dwColumns + nSelX + j].Char.UnicodeChar;
				}
			// at the end of each row we put \r\n (except for the last one where we put \0)
			pszData[i*(nSelColumns+2) + nSelColumns] = _TCHAR('\r');
			pszData[i*(nSelColumns+2) + nSelColumns + 1] = _TCHAR('\n');
		}
	pszData[(i-1)*(nSelColumns+2) + nSelColumns] = _TCHAR('\x0');
	
	::GlobalUnlock(hglbData);
	
	if (::SetClipboardData(CF_UNICODETEXT, hglbData) == NULL)
		{
			// we need to global-free data only if copying failed
		}
	::CloseClipboard();
	// !!! No call to GlobalFree here. Next app that uses clipboard will call EmptyClipboard to free the data
	
	ClearSelection();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::PasteClipoardText() 
{
	if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return;
	
	
	if (::OpenClipboard(m_hWnd)) 
		{
			HANDLE	hData = ::GetClipboardData(CF_UNICODETEXT);
			
			SendTextToConsole((TCHAR*)::GlobalLock(hData));
			
			::GlobalUnlock(hData);
			::CloseClipboard();
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::ShowReadmeFile() 
{
	// prepare editor parameters
	tstring strParams(m_strConfigEditorParams);
	
	if (strParams.length() == 0) 
		{
			// no params, just use the readme file
			strParams = m_strReadmeFile;
		} 
	else
		{
			int nPos = strParams.find(_T("%f"));
		
			if (nPos == tstring::npos)
				{
					// no '%f' in editor params, concatenate readme file name
					strParams += _T(" ");
					strParams += m_strReadmeFile;
				} 
			else
				{
					// replace '%f' with readme file name
					strParams = strParams.replace(nPos, 2, m_strReadmeFile);
				}
		}
	
	::ShellExecute(NULL, NULL, m_strConfigEditor.c_str(), strParams.c_str(), NULL, SW_SHOWNORMAL);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// About	- shows an about dialog

void Console::About()
{
	CAboutDlg dlg(m_hWnd);
	
	dlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::SendTextToConsole(LPCTSTR pszText) 
{
	if (!pszText || (_tcslen(pszText) == 0)) return;
	
	HANDLE hStdIn = ::CreateFile(_T("CONIN$"), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	
	DWORD	dwTextLen		= _tcslen(pszText);
	DWORD	dwTextWritten	= 0;
	
	INPUT_RECORD* pKeyEvents = new INPUT_RECORD[dwTextLen];
	::ZeroMemory(pKeyEvents, sizeof(INPUT_RECORD)*dwTextLen);
	
	for (DWORD i = 0; i < dwTextLen; ++i)
		{
			pKeyEvents[i].EventType = KEY_EVENT;
			pKeyEvents[i].Event.KeyEvent.bKeyDown = TRUE;
			pKeyEvents[i].Event.KeyEvent.wRepeatCount = 1;
			pKeyEvents[i].Event.KeyEvent.wVirtualKeyCode = 0;
			pKeyEvents[i].Event.KeyEvent.wVirtualScanCode = 0;
			pKeyEvents[i].Event.KeyEvent.uChar.UnicodeChar = pszText[i];
			pKeyEvents[i].Event.KeyEvent.dwControlKeyState = 0;
		}
	::WriteConsoleInput(hStdIn, pKeyEvents, dwTextLen, &dwTextWritten);
	
	DEL_ARR(pKeyEvents);
	::CloseHandle(hStdIn);
	
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

tstring Console::GetFullFilename(const tstring& strFilename) 
{
	TCHAR			szFilePath[MAX_PATH];
	TCHAR*			pszFilename;
	
	::ZeroMemory(szFilePath, sizeof(szFilePath));
	::GetFullPathName(strFilename.c_str(), MAX_PATH, szFilePath, &pszFilename);
	
	return tstring(szFilePath);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK Console::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
		{ 
		case WM_CREATE: 
			return 0; 
			
		case WM_PAINT: 
			// paint the window
			g_pConsole->OnPaint();
			return 0;

		case WM_ERASEBKGND:
			return -1;
			
		case WM_SIZE: 
			// set the size and position of the window
			return 0;
			
		case WM_CLOSE:
			::DestroyWindow(g_pConsole->m_hWnd);
			return 0; 
			
		case WM_DESTROY: 
			g_pConsole->OnDestroy();
			return 0;
			
		case WM_NCDESTROY:
			g_pConsole->OnNcDestroy();
			return 0;
			
		case WM_LBUTTONDOWN:
			g_pConsole->OnLButtonDown((UINT)wParam, MAKEPOINTS(lParam));
			return 0;
			
		case WM_LBUTTONUP:
			g_pConsole->OnLButtonUp((UINT)wParam, MAKEPOINTS(lParam));
			return 0;
			
		case WM_LBUTTONDBLCLK:
			g_pConsole->OnLButtonDblClick((UINT)wParam, MAKEPOINTS(lParam));
			return 0;
		
		case WM_RBUTTONUP:
			g_pConsole->OnRButtonUp((UINT)wParam, MAKEPOINTS(lParam));
			return 0;
			
		case WM_MBUTTONDOWN:
			g_pConsole->OnMButtonDown((UINT)wParam, MAKEPOINTS(lParam));
			return 0;
			
		case WM_MOUSEMOVE:
			g_pConsole->OnMouseMove((UINT)wParam, MAKEPOINTS(lParam));
			return 0;
			
		case WM_SETCURSOR:
			g_pConsole->OnSetCursor(LOWORD(lParam), HIWORD(lParam));
			::DefWindowProc(g_pConsole->m_hWnd, uMsg, wParam, lParam);
			return 0;

		case WM_INITMENUPOPUP:
			g_pConsole->OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), (BOOL)HIWORD(lParam));
			return 0;

		case WM_DROPFILES:
			g_pConsole->OnDropFiles((HDROP)wParam);
			return 0;
			
		case WM_WINDOWPOSCHANGING:
			g_pConsole->OnWindowPosChanging((LPWINDOWPOS)lParam);
			return 0;

		case WM_ACTIVATEAPP:
			g_pConsole->OnActivateApp((BOOL)wParam, (DWORD)lParam);
			return 0;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			MSG	msg;

			::ZeroMemory(&msg, sizeof(MSG));

			msg.hwnd	= g_pConsole->m_hWnd;
			msg.message	= uMsg;
			msg.wParam	= wParam;
			msg.lParam	= lParam;

			::TranslateMessage(&msg);
			::PostMessage(g_pConsole->m_hWndConsole, uMsg, wParam, lParam);
			return 0;
			
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_MOUSEWHEEL:
			::PostMessage(g_pConsole->m_hWndConsole, uMsg, wParam, lParam);
			return 0;


		case WM_VSCROLL:
			g_pConsole->OnVScroll(wParam);
			return 0;
			
		case WM_TIMER:
			switch (wParam) 
				{
				case TIMER_REPAINT_CHANGE:
				case TIMER_REPAINT_MASTER:
					g_pConsole->OnPaintTimer();
					return 0;
					
				case CURSOR_TIMER:
					g_pConsole->OnCursorTimer();
					return 0;

				case TIMER_SHOW_HIDE_CONSOLE:
					g_pConsole->ShowHideConsoleTimeout();
					return 0;

				default:
					return 1;
				}
			
		case WM_COMMAND:
			if (g_pConsole->OnCommand(wParam, lParam)) 
				{
					return ::DefWindowProc(hwnd, uMsg, wParam, lParam); 
				} 
			else
				{
					return 0;
				}
			
		case WM_SYSCOMMAND:
			if (g_pConsole->OnSysCommand(wParam, lParam)) 
				{
					return ::DefWindowProc(hwnd, uMsg, wParam, lParam); 
				} 
			else 
				{
					return 0;
				}

		case WM_TRAY_NOTIFY:
			g_pConsole->OnTrayNotify(wParam, lParam);
			return 0;

		case WM_SETTINGCHANGE:

			g_pConsole->OnWallpaperChanged((TCHAR*)lParam);
			return ::DefWindowProc(hwnd, uMsg, wParam, lParam);

		case WM_INPUTLANGCHANGEREQUEST:
			g_pConsole->OnInputLangChangeRequest(wParam, lParam);
			return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
			
			// process other messages
		default: 
			return ::DefWindowProc(hwnd, uMsg, wParam, lParam); 
		}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// MonitorThread	- shell activities monitor

DWORD WINAPI Console::MonitorThreadStatic(LPVOID lpParam) 
{
	return ((Console*)lpParam)->MonitorThread();
}

DWORD Console::MonitorThread() 
{
	HANDLE	arrHandles[] = {m_hConsoleProcess, m_hQuitEvent, m_hStdOut};
	
	while (1) 
		{
			DWORD dwWait = ::WaitForMultipleObjects(3, arrHandles, FALSE, INFINITE);
			
			if (dwWait == WAIT_OBJECT_0) 
				{
					::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
					break;
				} 
			else if (dwWait == WAIT_OBJECT_0 + 1) 
				{
					break;
				} 
			else if (dwWait == WAIT_OBJECT_0 + 2) 
				{
					::SetTimer(m_hWnd, TIMER_REPAINT_CHANGE, m_dwChangeRepaintInt, NULL);
					// we sleep here for a while, to prevent 'flooding' of m_hStdOut events
					::Sleep(m_dwChangeRepaintInt);
					::ResetEvent(m_hStdOut);
				}
		}
	
	return 0;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL WINAPI Console::CtrlHandler(DWORD dwCtrlType) 
{
	if ((dwCtrlType == CTRL_C_EVENT) || (dwCtrlType == CTRL_BREAK_EVENT)) 
		{
			return TRUE;
		} 
	else 
		{
			return FALSE;
		}
}

/////////////////////////////////////////////////////////////////////////////

