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
// Console.h - console class declaration

#pragma once

#include <string>
#include <vector>

using namespace std;

#include "FreeImagePlus.h"

typedef basic_string<TCHAR>			tstring;

#include "defines.h"

#include "configreader.h"

/////////////////////////////////////////////////////////////////////////////
// Console class

class Console
{
public: // ctor/dtor
	
	Console(LPCTSTR szConfigFile, 
					LPCTSTR szShellCmdLine, 
					LPCTSTR szConsoleTitle, 
					LPCTSTR pszReloadNewConfig);
	~Console();
	
public:
	
	// creates and shows Console window
	BOOL Create(TCHAR* pszConfigPath);
	
private:
	
	////////////////////
	// message handlers
	////////////////////
	
	////////////////////////////////
	// windows destruction messages
	
	// destroys GDI stuff and posts a quit message
	void OnDestroy();
	// deletes Console object
	void OnNcDestroy();
	
	
	/////////////////////
	// painting messages
	
	// handles painting from off-screen buffers
	void OnPaint();
	
	// handles master and 'change' timers
	void OnPaintTimer();
	
	// handles cursor timer (for animated cursors)
	void OnCursorTimer();
	
	
	/////////////////////////
	// window state messages
	
	// handles window position changes (for snapping to desktop edges)
	void OnWindowPosChanging(WINDOWPOS* lpWndPos);
	
	// handles activation message (used for setting alpha transparency and cursor states)
	void OnActivateApp(BOOL bActivate, DWORD dwFlags);
	
	// handles vertical scrolling
	void OnVScroll(WPARAM wParam);
	
	// handles keyboard layout change, posts the same message to the windows console window
	void OnInputLangChangeRequest(WPARAM wParam, LPARAM lParam);
	
	
	//////////////////
	// mouse messages
	
	// handles text selection start and window mouse drag start
	void OnLButtonDown(UINT uiFlags, POINTS points);
	
	// handles text selection end, window mouse drag end and text copy
	void OnLButtonUp(UINT uiFlags, POINTS points);
	
	// toggles always on top flag
	void OnLButtonDblClick(UINT uiFlags, POINTS points);
	
	// pops up the Console menu
	void OnRButtonUp(UINT uiFlags, POINTS points);
	
	// pastes text from clipboard
	void OnMButtonDown(UINT uiFlags, POINTS points);
	
	// handles mouse movement for text selection and window mouse drag
	void OnMouseMove(UINT uiFlags, POINTS points);
	
	// handles start/stop mouse drag for window border
	void OnSetCursor(WORD wHitTest, WORD wMouseMessage);
	
	
	//////////////////
	// other messages
	
	// called before the Console menu or system menu pops up 
	// (populates config files submenu with filenames)
	void OnInitMenuPopup(HMENU hMenu, UINT uiPos, BOOL bSysMenu);
	
	// handles drag-n-dropped filenames
	void OnDropFiles(HDROP hDrop);
	
	// handles commands from the Console popup menu
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	
	// handles commands from the system popup menu
	BOOL OnSysCommand(WPARAM wParam, LPARAM lParam);
	
	// handles tray icon messages
	void OnTrayNotify(WPARAM wParam, LPARAM lParam);
	
	// handles WM_SETTINGCHANGE (we handle only wallpaper changes here)
	void OnWallpaperChanged(const TCHAR* pszFilename);
	
	inline void Console::ShadowTextOut (HDC hdc, int nXStart, int nYStart, LPCTSTR lpString, int cbString);		
private:
	
	///////////////////////////////////////////
	// Console window creation/setup functions
	///////////////////////////////////////////
	
	// gets Console options
	BOOL GetOptions();
	
	// registers Console window classes
	BOOL RegisterWindowClasses();
	
	// adds stuff to system and popup menus
	BOOL SetupMenus();
	
	// creates Console window
	BOOL CreateConsoleWindow();
	
	// creates new Console font
	void CreateNewFont();
	
	// creates new background brush
	void CreateNewBrush();
	
	// creates the cursor
	void CreateCursor();
		
	// creates offscreen painting buffers
	void CreateOffscreenBuffers();
		
	// creates background bitmap
	void CreateBackgroundBitmap();

