
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

void Console::CreateOffscreenBuffers() 
{
	HDC hdcDesktop = ::GetDCEx(m_hWnd, NULL, 0);
	
	// create new window bitmap
	if (m_hbmpConsoleOld) ::SelectObject(m_hdcConsole, m_hbmpConsoleOld);
	if (m_hbmpConsole) ::DeleteObject(m_hbmpConsole);
	
	m_hbmpConsole = ::CreateCompatibleBitmap(hdcDesktop, m_nClientWidth, m_nClientHeight);
	m_hbmpConsoleOld = (HBITMAP)::SelectObject(m_hdcConsole, m_hbmpConsole);
	
	if (m_hbmpWindowOld) ::SelectObject(m_hdcWindow, m_hbmpWindowOld);
	if (m_hbmpWindow) ::DeleteObject(m_hbmpWindow);
	
	m_hbmpWindow = ::CreateCompatibleBitmap(hdcDesktop, m_nClientWidth, m_nClientHeight);
	m_hbmpWindowOld = (HBITMAP)::SelectObject(m_hdcWindow, m_hbmpWindow);
	
	// create selection bitmap
	if (m_hbmpSelectionOld) ::SelectObject(m_hdcSelection, m_hbmpSelectionOld);
	if (m_hbmpSelection) ::DeleteObject(m_hbmpSelection);
	
	m_hbmpSelection = ::CreateCompatibleBitmap(m_hdcWindow, m_nClientWidth, m_nClientHeight);
	m_hbmpSelectionOld = (HBITMAP)::SelectObject(m_hdcSelection, m_hbmpSelection);
	
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = m_nClientWidth;
	rect.bottom = m_nClientHeight;
	::FillRect(m_hdcSelection, &rect, m_hbrushSelection);
	
	::ReleaseDC(m_hWnd, hdcDesktop);
}

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////

