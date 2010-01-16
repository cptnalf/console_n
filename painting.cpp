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

void Console::ResizeConsoleWindow() 
{
	m_bInitializing = TRUE;
	
	if (m_nTextSelection) ClearSelection();
	
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	::GetConsoleScreenBufferInfo(m_hStdOutFresh, &csbi);
	
	// in case buffer rows increased since last resize, and we're not using text buffer
	if (!m_bUseTextBuffer) m_dwBufferRows = m_dwRows;
	
	COORD		coordBuffersSize;
	coordBuffersSize.X = (SHORT)m_dwColumns;
	coordBuffersSize.Y = (SHORT)m_dwBufferRows;
	
	SMALL_RECT	srConsoleRect;
	srConsoleRect.Top	= srConsoleRect.Left =0;
	srConsoleRect.Right	= (short)(m_dwColumns - 1);
	srConsoleRect.Bottom= (short)(m_dwRows - 1);
	
	// order of setting window size and screen buffer size depends on current and desired dimensions
	if ((DWORD) csbi.dwSize.X * csbi.dwSize.Y > (DWORD) m_dwColumns * m_dwBufferRows)
		{
		
		if (m_bUseTextBuffer && (csbi.dwSize.Y > (short)m_dwBufferRows)) 
			{
				m_dwBufferRows = coordBuffersSize.Y = csbi.dwSize.Y;
			}
		
		::SetConsoleWindowInfo(m_hStdOutFresh, TRUE, &srConsoleRect);
		::SetConsoleScreenBufferSize(m_hStdOutFresh, coordBuffersSize);
		
		//	} else if (((DWORD)csbi.dwSize.X < m_dwColumns) || ((DWORD)csbi.dwSize.Y < m_dwBufferRows) || ((DWORD)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1) != m_dwRows)) {
		} 
	else if ((DWORD) csbi.dwSize.X * csbi.dwSize.Y < (DWORD) m_dwColumns * m_dwBufferRows) 
		{
			
			if (csbi.dwSize.Y < (short)m_dwBufferRows) 
				{
					m_dwBufferRows = coordBuffersSize.Y = csbi.dwSize.Y;
				}
			
			::SetConsoleScreenBufferSize(m_hStdOutFresh, coordBuffersSize);
			::SetConsoleWindowInfo(m_hStdOutFresh, TRUE, &srConsoleRect);
		}
	
	SetScrollbarStuff();
	CalcWindowSize();
	SetWindowSizeAndPosition();
	CreateOffscreenBuffers();
	
	if (m_bBitmapBackground) CreateBackgroundBitmap();
	
	// resize screen buffer
	DEL_ARR(m_pScreenBuffer);
	m_pScreenBuffer = new CHAR_INFO[m_dwRows * m_dwColumns];
	
	m_bInitializing = FALSE;
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::RepaintWindow() 
{
	RECT rect;
	rect.top	= 0;
	rect.left	= 0;
	rect.bottom	= m_nClientHeight;
	rect.right	= m_nClientWidth;
	
	if (m_bBitmapBackground) 
		{
			if (m_bRelativeBackground) 
				{
					::BitBlt(m_hdcConsole, 0, 0, 
									 m_nClientWidth, m_nClientHeight, m_hdcBackground, 
									 m_nX+m_nXBorderSize-m_nBackgroundOffsetX, 
									 m_nY+m_nCaptionSize+m_nYBorderSize-m_nBackgroundOffsetY, 
									 SRCCOPY);
				} 
			else
				{
					::BitBlt(m_hdcConsole, 0, 0, 
									 m_nClientWidth, m_nClientHeight, m_hdcBackground, 0, 0, 
									 SRCCOPY);
				}
		} 
	else
		{
			::FillRect(m_hdcConsole, &rect, m_hBkBrush);
		}
	
	DWORD dwX			= m_nInsideBorder;
	DWORD dwY			= m_nInsideBorder;
	DWORD dwOffset		= 0;
	
	WORD attrBG;

	// stuff used for caching
	int			nBkMode		= TRANSPARENT;
	COLORREF	crBkColor	= RGB(0, 0, 0);
	COLORREF	crTxtColor	= RGB(0, 0, 0);
		
	bool		bTextOut		= false;
	
	tstring		strText(_T(""));
	
	if (m_nCharWidth > 0) 
		{
			// fixed pitch font
			for (DWORD i = 0; i < m_dwRows; ++i) 
				{
					dwX = m_nInsideBorder;
					dwY = i*m_nCharHeight + m_nInsideBorder;
					
					nBkMode			= TRANSPARENT;
					crBkColor		= RGB(0, 0, 0);
					crTxtColor		= RGB(0, 0, 0);
			
					bTextOut		= false;
			
					attrBG = m_pScreenBuffer[dwOffset].Attributes >> 4;
			
					// here we decide how to paint text over the backgound
					if (m_arrConsoleColors[attrBG] == m_crBackground) 
						{
							::SetBkMode(m_hdcConsole, TRANSPARENT);
							nBkMode		= TRANSPARENT;
						} 
					else
						{
							::SetBkMode(m_hdcConsole, OPAQUE);
							nBkMode		= OPAQUE;
							::SetBkColor(m_hdcConsole, m_arrConsoleColors[attrBG]);
							crBkColor	= m_arrConsoleColors[attrBG];
						}
			
					::SetTextColor(m_hdcConsole, m_bUseFontColor
												 ? m_crFontColor
												 : m_arrConsoleColors[m_pScreenBuffer[dwOffset].Attributes & 0xF]);
					crTxtColor		= m_bUseFontColor 
						? m_crFontColor
						: m_arrConsoleColors[m_pScreenBuffer[dwOffset].Attributes & 0xF];
					
					strText = m_pScreenBuffer[dwOffset].Char.UnicodeChar;
					++dwOffset;
					
					for (DWORD j = 1; j < m_dwColumns; ++j) 
						{
							attrBG = m_pScreenBuffer[dwOffset].Attributes >> 4;
							
							if (m_arrConsoleColors[attrBG] == m_crBackground) 
								{
									if (nBkMode != TRANSPARENT) 
										{
											nBkMode = TRANSPARENT;
											bTextOut = true;
										}
								} 
							else
								{
									if (nBkMode != OPAQUE)
										{
											nBkMode = OPAQUE;
											bTextOut = true;
										}
									if (crBkColor != m_arrConsoleColors[attrBG]) 
										{
											crBkColor = m_arrConsoleColors[attrBG];
											bTextOut = true;
										}
								}

							if (crTxtColor != (m_bUseFontColor 
																 ? m_crFontColor
																 : m_arrConsoleColors[m_pScreenBuffer[dwOffset].Attributes & 0xF])) 
								{
									crTxtColor = m_bUseFontColor
										? m_crFontColor
										: m_arrConsoleColors[m_pScreenBuffer[dwOffset].Attributes & 0xF];
									bTextOut = true;
								}
					
							if (bTextOut)
								{
									//::TextOut(m_hdcConsole, dwX, dwY, 
									//					strText.c_str(), strText.length());
									ShadowTextOut(m_hdcConsole, dwX, dwY, 
																strText.c_str(), strText.length());
									dwX += strText.length() * m_nCharWidth;
							
									::SetBkMode(m_hdcConsole, nBkMode);
									::SetBkColor(m_hdcConsole, crBkColor);
									::SetTextColor(m_hdcConsole, crTxtColor);
							
									strText = m_pScreenBuffer[dwOffset].Char.UnicodeChar;
							
								} 
							else
								{
									strText += m_pScreenBuffer[dwOffset].Char.UnicodeChar;
								}
					
							++dwOffset;
						}
			
					if (strText.length() > 0)
						{
							//::TextOut(m_hdcConsole, dwX, dwY, strText.c_str(), strText.length());
							ShadowTextOut(m_hdcConsole, dwX, dwY, strText.c_str(), strText.length());
						}
				}
		}
	else
		{
			// variable pitch font
			memcpy(m_pScreenBuffer, m_pScreenBufferNew, 
						 sizeof(CHAR_INFO)*m_dwRows * m_dwColumns);
		
			for (DWORD i = 0; i < m_dwRows; ++i)
				{
					dwX = m_nInsideBorder;
					dwY = i*m_nCharHeight + m_nInsideBorder;
					
					for (DWORD j = 0; j < m_dwColumns; ++j) 
						{
							attrBG = m_pScreenBuffer[dwOffset].Attributes >> 4;
							
							// here we decide how to paint text over the backgound
							if (m_arrConsoleColors[attrBG] == m_crBackground) 
								{
									::SetBkMode(m_hdcConsole, TRANSPARENT);
								} 
							else
								{
									::SetBkMode(m_hdcConsole, OPAQUE);
									::SetBkColor(m_hdcConsole, m_arrConsoleColors[attrBG]);
								}
				
							::SetTextColor(m_hdcConsole, m_bUseFontColor ? m_crFontColor : m_arrConsoleColors[m_pScreenBuffer[dwOffset].Attributes & 0xF]);
							ShadowTextOut(m_hdcConsole, dwX, dwY, &(m_pScreenBuffer[dwOffset].Char.UnicodeChar), 1);
							int nWidth;
							::GetCharWidth32(m_hdcConsole, m_pScreenBuffer[dwOffset].Char.UnicodeChar, m_pScreenBuffer[dwOffset].Char.UnicodeChar, &nWidth);
							dwX += nWidth;
							++dwOffset;
						}
				}
		}
	
	if (m_pCursor) DrawCursor(TRUE);
	
	::InvalidateRect(m_hWnd, NULL, FALSE);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::RepaintWindowChanges() 
{
	DWORD dwX			= m_nInsideBorder;
	DWORD dwY			= m_nInsideBorder;
	DWORD dwOffset		= 0;
	
	WORD attrBG;

	if (m_nCharWidth > 0) 
		{

			// fixed pitch font
			for (DWORD i = 0; i < m_dwRows; ++i) 
				{
			
					dwX = m_nInsideBorder;
					dwY = i*m_nCharHeight + m_nInsideBorder;

					for (DWORD j = 0; j < m_dwColumns; ++j) 
						{

							if (memcmp(&(m_pScreenBuffer[dwOffset]), &(m_pScreenBufferNew[dwOffset]), sizeof(CHAR_INFO))) 
								{

									memcpy(&(m_pScreenBuffer[dwOffset]), &(m_pScreenBufferNew[dwOffset]), sizeof(CHAR_INFO));

									RECT rect;
									rect.top	= dwY;
									rect.left	= dwX;
									rect.bottom	= dwY + m_nCharHeight;
									rect.right	= dwX + m_nCharWidth;
					
									if (m_bBitmapBackground) 
										{
											if (m_bRelativeBackground) 
												{
													::BitBlt(m_hdcConsole, dwX, dwY, m_nCharWidth, m_nCharHeight, m_hdcBackground, m_nX+m_nXBorderSize-m_nBackgroundOffsetX+(int)dwX, m_nY+m_nCaptionSize+m_nYBorderSize-m_nBackgroundOffsetY+(int)dwY, SRCCOPY);
												} 
											else
												{
													::BitBlt(m_hdcConsole, dwX, dwY, m_nCharWidth, m_nCharHeight, m_hdcBackground, dwX, dwY, SRCCOPY);
												}
										} 
									else
										{
											::FillRect(m_hdcConsole, &rect, m_hBkBrush);
										}
					

									attrBG = m_pScreenBuffer[dwOffset].Attributes >> 4;

									// here we decide how to paint text over the backgound
									if (m_arrConsoleColors[attrBG] == m_crBackground) 
										{
											::SetBkMode(m_hdcConsole, TRANSPARENT);
										} 
									else
										{
											::SetBkMode(m_hdcConsole, OPAQUE);
											::SetBkColor(m_hdcConsole, m_arrConsoleColors[attrBG]);
										}
					
									::SetTextColor(m_hdcConsole, m_bUseFontColor ? m_crFontColor : m_arrConsoleColors[m_pScreenBuffer[dwOffset].Attributes & 0xF]);
									ShadowTextOut(m_hdcConsole, dwX, dwY, &(m_pScreenBuffer[dwOffset].Char.UnicodeChar), 1);

									::InvalidateRect(m_hWnd, &rect, FALSE);
								}

							dwX += m_nCharWidth;
							++dwOffset;
						}
				}
		
		} 
	else
		{
			// variable pitch font
			if (memcmp(m_pScreenBuffer, m_pScreenBufferNew, 
								 sizeof(CHAR_INFO)*m_dwRows * m_dwColumns)) 
				{
					memcpy(m_pScreenBuffer, m_pScreenBufferNew, 
								 sizeof(CHAR_INFO)*m_dwRows * m_dwColumns);
				
					RECT rect;
					rect.top	= 0;
					rect.left	= 0;
					rect.bottom	= m_nClientHeight;
					rect.right	= m_nClientWidth;
		
					if (m_bBitmapBackground) 
						{
							if (m_bRelativeBackground) 
								{
									::BitBlt(m_hdcConsole, 0, 0, 
													 m_nClientWidth, m_nClientHeight, m_hdcBackground, 
													 m_nX+m_nXBorderSize-m_nBackgroundOffsetX, 
													 m_nY+m_nCaptionSize+m_nYBorderSize-m_nBackgroundOffsetY, 
													 SRCCOPY);
								} 
							else 
								{
									::BitBlt(m_hdcConsole, 0, 0, 
													 m_nClientWidth, m_nClientHeight, m_hdcBackground, 
													 0, 0, SRCCOPY);
								}
						} 
					else
						{
							::FillRect(m_hdcConsole, &rect, m_hBkBrush);
						}
		
					for (DWORD i = 0; i < m_dwRows; ++i) 
						{
							dwX = m_nInsideBorder;
							dwY = i*m_nCharHeight + m_nInsideBorder;
			
							for (DWORD j = 0; j < m_dwColumns; ++j) 
								{
				
									attrBG = m_pScreenBuffer[dwOffset].Attributes >> 4;
				
									// here we decide how to paint text over the backgound
									if (m_arrConsoleColors[attrBG] == m_crBackground) 
										{
											::SetBkMode(m_hdcConsole, TRANSPARENT);
										} 
									else
										{
											::SetBkMode(m_hdcConsole, OPAQUE);
											::SetBkColor(m_hdcConsole, m_arrConsoleColors[attrBG]);
										}
				
									::SetTextColor(m_hdcConsole, m_bUseFontColor ? m_crFontColor : m_arrConsoleColors[m_pScreenBuffer[dwOffset].Attributes & 0xF]);
									ShadowTextOut(m_hdcConsole, dwX, dwY, &(m_pScreenBuffer[dwOffset].Char.UnicodeChar), 1);
									int nWidth;
									::GetCharWidth32(m_hdcConsole, m_pScreenBuffer[dwOffset].Char.UnicodeChar, m_pScreenBuffer[dwOffset].Char.UnicodeChar, &nWidth);
									dwX += nWidth;
									++dwOffset;
								}
						}
		
					::InvalidateRect(m_hWnd, NULL, FALSE);
				}
		}
	
	if (m_pCursor) DrawCursor();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::DrawCursor(BOOL bOnlyCursor) 
{
	
	// first, erase old cursor image if needed
	if (!bOnlyCursor && m_bCursorVisible) 
{
		RECT rectCursorOld;
		DrawCursorBackground(rectCursorOld);
		::InvalidateRect(m_hWnd, &rectCursorOld, FALSE);
	}
	
	// now, see if the cursor is visible...
	CONSOLE_CURSOR_INFO	cinf;
	::GetConsoleCursorInfo(m_hStdOutFresh, &cinf);
	
	m_bCursorVisible = cinf.bVisible;
	
	// ... and draw it
	if (m_bCursorVisible) 
		{
		
			::GetConsoleScreenBufferInfo(m_hStdOutFresh, &m_csbiCursor);
		
			if (m_csbiCursor.dwCursorPosition.Y < m_csbiCursor.srWindow.Top 
					|| m_csbiCursor.dwCursorPosition.Y > m_csbiCursor.srWindow.Bottom) 
				{
					m_bCursorVisible = FALSE;
					return;
				}
		
			// set proper cursor offset
			m_csbiCursor.dwCursorPosition.Y -= m_csbiCursor.srWindow.Top;
		
			RECT rectCursor;
			if (!bOnlyCursor) 
				{
					DrawCursorBackground(rectCursor);
				} 
			else
				{
					GetCursorRect(rectCursor);
				}
			m_pCursor->Draw(&rectCursor);
			::InvalidateRect(m_hWnd, &rectCursor, FALSE);
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

inline void Console::DrawCursorBackground(RECT& rectCursor) 
{
	GetCursorRect(rectCursor);
	
	WORD attrBG = 
		m_pScreenBuffer[m_csbiCursor.dwCursorPosition.Y*m_dwColumns 
										+ m_csbiCursor.dwCursorPosition.X].Attributes >> 4;
	
	// here we decide how to paint text over the backgound
	
	if (m_arrConsoleColors[attrBG] == m_crBackground) 
		{
			::SetBkMode(m_hdcConsole, TRANSPARENT);
		} 
	else
		{
			::SetBkMode(m_hdcConsole, OPAQUE);
			::SetBkColor(m_hdcConsole, m_arrConsoleColors[attrBG]);
		}
	
	::SetTextColor(m_hdcConsole, 
								 m_bUseFontColor 
								 ? m_crFontColor
								 : m_arrConsoleColors[m_pScreenBuffer[m_csbiCursor.dwCursorPosition.Y*m_dwColumns + m_csbiCursor.dwCursorPosition.X].Attributes & 0xF]);
	
	if (m_bBitmapBackground) 
		{
			if (m_bRelativeBackground) 
				{
					::BitBlt(
									 m_hdcConsole, 
									 rectCursor.left, 
									 rectCursor.top, 
									 rectCursor.right - rectCursor.left, 
									 rectCursor.bottom - rectCursor.top, 
									 m_hdcBackground, 
									 m_nX+m_nXBorderSize-m_nBackgroundOffsetX + rectCursor.left, 
									 m_nY+m_nCaptionSize+m_nYBorderSize-m_nBackgroundOffsetY
									 + rectCursor.top, 
									 SRCCOPY);

				} 
			else
				{
					::BitBlt(
									 m_hdcConsole, 
									 rectCursor.left, 
									 rectCursor.top, 
									 rectCursor.right - rectCursor.left, 
									 rectCursor.bottom - rectCursor.top, 
									 m_hdcBackground, 
									 rectCursor.left, 
									 rectCursor.top, SRCCOPY);
				}
		} 
	else
		{
			::FillRect(m_hdcConsole, &rectCursor, m_hBkBrush);
		}

	ShadowTextOut(
								m_hdcConsole, 
								rectCursor.left, 
								rectCursor.top, 
								&(m_pScreenBuffer[m_csbiCursor.dwCursorPosition.Y*m_dwColumns
																	+ m_csbiCursor.dwCursorPosition.X].Char.UnicodeChar), 
								1);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

inline void Console::GetCursorRect(RECT& rectCursor) 
{
	
	::ZeroMemory(&rectCursor, sizeof(RECT));
	
	if (m_nCharWidth > 0) 
		{
			// fixed pitch
			
			rectCursor.left = m_csbiCursor.dwCursorPosition.X * m_nCharWidth + m_nInsideBorder;
			rectCursor.top = m_csbiCursor.dwCursorPosition.Y * m_nCharHeight + m_nInsideBorder;
			rectCursor.right = 
				m_csbiCursor.dwCursorPosition.X * m_nCharWidth + m_nCharWidth + m_nInsideBorder;
			rectCursor.bottom = 
				m_csbiCursor.dwCursorPosition.Y * m_nCharHeight + m_nCharHeight + m_nInsideBorder;
			
		} 
	else
		{
			
			// variable pitch, we do a little joggling here :-)
			RECT			rectLine;
			int				nLastCharWidth;
			auto_ptr<TCHAR>	pszLine(new TCHAR[m_csbiCursor.dwCursorPosition.X + 2]);
			::ZeroMemory(pszLine.get(), (m_csbiCursor.dwCursorPosition.X + 2)*sizeof(TCHAR));
			
			for (short i = 0; i <= m_csbiCursor.dwCursorPosition.X; ++i) 
				{ 
					pszLine.get()[i] = 
						m_pScreenBuffer[m_csbiCursor.dwCursorPosition.Y * m_dwColumns + i].Char.UnicodeChar;
				}
			
			rectLine.left	= rectLine.right	= 0;
			rectLine.top	= rectLine.bottom	= m_csbiCursor.dwCursorPosition.Y * m_nCharHeight;
			
			::DrawText(
								 m_hdcConsole, 
								 pszLine.get(),
								 -1,
								 &rectLine,
								 DT_CALCRECT);
			
			if (!::GetCharWidth32(
														m_hdcConsole, 
														pszLine.get()[m_csbiCursor.dwCursorPosition.X], 
														pszLine.get()[m_csbiCursor.dwCursorPosition.X], 
														&nLastCharWidth)) 
				{
					
					return;
				}
			
			rectCursor.left		= rectLine.right - (DWORD)nLastCharWidth + m_nInsideBorder;
			rectCursor.top		= rectLine.top + m_nInsideBorder;
			rectCursor.right	= rectLine.right + m_nInsideBorder;
			rectCursor.bottom	= rectLine.bottom + m_nInsideBorder;
		}
}

/////////////////////////////////////////////////////////////////////////////

/************************/
/* Console::ShadowTextOut */
/**********************/
inline void Console::ShadowTextOut (HDC hdc, 
																		int nXStart, int nYStart, LPCTSTR lpString, int cbString) 
{
	if (m_shadowDistance != 0) 
		{
			COLORREF origColor = ::SetTextColor(hdc, m_shadowColor);
			::TextOut(hdc, nXStart+m_shadowDistance, nYStart+m_shadowDistance, lpString, cbString);
			::SetTextColor(hdc, origColor);
		}
	::TextOut(hdc, nXStart, nYStart, lpString, cbString);
}