	// called by the ::EnumDisplayMonitors to create background for each display
	static BOOL CALLBACK BackgroundEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
		
	// calculates window and client area sizes
	void CalcWindowSize();

	// sets up window transparency (alpha, color key, fake)
	void SetWindowTransparency();

	// sets scrollbar stuff
	void SetScrollbarStuff();
		
	// sets default console colors
	//void SetDefaultConsoleColors();
		
	// sets window size and position
	void SetWindowSizeAndPosition();
		
	// sets Console's big, small and tray icons
	void SetWindowIcons();
		
	// sets up traybar icon
	BOOL SetTrayIcon(DWORD dwMessage);

	// destroys Console cursor
	void DestroyCursor();

	// opens the configuration file in a text editor
	void EditConfigFile();
		
	// reloads Console settings
	void ReloadSettings();


	/////////////////////////////
	// windows console functions
	/////////////////////////////
		
	// allocates the console and starts the command shell
	BOOL StartShellProcess();
		
	// refreshes m_hStdOutFresh handle
	void RefreshStdOut();
		
	// gets a fresh console output
	void RefreshScreenBuffer();

	// sets initial windows console size
	void InitConsoleWndSize(DWORD dwColumns);
		
	// resizes the windows console
	void ResizeConsoleWindow();
		
		
	//////////////////////
	// painting functions
	//////////////////////

	// repaints the memory hdc
	void RepaintWindow();
		
	// repaints the memory hdc (paint only changes)
	void RepaintWindowChanges();
		
	// draws the cursor
	void DrawCursor(BOOL bOnlyCursor = FALSE);
		
	// two helper functions for DrawCursor method

	// returns cursor rectangle
	inline void GetCursorRect(RECT& rectCursor);
	// draws cursor background and returns the cursor rectangle
	inline void DrawCursorBackground(RECT& rectCursor);

	// clears text selection
	void ClearSelection();
		
	// returns the console text change rate since the last painting
	// (using this value we decide whether to repaint entire window
	// or just the changes)
	DWORD GetChangeRate();


	//////////////////////////
	// window state functions
	//////////////////////////
		
	// shows/hides Console window
	void ShowHideWindow();
		
	// shows/hides windows console window
	void ShowHideConsole();

	// shows/hides windows console window after a timeout (used during
	// shell startup)
	void ShowHideConsoleTimeout();
		
	// toggles 'always on top' status
	void ToggleWindowOnTop();


	//////////////////
	// menu functions
	//////////////////

	// called by OnCommand and OnSysCommand to handle menu commands
	BOOL HandleMenuCommand(unsigned int dwID);
		
	// updates popup and system menus for 'always on top' status
	void UpdateOnTopMenuItem();
		
	// updates popup and system menus for 'hide console' status
	void UpdateHideConsoleMenuItem();

	// updates configuration files submenu
	void UpdateConfigFilesSubmenu();


	///////////////////////
	// clipboard functions
	///////////////////////
		
	// copies selected text to the clipboard
	void CopyTextToClipboard();
		
	// pastes text from the clipboard
	void PasteClipoardText();
		
	//////////////////
	// misc functions
	//////////////////

	// shows Readme.txt file
	void ShowReadmeFile();

	// shows about dialog
	void About();

	// sends text to the windows console
	void SendTextToConsole(LPCTSTR pszText);

	// returns the full filename
	tstring GetFullFilename(const tstring& strFilename);


	///////////////////////
	// 'gearbox' functions
	///////////////////////
		
	// Console window procedure
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// shell activities monitor
	static DWORD WINAPI MonitorThreadStatic(LPVOID lpParam);
	DWORD MonitorThread();

	// CTRL+C, CTRL+BREAK handler
	static BOOL WINAPI CtrlHandler(DWORD dwCtrlType);


public: // public data

	// Console window handle
	HWND	m_hWnd;


private: // private data
	void _resetVars();
	
	ConfigSettings* _settings;
	
	// readem filename
	tstring	m_strReadmeFile;

	BOOL	m_bInitializing;
	BOOL	m_bReloading;

