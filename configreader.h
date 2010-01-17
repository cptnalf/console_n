/* filename: configreader.h
   chiefengineer
   Sat Jan 16 20:21:22  2010
*/

#pragma once

class ConfigSettings
{
public:

	ConfigSettings();
	~ConfigSettings() { }
	
	void setConfigFile(const tstring filename) { m_strConfigFile = filename; }
	tstring configFile() const { return m_strConfigFile; }
	
	bool load();
	
	void setReloading(const bool reloading) { m_bReloading = reloading; }
	bool reloading() const { return m_bReloading; }
	
	tstring	configEditor() const { return m_strConfigEditor; }
	tstring configEditorParams() const { return m_strConfigEditorParams; }

	// one of the RELOAD_NEW_CONFIG_* constants
	// - specifies relaod behaviour when a new configuration is selected
	// (auto reload, don't reload, ask user)
	DWORD	reloadNewConfig() const { return m_dwReloadNewConfig; }
	void setReloadNewConfig(const DWORD nReloadNewConfig)
	{ m_dwReloadNewConfig = nReloadNewConfig; }
		
	tstring	shell() const { return m_strShell; }
	void setShell(const TCHAR* nShell) { m_strShell = nShell; }
	tstring	shellCmdLine() const { return m_strShellCmdLine; }
 	void setShellCmdLine(const TCHAR* nShellCmdLine) 
	{ m_strShellCmdLine = nShellCmdLine; }
	
	// master repaint timer interval (runs independent of changes in the 
	// console)
	DWORD	masterRepaintInt() const { return m_dwMasterRepaintInt; }

	// change repaint timer interval (when a change occurs, repainting 
	// will be postponed for this interval)
	DWORD	changeRepaintInt() const { return m_dwChangeRepaintInt; }
		
	// icon filename
	tstring	iconFilename() const { return m_strIconFilename; }
		
	// set to TRUE if the popup menu is disabled
	bool popupMenuDisabled() const { return m_bPopupMenuDisabled; }
		
	// holds the window title (default title, or the one from the config file)
	tstring	windowTitle() const { return m_strWindowTitle; }
	
	// font data
	tstring	fontName() const { return m_strFontName; }
	DWORD	fontSize() const { return m_dwFontSize; }
	BOOL	bold() const { return m_bBold; }
	BOOL	italic() const { return m_bItalic; }
	BOOL	useFontColor() const { return m_bUseFontColor; }
	COLORREF fontColor() const { return m_crFontColor; }

	// console colors
	COLORREF consoleColor(const int idx) const { return m_arrConsoleColors[idx]; }
	
	// window X and Y positions
	int getX() const { return m_nX; }
	void setX(const int x) { m_nX = x; }
 	int	getY() const { return m_nY; }
	void setY(const int y) { m_nY = y; }

	// client area inside border (gives a more 'relaxed' look to windows)
	int insideBorder() const { return m_nInsideBorder; }

	// window border type
	// 0 - none
	// 1 - regular window
	// 2 - 1-pixel thin border
	DWORD	windowBorder() const { return m_dwWindowBorder; }

	// scrollbar stuff
	int scrollbarStyle() const { return m_nScrollbarStyle; }
	COLORREF scrollbarColor() const { return m_crScrollbarColor; }
	int scrollbarWidth() const { return m_nScrollbarWidth; }
	int scrollbarButtonHeight() const { return m_nScrollbarButtonHeight; }
	int scrollbarThunmbHeight() const { return m_nScrollbarThunmbHeight; }

	// what to do with the taskbar button
	// if the taskbar button is hidden, or placed in the traybar, you 
	// can't ALT-TAB to console (take care when using with color key 
	// transparency :-)
	// 0 - nothing
	// 1 - hide it
	// 2 - put icon to traybar
	DWORD	taskbarButton() const { return m_dwTaskbarButton; }
		
	// set to TRUE if the window can be dragged by left-click hold
	bool mouseDragable() const { return m_bMouseDragable; }

	// snap distance
	int snapDst() const { return m_nSnapDst; }
		
	// window docking position
	// 0 - no dock
	// 1 - top left
	// 2 - top right
	// 3 - bottom right
	// 4 - bottom left
	DWORD	docked() const { return m_dwDocked; }
	void setDocked(const DWORD nDocked) { m_dwDocked = nDocked; }

	// window Z-ordering
	// 0 - regular
	// 1 - always on top
	// 2 - always on bottom
	DWORD	originalZOrder() const { return m_dwOriginalZOrder; }
	DWORD	currentZOrder() const { return m_dwCurrentZOrder; }
	void setCurrentZOrder(const DWORD nCurZOrder)
	{ m_dwCurrentZOrder = nCurZOrder; }
	
