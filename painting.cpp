#include "stdafx.h"
#include <atlbase.h>
#include <shellapi.h>
#include <commctrl.h>
#include <memory>

#include "resource.h"
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
	if (!_settings->useTextBuffer()) _settings->setBufferRows(_settings->rows());
	
	COORD		coordBuffersSize;
	coordBuffersSize.X = (SHORT)_settings->columns();
	coordBuffersSize.Y = (SHORT)_settings->bufferRows();
	
	SMALL_RECT	srConsoleRect;
	srConsoleRect.Top	= srConsoleRect.Left =0;
	srConsoleRect.Right	= (short)(_settings->columns() - 1);
	srConsoleRect.Bottom= (short)(_settings->rows() - 1);
	
	// order of setting window size and screen buffer size depends on current and desired dimensions
	if (((DWORD)csbi.dwSize.X * csbi.dwSize.Y) > ((DWORD)_settings->columns() * _settings->bufferRows()))
		{
			if (_settings->useTextBuffer() 
					&& (csbi.dwSize.Y > (short)_settings->bufferRows())) 
				{
					_settings->setBufferRows(coordBuffersSize.Y = csbi.dwSize.Y);
				}
			
			::SetConsoleWindowInfo(m_hStdOutFresh, TRUE, &srConsoleRect);
			::SetConsoleScreenBufferSize(m_hStdOutFresh, coordBuffersSize);
			
			//	} else if (((DWORD)csbi.dwSize.X < m_dwColumns) || ((DWORD)csbi.dwSize.Y < m_dwBufferRows) || ((DWORD)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1) != m_dwRows)) {
		}
	else if (((DWORD)csbi.dwSize.X * csbi.dwSize.Y) < ((DWORD)_settings->columns() *_settings->bufferRows())) 
		{
			if (csbi.dwSize.Y < (short)_settings->bufferRows()) 
				{
					_settings->setBufferRows(csbi.dwSize.Y);
					coordBuffersSize.Y = csbi.dwSize.Y;
				}
			
			::SetConsoleScreenBufferSize(m_hStdOutFresh, coordBuffersSize);
			::SetConsoleWindowInfo(m_hStdOutFresh, TRUE, &srConsoleRect);
		}
	
	SetScrollbarStuff();
	CalcWindowSize();
	SetWindowSizeAndPosition();
	CreateOffscreenBuffers();
	
	if (_settings->bitmapBackground()) CreateBackgroundBitmap();
	
	// resize screen buffer
	DEL_ARR(m_pScreenBuffer);
	m_pScreenBuffer = new CHAR_INFO[_settings->rows() * _settings->columns()];
	
	m_bInitializing = FALSE;
}

/////////////////////////////////////////////////////////////////////////////

void Console::_repaintBG(RECT* rect)
{
	if (_settings->bitmapBackground()) 
		{
			/* blit the background. */
			if (_settings->relativeBackground()) 
				{
					::BitBlt(m_hdcConsole, 
									 rect->left, rect->top, 
									 rect->right - rect->left, rect->bottom - rect->top,
									 m_hdcBackground, 
									 _settings->getX() + m_nXBorderSize - m_nBackgroundOffsetX + rect->left, 
									 _settings->getY() + m_nCaptionSize + m_nYBorderSize - m_nBackgroundOffsetY + rect->top, 
									 SRCCOPY);
				} 
			else
				{
					::BitBlt(m_hdcConsole,
									 rect->left, rect->top, 
									 rect->right - rect->left, rect->bottom - rect->top,
									 m_hdcBackground, 
									 rect->left, rect->top, 
									 SRCCOPY);
				}
		} 
	else
		{
			/* dump fill the background color */
			::FillRect(m_hdcConsole, rect, m_hBkBrush);
		}
}

/////////////////////////////////////////////////////////////////////////////

