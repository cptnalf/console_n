
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

void Console::SetWindowIcons() 
{
	HICON hOldIcon	= NULL;
	
	if (! _settings->iconFilename().empty()) 
		{
			// custom icon, try loading it...
			if ((m_hSmallIcon = (HICON)::LoadImage(NULL, _settings->iconFilename().c_str(),
																						 IMAGE_ICON, 16, 16,
																						 LR_LOADFROMFILE)) != NULL) 
				{
					// icon file exists, load and set the big icon as well
					m_hBigIcon = (HICON)::LoadImage(NULL, _settings->iconFilename().c_str(),
																					IMAGE_ICON, 0, 0,
																					LR_LOADFROMFILE | LR_DEFAULTSIZE);
					hOldIcon = (HICON)::SendMessage(m_hWnd, WM_SETICON, ICON_BIG,
																					(LPARAM)m_hBigIcon);
					if (hOldIcon) ::DestroyIcon(hOldIcon);
					
				} 
			else
				{
					m_hSmallIcon = (HICON)::LoadImage(g_hInstance, 
																						MAKEINTRESOURCE(IDI_ICON),
																						IMAGE_ICON, 16, 16, 0);
				}
			
		} 
	else
		{
			m_hSmallIcon = (HICON)::LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ICON),
																				IMAGE_ICON, 16, 16, 0);
		}
	
	hOldIcon = (HICON)::SendMessage(m_hWnd, WM_SETICON, ICON_SMALL,
																	(LPARAM)m_hSmallIcon);
	if (hOldIcon) ::DestroyIcon(hOldIcon);
	
	SetTrayIcon(_settings->taskbarButton() == TASKBAR_BUTTON_TRAY ? NIM_ADD : NIM_DELETE);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL Console::SetTrayIcon(DWORD dwMessage) 
{
	NOTIFYICONDATA tnd;
	tstring	strToolTip(_settings->windowTitle());
	
	tnd.cbSize				= sizeof(NOTIFYICONDATA);
	tnd.hWnd				= m_hWnd;
	tnd.uID					= TRAY_ICON_ID;
	tnd.uFlags				= NIF_MESSAGE|NIF_ICON|NIF_TIP;
	tnd.uCallbackMessage	= WM_TRAY_NOTIFY;
	tnd.hIcon				= m_hSmallIcon;
	
	if (strToolTip.length() > 63) 
		{
			strToolTip.resize(59);
			strToolTip += _T(" ...");
			
#ifdef _DEBUG
			DWORD dw = strToolTip.length();
			dw = 12 + dw - 12;
#endif
		}
	
	_tcscpy_s(tnd.szTip, sizeof(tnd.szTip)/sizeof(TCHAR), strToolTip.c_str());
	return ::Shell_NotifyIcon(dwMessage, &tnd);
}

/////////////////////////////////////////////////////////////////////////////

