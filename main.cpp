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


// Console.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#define COMPILE_MULTIMON_STUBS
#include <multimon.h>

#include "Console.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// Used for parsing cmd line

TCHAR* ParseNextToken(TCHAR* pSrc, LPTSTR pszDest) {

	DWORD dwPos;

	while (*pSrc && ((*pSrc == _TCHAR(' ')) || (*pSrc == _TCHAR('\t')))) ++pSrc;

	if (*pSrc == _TCHAR('\"')) {
		// if there are blanks in the string, it must be enclosed in quotes
		++pSrc;
		dwPos = 0;
		while (*pSrc) {
			if (*pSrc == _TCHAR('\"')) {
				++pSrc;
				if (*pSrc != _TCHAR('\"')) {
					break;
				}
			}
			pszDest[dwPos++] = *(pSrc++);
		}
	} else {
		//  just go to the blank
		dwPos = 0;
		while (*pSrc && (*pSrc != _TCHAR(' ')) && (*pSrc != _TCHAR('\t'))) {
			pszDest[dwPos++] = *(pSrc++);
		}
	}

	return pSrc;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

int APIENTRY _tWinMain(HINSTANCE hInstance,
											 HINSTANCE  hPrevInstance,
											 LPTSTR     lpCmdLine,
											 int        nCmdShow) 
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);
	
#ifdef _DEBUG
	int nDbgFlag = ::_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	nDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
	::_CrtSetDbgFlag(nDbgFlag);
#endif

	g_hInstance = hInstance;

	// determine Windows version (9x or NT/2000)
	g_osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	::GetVersionEx(&g_osInfo);
	
	if (g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			g_bWinNT = TRUE;
			g_bWin2000 = (g_osInfo.dwMajorVersion >= 5);
		} 
	else 
		{
			::MessageBox(NULL, 
									 _T("Console requires Windows NT/2000/XP"), 
									 _T("Wrong Windows version"), MB_OK);
			return FALSE;
		}

	// let's parse the command line
	TCHAR	szConfigFile[MAX_PATH];
	TCHAR	szShellCmdLine[MAX_PATH];
	TCHAR	szConsoleTitle[MAX_PATH];
	TCHAR	szReloadNewConfig[32];
	TCHAR	szTmp[MAX_PATH];
	TCHAR*	pPos = lpCmdLine;

	::ZeroMemory(szConfigFile, sizeof(szConfigFile));
	::ZeroMemory(szShellCmdLine, sizeof(szShellCmdLine));
	::ZeroMemory(szConsoleTitle, sizeof(szConsoleTitle));
	::ZeroMemory(szReloadNewConfig, sizeof(szReloadNewConfig));
	
	// skip blanks
	while (*pPos != 0x0) {

		::ZeroMemory(szTmp, sizeof(szTmp));
		pPos = ParseNextToken(pPos, szTmp);

		if ((szTmp[0] == _TCHAR('-')) || (szTmp[0] == _TCHAR('/'))) 
			{
				// cmd-line option
				if (szTmp[1] == _TCHAR('c')) 
					{
						pPos = ParseNextToken(pPos, szShellCmdLine);
					} 
				else if (szTmp[1] == _TCHAR('t')) 
					{
						pPos = ParseNextToken(pPos, szConsoleTitle);
					} 
				else if (szTmp[1] == _TCHAR('r')) 
					{
						pPos = ParseNextToken(pPos, szReloadNewConfig);
					}
			} 
		else if (szConfigFile[0] == 0x0) 
			{
				// config file
				_tcscpy_s(szConfigFile, sizeof(szConfigFile)/sizeof(TCHAR), szTmp);
			}
	}

	if (g_bWin2000)
		{
			// load User32.dll, and get some Win2k-specific functions
			g_hUser32 = ::LoadLibrary(_T("user32.dll"));
			if (g_hUser32 == NULL) 
				{
					::MessageBox(NULL, _T("Unable to load User32.dll! Exiting."), _T("Error"), MB_OK);
					return FALSE;
				}
			g_hMsimg32 = ::LoadLibrary(_T("Msimg32.dll"));
			if (g_hMsimg32 == NULL) 
				{
					::MessageBox(NULL, _T("Unable to load Msimg32.dll! Exiting."), _T("Error"), MB_OK);
					return FALSE;
				}
			
			g_pfnAlphaBlend			= (ALPHABLEND)::GetProcAddress(g_hMsimg32, "AlphaBlend");
			g_pfnSetLayeredWndAttr	= (SETLAYEREDWINDOWATTRIBUTES)::GetProcAddress(g_hUser32, "SetLayeredWindowAttributes");
			
			if ((g_osInfo.dwMajorVersion >= 6)
					&& (g_osInfo.dwMinorVersion >= 1))
				{
					/* so, windows 7. setup appusermodelid stuffs. */
					/*
					 * see:
					 * http://msdn.microsoft.com/en-us/library/dd378459(VS.85).aspx
					 * title: Application User Model IDs (AppUserModelIDs)
					 * 
					 */
					
					typedef HRESULT (*SetCurProcAppUserModelIDFx_T)(const wchar_t* appID);
					
					SetCurProcAppUserModelIDFx_T procSetModelID = NULL;
					
					HMODULE sh32 = GetModuleHandle(_T("shell32.dll"));
					
					if (sh32)
						{
							procSetModelID = (SetCurProcAppUserModelIDFx_T)
								GetProcAddress(sh32, "SetCurrentProcessExplicitAppUserModelID");
							
							if (procSetModelID) { procSetModelID(L"Ingenuity_Unlimited_Ltd.Console"); }
						}
				}
			
			InitMultipleMonitorStubs();
		}
	
	// create console class instance
	g_pConsole = new Console(szConfigFile, szShellCmdLine, szConsoleTitle, szReloadNewConfig);

	if (!g_pConsole->Create(NULL)) 
		{
			DEL_OBJ(g_pConsole);
			return 0;	
		}
	
	// pump messages (no TranslateMessage here, we do that in WinProc)
	MSG	msg;
	while (::GetMessage(&msg, NULL, 0, 0)) ::DispatchMessage(&msg);

	if (g_bWin2000) 
		{
			::FreeLibrary(g_hUser32);
			::FreeLibrary(g_hMsimg32);
		}
	
	return msg.wParam;
}

/////////////////////////////////////////////////////////////////////////////
