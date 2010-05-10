
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
			if (_settings->masterRepaintInt()) ::KillTimer(m_hWnd, TIMER_REPAINT_MASTER);
			
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

void Console::OnNcDestroy() 
{
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
			::CopyMemory(m_pScreenBuffer, m_pScreenBufferNew, 
									 sizeof(CHAR_INFO) * _settings->rows() * _settings->columns());
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
	
	nDelta = max(-nCurrentPos, 
							 min(nDelta, (int)(_settings->bufferRows() - _settings->rows()) - nCurrentPos));
	if (nDelta)
		{
			nCurrentPos += nDelta; 
			
			SMALL_RECT sr;
			sr.Top = (short)nDelta;
			sr.Bottom = (short)nDelta;
			sr.Left = sr.Right = 0;
			::SetConsoleWindowInfo(m_hStdOutFresh, FALSE, &sr);
			
			SCROLLINFO si;
			si.cbSize = sizeof(si); 
			si.fMask  = SIF_POS; 
			si.nPos   = nCurrentPos; 
			::FlatSB_SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
		
			// this seems to work/look much better than direct repainting...
			::SetTimer(m_hWnd, TIMER_REPAINT_CHANGE,_settings->changeRepaintInt(), NULL);
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
	
	if (!_settings->mouseDragable() ||
			(_settings->inverseShift() == !(uiFlags & MK_SHIFT))) 
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
					
					m_coordSelOrigin.X = 	min((short)(max(points.x - _settings->insideBorder(), 0) / m_nCharWidth), 
																		(short)(_settings->columns() - 1));
					m_coordSelOrigin.Y = min((short)(max(points.y - _settings->insideBorder(), 0) / m_nCharHeight), 
																	 (short)(_settings->rows() - 1));
					
					m_rectSelection.left = m_rectSelection.right = 
						m_coordSelOrigin.X * m_nCharWidth + _settings->insideBorder();
					m_rectSelection.top = m_rectSelection.bottom = 
						m_coordSelOrigin.Y * m_nCharHeight + _settings->insideBorder();
					
					TRACE(_T("Starting point: %ix%i\n"), m_coordSelOrigin.X, m_coordSelOrigin.Y);
				}
			
		} 
	else
		{
			if (m_nTextSelection) 
				{
					return;
				} 
			else if (_settings->mouseDragable()) 
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
	UNREFERENCED_PARAMETER(uiFlags);
	UNREFERENCED_PARAMETER(points);
	
	if ((m_nTextSelection == TEXT_SELECTION_SELECTED) ||
			// X Windows select/copy style
			((m_nTextSelection == TEXT_SELECTION_SELECTING) && (_settings->copyOnSelect()))) 
		{
			
			// if the user clicked inside the selection rectangle, copy data
			if ((points.x >= m_rectSelection.left) && 
					(points.x <= m_rectSelection.right) && 
					(points.y >= m_rectSelection.top) && 
					(points.y <= m_rectSelection.bottom)) 
				{
					
					CopyTextToClipboard();
				}
			
			if (_settings->copyOnSelect()) ::ReleaseCapture();
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
	else if (_settings->mouseDragable()) 
		{
			// end window drag
			::ReleaseCapture();
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnLButtonDblClick(UINT uiFlags, POINTS points) 
{
	UNREFERENCED_PARAMETER(uiFlags);
	UNREFERENCED_PARAMETER(points);
	
	ToggleWindowOnTop();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnRButtonUp(UINT uiFlags, POINTS points) 
{
	if (uiFlags & MK_SHIFT) 
		{
			PasteClipoardText();
		}
	else 
		{
			
			if (_settings->popupMenuDisabled()) return;
			
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

void Console::OnMButtonDown(UINT uiFlags, POINTS points) 
{
	UNREFERENCED_PARAMETER(uiFlags);
	UNREFERENCED_PARAMETER(points);
	PasteClipoardText();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnMouseMove(UINT uiFlags, POINTS points) 
{
	
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
							if ((!_settings->mouseDragable())
									|| (_settings->inverseShift() == !(uiFlags & MK_SHIFT))) 
								{
									
									// some text has been selected, just return
									if (m_nTextSelection == TEXT_SELECTION_SELECTED) return;
									
									// selecting text for copy/paste
									COORD coordSel;
									
									::InvalidateRect(m_hWnd, &m_rectSelection, FALSE);
									
									coordSel.X = min((short)(max(points.x - _settings->insideBorder(), 0) / m_nCharWidth),
																	 (short)(_settings->columns() - 1));
									coordSel.Y = min((short)(max(points.y - _settings->insideBorder(), 0) / m_nCharHeight),
																	 (short)(_settings->rows() - 1));
									
									TRACE(_T("End point: %ix%i\n"), coordSel.X, coordSel.Y);
									
									if (coordSel.X >= m_coordSelOrigin.X) 
										{
											m_rectSelection.left = m_coordSelOrigin.X * m_nCharWidth + _settings->insideBorder();
											m_rectSelection.right = (coordSel.X + 1) * m_nCharWidth + _settings->insideBorder();
										} 
									else
										{
											m_rectSelection.left = coordSel.X * m_nCharWidth + _settings->insideBorder();
											m_rectSelection.right = (m_coordSelOrigin.X + 1) * m_nCharWidth + _settings->insideBorder();
										}
									
									if (coordSel.Y >= m_coordSelOrigin.Y) 
										{
											m_rectSelection.top = m_coordSelOrigin.Y * m_nCharHeight + _settings->insideBorder();
											m_rectSelection.bottom = (coordSel.Y + 1) * m_nCharHeight + _settings->insideBorder();
										} 
									else 
										{
											m_rectSelection.top = coordSel.Y * m_nCharHeight + _settings->insideBorder();
											m_rectSelection.bottom = (m_coordSelOrigin.Y + 1) * m_nCharHeight + _settings->insideBorder();
										}
									
									TRACE(_T("Selection rect: %i,%i x %i,%i\n"), 
												m_rectSelection.left, m_rectSelection.top, 
												m_rectSelection.right, m_rectSelection.bottom);
									
									::InvalidateRect(m_hWnd, &m_rectSelection, FALSE);
								}
							
						} 
					else if (_settings->mouseDragable()) 
						{
							
							// moving the window
							HWND hwndZ;
							switch (_settings->currentZOrder())
								{
								case Z_ORDER_ONTOP		: hwndZ = HWND_TOPMOST; break;
								case Z_ORDER_ONBOTTOM	: hwndZ = HWND_BOTTOM; break;
									
								default:
								case Z_ORDER_REGULAR	: hwndZ = HWND_NOTOPMOST; break;
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

BOOL Console::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

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
	UNREFERENCED_PARAMETER(lParam);
	
	return HandleMenuCommand(wParam);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnTrayNotify(WPARAM wParam, LPARAM lParam) 
{
	UNREFERENCED_PARAMETER(wParam);
	
	switch (lParam) 
		{
		case WM_RBUTTONUP: 
			{
				
				if (_settings->popupMenuDisabled()) return;
				
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

void Console::OnWindowPosChanging(WINDOWPOS* lpWndPos) 
{
	
	if (!(lpWndPos->flags & SWP_NOMOVE)) 
		{
			if (_settings->snapDst() >= 0) 
				{
					// we'll snap Console window to the desktop edges
					RECT rectDesktop;
					
					ZeroMemory(&rectDesktop, sizeof(rectDesktop));
					
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
					
					_settings->setDocked(DOCK_NONE);
					DWORD	dwLeftRight = 0;
					DWORD	dwTopBottom = 0;
					
					// now, see if we're close to the edges
					if (lpWndPos->x <= rectDesktop.left + _settings->snapDst()) 
						{
							lpWndPos->x = rectDesktop.left;
							dwLeftRight = 1;
						}
					
					if (lpWndPos->x >= rectDesktop.right - m_nWindowWidth - _settings->snapDst()) 
						{
							lpWndPos->x = rectDesktop.right - m_nWindowWidth;
							dwLeftRight = 2;
						}
					
					if (lpWndPos->y <= rectDesktop.top + _settings->snapDst()) 
						{
							lpWndPos->y = rectDesktop.top;
							dwTopBottom = 1;
						}
					
					if (lpWndPos->y >= rectDesktop.bottom - m_nWindowHeight - _settings->snapDst()) 
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
									_settings->setDocked(DOCK_TOP_LEFT);
								} 
							else if (dwTopBottom == 2) 
								{
									// bottom left
									_settings->setDocked(DOCK_BOTTOM_LEFT);
								}
						} 
					else if (dwLeftRight == 2) 
						{
							// right edge
							if (dwTopBottom == 1) 
								{
									// top right
									_settings->setDocked(DOCK_TOP_RIGHT);
								} 
							else if (dwTopBottom == 2) 
								{
									// bottom right
									_settings->setDocked(DOCK_BOTTOM_RIGHT);
								}
						}
				}
			
			_settings->setX(lpWndPos->x);
			_settings->setY(lpWndPos->y);
			
			//		TRACE(_T("Win pos: %ix%i\n"), m_nX, m_nY);
			
			// we need to repaint for relative backgrounds
			if (_settings->relativeBackground() && !m_bInitializing) RepaintWindow();
		}
	
	if (_settings->currentZOrder() == Z_ORDER_ONBOTTOM) lpWndPos->hwndInsertAfter = HWND_BOTTOM;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnActivateApp(BOOL bActivate, DWORD dwFlags) 
{
	UNREFERENCED_PARAMETER(dwFlags);
	
	if (m_pCursor) 
		{
			m_pCursor->SetState(bActivate);
			DrawCursor();
		}
	
	if ((_settings->transparency() == TRANSPARENCY_ALPHA) 
			&& (_settings->inactiveAlpha() > 0)) 
		{
			if (bActivate) 
				{
					g_pfnSetLayeredWndAttr(m_hWnd, _settings->background(), _settings->alpha(), LWA_ALPHA);
				} 
			else
				{
					g_pfnSetLayeredWndAttr(m_hWnd, 
																 _settings->background(), _settings->inactiveAlpha(), LWA_ALPHA);
				}
			
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::OnInitMenuPopup(HMENU hMenu, UINT uiPos, BOOL bSysMenu) 
{
	UNREFERENCED_PARAMETER(uiPos);
	UNREFERENCED_PARAMETER(bSysMenu);
	
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

void Console::OnWallpaperChanged(const TCHAR* pszFilename) 
{
	UNREFERENCED_PARAMETER(pszFilename);
	
	if (_settings->transparency() == TRANSPARENCY_FAKE) 
		{
		SetWindowTransparency();
		CreateBackgroundBitmap();
		RepaintWindow();
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
					if (_settings->mouseDragable()) 
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
					if (_settings->mouseDragable()) 
						{
							// end window drag
							::ReleaseCapture();
						}
				}
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::EditConfigFile() 
{
	// prepare editor parameters
	tstring strParams(_settings->configEditorParams());
	
	if (strParams.empty()) 
		{
			// no params, just use the config file
			strParams = _settings->configFile();
		} 
	else
		{
			size_t nPos = strParams.find(_T("%f"));
			
			if (nPos == tstring::npos) 
				{
					// no '%f' in editor params, concatenate config file name
					strParams += _T(" ");
					strParams += _settings->configFile();
				} 
			else
				{
					// replace '%f' with config file name
					strParams = strParams.replace(nPos, 2, _settings->configFile());
				}
		}
	
	// start editor
	::ShellExecute(NULL, NULL, _settings->configEditor().c_str(), 
								 strParams.c_str(), NULL, SW_SHOWNORMAL);
}

/////////////////////////////////////////////////////////////////////////////