void Console::RepaintWindow() 
{
	RECT rect;
	rect.top	= 0;
	rect.left	= 0;
	rect.bottom	= m_nClientHeight;
	rect.right	= m_nClientWidth;

	_repaintBG( &rect);
	
	DWORD dwX			= _settings->insideBorder();
	DWORD dwY			= _settings->insideBorder();
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
			for (DWORD i = 0; i < _settings->rows(); ++i) 
				{
					dwX = _settings->insideBorder();
					dwY = i * m_nCharHeight + _settings->insideBorder();
					
					nBkMode			= TRANSPARENT;
					crBkColor		= RGB(0, 0, 0);
					crTxtColor		= RGB(0, 0, 0);
			
					bTextOut		= false;
			
					attrBG = m_pScreenBuffer[dwOffset].Attributes >> 4;
			
					// here we decide how to paint text over the backgound
					if (_settings->consoleColor(attrBG) == _settings->background()) 
						{
							::SetBkMode(m_hdcConsole, TRANSPARENT);
							nBkMode		= TRANSPARENT;
						} 
					else
						{
							::SetBkMode(m_hdcConsole, OPAQUE);
							nBkMode		= OPAQUE;
							::SetBkColor(m_hdcConsole, _settings->consoleColor(attrBG));
							crBkColor	= _settings->consoleColor(attrBG);
						}
					
					/*duplication detected... */
					::SetTextColor(m_hdcConsole, 
												 _settings->useFontColor()
												 ? _settings->fontColor()
												 : _settings->consoleColor(m_pScreenBuffer[dwOffset].Attributes & 0xF));
					crTxtColor		= 
						(_settings->useFontColor() 
						 ? _settings->fontColor()
						 : _settings->consoleColor(m_pScreenBuffer[dwOffset].Attributes & 0xF)
						 );
						 
					strText = m_pScreenBuffer[dwOffset].Char.UnicodeChar;
					++dwOffset;
					
					for (DWORD j = 1; j < _settings->columns(); ++j) 
						{
							attrBG = m_pScreenBuffer[dwOffset].Attributes >> 4;
							
							if (_settings->consoleColor(attrBG) == _settings->background()) 
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
									if (crBkColor != _settings->consoleColor(attrBG)) 
										{
											crBkColor = _settings->consoleColor(attrBG);
											bTextOut = true;
										}
								}
							
							/* duplication detected! */
							if (crTxtColor != (_settings->useFontColor()
																 ? _settings->fontColor()
																 : _settings->consoleColor(m_pScreenBuffer[dwOffset].Attributes & 0xF))) 
								{
									crTxtColor = _settings->useFontColor()
										? _settings->fontColor()
										: _settings->consoleColor(m_pScreenBuffer[dwOffset].Attributes & 0xF);
									bTextOut = true;
								}
					
							if (bTextOut)
								{
									/* so, the settings changed, print what we've got
									 * the setup the new settings.
									 */
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
									/* otherwise just queue it. */
									strText += m_pScreenBuffer[dwOffset].Char.UnicodeChar;
								}
					
							++dwOffset;
						}
					
					/* if we've got any chars left, print them out. */
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
						 sizeof(CHAR_INFO)*_settings->rows() * _settings->columns());
		
			for (DWORD i = 0; i < _settings->rows(); ++i)
				{
					dwX = _settings->insideBorder();
					dwY = i * m_nCharHeight + _settings->insideBorder();
					
					for (DWORD j = 0; j < _settings->columns(); ++j) 
						{
							attrBG = m_pScreenBuffer[dwOffset].Attributes >> 4;
							
							// here we decide how to paint text over the backgound
							if (_settings->consoleColor(attrBG) == _settings->background()) 
								{
									::SetBkMode(m_hdcConsole, TRANSPARENT);
								} 
							else
								{
									::SetBkMode(m_hdcConsole, OPAQUE);
									::SetBkColor(m_hdcConsole, _settings->consoleColor(attrBG));
								}
				
							::SetTextColor(m_hdcConsole, 
														 _settings->useFontColor()
														 ? _settings->fontColor()
														 : _settings->consoleColor(m_pScreenBuffer[dwOffset].Attributes & 0xF));
							ShadowTextOut(m_hdcConsole, dwX, dwY, &(m_pScreenBuffer[dwOffset].Char.UnicodeChar), 1);
							int nWidth;
							::GetCharWidth32(m_hdcConsole, m_pScreenBuffer[dwOffset].Char.UnicodeChar,
															 m_pScreenBuffer[dwOffset].Char.UnicodeChar, &nWidth);
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
	DWORD dwX			= _settings->insideBorder();
	DWORD dwY			= _settings->insideBorder();
	DWORD dwOffset		= 0;
	
	WORD attrBG;

	if (m_nCharWidth > 0) 
		{
			// fixed pitch font
			for (DWORD i = 0; i < _settings->rows(); ++i) 
				{
			
					dwX = _settings->insideBorder();
					dwY = i*m_nCharHeight + _settings->insideBorder();

					for (DWORD j = 0; j < _settings->columns(); ++j) 
						{

							if (memcmp(&(m_pScreenBuffer[dwOffset]), 
												 &(m_pScreenBufferNew[dwOffset]), sizeof(CHAR_INFO))) 
								{
									memcpy(&(m_pScreenBuffer[dwOffset]), 
												 &(m_pScreenBufferNew[dwOffset]), sizeof(CHAR_INFO));

									RECT rect;
									rect.top	= dwY;
									rect.left	= dwX;
									rect.bottom	= dwY + m_nCharHeight;
									rect.right	= dwX + m_nCharWidth;
									
									_repaintBG(&rect);
									
									attrBG = m_pScreenBuffer[dwOffset].Attributes >> 4;
									
									// here we decide how to paint text over the backgound
									if (_settings->consoleColor(attrBG) == _settings->background()) 
										{
											::SetBkMode(m_hdcConsole, TRANSPARENT);
										} 
									else
										{
											::SetBkMode(m_hdcConsole, OPAQUE);
											::SetBkColor(m_hdcConsole, _settings->consoleColor(attrBG));
										}
					
									::SetTextColor(m_hdcConsole, 
																 _settings->useFontColor()
																 ? _settings->fontColor()
																 : _settings->consoleColor(m_pScreenBuffer[dwOffset].Attributes & 0xF));
									ShadowTextOut(m_hdcConsole, dwX, dwY, 
																&(m_pScreenBuffer[dwOffset].Char.UnicodeChar), 1);
									
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
								 sizeof(CHAR_INFO)*_settings->rows() * _settings->columns())) 
				{
					memcpy(m_pScreenBuffer, m_pScreenBufferNew, 
								 sizeof(CHAR_INFO)*_settings->rows() * _settings->columns());
				
					RECT rect;
					rect.top	= 0;
					rect.left	= 0;
					rect.bottom	= m_nClientHeight;
					rect.right	= m_nClientWidth;

					_repaintBG(&rect);
		
					for (DWORD i = 0; i < _settings->rows(); ++i) 
						{
							dwX = _settings->insideBorder();
							dwY = i*m_nCharHeight + _settings->insideBorder();
			
							for (DWORD j = 0; j < _settings->columns(); ++j) 
								{
				
									attrBG = m_pScreenBuffer[dwOffset].Attributes >> 4;
				
									// here we decide how to paint text over the backgound
									if (_settings->consoleColor(attrBG) == _settings->background()) 
										{
											::SetBkMode(m_hdcConsole, TRANSPARENT);
										} 
									else
										{
											::SetBkMode(m_hdcConsole, OPAQUE);
											::SetBkColor(m_hdcConsole, _settings->consoleColor(attrBG));
										}
				
									::SetTextColor(m_hdcConsole, 
																 _settings->useFontColor()
																 ? _settings->fontColor()
																 : _settings->consoleColor(m_pScreenBuffer[dwOffset].Attributes & 0xF));
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
	if (!bOnlyCursor && _settings->cursorVisible()) 
{
		RECT rectCursorOld;
		DrawCursorBackground(rectCursorOld);
		::InvalidateRect(m_hWnd, &rectCursorOld, FALSE);
	}
	
	// now, see if the cursor is visible...
	CONSOLE_CURSOR_INFO	cinf;
	::GetConsoleCursorInfo(m_hStdOutFresh, &cinf);
	
	_settings->setCursorVisible(cinf.bVisible == TRUE);
	
	// ... and draw it
	if (_settings->cursorVisible()) 
		{
		
			::GetConsoleScreenBufferInfo(m_hStdOutFresh, &m_csbiCursor);
		
			if (m_csbiCursor.dwCursorPosition.Y < m_csbiCursor.srWindow.Top 
					|| m_csbiCursor.dwCursorPosition.Y > m_csbiCursor.srWindow.Bottom) 
				{
					_settings->setCursorVisible(false);
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
		m_pScreenBuffer[m_csbiCursor.dwCursorPosition.Y * _settings->columns() 
										+ m_csbiCursor.dwCursorPosition.X].Attributes >> 4;
	
	// here we decide how to paint text over the backgound
	
	if (_settings->consoleColor(attrBG) == _settings->background()) 
		{
			::SetBkMode(m_hdcConsole, TRANSPARENT);
		} 
	else
		{
			::SetBkMode(m_hdcConsole, OPAQUE);
			::SetBkColor(m_hdcConsole, _settings->consoleColor(attrBG));
		}
	
	::SetTextColor(m_hdcConsole, 
								 _settings->useFontColor()
								 ? _settings->fontColor()
								 : _settings->consoleColor(m_pScreenBuffer[m_csbiCursor.dwCursorPosition.Y * _settings->columns() + m_csbiCursor.dwCursorPosition.X].Attributes & 0xF));
	
	_repaintBG(&rectCursor);

	ShadowTextOut(
								m_hdcConsole, 
								rectCursor.left, 
								rectCursor.top, 
								&(m_pScreenBuffer[m_csbiCursor.dwCursorPosition.Y * _settings->columns()
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
			
			rectCursor.left = m_csbiCursor.dwCursorPosition.X * m_nCharWidth + _settings->insideBorder();
			rectCursor.top = m_csbiCursor.dwCursorPosition.Y * m_nCharHeight + _settings->insideBorder();
			rectCursor.right = 
				m_csbiCursor.dwCursorPosition.X * m_nCharWidth + m_nCharWidth + _settings->insideBorder();
			rectCursor.bottom = 
				m_csbiCursor.dwCursorPosition.Y * m_nCharHeight + m_nCharHeight + _settings->insideBorder();
			
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
						m_pScreenBuffer[m_csbiCursor.dwCursorPosition.Y * _settings->columns() + i].Char.UnicodeChar;
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
			
			rectCursor.left		= rectLine.right - (DWORD)nLastCharWidth + _settings->insideBorder();
			rectCursor.top		= rectLine.top + _settings->insideBorder();
			rectCursor.right	= rectLine.right + _settings->insideBorder();
			rectCursor.bottom	= rectLine.bottom + _settings->insideBorder();
		}
}

/////////////////////////////////////////////////////////////////////////////

/************************/
/* Console::ShadowTextOut */
/**********************/
inline void Console::ShadowTextOut (HDC hdc, 
																		int nXStart, int nYStart, LPCTSTR lpString, int cbString) 
{
	if (_settings->shadowDistance() != 0) 
		{
			COLORREF origColor = ::SetTextColor(hdc, _settings->shadowColor());
			::TextOut(hdc,
								nXStart + _settings->shadowDistance(),
								nYStart + _settings->shadowDistance(), lpString, cbString);
			::SetTextColor(hdc, origColor);
		}
	::TextOut(hdc, nXStart, nYStart, lpString, cbString);
}