	// Win2000/XP transparency
	// 0 - none
	// 1 - alpha blending
	// 2 - colorkey
	// 3 - fake transparency
	DWORD	transparency() const { return m_dwTransparency; }

	// alpha value for alpha blending (Win2000 and later only!)
	BYTE alpha() const { return m_byAlpha; }

	// alpha value for inactive window
	BYTE inactiveAlpha()  const { return m_byInactiveAlpha; }

	bool bkColorSet() const { return m_bBkColorSet; }
	COLORREF background() const { return m_crBackground; }
	const COLORREF* backgroundPtr() const { return &m_crBackground; }

	// this is used for tinting background images and fake transparencies
	bool tintSet() const { return m_bTintSet; }
	BYTE tintOpacity() const { return m_byTintOpacity; }
	BYTE tintR() const { return m_byTintR; }
	BYTE tintG() const { return m_byTintG; }
	BYTE tintB() const { return m_byTintB; }
		
	// used when background is an image
	bool bitmapBackground() const { return m_bBitmapBackground; }
	tstring backgroundFile() const { return m_strBackgroundFile; }
	
	void setBitmapBackground(const TCHAR* bitmapFile)
	{
		m_bBitmapBackground = true;
		m_strBackgroundFile = bitmapFile;
	}

	// background attributes
	
	// one of BACKGROUND_STYLE_ #defines
	DWORD backgroundStyle() const { return m_dwBackgroundStyle; }
	void setBackgroundStyle(const DWORD style) { m_dwBackgroundStyle = style; }
	
	// set to true for relative background
	bool relativeBackground()  const { return m_bRelativeBackground; }
	void setRelativeBackground(const bool nRelBG)
	{ m_bRelativeBackground = nRelBG; }
	
	// set to true to extend the background to all monitors
	bool extendBackground() const { return m_bExtendBackground; }
	void setExtendBackground(const bool nExtBG)
	{ m_bExtendBackground = nExtBG; }
		
	// set to TRUE when the real console is hidden
	bool hideConsole() const { return m_bHideConsole; }
	void setHideConsole(const bool nHideConsole)
	{ m_bHideConsole = nHideConsole; }

	// timeout used when hiding console window for the first time (some
	// shells need console window visible during startup)
	DWORD	hideConsoleTimeout() const { return m_dwHideConsoleTimeout; }

	// if set to TRUE, Console will be started minimized
	bool startMinimized() const { return m_bStartMinimized; }
		
	// cursor style
	// 0 - none
	// 1 - XTerm
	// 2 - block cursor
	// 3 - bar cursor
	// 4 - console cursor
	// 5 - horizontal line
	// 6 - vertical line
	// 7 - pulse rect
	// 8 - fading block
	DWORD	cursorStyle() const { return m_dwCursorStyle; }

	COLORREF cursorColor() const { return m_crCursorColor; }
		
	// wether console cursor is visible or not
	bool cursorVisible()  const { return m_bCursorVisible; }
	void setCursorVisible(const bool curVis) { m_bCursorVisible = curVis; }
	
	// console rows & columns
	DWORD	rows() const { return m_dwRows; }
	void setRows(const DWORD nRows) { m_dwRows = nRows; }
	
	DWORD	columns() const { return m_dwColumns; }
	void setColumns(const DWORD nColumns) { m_dwColumns = nColumns; }
	
	DWORD	bufferRows() const { return m_dwBufferRows; }
	void setBufferRows(const DWORD nBufferRows) { m_dwBufferRows = nBufferRows; }
	bool useTextBuffer() const { return m_bUseTextBuffer; }

	// X Windows style copy-on-select
	bool copyOnSelect() const { return m_bCopyOnSelect; }

	// Inverse the shift behaviour for selecting and dragging
	bool inverseShift() const { return m_bInverseShift; }
	
	int shadowDistance()  const { return m_shadowDistance; }
	int shadowColor() const { return m_shadowColor; }
	
private:
	void _setDefaults();

	bool	m_bReloading;
										
	tstring	m_strConfigFile;
	tstring	m_strConfigEditor;
	tstring m_strConfigEditorParams;

	// one of the RELOAD_NEW_CONFIG_* constants
	// - specifies relaod behaviour when a new configuration is selected
	// (auto reload, don't reload, ask user)
	DWORD	m_dwReloadNewConfig;
		
