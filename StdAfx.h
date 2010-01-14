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


// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _WIN32_WINNT	0x0500 
#define WINVER			0x0500

#include <windows.h>
#include <tchar.h>

#if !defined(HMONITOR_DECLARED)
//DECLARE_HANDLE(HMONITOR);
#define HMONITOR_DECLARED
#endif
#include <multimon.h>


// taken from winuser.h
#define WS_EX_LAYERED           0x00080000

#define LWA_COLORKEY            0x00000001
#define LWA_ALPHA               0x00000002


/////////////////////////////////////////////////////////////////////////////
// Memory allocation tracking

#ifdef _DEBUG
#include <crtdbg.h>
void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
#define DEBUG_NEW new(THIS_FILE, __LINE__)
#if _MSC_VER >= 1200
void __cdecl operator delete(void* p, LPCSTR lpszFileName, int nLine);
#endif
#endif

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Global variables

/////////////////////////////////////////////////////////////////////////////
// global EXE instance handle
extern HINSTANCE	g_hInstance;

/////////////////////////////////////////////////////////////////////////////
// global pointer to Console object
extern class Console*	g_pConsole;

/////////////////////////////////////////////////////////////////////////////
// global var for OS version (TRUE - running on NT/2000, FALSE - running on Win9x)
extern BOOL	g_bWinNT;

/////////////////////////////////////////////////////////////////////////////
// global var for OS version (TRUE - Win2000, FALSE - otherwise)
extern BOOL g_bWin2000;


/////////////////////////////////////////////////////////////////////////////
// trace function and TRACE macro

#ifdef _DEBUG

void Trace(const TCHAR* pszFormat, ...);

#define TRACE		::Trace

#else

#define TRACE		(void)0

#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

#define SAFERELEASE(p) if (p) {(p)->Release(); p = NULL;}

#define DEL_OBJ(p) if (p) {delete p; p = NULL;}

#define DEL_ARR(p) if (p) {delete [] p; p = NULL;}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// struct used for painting background on multiple monitors

typedef struct {
	HDC		hdcImage;
	DWORD	dwImageWidth;
	DWORD	dwImageHeight;
} IMAGE_DATA;

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Stuff we need for WinNT compatibility

typedef BOOL (WINAPI* ALPHABLEND)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
typedef BOOL (WINAPI* SETLAYEREDWINDOWATTRIBUTES)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);

extern HMODULE						g_hUser32;
extern HMODULE						g_hMsimg32;
extern SETLAYEREDWINDOWATTRIBUTES	g_pfnSetLayeredWndAttr;
extern ALPHABLEND					g_pfnAlphaBlend;
// these can be found in multimon.h
extern HMONITOR (WINAPI* g_pfnMonitorFromPoint)(POINT, DWORD);
extern BOOL     (WINAPI* g_pfnGetMonitorInfo)(HMONITOR, LPMONITORINFO);


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
