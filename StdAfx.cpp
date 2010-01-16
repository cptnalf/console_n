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


// stdafx.cpp : source file that includes just the standard includes
//	Console.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "strsafe.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables

/////////////////////////////////////////////////////////////////////////////
// global EXE instance handle
HINSTANCE	g_hInstance		= NULL;

/////////////////////////////////////////////////////////////////////////////
// global pointer to Console object
Console*	g_pConsole		= NULL;

/////////////////////////////////////////////////////////////////////////////
// global var for OS version (TRUE - running on NT/2000, FALSE - running on Win9x)
BOOL g_bWinNT = FALSE;

/////////////////////////////////////////////////////////////////////////////
// global var for OS version (TRUE - Win2000, FALSE - otherwise)
BOOL g_bWin2000 = FALSE;

/////////////////////////////////////////////////////////////////////////////
// Win2k library stuff
HMODULE						g_hUser32				= NULL;
HMODULE						g_hMsimg32				= NULL;
SETLAYEREDWINDOWATTRIBUTES	g_pfnSetLayeredWndAttr	= NULL;
ALPHABLEND					g_pfnAlphaBlend			= NULL;


/////////////////////////////////////////////////////////////////////////////
// Memory allocation tracking

#ifdef _DEBUG

void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine)
{
	return ::_malloc_dbg(nSize, 1, lpszFileName, nLine);
}

#if _MSC_VER >= 1200
void __cdecl operator delete(void* pData, LPCSTR /* lpszFileName */,
							 int /* nLine */)
{
	::operator delete(pData);
}
#endif

#endif

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// trace function and TRACE macro

//#ifdef _DEBUG
#include <stdio.h>

void Trace(const TCHAR* pszFormat, ...)
{
	TCHAR szOutput[512];
	va_list	vaList;
	
	va_start(vaList, pszFormat);
	_vstprintf_s(szOutput, sizeof(szOutput), pszFormat, vaList);
	::OutputDebugString(szOutput);
}

//#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