	// one of the RELOAD_NEW_CONFIG_* constants
	// - specifies relaod behaviour when a new configuration is selected
	// (auto reload, don't reload, ask user)
	DWORD	m_dwReloadNewConfigDefault;
	DWORD	m_dwReloadNewConfig;
		
	// handle to invisible window - used for hiding the taskbar button in tray and 'hidden' modes
	HWND m_hwndInvisParent;

	// memory device context that holds console output image
	HDC		m_hdcConsole;
	// memory device context for an off-screen window buffer (used to 
	// compose an image from m_hdcConsole and m_hdcSelection if text 
	// selection is active)
	HDC		m_hdcWindow;
		
	// a bitmap used for drawing in the console memory DC
	HBITMAP	m_hbmpConsole;
	HBITMAP	m_hbmpConsoleOld;

	// a bitmap used for drawing in the window memory DC
	HBITMAP	m_hbmpWindow;
	HBITMAP	m_hbmpWindowOld;
		
	// brush for painting background
	HBRUSH	m_hBkBrush;

	// program icons
	HICON	m_hSmallIcon;
	HICON	m_hBigIcon;
		
	// popup menu
	HMENU	m_hPopupMenu;
		
	// system (taskbar button) menu
	HMENU	m_hSysMenu;

	// submenu for the XML (config) files
	HMENU	m_hConfigFilesMenu;
		
	// Console window title variables
	// holds the default console title ("console" or the one passed in the cmdline param)
	tstring m_strWindowTitleDefault;
	// holds the current window title
	tstring m_strWindowTitleCurrent;
		
	// font data
	HFONT	m_hFont;
	HFONT	m_hFontOld;
		
	// window width and height
	int		m_nWindowWidth;
	int		m_nWindowHeight;

	// window border sizes
	int		m_nXBorderSize;
	int		m_nYBorderSize;
	int		m_nCaptionSize;

	// client area widht/height
	int		m_nClientWidth;
	int		m_nClientHeight;

	// char height and width (used in window repainting) 
	// Note: width is used only for fixed-pitch fonts to speed up 
	// repainting
	int		m_nCharHeight;
	int		m_nCharWidth;

	// scrollbar stuff
	BOOL	m_bShowScrollbar;
		
	// used when background is an image
	HDC		m_hdcBackground;
	HBITMAP	m_hbmpBackground;
	HBITMAP	m_hbmpBackgroundOld;
		
	// offsets used for multiple monitors and relative backgrounds (fake transparency, too)
	int		m_nBackgroundOffsetX;
	int		m_nBackgroundOffsetY;
	
	// console screen buffer info for cursor
	CONSOLE_SCREEN_BUFFER_INFO m_csbiCursor;
	
	// wether console cursor is visible or not
	BOOL	m_bCursorVisible;

	class Cursor*	m_pCursor;

	// mouse cursor offset within the window (used for moving the window)
	POINT	m_mouseCursorOffset;
		
	/////////////////////
	// console stuff

	// used for showing/hiding main window
	BOOL	m_bHideWindow;

	// console window handle
	HWND	m_hWndConsole;

	// console screen buffer info for console repainting
	CONSOLE_SCREEN_BUFFER_INFO m_csbiConsole;
		
	// console stdouts
	HANDLE	m_hStdOut;
	HANDLE	m_hStdOutFresh;
		
	// set when quitting the application
	HANDLE	m_hQuitEvent;

	// set by monitor thread when detects console process exit
	HANDLE	m_hConsoleProcess;

	// handle to monitor thread
	HANDLE	m_hMonitorThread;

	// set to one of TEXT_SELECTION_ #defines
	int		m_nTextSelection;

	COORD	m_coordSelOrigin;
	RECT	m_rectSelection;
	HDC		m_hdcSelection;
	HBITMAP	m_hbmpSelection;
	HBITMAP	m_hbmpSelectionOld;
	HBRUSH	m_hbrushSelection;

	CHAR_INFO*	m_pScreenBuffer;
	CHAR_INFO*	m_pScreenBufferNew;


	// Console window class names
	static const TCHAR m_szConsoleClass[];
	static const TCHAR m_szHiddenParentClass[];

	// win console window title
	static const tstring m_strWinConsoleTitle;
};



/////////////////////////////////////////////////////////////////////////////
