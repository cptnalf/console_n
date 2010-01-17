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
	UNREFERENCED_PARAMETER(pszConfigPath);
	
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
		case Z_ORDER_ONTOP		: hwndZ = HWND_TOPMOST; break;
		case Z_ORDER_ONBOTTOM	: hwndZ = HWND_BOTTOM; break;
		default:
		case Z_ORDER_REGULAR	: hwndZ = HWND_NOTOPMOST; break;
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
	_sntprintf(szConsoleTitle, sizeof(szConsoleTitle)/sizeof(TCHAR), _T("%i"), ::GetCurrentThreadId());
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
		case Z_ORDER_ONTOP		: hwndZ = HWND_TOPMOST; break;
		case Z_ORDER_ONBOTTOM	: hwndZ = HWND_BOTTOM; break;
			
		default:
		case Z_ORDER_REGULAR	: hwndZ = HWND_NOTOPMOST; break;
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
		
			_sntprintf(szFilename, 
								 sizeof(szFilename)/sizeof(TCHAR), 
								 _T("%s%s"),
								 strConfigFileDir.c_str(),
								 wfd.cFileName);
			
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
			size_t nPos = strParams.find(_T("%f"));
		
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

