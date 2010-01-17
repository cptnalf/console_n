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

void Console::SendTextToConsole(LPCTSTR pszText) 
{
	if (!pszText || (_tcslen(pszText) == 0)) return;
	
	HANDLE hStdIn = ::CreateFile(_T("CONIN$"), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	
	DWORD	dwTextLen		= _tcslen(pszText);
	DWORD	dwTextWritten	= 0;
	
	INPUT_RECORD* pKeyEvents = new INPUT_RECORD[dwTextLen];
	::ZeroMemory(pKeyEvents, sizeof(INPUT_RECORD)*dwTextLen);
	
	for (DWORD i = 0; i < dwTextLen; ++i)
		{
			pKeyEvents[i].EventType = KEY_EVENT;
			pKeyEvents[i].Event.KeyEvent.bKeyDown = TRUE;
			pKeyEvents[i].Event.KeyEvent.wRepeatCount = 1;
			pKeyEvents[i].Event.KeyEvent.wVirtualKeyCode = 0;
			pKeyEvents[i].Event.KeyEvent.wVirtualScanCode = 0;
			pKeyEvents[i].Event.KeyEvent.uChar.UnicodeChar = pszText[i];
			pKeyEvents[i].Event.KeyEvent.dwControlKeyState = 0;
		}
	::WriteConsoleInput(hStdIn, pKeyEvents, dwTextLen, &dwTextWritten);
	
	DEL_ARR(pKeyEvents);
	::CloseHandle(hStdIn);
	
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::PasteClipoardText() 
{
	if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return;
	
	
	if (::OpenClipboard(m_hWnd)) 
		{
			HANDLE	hData = ::GetClipboardData(CF_UNICODETEXT);
			
			SendTextToConsole((TCHAR*)::GlobalLock(hData));
			
			::GlobalUnlock(hData);
			::CloseClipboard();
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::CopyTextToClipboard() 
{
	int nSelX = (m_rectSelection.left - _settings->insideBorder())/m_nCharWidth;
	int nSelY = (m_rectSelection.top - _settings->insideBorder())/m_nCharHeight;
	int nSelColumns = (m_rectSelection.right-m_rectSelection.left)/m_nCharWidth;
	int nSelRows = (m_rectSelection.bottom-m_rectSelection.top)/m_nCharHeight;
	
	// nothing selected, just return
	if ((nSelColumns == 0) || (nSelRows == 0)) 
		{
			ClearSelection();
			return;
		}
	
	if (!::OpenClipboard(m_hWnd)) 
		{
			ClearSelection();
			return;
		}
	
	::EmptyClipboard();
	
	HGLOBAL hglbData = ::GlobalAlloc(GMEM_MOVEABLE, (nSelColumns+2)*nSelRows*sizeof(TCHAR)); 
	if (hglbData == NULL)
		{ 
			::CloseClipboard();
			ClearSelection();
			return;
		} 
	
	// lock the handle and copy the text to the buffer. 
	TCHAR* pszData = (TCHAR*)::GlobalLock(hglbData); 
	int i =0;
	for (i = 0; i < nSelRows; ++i)
		{
			for (int j = 0; j < nSelColumns; ++j)
				{
					pszData[i*(nSelColumns+2) + j] = 
						m_pScreenBuffer[(nSelY+i)*_settings->columns() + nSelX + j].Char.UnicodeChar;
				}
			// at the end of each row we put \r\n (except for the last one where we put \0)
			pszData[i*(nSelColumns+2) + nSelColumns] = _TCHAR('\r');
			pszData[i*(nSelColumns+2) + nSelColumns + 1] = _TCHAR('\n');
		}
	pszData[(i-1)*(nSelColumns+2) + nSelColumns] = _TCHAR('\x0');
	
	::GlobalUnlock(hglbData);
	
	if (::SetClipboardData(CF_UNICODETEXT, hglbData) == NULL)
		{
			// we need to global-free data only if copying failed
		}
	::CloseClipboard();
	// !!! No call to GlobalFree here. Next app that uses clipboard will call EmptyClipboard to free the data
	
	ClearSelection();
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::ClearSelection() 
{
	m_nTextSelection = TEXT_SELECTION_NONE;
	m_coordSelOrigin.X = 0;
	m_coordSelOrigin.Y = 0;
	::ZeroMemory(&m_rectSelection, sizeof(RECT));
	
	RepaintWindow();
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


/////////////////////////////////////////////////////////////////////////////

void Console::InitConsoleWndSize(DWORD dwColumns) 
{
	
	if (m_nTextSelection) ClearSelection();
	
	COORD coordConsoleSize;
	coordConsoleSize.X = (SHORT)dwColumns;
	coordConsoleSize.Y = (SHORT)_settings->bufferRows();
	
	SMALL_RECT	srConsoleRect;
	srConsoleRect.Top	= srConsoleRect.Left =0;
	srConsoleRect.Right	= (short)(dwColumns - 1);
	srConsoleRect.Bottom= (short)(_settings->rows() - 1);
	
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	::GetConsoleScreenBufferInfo(m_hStdOutFresh, &csbi);
	
	if ((DWORD) csbi.dwSize.X * csbi.dwSize.Y > (DWORD) dwColumns * _settings->bufferRows()) 
		{
			::SetConsoleWindowInfo(m_hStdOutFresh, TRUE, &srConsoleRect);
			::SetConsoleScreenBufferSize(m_hStdOutFresh, coordConsoleSize);
		} 
	else if (((DWORD)csbi.dwSize.X < dwColumns) 
					 || ((DWORD)csbi.dwSize.Y < _settings->bufferRows()) 
					 || ((DWORD)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1) != _settings->rows())) 
		{
			::SetConsoleScreenBufferSize(m_hStdOutFresh, coordConsoleSize);
			::SetConsoleWindowInfo(m_hStdOutFresh, TRUE, &srConsoleRect);
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::RefreshStdOut() 
{
	if (m_hStdOutFresh) ::CloseHandle(m_hStdOutFresh);
	m_hStdOutFresh = ::CreateFile(_T("CONOUT$"), 
																GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 
																NULL, OPEN_EXISTING, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::RefreshScreenBuffer() 
{
	// if we're initializing, do nothing
	if (m_bInitializing) return;
	
	::GetConsoleScreenBufferInfo(m_hStdOutFresh, &m_csbiConsole);

	SCROLLINFO si;
	si.cbSize = sizeof(si); 
	si.fMask  = SIF_POS; 
	si.nPos   = (int)m_csbiConsole.srWindow.Top;
	::FlatSB_SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);
	
	if ((m_csbiConsole.srWindow.Right - m_csbiConsole.srWindow.Left + 1 != _settings->columns()) 
			|| (m_csbiConsole.srWindow.Bottom - m_csbiConsole.srWindow.Top + 1 != _settings->rows()) 
			|| (m_csbiConsole.dwSize.Y != _settings->bufferRows())) 
		{
			_settings->setColumns(m_csbiConsole.srWindow.Right - m_csbiConsole.srWindow.Left + 1);
			_settings->setRows(m_csbiConsole.srWindow.Bottom - m_csbiConsole.srWindow.Top + 1);
			ResizeConsoleWindow();
		}
	
	COORD		coordBufferSize;
	COORD		coordStart;
	SMALL_RECT	srRegion;
	
	coordStart.X		= 0;
	coordStart.Y		= 0;
	
	coordBufferSize.X	= (short)_settings->columns();
	coordBufferSize.Y	= (short)_settings->rows();
	
	srRegion.Top		= m_csbiConsole.srWindow.Top;
	srRegion.Left		= 0;
	srRegion.Bottom		= (short)(m_csbiConsole.srWindow.Top + _settings->rows() - 1);
	srRegion.Right		= (short)(_settings->columns() - 1);

	DEL_ARR(m_pScreenBufferNew);
	m_pScreenBufferNew = new CHAR_INFO[_settings->rows() * _settings->columns()];
	
	::ReadConsoleOutput(m_hStdOutFresh, m_pScreenBufferNew, coordBufferSize, coordStart, &srRegion);
	
	// set console window title
	TCHAR szWinConsoleTitle[MAX_PATH+1];
	::GetConsoleTitle(szWinConsoleTitle, MAX_PATH);

	tstring strWinConsoleTitle(szWinConsoleTitle);
	tstring strConsoleTitle(_T(""));

	// Here we decide about updating Console window title.
	// There are 2 possibilities:

	if (m_strWinConsoleTitle.compare(0, m_strWinConsoleTitle.length(), 
																	 strWinConsoleTitle, 0, m_strWinConsoleTitle.length()) == 0) 
		{
			// 1. Windows console title starts with the original title, just see if
			//    windows titles differ, and if they do, update it.
			if ((_settings->windowTitle().empty())  &&
					(strWinConsoleTitle[m_strWinConsoleTitle.length()] == ' ') &&
			(strWinConsoleTitle[m_strWinConsoleTitle.length()+1] == '-')) 
				{
					strConsoleTitle = strWinConsoleTitle.substr(m_strWinConsoleTitle.length()+3);
				} 
			else 
				{
					strConsoleTitle = 
						_settings->windowTitle() + strWinConsoleTitle.substr(m_strWinConsoleTitle.length());
				}
			
		} 
	else 
		{
			// 2. Windows console title is completely changed. To set Console title, 
			//    we need to get Windows console title and concatenate it to our 
			//    original Console title (if it changed since the last update)
			if (_settings->windowTitle().empty()) 
				{
					strConsoleTitle = strWinConsoleTitle;
				} 
			else 
				{
					strConsoleTitle = _settings->windowTitle() + tstring(_T(" - ")) + strWinConsoleTitle;
				}
		}
	
	if (m_strWindowTitleCurrent.compare(strConsoleTitle) != 0) 
		{
			m_strWindowTitleCurrent = strConsoleTitle;
			::SetWindowText(m_hWnd, m_strWindowTitleCurrent.c_str());
		}
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::SetScrollbarStuff()
{
	
	if (_settings->useTextBuffer())
		{
			m_bShowScrollbar = _settings->bufferRows() > _settings->rows();
		} 
	else 
		{
			m_bShowScrollbar = FALSE;
		}
	
	// we don't call InitializeFlatSB due to some problems on Win2k, Windowblinds and new-style scrollbars
	if (_settings->scrollbarStyle() != FSB_REGULAR_MODE) ::InitializeFlatSB(m_hWnd);
	::FlatSB_ShowScrollBar(m_hWnd, SB_VERT, m_bShowScrollbar);
	::FlatSB_SetScrollRange(m_hWnd, SB_VERT, 0, _settings->bufferRows() - _settings->rows(), FALSE);
	
	// set scrollbar properties
	::FlatSB_SetScrollProp(m_hWnd, WSB_PROP_VSTYLE, _settings->scrollbarStyle(), FALSE);
	::FlatSB_SetScrollProp(m_hWnd, WSB_PROP_VBKGCOLOR, _settings->scrollbarColor(), FALSE);
	::FlatSB_SetScrollProp(m_hWnd, WSB_PROP_CXVSCROLL , _settings->scrollbarWidth(), FALSE);
	::FlatSB_SetScrollProp(m_hWnd, WSB_PROP_CYVSCROLL, _settings->scrollbarButtonHeight(), FALSE);
	::FlatSB_SetScrollProp(m_hWnd, WSB_PROP_CYVTHUMB, _settings->scrollbarThunmbHeight(), TRUE);
}

/////////////////////////////////////////////////////////////////////////////