	tstring	m_strShell;
	tstring	m_strShellCmdLine;
		
	// master repaint timer interval (runs independent of changes in the 
	// console)
	DWORD	m_dwMasterRepaintInt;

	// change repaint timer interval (when a change occurs, repainting 
	// will be postponed for this interval)
	DWORD	m_dwChangeRepaintInt;
		
	// icon filename
	tstring	m_strIconFilename;
		
	// set to TRUE if the popup menu is disabled
	bool	m_bPopupMenuDisabled;
		
	// holds the window title (default title, or the one from the config file)
	tstring	m_strWindowTitle;
	
	// font data
	tstring	m_strFontName;
	DWORD	m_dwFontSize;
	bool	m_bBold;
	bool	m_bItalic;
	bool	m_bUseFontColor;
	COLORREF m_crFontColor;

	// console colors
	COLORREF	m_arrConsoleColors[16];
	
	// window X and Y positions
	int		m_nX;
	int		m_nY;

	// client area inside border (gives a more 'relaxed' look to windows)
	int		m_nInsideBorder;

	// window border type
	// 0 - none
	// 1 - regular window
	// 2 - 1-pixel thin border
	DWORD	m_dwWindowBorder;

	// scrollbar stuff
	int		m_nScrollbarStyle;
	COLORREF m_crScrollbarColor;
	int		m_nScrollbarWidth;
	int		m_nScrollbarButtonHeight;
	int		m_nScrollbarThunmbHeight;

	// what to do with the taskbar button
	// if the taskbar button is hidden, or placed in the traybar, you 
	// can't ALT-TAB to console (take care when using with color key 
	// transparency :-)
	// 0 - nothing
	// 1 - hide it
	// 2 - put icon to traybar
	DWORD	m_dwTaskbarButton;
		
	// set to TRUE if the window can be dragged by left-click hold
	bool	m_bMouseDragable;

	// snap distance
	int		m_nSnapDst;
		
	// window docking position
	// 0 - no dock
	// 1 - top left
	// 2 - top right
	// 3 - bottom right
	// 4 - bottom left
	DWORD	m_dwDocked;

	// window Z-ordering
	// 0 - regular
	// 1 - always on top
	// 2 - always on bottom
	DWORD	m_dwOriginalZOrder;
	DWORD	m_dwCurrentZOrder;

	// Win2000/XP transparency
	// 0 - none
	// 1 - alpha blending
	// 2 - colorkey
	// 3 - fake transparency
	DWORD	m_dwTransparency;

	// alpha value for alpha blending (Win2000 and later only!)
	BYTE	m_byAlpha;

	// alpha value for inactive window
	BYTE	m_byInactiveAlpha;

	bool	m_bBkColorSet;
	COLORREF m_crBackground;

	// this is used for tinting background images and fake transparencies
	bool	m_bTintSet;
	BYTE	m_byTintOpacity;
	BYTE	m_byTintR;
	BYTE	m_byTintG;
	BYTE	m_byTintB;
		
	// used when background is an image
	bool	m_bBitmapBackground;
	tstring	m_strBackgroundFile;

	// background attributes

	// one of BACKGROUND_STYLE_ #defines
	DWORD	m_dwBackgroundStyle;
	// set to true for relative background
	bool	m_bRelativeBackground;
	// set to true to extend the background to all monitors
	bool	m_bExtendBackground;
		
	// set to TRUE when the real console is hidden
	bool	m_bHideConsole;

	// timeout used when hiding console window for the first time (some
	// shells need console window visible during startup)
	DWORD	m_dwHideConsoleTimeout;

	// if set to TRUE, Console will be started minimized
	bool	m_bStartMinimized;
		
		
	// cursor style
	// 0 - none
	// 1 - XTerm
	// 2 - block cursor
	// 3 - bar cursor
	// 4 - console cursor
	// 5 - horizontal line
	// 6 - vertical line
	// 7 - pulse rect
	// 8 - fading block
	DWORD	m_dwCursorStyle;

	COLORREF m_crCursorColor;
		
	// wether console cursor is visible or not
	bool	m_bCursorVisible;
	
	// console rows & columns
	DWORD	m_dwRows;
	DWORD	m_dwColumns;
	DWORD	m_dwBufferRows;
	bool	m_bUseTextBuffer;

	// X Windows style copy-on-select
	bool	m_bCopyOnSelect;

	// Inverse the shift behaviour for selecting and dragging
	bool	m_bInverseShift;
	
	int m_shadowDistance;
	int m_shadowColor;
};