void Console::CreateBackgroundBitmap() 
{
	USES_CONVERSION;
	
	if (m_hbmpBackgroundOld) ::SelectObject(m_hdcBackground, m_hbmpBackgroundOld);
	if (m_hbmpBackground) ::DeleteObject(m_hbmpBackground);
	if (m_hdcBackground) ::DeleteDC(m_hdcBackground);
	
	if (! _settings->bitmapBackground()) return;
	
	// determine the total size of the background bitmap
	DWORD	dwPrimaryDisplayWidth	= ::GetSystemMetrics(SM_CXSCREEN);
	DWORD	dwPrimaryDisplayHeight	= ::GetSystemMetrics(SM_CYSCREEN);
	
	DWORD	dwBackgroundWidth		= 0;
	DWORD	dwBackgroundHeight		= 0;

	if (_settings->relativeBackground()) 
		{
			if (g_bWin2000) 
				{
					// Win2K and later can handle multiple monitors
					dwBackgroundWidth	= ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
					dwBackgroundHeight	= ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
					
					// get offsets for virtual display
					m_nBackgroundOffsetX = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
					m_nBackgroundOffsetY = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
				} 
			else
				{
					// WinNT compatibility (hope it works, I didn't test it)
					dwBackgroundWidth	= dwPrimaryDisplayWidth;
					dwBackgroundHeight	= dwBackgroundHeight;
				}
			
		} 
	else
		{
			dwBackgroundWidth	= m_nClientWidth;
			dwBackgroundHeight	= m_nClientHeight;
		}
	
	// now, load the image...
	fipImage	image;
	IMAGE_DATA	imageData;

	if (!image.load(T2A(_settings->backgroundFile().c_str()))) 
		{
			_settings->setBitmapBackground( false);
			return;
		}
	
	imageData.hdcImage		= NULL;
	imageData.dwImageWidth	= image.getWidth();
	imageData.dwImageHeight = image.getHeight();

	image.convertTo24Bits();
	
	// ... if needed, tint the background image
	if (_settings->tintSet())
		{
			BYTE*	pPixels = image.accessPixels();
			BYTE*	pPixelsEnd =  pPixels + 3 * image.getWidth() * image.getHeight();
			BYTE*	pPixelSubel = pPixels;
			
			unsigned long opacityPart = (100 - _settings->tintOpacity());
			unsigned long bPart = _settings->tintB() * _settings->tintOpacity();
			unsigned long gPart = _settings->tintG() * _settings->tintOpacity();
			unsigned long rPart = _settings->tintR() * _settings->tintOpacity();
			
			while (pPixelSubel < pPixelsEnd) 
				{

					*pPixelSubel = (BYTE) ((unsigned long)(*pPixelSubel * opacityPart + bPart) / 100); 
					++pPixelSubel;
					*pPixelSubel = (BYTE) ((unsigned long)(*pPixelSubel * opacityPart + gPart) / 100);
					++pPixelSubel;
					*pPixelSubel = (BYTE) ((unsigned long)(*pPixelSubel * opacityPart + rPart) / 100);
					++pPixelSubel;
				}
		}		
	
	// create the basic image
	HBITMAP		hbmpImage		= NULL;
	HBITMAP		hbmpImageOld	= NULL;

	if (_settings->backgroundStyle() == BACKGROUND_STYLE_RESIZE) 
		{

			if (_settings->relativeBackground()) 
				{
					if (_settings->extendBackground()) 
						{
							imageData.dwImageWidth	= dwBackgroundWidth;
							imageData.dwImageHeight	= dwBackgroundHeight;
						} 
					else
						{
							imageData.dwImageWidth	= dwPrimaryDisplayWidth;
							imageData.dwImageHeight	= dwPrimaryDisplayHeight;
						}
				} 
			else
				{
					imageData.dwImageWidth	= (DWORD)m_nClientWidth;
					imageData.dwImageHeight	= (DWORD)m_nClientHeight;
				}

			if ((image.getWidth() != imageData.dwImageWidth) 
					|| (image.getHeight() != imageData.dwImageHeight)) 
				{
					image.rescale((unsigned short)imageData.dwImageWidth, (unsigned short)imageData.dwImageHeight, FILTER_LANCZOS3);
				}
		}
	
	
	// now, create a DC compatible with the screen and create the basic bitmap
	HDC hdcDesktop		= ::GetDCEx(m_hWnd, NULL, 0);
	imageData.hdcImage	= ::CreateCompatibleDC(hdcDesktop);
	hbmpImage			= ::CreateDIBitmap(
																	 hdcDesktop, 
																	 image.getInfoHeader(), 
																	 CBM_INIT, 
																	 image.accessPixels(), 
																	 image.getInfo(), 
																	 DIB_RGB_COLORS);
	hbmpImageOld= (HBITMAP)::SelectObject(imageData.hdcImage, hbmpImage);
	::ReleaseDC(m_hWnd, hdcDesktop);
	
	// create the background image
	m_hdcBackground	= ::CreateCompatibleDC(imageData.hdcImage);
	m_hbmpBackground = ::CreateCompatibleBitmap(imageData.hdcImage, dwBackgroundWidth, dwBackgroundHeight);
	m_hbmpBackgroundOld = (HBITMAP)::SelectObject(m_hdcBackground, m_hbmpBackground);
	
	RECT rectBackground;
	rectBackground.left		= 0;
	rectBackground.top		= 0;
	rectBackground.right	= dwBackgroundWidth;
	rectBackground.bottom	= dwBackgroundHeight;
	
	// fill the background with the proper background color in case the 
	// bitmap doesn't cover the entire background, 
	COLORREF crBackground;

	if (_settings->transparency() == TRANSPARENCY_FAKE) 
		{
			// get desktop background color
			HKEY hkeyColors;
			if (::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Control Panel\\Colors"), 0, KEY_READ, &hkeyColors) == ERROR_SUCCESS) 
				{
					TCHAR	szData[MAX_PATH+1];
					DWORD	dwDataSize = MAX_PATH;
					
					BYTE	r = 0;
					BYTE	g = 0;
					BYTE	b = 0;
					
					::ZeroMemory(szData, sizeof(szData));
					::RegQueryValueEx(hkeyColors, _T("Background"), NULL, NULL, (BYTE*)szData, &dwDataSize);
					
					_stscanf_s(szData, _T("%i %i %i"), &r, &g, &b);
					crBackground = RGB(r, g, b);
					
					::RegCloseKey(hkeyColors);
				}
		} 
	else
		{
			::CopyMemory(&crBackground, _settings->backgroundPtr(), sizeof(COLORREF));
		}
	
	HBRUSH hBkBrush = ::CreateSolidBrush(crBackground);
	::FillRect(m_hdcBackground, &rectBackground, hBkBrush);
	::DeleteObject(hBkBrush);
	
	if (_settings->backgroundStyle() == BACKGROUND_STYLE_TILE) 
		{
			// we're tiling the image, starting at coordinates (0, 0) of the virtual screen
			DWORD dwX = 0;
			DWORD dwY = 0;
		
			DWORD dwImageOffsetX = 0;
			DWORD dwImageOffsetY = imageData.dwImageHeight + (m_nBackgroundOffsetY - (int)imageData.dwImageHeight*(m_nBackgroundOffsetY/(int)imageData.dwImageHeight));
		
			while (dwY < dwBackgroundHeight) 
				{
					dwX				= 0;
					dwImageOffsetX	= imageData.dwImageWidth + (m_nBackgroundOffsetX - (int)imageData.dwImageWidth*(m_nBackgroundOffsetX/(int)imageData.dwImageWidth));
				
					while (dwX < dwBackgroundWidth) 
						{
							::BitBlt(
											 m_hdcBackground, 
											 dwX, 
											 dwY, 
											 imageData.dwImageWidth, 
											 imageData.dwImageHeight,
											 imageData.hdcImage,
											 dwImageOffsetX,
											 dwImageOffsetY,
											 SRCCOPY);
						
							dwX += imageData.dwImageWidth - dwImageOffsetX;
							dwImageOffsetX = 0;
						}
				
					dwY += imageData.dwImageHeight - dwImageOffsetY;
					dwImageOffsetY = 0;
				}
		
		}
	else if (_settings->extendBackground() || !_settings->relativeBackground()) 
		{
			switch (_settings->backgroundStyle()) 
				{
				case BACKGROUND_STYLE_RESIZE :
					::BitBlt(
									 m_hdcBackground, 
									 0,
									 0,
									 dwBackgroundWidth, 
									 dwBackgroundHeight,
									 imageData.hdcImage,
									 0,
									 0,
									 SRCCOPY);
					break;
					
				case BACKGROUND_STYLE_CENTER :
					::BitBlt(
									 m_hdcBackground, 
									 (dwBackgroundWidth <= imageData.dwImageWidth) ? 0 : (dwBackgroundWidth - imageData.dwImageWidth)/2,
									 (dwBackgroundHeight <= imageData.dwImageHeight) ? 0 : (dwBackgroundHeight - imageData.dwImageHeight)/2,
									 imageData.dwImageWidth, 
									 imageData.dwImageHeight,
									 imageData.hdcImage,
									 (dwBackgroundWidth < imageData.dwImageWidth) ? (imageData.dwImageWidth - dwBackgroundWidth)/2 : 0,
									 (dwBackgroundHeight < imageData.dwImageHeight) ? (imageData.dwImageHeight - dwBackgroundHeight)/2 : 0,
									 SRCCOPY);
					break;
				}
		} 
	else
		{
			::EnumDisplayMonitors(NULL, NULL, Console::BackgroundEnumProc, (DWORD)&imageData);
		}

	::SelectObject(imageData.hdcImage, hbmpImageOld);
	::DeleteObject(hbmpImage);
	::DeleteDC(imageData.hdcImage);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK Console::BackgroundEnumProc(HMONITOR hMonitor, HDC hdcMonitor, 
																					LPRECT lprcMonitor, LPARAM dwData) 
{
	UNREFERENCED_PARAMETER(hdcMonitor);
	UNREFERENCED_PARAMETER(hMonitor);
	
	IMAGE_DATA* pImageData = (IMAGE_DATA*)dwData;

	DWORD	dwDisplayWidth	= lprcMonitor->right - lprcMonitor->left;
	DWORD	dwDisplayHeight	= lprcMonitor->bottom - lprcMonitor->top;
	
	DWORD	dwPrimaryDisplayWidth	= ::GetSystemMetrics(SM_CXSCREEN);
	DWORD	dwPrimaryDisplayHeight	= ::GetSystemMetrics(SM_CYSCREEN);
	
	// center the image according to current display's size and position
	switch (g_pConsole->_settings->backgroundStyle()) 
		{
		
		case BACKGROUND_STYLE_RESIZE :
			::BitBlt(
							 g_pConsole->m_hdcBackground, 
							 (dwDisplayWidth <= dwPrimaryDisplayWidth) ? lprcMonitor->left-g_pConsole->m_nBackgroundOffsetX : lprcMonitor->left-g_pConsole->m_nBackgroundOffsetX + (dwDisplayWidth - dwPrimaryDisplayWidth)/2,
							 (dwDisplayHeight <= dwPrimaryDisplayHeight) ? lprcMonitor->top-g_pConsole->m_nBackgroundOffsetY : lprcMonitor->top-g_pConsole->m_nBackgroundOffsetY + (dwDisplayHeight - dwPrimaryDisplayHeight)/2,
							 dwDisplayWidth, 
							 dwDisplayHeight,
							 pImageData->hdcImage,
							 (dwDisplayWidth < dwPrimaryDisplayWidth) ? (dwPrimaryDisplayWidth - dwDisplayWidth)/2 : 0,
							 (dwDisplayHeight < dwPrimaryDisplayHeight) ? (dwPrimaryDisplayHeight - dwDisplayHeight)/2 : 0,
							 SRCCOPY);
			
			break;
			
		case BACKGROUND_STYLE_CENTER :
			::BitBlt(
							 g_pConsole->m_hdcBackground, 
							 (dwDisplayWidth <= pImageData->dwImageWidth) ? lprcMonitor->left-g_pConsole->m_nBackgroundOffsetX : lprcMonitor->left-g_pConsole->m_nBackgroundOffsetX + (dwDisplayWidth - pImageData->dwImageWidth)/2,
							 (dwDisplayHeight <= pImageData->dwImageHeight) ? lprcMonitor->top-g_pConsole->m_nBackgroundOffsetY : lprcMonitor->top-g_pConsole->m_nBackgroundOffsetY + (dwDisplayHeight - pImageData->dwImageHeight)/2,
							 dwDisplayWidth, 
							 dwDisplayHeight,
							 pImageData->hdcImage,
							 (dwDisplayWidth < pImageData->dwImageWidth) ? (pImageData->dwImageWidth - dwDisplayWidth)/2 : 0,
							 (dwDisplayHeight < pImageData->dwImageHeight) ? (pImageData->dwImageHeight - dwDisplayHeight)/2 : 0,
							 SRCCOPY);
			
			break;
		}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

