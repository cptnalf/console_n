
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

BOOL Console::GetOptions()
{
	class XmlException {
	public: XmlException(BOOL bRet) : m_bRet(bRet){};
		BOOL m_bRet;
	};
	
	BOOL bRet = FALSE;
	
	IStream*				pFileStream			= NULL;
	IXMLDocument*			pConfigDoc			= NULL;
	IPersistStreamInit*		pPersistStream		= NULL;
	IXMLElement*			pRootElement		= NULL;
	IXMLElementCollection*	pColl				= NULL;
	IXMLElement*			pFontElement		= NULL;
	IXMLElement*			pPositionElement	= NULL;
	IXMLElement*			pAppearanceElement	= NULL;
	IXMLElement*			pScrollbarElement	= NULL;
	IXMLElement*			pBackgroundElement	= NULL;
	IXMLElement*			pCursorElement		= NULL;
	IXMLElement*			pBehaviorElement	= NULL;
	
	try {
		::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		
		USES_CONVERSION;
		
		// open file stream
		if (!SUCCEEDED(CreateFileStream(
			m_strConfigFile.c_str(), 
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			0, 
			NULL,
			&pFileStream))) {
			
			throw XmlException(FALSE);
		}
		
		// create XML document instance
		if (!SUCCEEDED(::CoCreateInstance(
			CLSID_XMLDocument, 
			NULL, 
			CLSCTX_INPROC_SERVER, 
			IID_IXMLDocument, 
			(void**)&pConfigDoc))) {
			
			throw XmlException(FALSE);
		}
		
		// load the configuration file
		pConfigDoc->QueryInterface(IID_IPersistStreamInit, (void **)&pPersistStream);
		
		if (!SUCCEEDED(pPersistStream->Load(pFileStream))) throw XmlException(FALSE);
		
		// see if we're dealing with the skin
		if (!SUCCEEDED(pConfigDoc->get_root(&pRootElement))) throw XmlException(FALSE);
		
		CComVariantOut	varAttValue;
		CComBSTROut		bstr;
		CComBSTROut		strText;
		tstring			strTempText(_T(""));
		
		// root element must be CONSOLE
		pRootElement->get_tagName(bstr.Out());
		bstr.ToUpper();
		
		if (!(bstr == CComBSTR(_T("CONSOLE")))) throw XmlException(FALSE);

		pRootElement->getAttribute(CComBSTR(_T("title")), varAttValue.Out());
		if (varAttValue.vt == VT_BSTR) {
			m_strWindowTitle = OLE2T(varAttValue.bstrVal);
			m_strWindowTitleCurrent = m_strWindowTitle;
		}
		
		pRootElement->getAttribute(CComBSTR(_T("refresh")), varAttValue.Out());
		if (varAttValue.vt == VT_BSTR) m_dwMasterRepaintInt = _ttol(OLE2T(varAttValue.bstrVal));
		
		pRootElement->getAttribute(CComBSTR(_T("change_refresh")), varAttValue.Out());
		if (varAttValue.vt == VT_BSTR) m_dwChangeRepaintInt = _ttol(OLE2T(varAttValue.bstrVal));
		if ((int)m_dwChangeRepaintInt < 5) m_dwChangeRepaintInt = 5;
		
		pRootElement->getAttribute(CComBSTR(_T("shell")), varAttValue.Out());
		if (varAttValue.vt == VT_BSTR) m_strShell = OLE2T(varAttValue.bstrVal);
		
		pRootElement->getAttribute(CComBSTR(_T("editor")), varAttValue.Out());
		if (varAttValue.vt == VT_BSTR) m_strConfigEditor = OLE2T(varAttValue.bstrVal);

		pRootElement->getAttribute(CComBSTR(_T("editor_params")), varAttValue.Out());
		if (varAttValue.vt == VT_BSTR) m_strConfigEditorParams = OLE2T(varAttValue.bstrVal);
		
		pRootElement->get_children(&pColl);
		if (!pColl) throw XmlException(TRUE);

		// get font settings
		IXMLElementCollection*	pFontColl = NULL;
		if (!SUCCEEDED(pColl->item(CComVariant(_T("font")), CComVariant(0), (IDispatch**)&pFontElement))) throw XmlException(FALSE);
		if (pFontElement) {
			if (!SUCCEEDED(pFontElement->get_children(&pFontColl))) throw XmlException(FALSE);

			if (pFontColl) {
				IXMLElement* pFontSubelement = NULL;
				
				if (!SUCCEEDED(pFontColl->item(CComVariant(_T("size")), CComVariant(0), (IDispatch**)&pFontSubelement))) throw XmlException(FALSE);
				if (pFontSubelement) {
					pFontSubelement->get_text(strText.Out());
					if (strText.Length() > 0) m_dwFontSize = _ttoi(OLE2T(strText));
				}
				SAFERELEASE(pFontSubelement);

				if (!SUCCEEDED(pFontColl->item(CComVariant(_T("italic")), CComVariant(0), (IDispatch**)&pFontSubelement))) throw XmlException(FALSE);
				if (pFontSubelement) {
					pFontSubelement->get_text(strText.Out());
					m_bItalic = !_tcsicmp(OLE2T(strText), _T("true"));
				}
				SAFERELEASE(pFontSubelement);
				
				if (!SUCCEEDED(pFontColl->item(CComVariant(_T("bold")), CComVariant(0), (IDispatch**)&pFontSubelement))) throw XmlException(FALSE);
				if (pFontSubelement) {
					pFontSubelement->get_text(strText.Out());
					m_bBold = !_tcsicmp(OLE2T(strText), _T("true"));
				}
				SAFERELEASE(pFontSubelement);
				
				if (!SUCCEEDED(pFontColl->item(CComVariant(_T("name")), CComVariant(0), (IDispatch**)&pFontSubelement))) throw XmlException(FALSE);
				if (pFontSubelement) {
					pFontSubelement->get_text(strText.Out());
					if (strText.Length() > 0) m_strFontName = OLE2T(strText);
				}
				SAFERELEASE(pFontSubelement);
				
				if (!SUCCEEDED(pFontColl->item(CComVariant(_T("shadow")), CComVariant(0), (IDispatch**)&pFontSubelement))) throw XmlException(FALSE);
				if (pFontSubelement) {
					BYTE r = 0;
					BYTE g = 0;
					BYTE b = 0;

					varAttValue.Clear();
					pFontSubelement->getAttribute(CComBSTR(_T("r")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) r = _ttoi(OLE2T(varAttValue.bstrVal));
					varAttValue.Clear();
					pFontSubelement->getAttribute(CComBSTR(_T("g")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) g = _ttoi(OLE2T(varAttValue.bstrVal));
					varAttValue.Clear();
					pFontSubelement->getAttribute(CComBSTR(_T("b")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) b = _ttoi(OLE2T(varAttValue.bstrVal));
					varAttValue.Clear();
					
					m_shadowColor = RGB(r, g, b);

					pFontSubelement->getAttribute(CComBSTR(_T("distance")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) m_shadowDistance = _ttoi(OLE2T(varAttValue.bstrVal));
				}


				
				if (!SUCCEEDED(pFontColl->item(CComVariant(_T("color")), CComVariant(0), (IDispatch**)&pFontSubelement))) throw XmlException(FALSE);
				if (pFontSubelement) {
					BYTE r = 0;
					BYTE g = 0;
					BYTE b = 0;
					
					varAttValue.Clear();
					pFontSubelement->getAttribute(CComBSTR(_T("r")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) r = _ttoi(OLE2T(varAttValue.bstrVal));
					varAttValue.Clear();
					pFontSubelement->getAttribute(CComBSTR(_T("g")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) g = _ttoi(OLE2T(varAttValue.bstrVal));
					varAttValue.Clear();
					pFontSubelement->getAttribute(CComBSTR(_T("b")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) b = _ttoi(OLE2T(varAttValue.bstrVal));
					
					m_bUseFontColor = TRUE;
					m_crFontColor = RGB(r, g, b);
				}
				SAFERELEASE(pFontSubelement);

				// get font color mapping
				if (!SUCCEEDED(pFontColl->item(CComVariant(_T("colors")), CComVariant(0), (IDispatch**)&pFontSubelement))) throw XmlException(FALSE);
				if (pFontSubelement) {

					IXMLElementCollection*	pColorsColl = NULL;
					
					if (!SUCCEEDED(pFontSubelement->get_children(&pColorsColl))) throw XmlException(FALSE);
					
					if (pColorsColl) {

						for (int i = 0; i < 16; ++i) {
							IXMLElement* pColorSubelement = NULL;
							TCHAR szColorName[32];

							_sntprintf(szColorName, sizeof(szColorName)/sizeof(TCHAR), _T("color_%02i"), i);
							
							if (!SUCCEEDED(pColorsColl->item(CComVariant(szColorName), CComVariant(0), (IDispatch**)&pColorSubelement))) throw XmlException(FALSE);
							if (pColorSubelement) {

								BYTE r = 0;
								BYTE g = 0;
								BYTE b = 0;
								
								varAttValue.Clear();
								pColorSubelement->getAttribute(CComBSTR(_T("r")), varAttValue.Out());
								if (varAttValue.vt == VT_BSTR) r = _ttoi(OLE2T(varAttValue.bstrVal));
								varAttValue.Clear();
								pColorSubelement->getAttribute(CComBSTR(_T("g")), varAttValue.Out());
								if (varAttValue.vt == VT_BSTR) g = _ttoi(OLE2T(varAttValue.bstrVal));
								varAttValue.Clear();
								pColorSubelement->getAttribute(CComBSTR(_T("b")), varAttValue.Out());
								if (varAttValue.vt == VT_BSTR) b = _ttoi(OLE2T(varAttValue.bstrVal));
								
								Console::m_arrConsoleColors[i] = RGB(r, g, b);
							}

							SAFERELEASE(pColorSubelement);
						}
					}
					SAFERELEASE(pColorsColl);
				}
				SAFERELEASE(pFontSubelement);
			}				
			SAFERELEASE(pFontColl);
		}
	
		// get position settings
		IXMLElementCollection*	pPositionColl = NULL;
		if (!SUCCEEDED(pColl->item(CComVariant(_T("position")), CComVariant(0), (IDispatch**)&pPositionElement))) throw XmlException(FALSE);
		if (pPositionElement) {
			if (!SUCCEEDED(pPositionElement->get_children(&pPositionColl))) throw XmlException(FALSE);
			
			if (pPositionColl) {
				IXMLElement* pPositionSubelement = NULL;
				
				if (!m_bReloading) {
					if (!SUCCEEDED(pPositionColl->item(CComVariant(_T("x")), CComVariant(0), (IDispatch**)&pPositionSubelement))) throw XmlException(FALSE);
					if (pPositionSubelement) {
						pPositionSubelement->get_text(strText.Out());
						if (strText.Length() > 0) m_nX = _ttoi(OLE2T(strText));
					}
					SAFERELEASE(pPositionSubelement);
					
					if (!SUCCEEDED(pPositionColl->item(CComVariant(_T("y")), CComVariant(0), (IDispatch**)&pPositionSubelement))) throw XmlException(FALSE);
					if (pPositionSubelement) {
						pPositionSubelement->get_text(strText.Out());
						if (strText.Length() > 0) m_nY = _ttoi(OLE2T(strText));
					}
					SAFERELEASE(pPositionSubelement);
				}
				
				if (!SUCCEEDED(pPositionColl->item(CComVariant(_T("docked")), CComVariant(0), (IDispatch**)&pPositionSubelement))) throw XmlException(FALSE);
				if (pPositionSubelement) {
					pPositionSubelement->get_text(strText.Out());
					strTempText = OLE2T(strText);
					
					if (!_tcsicmp(strTempText.c_str(), _T("top left"))) {
						m_dwDocked = DOCK_TOP_LEFT;
					} else if (!_tcsicmp(strTempText.c_str(), _T("top right"))) {
						m_dwDocked = DOCK_TOP_RIGHT;
					} else if (!_tcsicmp(strTempText.c_str(), _T("bottom right"))) {
						m_dwDocked = DOCK_BOTTOM_RIGHT;
					} else if (!_tcsicmp(strTempText.c_str(), _T("bottom left"))) {
						m_dwDocked = DOCK_BOTTOM_LEFT;
					} else {
						m_dwDocked = DOCK_NONE;
					}
				}
				SAFERELEASE(pPositionSubelement);
				
				if (!SUCCEEDED(pPositionColl->item(CComVariant(_T("snap_distance")), CComVariant(0), (IDispatch**)&pPositionSubelement))) throw XmlException(FALSE);
				if (pPositionSubelement) {
					pPositionSubelement->get_text(strText.Out());
					if (strText.Length() > 0) m_nSnapDst = _ttoi(OLE2T(strText));
				}
				SAFERELEASE(pPositionSubelement);
				
				if (!SUCCEEDED(pPositionColl->item(CComVariant(_T("z_order")), CComVariant(0), (IDispatch**)&pPositionSubelement))) throw XmlException(FALSE);
				if (pPositionSubelement) {
					pPositionSubelement->get_text(strText.Out());
					strTempText = OLE2T(strText);
					
					if (!_tcsicmp(strTempText.c_str(), _T("regular"))) {
						m_dwCurrentZOrder = Z_ORDER_REGULAR;
						m_dwOriginalZOrder = Z_ORDER_REGULAR;
					} else if (!_tcsicmp(strTempText.c_str(), _T("on top"))) {
						m_dwCurrentZOrder = Z_ORDER_ONTOP;
						m_dwOriginalZOrder = Z_ORDER_ONTOP;
					} else if (!_tcsicmp(strTempText.c_str(), _T("on bottom"))) {
						m_dwCurrentZOrder = Z_ORDER_ONBOTTOM;
						m_dwOriginalZOrder = Z_ORDER_ONBOTTOM;
					} else {
						m_dwCurrentZOrder = Z_ORDER_REGULAR;
						m_dwOriginalZOrder = Z_ORDER_REGULAR;
					}
				}
				SAFERELEASE(pPositionSubelement);
			}
			
			SAFERELEASE(pPositionColl);
		}

		// get appearance settings
		IXMLElementCollection*	pAppearanceColl = NULL;
		if (!SUCCEEDED(pColl->item(CComVariant(_T("appearance")), CComVariant(0), (IDispatch**)&pAppearanceElement))) throw XmlException(FALSE);
		if (pAppearanceElement) {
			if (!SUCCEEDED(pAppearanceElement->get_children(&pAppearanceColl))) throw XmlException(FALSE);
			
			if (pAppearanceColl) {
				IXMLElement* pAppearanaceSubelement = NULL;

				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("hide_console")), CComVariant(0), (IDispatch**)&pAppearanaceSubelement))) throw XmlException(FALSE);
				if (pAppearanaceSubelement) {
					pAppearanaceSubelement->get_text(strText.Out());
					if (!_tcsicmp(OLE2T(strText), _T("true"))) {
						m_bHideConsole = TRUE;
					} else {
						m_bHideConsole = FALSE;
					}
				}
				SAFERELEASE(pAppearanaceSubelement);

				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("hide_console_timeout")), CComVariant(0), (IDispatch**)&pAppearanaceSubelement))) throw XmlException(FALSE);
				if (pAppearanaceSubelement) {
					pAppearanaceSubelement->get_text(strText.Out());
					if (strText.Length() > 0) m_dwHideConsoleTimeout = _ttoi(OLE2T(strText));
				}
				SAFERELEASE(pAppearanaceSubelement);

				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("start_minimized")), CComVariant(0), (IDispatch**)&pAppearanaceSubelement))) throw XmlException(FALSE);
				if (pAppearanaceSubelement) {
					pAppearanaceSubelement->get_text(strText.Out());
					if (!_tcsicmp(OLE2T(strText), _T("true"))) {
						m_bStartMinimized = TRUE;
					} else {
						m_bStartMinimized = FALSE;
					}
				}
				SAFERELEASE(pAppearanaceSubelement);

				IXMLElementCollection*	pScrollbarColl = NULL;
				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("scrollbar")), CComVariant(0), (IDispatch**)&pScrollbarElement))) throw XmlException(FALSE);
				if (pScrollbarElement) {
					if (!SUCCEEDED(pScrollbarElement->get_children(&pScrollbarColl))) throw XmlException(FALSE);
					
					if (pScrollbarColl) {
						IXMLElement* pScrollbarSubelement = NULL;
						
						if (!SUCCEEDED(pScrollbarColl->item(CComVariant(_T("color")), CComVariant(0), (IDispatch**)&pScrollbarSubelement))) throw XmlException(FALSE);
						if (pScrollbarSubelement) {
							BYTE r = 0;
							BYTE g = 0;
							BYTE b = 0;
							
							pScrollbarSubelement->getAttribute(CComBSTR(_T("r")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) r = _ttoi(OLE2T(varAttValue.bstrVal));
							pScrollbarSubelement->getAttribute(CComBSTR(_T("g")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) g = _ttoi(OLE2T(varAttValue.bstrVal));
							pScrollbarSubelement->getAttribute(CComBSTR(_T("b")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) b = _ttoi(OLE2T(varAttValue.bstrVal));
							
							m_crScrollbarColor = RGB(r, g, b);
						}
						SAFERELEASE(pScrollbarSubelement);
						
						if (!SUCCEEDED(pScrollbarColl->item(CComVariant(_T("style")), CComVariant(0), (IDispatch**)&pScrollbarSubelement))) throw XmlException(FALSE);
						if (pScrollbarSubelement) {
							pScrollbarSubelement->get_text(strText.Out());
							strTempText = OLE2T(strText);
							
							if (!_tcsicmp(strTempText.c_str(), _T("regular"))) {
								m_nScrollbarStyle = FSB_REGULAR_MODE;
							} else if (!_tcsicmp(strTempText.c_str(), _T("flat"))) {
								m_nScrollbarStyle = FSB_FLAT_MODE;
							} else if (!_tcsicmp(strTempText.c_str(), _T("encarta"))) {
								m_nScrollbarStyle = FSB_ENCARTA_MODE;
							}
						}
						SAFERELEASE(pScrollbarSubelement);


						if (!SUCCEEDED(pScrollbarColl->item(CComVariant(_T("width")), CComVariant(0), (IDispatch**)&pScrollbarSubelement))) throw XmlException(FALSE);
						if (pScrollbarSubelement) {
							pScrollbarSubelement->get_text(strText.Out());
							if (strText.Length() > 0) m_nScrollbarWidth = _ttoi(OLE2T(strText));
						}
						SAFERELEASE(pScrollbarSubelement);
						
						if (!SUCCEEDED(pScrollbarColl->item(CComVariant(_T("button_height")), CComVariant(0), (IDispatch**)&pScrollbarSubelement))) throw XmlException(FALSE);
						if (pScrollbarSubelement) {
							pScrollbarSubelement->get_text(strText.Out());
							if (strText.Length() > 0) m_nScrollbarButtonHeight = _ttoi(OLE2T(strText));
						}
						SAFERELEASE(pScrollbarSubelement);

						if (!SUCCEEDED(pScrollbarColl->item(CComVariant(_T("thumb_height")), CComVariant(0), (IDispatch**)&pScrollbarSubelement))) throw XmlException(FALSE);
						if (pScrollbarSubelement) {
							pScrollbarSubelement->get_text(strText.Out());
							if (strText.Length() > 0) m_nScrollbarThunmbHeight = _ttoi(OLE2T(strText));
						}
						SAFERELEASE(pScrollbarSubelement);
					}
					SAFERELEASE(pScrollbarColl);
				}

				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("border")), CComVariant(0), (IDispatch**)&pAppearanaceSubelement))) throw XmlException(FALSE);
				if (pAppearanaceSubelement) {
					pAppearanaceSubelement->get_text(strText.Out());
					if (!_tcsicmp(OLE2T(strText), _T("true")) || !_tcsicmp(OLE2T(strText), _T("regular"))) {
						m_dwWindowBorder = BORDER_REGULAR;
					} else if (!_tcsicmp(OLE2T(strText), _T("thin"))) {
						m_dwWindowBorder = BORDER_THIN;
					} else {
						m_dwWindowBorder = BORDER_NONE;
					}
				}
				SAFERELEASE(pAppearanaceSubelement);

				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("inside_border")), CComVariant(0), (IDispatch**)&pAppearanaceSubelement))) throw XmlException(FALSE);
				if (pAppearanaceSubelement) {
					pAppearanaceSubelement->get_text(strText.Out());
					if (strText.Length() > 0) m_nInsideBorder = _ttoi(OLE2T(strText));
				}
				SAFERELEASE(pAppearanaceSubelement);
				
				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("taskbar_button")), CComVariant(0), (IDispatch**)&pAppearanaceSubelement))) throw XmlException(FALSE);
				if (pAppearanaceSubelement) {
					pAppearanaceSubelement->get_text(strText.Out());
					if (!_tcsicmp(OLE2T(strText), _T("hide"))) {
						m_dwTaskbarButton = TASKBAR_BUTTON_HIDE;
					} else if (!_tcsicmp(OLE2T(strText), _T("tray"))) { 
						m_dwTaskbarButton = TASKBAR_BUTTON_TRAY;
					} else {
						m_dwTaskbarButton = TASKBAR_BUTTON_NORMAL;
					}
				}
				SAFERELEASE(pAppearanaceSubelement);

				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("size")), CComVariant(0), (IDispatch**)&pAppearanaceSubelement))) throw XmlException(FALSE);
				if (pAppearanaceSubelement) {
					pAppearanaceSubelement->getAttribute(CComBSTR(_T("rows")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) m_dwRows = _ttoi(OLE2T(varAttValue.bstrVal));
					pAppearanaceSubelement->getAttribute(CComBSTR(_T("columns")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) m_dwColumns = _ttoi(OLE2T(varAttValue.bstrVal));
					pAppearanaceSubelement->getAttribute(CComBSTR(_T("buffer_rows")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) {
						m_dwBufferRows = _ttoi(OLE2T(varAttValue.bstrVal));
						m_bUseTextBuffer = TRUE;
					} else {
						m_dwBufferRows = m_dwRows;
					}
					if (m_dwBufferRows < m_dwRows) m_dwBufferRows = m_dwRows;
				}
				SAFERELEASE(pAppearanaceSubelement);
				
				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("transparency")), CComVariant(0), (IDispatch**)&pAppearanaceSubelement))) throw XmlException(FALSE);
				if (pAppearanaceSubelement) {
					pAppearanaceSubelement->getAttribute(CComBSTR(_T("alpha")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) m_byAlpha = (BYTE)_ttoi(OLE2T(varAttValue.bstrVal));
					pAppearanaceSubelement->getAttribute(CComBSTR(_T("inactive_alpha")), varAttValue.Out());
					if (varAttValue.vt == VT_BSTR) m_byInactiveAlpha = (BYTE)_ttoi(OLE2T(varAttValue.bstrVal));

					pAppearanaceSubelement->get_text(strText.Out());
					strTempText = OLE2T(strText);
					
					if (!_tcsicmp(strTempText.c_str(), _T("none"))) {
						m_dwTransparency = TRANSPARENCY_NONE;
					} else if (!_tcsicmp(strTempText.c_str(), _T("alpha"))) {
						m_dwTransparency = TRANSPARENCY_ALPHA;
					} else if (!_tcsicmp(strTempText.c_str(), _T("color key"))) {
						m_dwTransparency = TRANSPARENCY_COLORKEY;
					} else if (!_tcsicmp(strTempText.c_str(), _T("fake"))) {
						m_dwTransparency = TRANSPARENCY_FAKE;
					}
					
				}
				SAFERELEASE(pAppearanaceSubelement);
				
				// get background settings
				IXMLElementCollection*	pBackgroundColl = NULL;
				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("background")), CComVariant(0), (IDispatch**)&pBackgroundElement))) throw XmlException(FALSE);
				if (pBackgroundElement) {
					if (!SUCCEEDED(pBackgroundElement->get_children(&pBackgroundColl))) throw XmlException(FALSE);
					
					if (pBackgroundColl) {
						IXMLElement* pBackgroundSubelement = NULL;
						
						if (!SUCCEEDED(pBackgroundColl->item(CComVariant(_T("color")), CComVariant(0), (IDispatch**)&pBackgroundSubelement))) throw XmlException(FALSE);
						if (pBackgroundSubelement) {
							BYTE r = 0;
							BYTE g = 0;
							BYTE b = 0;
							
							pBackgroundSubelement->getAttribute(CComBSTR(_T("r")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) r = _ttoi(OLE2T(varAttValue.bstrVal));
							pBackgroundSubelement->getAttribute(CComBSTR(_T("g")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) g = _ttoi(OLE2T(varAttValue.bstrVal));
							pBackgroundSubelement->getAttribute(CComBSTR(_T("b")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) b = _ttoi(OLE2T(varAttValue.bstrVal));
							
							m_crBackground = RGB(r, g, b);
							m_bBkColorSet = TRUE;								
						}
						SAFERELEASE(pBackgroundSubelement);

						if (!SUCCEEDED(pBackgroundColl->item(CComVariant(_T("tint")), CComVariant(0), (IDispatch**)&pBackgroundSubelement))) throw XmlException(FALSE);
						if (pBackgroundSubelement) {
							BYTE r = 0;
							BYTE g = 0;
							BYTE b = 0;
							
							pBackgroundSubelement->getAttribute(CComBSTR(_T("r")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) m_byTintR = _ttoi(OLE2T(varAttValue.bstrVal));
							pBackgroundSubelement->getAttribute(CComBSTR(_T("g")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) m_byTintG = _ttoi(OLE2T(varAttValue.bstrVal));
							pBackgroundSubelement->getAttribute(CComBSTR(_T("b")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) m_byTintB = _ttoi(OLE2T(varAttValue.bstrVal));
							pBackgroundSubelement->getAttribute(CComBSTR(_T("opacity")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) m_byTintOpacity = _ttoi(OLE2T(varAttValue.bstrVal));
							
							if (m_byTintOpacity > 100) m_byTintOpacity = 50;
							m_bTintSet = TRUE;								
						}
						SAFERELEASE(pBackgroundSubelement);
						
						if (!SUCCEEDED(pBackgroundColl->item(CComVariant(_T("image")), CComVariant(0), (IDispatch**)&pBackgroundSubelement))) throw XmlException(FALSE);
						if (pBackgroundSubelement) {

							pBackgroundSubelement->getAttribute(CComBSTR(_T("style")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) {
								if (!_tcsicmp(OLE2T(varAttValue.bstrVal), _T("resize"))) {
									m_dwBackgroundStyle = BACKGROUND_STYLE_RESIZE;
								} else if (!_tcsicmp(OLE2T(varAttValue.bstrVal), _T("center"))) {
									m_dwBackgroundStyle = BACKGROUND_STYLE_CENTER;
								} else if (!_tcsicmp(OLE2T(varAttValue.bstrVal), _T("tile"))) {
									m_dwBackgroundStyle = BACKGROUND_STYLE_TILE;
								}
							}

							pBackgroundSubelement->getAttribute(CComBSTR(_T("relative")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) {
								if (!_tcsicmp(OLE2T(varAttValue.bstrVal), _T("true"))) {
									m_bRelativeBackground = TRUE;
								} else {
									m_bRelativeBackground = FALSE;
								}
							}

							pBackgroundSubelement->getAttribute(CComBSTR(_T("extend")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) {
								if (!_tcsicmp(OLE2T(varAttValue.bstrVal), _T("true"))) {
									m_bExtendBackground = TRUE;
								} else {
									m_bExtendBackground = FALSE;
								}
							}
							
							pBackgroundSubelement->get_text(strText.Out());
							m_strBackgroundFile = OLE2T(strText);
							m_bBitmapBackground = TRUE;
						}
						SAFERELEASE(pBackgroundSubelement);
					}
					SAFERELEASE(pBackgroundColl);
				}
				
				IXMLElementCollection*	pCursorColl = NULL;
				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("cursor")), CComVariant(0), (IDispatch**)&pCursorElement))) throw XmlException(FALSE);
				if (pCursorElement) {
					if (!SUCCEEDED(pCursorElement->get_children(&pCursorColl))) throw XmlException(FALSE);
					
					if (pCursorColl) {
						IXMLElement* pCursorSubelement = NULL;

						if (!SUCCEEDED(pCursorColl->item(CComVariant(_T("color")), CComVariant(0), (IDispatch**)&pCursorSubelement))) throw XmlException(FALSE);
						if (pCursorSubelement) {
							BYTE r = 0;
							BYTE g = 0;
							BYTE b = 0;
							
							pCursorSubelement->getAttribute(CComBSTR(_T("r")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) r = _ttoi(OLE2T(varAttValue.bstrVal));
							pCursorSubelement->getAttribute(CComBSTR(_T("g")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) g = _ttoi(OLE2T(varAttValue.bstrVal));
							pCursorSubelement->getAttribute(CComBSTR(_T("b")), varAttValue.Out());
							if (varAttValue.vt == VT_BSTR) b = _ttoi(OLE2T(varAttValue.bstrVal));
							
							m_crCursorColor = RGB(r, g, b);
						}
						SAFERELEASE(pCursorSubelement);
						
						if (!SUCCEEDED(pCursorColl->item(CComVariant(_T("style")), CComVariant(0), (IDispatch**)&pCursorSubelement))) throw XmlException(FALSE);
						if (pCursorSubelement) {
							pCursorSubelement->get_text(strText.Out());
							strTempText = OLE2T(strText);
							
							if (!_tcsicmp(strTempText.c_str(), _T("none"))) {
								m_dwCursorStyle = CURSOR_STYLE_NONE;
							} else if (!_tcsicmp(strTempText.c_str(), _T("XTerm"))) {
								m_dwCursorStyle = CURSOR_STYLE_XTERM;
							} else if (!_tcsicmp(strTempText.c_str(), _T("block"))) {
								m_dwCursorStyle = CURSOR_STYLE_BLOCK;
							} else if (!_tcsicmp(strTempText.c_str(), _T("noblink block"))) {
								m_dwCursorStyle = CURSOR_STYLE_NBBLOCK;
							} else if (!_tcsicmp(strTempText.c_str(), _T("pulse block"))) {
								m_dwCursorStyle = CURSOR_STYLE_PULSEBLOCK;
							} else if (!_tcsicmp(strTempText.c_str(), _T("bar"))) {
								m_dwCursorStyle = CURSOR_STYLE_BAR;
							} else if (!_tcsicmp(strTempText.c_str(), _T("console"))) {
								m_dwCursorStyle = CURSOR_STYLE_CONSOLE;
							} else if (!_tcsicmp(strTempText.c_str(), _T("noblink line"))) {
								m_dwCursorStyle = CURSOR_STYLE_NBHLINE;
							} else if (!_tcsicmp(strTempText.c_str(), _T("horizontal line"))) {
								m_dwCursorStyle = CURSOR_STYLE_HLINE;
							} else if (!_tcsicmp(strTempText.c_str(), _T("vertical line"))) {
								m_dwCursorStyle = CURSOR_STYLE_VLINE;
							} else if (!_tcsicmp(strTempText.c_str(), _T("rect"))) {
								m_dwCursorStyle = CURSOR_STYLE_RECT;
							} else if (!_tcsicmp(strTempText.c_str(), _T("noblink rect"))) {
								m_dwCursorStyle = CURSOR_STYLE_NBRECT;
							} else if (!_tcsicmp(strTempText.c_str(), _T("pulse rect"))) {
								m_dwCursorStyle = CURSOR_STYLE_PULSERECT;
							} else if (!_tcsicmp(strTempText.c_str(), _T("fading block"))) {
								m_dwCursorStyle = CURSOR_STYLE_FADEBLOCK;
							}
						}
						SAFERELEASE(pCursorSubelement);
					}
					SAFERELEASE(pCursorColl);
				}
				
				if (!SUCCEEDED(pAppearanceColl->item(CComVariant(_T("icon")), CComVariant(0), (IDispatch**)&pAppearanaceSubelement))) throw XmlException(FALSE);
				if (pAppearanaceSubelement) {
					pAppearanaceSubelement->get_text(strText.Out());
					if (strText.Length() > 0) m_strIconFilename = OLE2T(strText);
				}
				SAFERELEASE(pAppearanaceSubelement);
				
			}
			SAFERELEASE(pAppearanceColl);
		}

		// get behaviour settings
		IXMLElementCollection*	pBehaviorColl = NULL;
		if (!SUCCEEDED(pColl->item(CComVariant(_T("behaviour")), CComVariant(0), (IDispatch**)&pBehaviorElement))) {
			if (!SUCCEEDED(pColl->item(CComVariant(_T("behavior")), CComVariant(0), (IDispatch**)&pBehaviorElement))) throw XmlException(FALSE);
		}

		if (pBehaviorElement) {
			if (!SUCCEEDED(pBehaviorElement->get_children(&pBehaviorColl))) throw XmlException(FALSE);
			
			if (pBehaviorColl) {
				IXMLElement* pBehaviourSubelement = NULL;
				
				if (!SUCCEEDED(pBehaviorColl->item(CComVariant(_T("mouse_drag")), CComVariant(0), (IDispatch**)&pBehaviourSubelement))) throw XmlException(FALSE);
				if (pBehaviourSubelement) {
					pBehaviourSubelement->get_text(strText.Out());
					if (!_tcsicmp(OLE2T(strText), _T("true"))) {
						m_bMouseDragable = TRUE;
					} else {
						m_bMouseDragable = FALSE;
					}
				}
				SAFERELEASE(pBehaviourSubelement);
				
				if (!SUCCEEDED(pBehaviorColl->item(CComVariant(_T("copy_on_select")), CComVariant(0), (IDispatch**)&pBehaviourSubelement))) throw XmlException(FALSE);
				if (pBehaviourSubelement) {
					pBehaviourSubelement->get_text(strText.Out());
					if (!_tcsicmp(OLE2T(strText), _T("true"))) {
						m_bCopyOnSelect = TRUE;
					} else {
						m_bCopyOnSelect = FALSE;
					}
				}
				SAFERELEASE(pBehaviourSubelement);
				
				if (!SUCCEEDED(pBehaviorColl->item(CComVariant(_T("inverse_shift")), CComVariant(0), (IDispatch**)&pBehaviourSubelement))) throw XmlException(FALSE);
				if (pBehaviourSubelement) {
					pBehaviourSubelement->get_text(strText.Out());
					if (!_tcsicmp(OLE2T(strText), _T("true"))) {
						m_bInverseShift = TRUE;
					} else {
						m_bInverseShift = FALSE;
					}
				}
				SAFERELEASE(pBehaviourSubelement);
				
				if (!SUCCEEDED(pBehaviorColl->item(CComVariant(_T("reload_new_config")), CComVariant(0), (IDispatch**)&pBehaviourSubelement))) throw XmlException(FALSE);
				if (pBehaviourSubelement) {
					pBehaviourSubelement->get_text(strText.Out());
					if (!_tcsicmp(OLE2T(strText), _T("yes"))) {
						m_dwReloadNewConfig = RELOAD_NEW_CONFIG_YES;
					} else if (!_tcsicmp(OLE2T(strText), _T("no"))) {
						m_dwReloadNewConfig = RELOAD_NEW_CONFIG_NO;
					} else {
						m_dwReloadNewConfig = RELOAD_NEW_CONFIG_PROMPT;
					}
				}
				SAFERELEASE(pBehaviourSubelement);

				if (!SUCCEEDED(pBehaviorColl->item(CComVariant(_T("disable_menu")), CComVariant(0), (IDispatch**)&pBehaviourSubelement))) throw XmlException(FALSE);
				if (pBehaviourSubelement) {
					pBehaviourSubelement->get_text(strText.Out());
					if (!_tcsicmp(OLE2T(strText), _T("true"))) {
						m_bPopupMenuDisabled = TRUE;
					} else {
						m_bPopupMenuDisabled = FALSE;
					}
				}
				SAFERELEASE(pBehaviourSubelement);
				
			}
			SAFERELEASE(pBehaviorColl);
		}
		
		bRet = TRUE;

	} catch (const XmlException& e) {
		bRet = e.m_bRet;
	}

	SAFERELEASE(pBehaviorElement);
	SAFERELEASE(pCursorElement);
	SAFERELEASE(pBackgroundElement);
	SAFERELEASE(pScrollbarElement);
	SAFERELEASE(pAppearanceElement);
	SAFERELEASE(pPositionElement);
	SAFERELEASE(pFontElement);
	SAFERELEASE(pColl);
	SAFERELEASE(pRootElement);
	SAFERELEASE(pPersistStream);
	SAFERELEASE(pConfigDoc);
	SAFERELEASE(pFileStream);

	::CoUninitialize();
	return bRet;
}


/////////////////////////////////////////////////////////////////////////////

void Console::EditConfigFile() {

	// prepare editor parameters
	tstring strParams(m_strConfigEditorParams);

	if (strParams.length() == 0) {
		// no params, just use the config file
		strParams = m_strConfigFile;
	} else {
		int nPos = strParams.find(_T("%f"));

		if (nPos == tstring::npos) {
			// no '%f' in editor params, concatenate config file name
			strParams += _T(" ");
			strParams += m_strConfigFile;
		} else {
			// replace '%f' with config file name
			strParams = strParams.replace(nPos, 2, m_strConfigFile);
		}
	}

	// start editor
	::ShellExecute(NULL, NULL, m_strConfigEditor.c_str(), strParams.c_str(), NULL, SW_SHOWNORMAL);
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

void Console::ReloadSettings() {

	m_bInitializing = TRUE;
	m_bReloading	= TRUE;

	// suspend console monitor thread
	::SuspendThread(m_hMonitorThread);
	::KillTimer(m_hWnd, TIMER_REPAINT_CHANGE);
	if (m_dwMasterRepaintInt) ::KillTimer(m_hWnd, TIMER_REPAINT_MASTER);

	// hide Windows console
	::ShowWindow(m_hWndConsole, SW_HIDE);

	// destroy icons
	SetTrayIcon(NIM_DELETE);
	if (m_hSmallIcon) ::DestroyIcon(m_hSmallIcon);
	if (m_hBigIcon) ::DestroyIcon(m_hBigIcon);

	// destroy cursor
	DestroyCursor();

	// destory menus
	::DestroyMenu(m_hConfigFilesMenu);
	::DestroyMenu(m_hPopupMenu);
	
	// uninitialize flat scrollbars
	if (m_nScrollbarStyle != FSB_REGULAR_MODE) ::UninitializeFlatSB(m_hWnd);

	// destory window
	::DestroyWindow(m_hWnd);
	if (m_hwndInvisParent) ::DestroyWindow(m_hwndInvisParent);
	
	// set default values
	m_strShell				= _T("");
	m_strConfigEditor		= _T("notepad.exe");
	m_strConfigEditorParams	= _T("");
	m_dwReloadNewConfig		= m_dwReloadNewConfigDefault;
	m_hwndInvisParent		= NULL;
	m_dwMasterRepaintInt	= 500;
	m_dwChangeRepaintInt	= 50;
	m_strIconFilename		= _T("");
	m_hSmallIcon			= NULL;
	m_hBigIcon				= NULL;
	m_hPopupMenu			= NULL;
	m_hConfigFilesMenu		= NULL;
	m_bPopupMenuDisabled	= FALSE;
	m_strWindowTitle		= m_strWindowTitleDefault;
	m_strWindowTitleCurrent	= m_strWindowTitleDefault;
	m_strFontName			= _T("Lucida Console");
	m_dwFontSize			= 8;
	m_bBold					= FALSE;
	m_bItalic				= FALSE;
	m_bUseFontColor			= FALSE;
	m_crFontColor			= RGB(0, 0, 0);
//	m_nX					= 0;
//	m_nY					= 0;
	m_nWindowWidth			= 0;
	m_nWindowHeight			= 0;
	m_nXBorderSize			= 0;
	m_nYBorderSize			= 0;
	m_nCaptionSize			= 0;
	m_nClientWidth			= 0;
	m_nClientHeight			= 0;
	m_nCharHeight			= 0;
	m_nCharWidth			= 0;
	m_dwWindowBorder		= BORDER_NONE;
	m_bShowScrollbar		= FALSE;
	m_nScrollbarStyle		= FSB_REGULAR_MODE;
	m_crScrollbarColor		= ::GetSysColor(COLOR_3DHILIGHT);
	m_nScrollbarWidth		= ::GetSystemMetrics(SM_CXVSCROLL);
	m_nScrollbarButtonHeight= ::GetSystemMetrics(SM_CYVSCROLL);
	m_nScrollbarThunmbHeight= ::GetSystemMetrics(SM_CYVTHUMB);
	m_dwTaskbarButton		= TASKBAR_BUTTON_NORMAL;
	m_bMouseDragable		= TRUE;
	m_nSnapDst				= 10;
	m_dwDocked				= DOCK_NONE;
	m_dwOriginalZOrder		= Z_ORDER_REGULAR;
	m_dwCurrentZOrder		= Z_ORDER_REGULAR;
	m_dwTransparency		= TRANSPARENCY_NONE;
	m_byAlpha				= 150;
	m_byInactiveAlpha		= 150;
	m_bBkColorSet			= FALSE;
	m_crBackground			= RGB(0, 0, 0);
	m_bTintSet				= FALSE;
	m_byTintOpacity			= 50;
	m_byTintR				= 0;
	m_byTintG				= 0;
	m_byTintB				= 0;
	m_bBitmapBackground		= FALSE;
	m_strBackgroundFile		= _T("");
	m_dwBackgroundStyle		= BACKGROUND_STYLE_RESIZE;
	m_bRelativeBackground	= FALSE;
	m_bExtendBackground		= FALSE;
	m_bHideWindow			= FALSE;
	m_bHideConsole			= TRUE;
	m_dwCursorStyle			= CURSOR_STYLE_CONSOLE;
	m_bStartMinimized		= FALSE;
	m_crCursorColor			= RGB(255, 255, 255);
	m_bCursorVisible		= FALSE;
	m_dwRows				= 25;
	m_dwColumns				= 80;
	m_dwBufferRows			= 25;
	m_bUseTextBuffer		= FALSE;
	m_bCopyOnSelect			= FALSE;

	SetDefaultConsoleColors();

	GetOptions();

	CreateConsoleWindow();

	SetupMenus();

	CreateNewFont();
	CreateNewBrush();
	
	SetWindowTransparency();
	SetWindowIcons();
	
	CreateCursor();

	RefreshStdOut();
	InitConsoleWndSize(m_dwColumns);
	ResizeConsoleWindow();
	ShowHideConsole();
	
	m_bInitializing = FALSE;
	m_bReloading	= FALSE;

	RefreshScreenBuffer();
	::CopyMemory(m_pScreenBuffer, m_pScreenBufferNew, sizeof(CHAR_INFO) * m_dwRows * m_dwColumns);
	RepaintWindow();

	if (m_dwMasterRepaintInt) ::SetTimer(m_hWnd, TIMER_REPAINT_MASTER, m_dwMasterRepaintInt, NULL);
	
	::ResumeThread(m_hMonitorThread);

	if (m_bStartMinimized) {
		if (m_dwTaskbarButton > TASKBAR_BUTTON_NORMAL) {
			m_bHideWindow = TRUE;
		} else {
			::ShowWindow(m_hWnd, SW_MINIMIZE);
		}
	} else {
		::ShowWindow(m_hWnd, SW_SHOW);
	}
	
	::SetForegroundWindow(m_hWnd);
}

/////////////////////////////////////////////////////////////////////////////

