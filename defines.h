/* filename: defines.h
   chiefengineer
   Wed Jan 13 20:10:59  2010
*/

#pragma once

// tray icon message
#define WM_TRAY_NOTIFY				WM_USER + 0x123

#define TRAY_ICON_ID				1

// timer #defines
#define TIMER_REPAINT_CHANGE		42	// timer that is started after there 
										// were some changes in the console

#define TIMER_REPAINT_MASTER		43	// master timer (needed to repaint 
										// for some DOS programs, can be 
										// disabled for lower CPU usage)

#define TIMER_SHOW_HIDE_CONSOLE		100	// used to hide console window when 
										// starting shell process after a 
										// defined period of time (some 
										// shells, like 4NT need console 
										// window visible during startup for
										// all init options to work properly

// transparency #defines
#define TRANSPARENCY_NONE			0
#define TRANSPARENCY_ALPHA			1
#define TRANSPARENCY_COLORKEY		2
#define TRANSPARENCY_FAKE			3

// cusror style #defines
#define CURSOR_STYLE_NONE			0
#define CURSOR_STYLE_XTERM			1
#define CURSOR_STYLE_BLOCK			2
#define CURSOR_STYLE_NBBLOCK		3
#define CURSOR_STYLE_PULSEBLOCK		4
#define CURSOR_STYLE_BAR			5
#define CURSOR_STYLE_CONSOLE		6
#define CURSOR_STYLE_NBHLINE		7
#define CURSOR_STYLE_HLINE			8
#define CURSOR_STYLE_VLINE			9
#define CURSOR_STYLE_RECT			10
#define CURSOR_STYLE_NBRECT			11
#define CURSOR_STYLE_PULSERECT		12
#define CURSOR_STYLE_FADEBLOCK		13

// docking #defines
#define DOCK_NONE					0
#define DOCK_TOP_LEFT				1
#define DOCK_TOP_RIGHT				2
#define DOCK_BOTTOM_RIGHT			3
#define DOCK_BOTTOM_LEFT			4

// Z-order #defines
#define Z_ORDER_REGULAR				0
#define Z_ORDER_ONTOP				1
#define Z_ORDER_ONBOTTOM			2

// window border defines
#define BORDER_NONE					0
#define BORDER_REGULAR				1
#define BORDER_THIN					2

// window background style
#define	BACKGROUND_STYLE_RESIZE		0
#define	BACKGROUND_STYLE_CENTER		1
#define	BACKGROUND_STYLE_TILE		2

// taskbar button defines
#define TASKBAR_BUTTON_NORMAL		0
#define TASKBAR_BUTTON_HIDE			1
#define TASKBAR_BUTTON_TRAY			2

// new configuration auto-reload defines
#define RELOAD_NEW_CONFIG_PROMPT	0
#define RELOAD_NEW_CONFIG_YES		1
#define RELOAD_NEW_CONFIG_NO		2

#define TEXT_SELECTION_NONE			0
#define TEXT_SELECTION_SELECTING	1
#define TEXT_SELECTION_SELECTED		2
