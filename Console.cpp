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
	, m_hdcConsole(NULL)
	, m_hdcWindow(NULL)
	, m_hbmpConsole(NULL)
	, m_hbmpConsoleOld(NULL)
	, m_hbmpWindow(NULL)
	, m_hbmpWindowOld(NULL)
	, m_hBkBrush(NULL)
	, m_hSysMenu(NULL)
	, m_hFont(NULL)
	, m_hFontOld(NULL)
	, m_hdcBackground(NULL)
	, m_hbmpBackground(NULL)
	, m_hbmpBackgroundOld(NULL)
	, m_nBackgroundOffsetX(0)
	, m_nBackgroundOffsetY(0)
	, m_pCursor(NULL)
	, m_hWndConsole(NULL)
	, m_hStdOut(NULL)
	, m_hStdOutFresh(NULL)
	, m_hQuitEvent(NULL)
	, m_hConsoleProcess(NULL)
	, m_hMonitorThread(NULL)
	, m_nTextSelection(TEXT_SELECTION_NONE)
	, m_hdcSelection(NULL)
	, m_hbmpSelection(NULL)
	, m_hbmpSelectionOld(NULL)
	, m_hbrushSelection(::CreateSolidBrush(RGB(0xff, 0xff, 0xff)))
	, m_pScreenBuffer(NULL)
	, m_pScreenBufferNew(NULL)
	, m_bShowScrollbar(FALSE)
{
	_resetVars();
	_settings = new ConfigSettings();
	
	_settings->setShellCmdLine(pszShellCmdLine);
	_settings->setConfigFile(GetFullFilename(pszConfigFile));
	_settings->setWindowTitle(pszConsoleTitle);

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
	
	_settings->setReloadNewConfig(m_dwReloadNewConfigDefault);
	
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
	if (_settings->configFile().empty()) 
		{
			_settings->setConfigFile(strExeDir + tstring(_T("console.xml")));
		}
	
	// get readme filename
	m_strReadmeFile = strExeDir + tstring(_T("Readme.txt"));
	
	::ZeroMemory(&m_csbiCursor, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
	::ZeroMemory(&m_csbiConsole, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
	
	//SetDefaultConsoleColors();
	_resetVars();
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


void Console::_resetVars()
{
	m_hwndInvisParent		= NULL;
	m_hSmallIcon			= NULL;
	m_hBigIcon				= NULL;
	m_hPopupMenu			= NULL;
	m_hConfigFilesMenu		= NULL;
	m_strWindowTitleCurrent.clear();
	
	m_nWindowWidth			= 0;
	m_nWindowHeight			= 0;
	m_nXBorderSize			= 0;
	m_nYBorderSize			= 0;
	m_nCaptionSize			= 0;
	m_nClientWidth			= 0;
	m_nClientHeight			= 0;
	m_nCharHeight			= 0;
	m_nCharWidth			= 0;

	m_bHideWindow			= FALSE;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Public methods

/////////////////////////////////////////////////////////////////////////////
// creates and shows Console window

BOOL Console::Create(TCHAR* pszConfigPath) 
{
	UNREFERENCED_PARAMETER(pszConfigPath);
	
	if (!_settings->load()) return FALSE;
	
	m_strWindowTitleCurrent = _settings->windowTitle();
	
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
	if (_settings->masterRepaintInt()) 
		{
			::SetTimer(m_hWnd, TIMER_REPAINT_MASTER, _settings->masterRepaintInt(), NULL);
		}
	
	m_bInitializing = FALSE;
	
	// register for files/folders drag-n-drop
	::DragAcceptFiles(m_hWnd, TRUE);

	RefreshScreenBuffer();
	::CopyMemory(m_pScreenBuffer, m_pScreenBufferNew, 
							 sizeof(CHAR_INFO) * _settings->rows() * _settings->columns());
	RepaintWindow();
	::UpdateWindow(m_hWnd);

	if (_settings->startMinimized()) 
		{
			if (_settings->taskbarButton() > TASKBAR_BUTTON_NORMAL) 
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
	if ((m_hPopupMenu = ::LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_POPUP_MENU))) == NULL) 
		{ return FALSE; }
	
	if ((m_hConfigFilesMenu = ::CreateMenu()) == NULL) { return FALSE; }
	
	// add stuff to the system menu
	TCHAR*			arrMenuItems[] = 
		{
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
			_T(""),
		};
	
	DWORD			arrMenuTypes[] = 
		{
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
			MFT_SEPARATOR,
		};
	DWORD			arrMenuIDs[] = 
		{
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
			0,
		};
	
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
	switch (_settings->windowBorder()) 
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
	if (_settings->taskbarButton() > TASKBAR_BUTTON_NORMAL) 
		{
			if ((m_hwndInvisParent = 
					 ::CreateWindowEx(
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
			if (_settings->taskbarButton() == TASKBAR_BUTTON_HIDE)
				{
					dwWindowStyle &= ~WS_MINIMIZEBOX;
				}
		}
	
	if ((m_hWnd = ::CreateWindowEx(
																 0, 
																 //m_dwTaskbarButton > TASKBAR_BUTTON_NORMAL ? WS_EX_TOOLWINDOW : 0,
																 
																 Console::m_szConsoleClass, 
																 _settings->windowTitle().c_str(),
																 dwWindowStyle,
																 ((_settings->getX() == -1) || (_settings->getY() == -1)) ? CW_USEDEFAULT : 0,
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
	
	if ((_settings->getX() == -1) || (_settings->getY() == -1)) 
		{
			RECT rectWindow;
			
			::GetWindowRect(m_hWnd, &rectWindow);
			_settings->setX((int)rectWindow.left);
			_settings->setY((int)rectWindow.top);
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
												 -MulDiv(_settings->fontSize(),
																 ::GetDeviceCaps(m_hdcConsole, LOGPIXELSY), 72),
												 0,
												 0,
												 0,
												 _settings->bold() ? FW_BOLD : 0,
												 _settings->italic(),
												 FALSE,
												 FALSE,
												 DEFAULT_CHARSET,						
												 OUT_DEFAULT_PRECIS,
												 CLIP_DEFAULT_PRECIS,
												 DEFAULT_QUALITY,
												 DEFAULT_PITCH,
												 _settings->fontName().c_str());
	
	m_hFontOld = (HFONT)::SelectObject(m_hdcConsole, m_hFont);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::CreateNewBrush() 
{
	// create a new background brush
	if (m_hBkBrush) ::DeleteObject(m_hBkBrush);
	m_hBkBrush = ::CreateSolidBrush(_settings->background());
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::CreateCursor() 
{
	
	switch (_settings->cursorStyle()) 
		{
		case CURSOR_STYLE_XTERM :
			m_pCursor = new XTermCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_BLOCK :
			m_pCursor = new BlockCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_NBBLOCK :
			m_pCursor = new NBBlockCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_PULSEBLOCK :
			m_pCursor = new PulseBlockCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_BAR :
			m_pCursor = new BarCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_CONSOLE :
			m_pCursor = new ConsoleCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_NBHLINE :
			m_pCursor = new NBHLineCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_HLINE :
			m_pCursor = new HLineCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_VLINE :
			m_pCursor = new VLineCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_RECT :
			m_pCursor = new RectCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_NBRECT :
			m_pCursor = new NBRectCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_PULSERECT :
			m_pCursor = new PulseRectCursor(m_hWnd, m_hdcConsole, _settings->cursorColor());
			break;
		case CURSOR_STYLE_FADEBLOCK :
			m_pCursor = new FadeBlockCursor(m_hWnd, m_hdcConsole, 
																			_settings->cursorColor(), 
																			_settings->background());
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
	m_nClientHeight	= _settings->rows() * m_nCharHeight + (2 * _settings->insideBorder());
	
	if (!(textMetric.tmPitchAndFamily & TMPF_FIXED_PITCH)) 
		{
			// fixed pitch font (TMPF_FIXED_PITCH is cleared!!!)
			m_nCharWidth= textMetric.tmAveCharWidth;
			m_nClientWidth	= _settings->columns() * m_nCharWidth + 2 * _settings->insideBorder();
		} 
	else 
		{
			// variable pitch font
			int nCharWidth;
			::GetCharWidth32(m_hdcConsole, _TCHAR('m'), _TCHAR('m'), &nCharWidth);
			m_nClientWidth	= _settings->columns() * nCharWidth;
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
	m_nCaptionSize = ((titlebarInfo.rgstate[0] & STATE_SYSTEM_INVISIBLE) 
										? 0 
										: titlebarInfo.rcTitleBar.bottom - titlebarInfo.rcTitleBar.top);
	
	switch (_settings->windowBorder()) 
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
	
	if (m_bShowScrollbar) { m_nWindowWidth += _settings->scrollbarWidth(); }
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::SetWindowTransparency() 
{
	
	// set alpha transparency (Win2000 and later only!)
	if (g_bWin2000 && ((_settings->transparency() == TRANSPARENCY_ALPHA) 
										 || (_settings->transparency() == TRANSPARENCY_COLORKEY))) 
		{
			
			::SetWindowLong(m_hWnd, GWL_EXSTYLE, ::GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
			g_pfnSetLayeredWndAttr(m_hWnd, _settings->background(), _settings->alpha(), 
														 (_settings->transparency() == TRANSPARENCY_ALPHA 
															? LWA_ALPHA 
															: LWA_COLORKEY));
			
		} 
	else if (_settings->transparency() == TRANSPARENCY_FAKE) 
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
							_settings->setBitmapBackground(szData);
							_settings->setRelativeBackground(true);
							_settings->setExtendBackground(false);
							
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
									_settings->setBackgroundStyle(BACKGROUND_STYLE_TILE);
								}
							else
								{
									
									if (dwWallpaperStyle == 0) 
										{ _settings->setBackgroundStyle(BACKGROUND_STYLE_CENTER); } 
									else 
										{ _settings->setBackgroundStyle(BACKGROUND_STYLE_RESIZE); }
								}
						}
					
					::RegCloseKey(hkeyDesktop);
				}
		}
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
	
	switch (_settings->docked()) 
		{
		case DOCK_TOP_LEFT:
			// top left
			_settings->setX(dwLeft);
			_settings->setY(dwTop);
			break;
			
		case DOCK_TOP_RIGHT:
			// top right
			_settings->setX( dwScreenWidth - m_nWindowWidth);
			_settings->setY( dwTop);
			break;
			
		case DOCK_BOTTOM_RIGHT:
			// bottom right
			_settings->setX( dwScreenWidth - m_nWindowWidth);
			_settings->setY( dwScreenHeight - m_nWindowHeight);
			break;
			
		case DOCK_BOTTOM_LEFT:
			// bottom left
			_settings->setX( dwLeft);
			_settings->setY( dwScreenHeight - m_nWindowHeight);
			break;
		}
	
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
								 _settings->getX(), 
								 _settings->getY(), 
								 m_nWindowWidth, 
								 m_nWindowHeight, 
								 0);
	
	UpdateOnTopMenuItem();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::DestroyCursor() { if (m_pCursor) DEL_OBJ(m_pCursor); }

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL Console::StartShellProcess() 
{
	tstring	strShellCmdLine(_settings->shell());
	if (! _settings->shellCmdLine().empty()) 
		{
			strShellCmdLine += _T(" ");
			strShellCmdLine += _settings->shellCmdLine();
		}
	
	// create the console window
	TCHAR	szConsoleTitle[MAX_PATH];
	::AllocConsole();
	
	// we use this to avoid possible problems with multiple console instances running
	_sntprintf(szConsoleTitle, sizeof(szConsoleTitle)/sizeof(TCHAR), 
						 _T("%i"), ::GetCurrentThreadId());
	
	::SetConsoleTitle(szConsoleTitle);
	m_hStdOut	= ::GetStdHandle(STD_OUTPUT_HANDLE);
	while ((m_hWndConsole = ::FindWindow(NULL, szConsoleTitle)) == NULL)
		{ ::Sleep(50); }
	
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
	
	if (_settings->hideConsoleTimeout() > 0) 
		{
			::ShowWindow(m_hWndConsole, SW_MINIMIZE);
			::SetTimer(m_hWnd, TIMER_SHOW_HIDE_CONSOLE, _settings->hideConsoleTimeout(), NULL);
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
	DWORD dwCount				= _settings->rows() * _settings->columns();
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
	::ShowWindow(m_hWndConsole, _settings->hideConsole() ? SW_HIDE : SW_SHOWNORMAL);
	if (!_settings->hideConsole()) ::SetForegroundWindow(m_hWndConsole);
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
	if (_settings->currentZOrder() == _settings->originalZOrder()) 
		{
			_settings->setCurrentZOrder(Z_ORDER_ONTOP);
		} 
	else 
		{
			_settings->setCurrentZOrder(_settings->originalZOrder());
		}
	
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
			_settings->setHideConsole(!_settings->hideConsole());
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
			if (_settings->taskbarButton() > TASKBAR_BUTTON_NORMAL) 
				{
					m_bHideWindow = !m_bHideWindow;
					ShowHideWindow();
					::SetForegroundWindow(m_hWnd);
					return FALSE;
				}
			return TRUE;
		}
	
	// check if it's one of config file submenu items
	if ((dwID >= ID_FIRST_XML_FILE) 
			&& (dwID <= ID_LAST_XML_FILE)) 
		{
			TCHAR	szFilename[MAX_PATH];
			::ZeroMemory(szFilename, sizeof(szFilename));
			::GetMenuString(m_hConfigFilesMenu, dwID, szFilename, MAX_PATH, MF_BYCOMMAND);
			_settings->setConfigFile(szFilename);
			
			if (_settings->reloadNewConfig() == RELOAD_NEW_CONFIG_PROMPT) 
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
			else if (_settings->reloadNewConfig() == RELOAD_NEW_CONFIG_YES) 
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
									MF_BYCOMMAND | ((_settings->currentZOrder() == Z_ORDER_ONTOP)
																	? MF_CHECKED : MF_UNCHECKED));
	::CheckMenuItem(m_hSysMenu, ID_TOGGLE_ONTOP, 
									MF_BYCOMMAND | ((_settings->currentZOrder() == Z_ORDER_ONTOP)
																	? MF_CHECKED : MF_UNCHECKED));
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::UpdateHideConsoleMenuItem() 
{
	::CheckMenuItem(::GetSubMenu(m_hPopupMenu, 0), ID_HIDE_CONSOLE, 
									MF_BYCOMMAND | (_settings->hideConsole() ? MF_CHECKED : MF_UNCHECKED));
	::CheckMenuItem(m_hSysMenu, ID_HIDE_CONSOLE, 
									MF_BYCOMMAND | (_settings->hideConsole() ? MF_CHECKED : MF_UNCHECKED));
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
	int		nBackslashPos = _settings->configFile().rfind(_TCHAR('\\'));
	tstring	strConfigFileDir(_settings->configFile().substr(0, nBackslashPos+1));
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
		
			if (_tcsicmp(szFilename, _settings->configFile().c_str()) == 0) 
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
	tstring strParams(_settings->configEditorParams());
	
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
	
	::ShellExecute(NULL, NULL, 
								 _settings->configEditor().c_str(), strParams.c_str(), NULL, SW_SHOWNORMAL);
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
					::SetTimer(m_hWnd, TIMER_REPAINT_CHANGE, _settings->changeRepaintInt(), NULL);
					// we sleep here for a while, to prevent 'flooding' of m_hStdOut events
					::Sleep(_settings->changeRepaintInt());
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

