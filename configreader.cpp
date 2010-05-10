
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

namespace
{
	class XmlException 
	{
	public: XmlException(BOOL bRet) : m_bRet(bRet){};
		BOOL m_bRet;
	};
	
	IXMLElement* GetElement(IXMLElementCollection* collection, const TCHAR* elementName)
	{
		IXMLElement* pElement = NULL;
		
		if (!SUCCEEDED(collection->item(CComVariant(elementName), CComVariant(0), 
															 (IDispatch**)&pElement)))
			{ throw new XmlException(FALSE); }
		
		return pElement;
	}
	
	IXMLElementCollection* GetCollection(IXMLElement* element)
	{
		IXMLElementCollection* pColl = NULL;
		
		if (!SUCCEEDED(element->get_children(&pColl))) throw new XmlException(FALSE);
		
		return pColl;
	}

	
	template<typename T>
	void GetAttribute(IXMLElement* element, const TCHAR* attribute, T& outItem)
	{
		throw 12;
	}
	
	template<>
	void GetAttribute<tstring>(IXMLElement* element, const TCHAR* attribute, tstring& outStr)
	{
		USES_CONVERSION;
		CComVariantOut value;
		
		element->getAttribute(CComBSTR(attribute), value.Out());
		if (value.vt == VT_BSTR) { outStr = OLE2T(value.bstrVal); }
	}
	
	template<>
	void GetAttribute<unsigned long>(IXMLElement* element, 
																	 const TCHAR* attribute, unsigned long& outUlong)
	{
		USES_CONVERSION;
		CComVariantOut value;
		
		element->getAttribute(CComBSTR(attribute), value.Out());
		if (value.vt == VT_BSTR) { outUlong = _ttol(OLE2T(value.bstrVal)); }
	}

	template<>
	void GetAttribute<unsigned int>(IXMLElement* element, 
																	const TCHAR* attribute, unsigned int& outUint)
	{
		USES_CONVERSION;
		CComVariantOut value;
		
		element->getAttribute(CComBSTR(attribute), value.Out());
		if (value.vt == VT_BSTR) { outUint = _ttoi(OLE2T(value.bstrVal)); }
	}
	
	template<>
	void GetAttribute<int>(IXMLElement* element, 
												 const TCHAR* attribute, int& outInt)
	{
		USES_CONVERSION;
		CComVariantOut value;
		
		element->getAttribute(CComBSTR(attribute), value.Out());
		if (value.vt == VT_BSTR) { outInt = _ttoi(OLE2T(value.bstrVal)); }
	}
	
	template<>
	void GetAttribute<unsigned char>(IXMLElement* element, 
																	 const TCHAR* attribute, unsigned char& outUchar)
	{
		USES_CONVERSION;
		CComVariantOut value;
		
		element->getAttribute(CComBSTR(attribute), value.Out());
		if (value.vt == VT_BSTR) { outUchar = (unsigned char)_ttoi(OLE2T(value.bstrVal)); }
	}
};

ConfigSettings::ConfigSettings()
{
	_setDefaults();
}

bool ConfigSettings::load()
{
	bool bRet = false;

	/* sanity check. */
	if (m_strConfigFile.empty()) return false;

	COLORREF defColors[] =
		{
			0x000000,
			0x800000,
			0x008000,
			0x808000,
			0x000080,
			0x800080,
			0x008080,
			0xC0C0C0,
			0x808080,
			0xFF0000,
			0x00FF00,
			0xFFFF00,
			0x0000FF,
			0xFF00FF,
			0x00FFFF,
			0xFFFFFF,
		};
	
	memcpy(m_arrConsoleColors, defColors, sizeof(defColors));
	
	
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
																		&pFileStream))) 
			{
				throw XmlException(FALSE);
			}
		
		// create XML document instance
		if (!SUCCEEDED(::CoCreateInstance(
																			CLSID_XMLDocument, 
																			NULL, 
																			CLSCTX_INPROC_SERVER, 
																			IID_IXMLDocument, 
																			(void**)&pConfigDoc))) 
			{
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
		
		GetAttribute(pRootElement, _T("title"), m_strWindowTitle);
		//m_strWindowTitleCurrent = m_strWindowTitle;
		
		GetAttribute(pRootElement, _T("refresh"), m_dwMasterRepaintInt);
		
		GetAttribute(pRootElement, _T("change_refresh"), m_dwChangeRepaintInt);
		if ((int)m_dwChangeRepaintInt < 5) m_dwChangeRepaintInt = 5;
		
		GetAttribute(pRootElement, _T("shell"), m_strShell);
		GetAttribute(pRootElement, _T("editor"), m_strConfigEditor);
		GetAttribute(pRootElement, _T("editor_params"), m_strConfigEditorParams);
		
		pRootElement->get_children(&pColl);
		if (!pColl) throw XmlException(TRUE);
		
		// get font settings
		IXMLElementCollection*	pFontColl = NULL;
		pFontElement = GetElement(pColl, _T("font"));
		
		if (pFontElement)
			{
				pFontColl = GetCollection(pFontElement);
				
				if (pFontColl) 
					{
						IXMLElement* pFontSubelement = GetElement(pFontColl, _T("size"));
						
						if (pFontSubelement) 
							{
								pFontSubelement->get_text(strText.Out());
								if (strText.Length() > 0) m_dwFontSize = _ttoi(OLE2T(strText));
							}
						SAFERELEASE(pFontSubelement);
						
						pFontSubelement = GetElement(pFontColl, _T("italic"));
						if (pFontSubelement) 
							{
								pFontSubelement->get_text(strText.Out());
								m_bItalic = !_tcsicmp(OLE2T(strText), _T("true"));
							}
						SAFERELEASE(pFontSubelement);
				
						pFontSubelement = GetElement(pFontColl, _T("bold"));
						if (pFontSubelement) 
							{
								pFontSubelement->get_text(strText.Out());
								m_bBold = !_tcsicmp(OLE2T(strText), _T("true"));
							}
						SAFERELEASE(pFontSubelement);
						
						pFontSubelement = GetElement(pFontColl, _T("name"));
						if (pFontSubelement) 
							{
								pFontSubelement->get_text(strText.Out());
								if (strText.Length() > 0) m_strFontName = OLE2T(strText);
							}
						SAFERELEASE(pFontSubelement);
						
						pFontSubelement = GetElement(pFontColl, _T("shadow"));
						if (pFontSubelement) 
							{
								BYTE r = 0;
								BYTE g = 0;
								BYTE b = 0;
								
								GetAttribute(pFontSubelement, _T("r"), r);
								GetAttribute(pFontSubelement, _T("g"), g);
								GetAttribute(pFontSubelement, _T("b"), b);
								
								m_shadowColor = RGB(r, g, b);
								
								GetAttribute(pFontSubelement, _T("distance"), m_shadowDistance);
							}
						
						pFontSubelement = GetElement(pFontColl, _T("color"));
						if (pFontSubelement) 
							{
								BYTE r = 0;
								BYTE g = 0;
								BYTE b = 0;
								
								GetAttribute(pFontSubelement, _T("r"), r);
								GetAttribute(pFontSubelement, _T("g"), g);
								GetAttribute(pFontSubelement, _T("b"), b);
								
								m_bUseFontColor = TRUE;
								m_crFontColor = RGB(r, g, b);
							}
						SAFERELEASE(pFontSubelement);

						// get font color mapping
						pFontSubelement = GetElement(pFontColl, _T("colors"));
						if (pFontSubelement) 
							{
								IXMLElementCollection* pColorsColl = NULL;
								pColorsColl = GetCollection(pFontSubelement);
								
								if (pColorsColl) 
									{
										for (int i = 0; i < 16; ++i) 
											{
												IXMLElement* pColorSubelement = NULL;
												TCHAR szColorName[32];
												
												_sntprintf(szColorName,
																	 sizeof(szColorName)/sizeof(TCHAR),
																	 _T("color_%02i"), i);
												
												pColorSubelement = GetElement(pColorsColl, szColorName);
												if (pColorSubelement)
													{
														BYTE r = 0;
														BYTE g = 0;
														BYTE b = 0;
														
														GetAttribute(pColorSubelement, _T("r"), r);
														GetAttribute(pColorSubelement, _T("g"), g);
														GetAttribute(pColorSubelement, _T("b"), b);
														
														m_arrConsoleColors[i] = RGB(r, g, b);
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
		
		pPositionElement = GetElement(pColl, _T("position"));
		if (pPositionElement) 
			{
				pPositionColl = GetCollection(pPositionElement);
				if (pPositionColl) 
					{
						IXMLElement* pPositionSubelement = NULL;
						
						if (!m_bReloading)
							{
								pPositionSubelement = GetElement(pPositionColl, _T("x"));
								
								if (pPositionSubelement) 
									{
										pPositionSubelement->get_text(strText.Out());
										if (strText.Length() > 0) m_nX = _ttoi(OLE2T(strText));
									}
								SAFERELEASE(pPositionSubelement);
								
								pPositionSubelement = GetElement(pPositionColl, _T("y"));
								
								if (pPositionSubelement) 
									{
										pPositionSubelement->get_text(strText.Out());
										if (strText.Length() > 0) m_nY = _ttoi(OLE2T(strText));
									}
								SAFERELEASE(pPositionSubelement);
							}
				
						pPositionSubelement = GetElement(pPositionColl, _T("docked"));
						if (pPositionSubelement) 
							{
								pPositionSubelement->get_text(strText.Out());
								strTempText = OLE2T(strText);
								
								if (!_tcsicmp(strTempText.c_str(), _T("top left"))) 
									{
										m_dwDocked = DOCK_TOP_LEFT;
									} 
								else if (!_tcsicmp(strTempText.c_str(), _T("top right"))) 
									{
										m_dwDocked = DOCK_TOP_RIGHT;
									} 
								else if (!_tcsicmp(strTempText.c_str(), _T("bottom right"))) 
									{
										m_dwDocked = DOCK_BOTTOM_RIGHT;
									} 
								else if (!_tcsicmp(strTempText.c_str(), _T("bottom left"))) 
									{
										m_dwDocked = DOCK_BOTTOM_LEFT;
									} 
								else 
									{
										m_dwDocked = DOCK_NONE;
									}
							}
						SAFERELEASE(pPositionSubelement);
						
						pPositionSubelement = GetElement(pPositionColl, _T("snap_distance"));
						if (pPositionSubelement) 
							{
								pPositionSubelement->get_text(strText.Out());
								if (strText.Length() > 0) m_nSnapDst = _ttoi(OLE2T(strText));
							}
						SAFERELEASE(pPositionSubelement);
						
						pPositionSubelement = GetElement(pPositionColl, _T("z_order"));
						if (pPositionSubelement) 
							{
								pPositionSubelement->get_text(strText.Out());
								strTempText = OLE2T(strText);
								
								if (!_tcsicmp(strTempText.c_str(), _T("regular"))) 
									{
										m_dwCurrentZOrder = Z_ORDER_REGULAR;
										m_dwOriginalZOrder = Z_ORDER_REGULAR;
									} 
								else if (!_tcsicmp(strTempText.c_str(), _T("on top"))) 
									{
										m_dwCurrentZOrder = Z_ORDER_ONTOP;
										m_dwOriginalZOrder = Z_ORDER_ONTOP;
									} 
								else if (!_tcsicmp(strTempText.c_str(), _T("on bottom"))) 
									{
										m_dwCurrentZOrder = Z_ORDER_ONBOTTOM;
										m_dwOriginalZOrder = Z_ORDER_ONBOTTOM;
									} 
								else
									{
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
		
		pAppearanceElement = GetElement(pColl, _T("appearance"));
		if (pAppearanceElement) 
			{
				pAppearanceColl = GetCollection(pAppearanceElement);
				if (pAppearanceColl) 
					{
						IXMLElement* pAppearanaceSubelement = NULL;
						
						pAppearanaceSubelement = GetElement(pAppearanceColl, _T("hide_console"));
						
						if (pAppearanaceSubelement) 
							{
								pAppearanaceSubelement->get_text(strText.Out());
								if (!_tcsicmp(OLE2T(strText), _T("true"))) 
									{
										m_bHideConsole = TRUE;
									} 
								else 
									{
										m_bHideConsole = FALSE;
									}
							}
						SAFERELEASE(pAppearanaceSubelement);
						
						pAppearanaceSubelement = GetElement(pAppearanceColl, _T("hide_console_timeout"));
						if (pAppearanaceSubelement) 
							{
								pAppearanaceSubelement->get_text(strText.Out());
								if (strText.Length() > 0) m_dwHideConsoleTimeout = _ttoi(OLE2T(strText));
							}
						SAFERELEASE(pAppearanaceSubelement);
						
						pAppearanaceSubelement = GetElement(pAppearanceColl, _T("start_minimized"));
						if (pAppearanaceSubelement) 
							{
								pAppearanaceSubelement->get_text(strText.Out());
								if (!_tcsicmp(OLE2T(strText), _T("true"))) 
									{
										m_bStartMinimized = TRUE;
									} 
								else
									{
										m_bStartMinimized = FALSE;
									}
							}
						SAFERELEASE(pAppearanaceSubelement);
						
						IXMLElementCollection*	pScrollbarColl = NULL;
						pScrollbarElement = GetElement(pAppearanceColl, _T("scrollbar"));
						if (pScrollbarElement) 
							{
								pScrollbarColl = GetCollection(pScrollbarElement);
								
								if (pScrollbarColl) 
									{
										IXMLElement* pScrollbarSubelement = NULL;
										
										pScrollbarSubelement = GetElement(pScrollbarColl, _T("color"));
										if (pScrollbarSubelement) 
											{
												BYTE r = 0;
												BYTE g = 0;
												BYTE b = 0;
												
												GetAttribute(pScrollbarSubelement, _T("r"), r);
												GetAttribute(pScrollbarSubelement, _T("g"), g);
												GetAttribute(pScrollbarSubelement, _T("b"), b);
												
												m_crScrollbarColor = RGB(r, g, b);
											}
										SAFERELEASE(pScrollbarSubelement);
										
										pScrollbarSubelement = GetElement(pScrollbarColl, _T("style"));
										if (pScrollbarSubelement) 
											{
												pScrollbarSubelement->get_text(strText.Out());
												strTempText = OLE2T(strText);
												
												if (!_tcsicmp(strTempText.c_str(), _T("regular"))) 
													{
														m_nScrollbarStyle = FSB_REGULAR_MODE;
													} 
												else if (!_tcsicmp(strTempText.c_str(), _T("flat"))) 
													{
														m_nScrollbarStyle = FSB_FLAT_MODE;
													} 
												else if (!_tcsicmp(strTempText.c_str(), _T("encarta"))) 
													{
														m_nScrollbarStyle = FSB_ENCARTA_MODE;
													}
											}
										SAFERELEASE(pScrollbarSubelement);
										
										pScrollbarSubelement = GetElement(pScrollbarColl, _T("width"));
										if (pScrollbarSubelement) 
											{
												pScrollbarSubelement->get_text(strText.Out());
												if (strText.Length() > 0) m_nScrollbarWidth = _ttoi(OLE2T(strText));
											}
										SAFERELEASE(pScrollbarSubelement);
										
										pScrollbarSubelement = GetElement(pScrollbarColl, _T("button_height"));
										if (pScrollbarSubelement) 
											{
												pScrollbarSubelement->get_text(strText.Out());
												if (strText.Length() > 0) m_nScrollbarButtonHeight = _ttoi(OLE2T(strText));
											}
										SAFERELEASE(pScrollbarSubelement);
										
										pScrollbarSubelement = GetElement(pScrollbarColl, _T("thumb_height"));
										if (pScrollbarSubelement) 
											{
												pScrollbarSubelement->get_text(strText.Out());
												if (strText.Length() > 0) m_nScrollbarThunmbHeight = _ttoi(OLE2T(strText));
											}
										SAFERELEASE(pScrollbarSubelement);
									}
								SAFERELEASE(pScrollbarColl);
							}
						
						pAppearanaceSubelement = GetElement(pAppearanceColl, _T("border"));
						if (pAppearanaceSubelement) 
							{
								pAppearanaceSubelement->get_text(strText.Out());
								if (!_tcsicmp(OLE2T(strText), _T("true")) 
										|| !_tcsicmp(OLE2T(strText), _T("regular"))) 
									{
										m_dwWindowBorder = BORDER_REGULAR;
									} 
								else if (!_tcsicmp(OLE2T(strText), _T("thin"))) 
									{
										m_dwWindowBorder = BORDER_THIN;
									} 
								else
									{
										m_dwWindowBorder = BORDER_NONE;
									}
							}
						SAFERELEASE(pAppearanaceSubelement);
						
						pAppearanaceSubelement = GetElement(pAppearanceColl, _T("inside_border"));
						if (pAppearanaceSubelement) 
							{
								pAppearanaceSubelement->get_text(strText.Out());
								if (strText.Length() > 0) m_nInsideBorder = _ttoi(OLE2T(strText));
							}
						SAFERELEASE(pAppearanaceSubelement);
						
						pAppearanaceSubelement = GetElement(pAppearanceColl, _T("taskbar_button"));
						if (pAppearanaceSubelement) 
							{
								pAppearanaceSubelement->get_text(strText.Out());
								if (!_tcsicmp(OLE2T(strText), _T("hide"))) 
									{
										m_dwTaskbarButton = TASKBAR_BUTTON_HIDE;
									} 
								else if (!_tcsicmp(OLE2T(strText), _T("tray"))) 
									{ 
										m_dwTaskbarButton = TASKBAR_BUTTON_TRAY;
									} 
								else
									{
										m_dwTaskbarButton = TASKBAR_BUTTON_NORMAL;
									}
							}
						SAFERELEASE(pAppearanaceSubelement);
						
						pAppearanaceSubelement = GetElement(pAppearanceColl, _T("size"));
						if (pAppearanaceSubelement) 
							{
								GetAttribute(pAppearanaceSubelement, _T("rows"), m_dwRows);
								GetAttribute(pAppearanaceSubelement, _T("columns"), m_dwColumns);
								
								pAppearanaceSubelement->getAttribute(CComBSTR(_T("buffer_rows")), varAttValue.Out());
								if (varAttValue.vt == VT_BSTR)
									{
										m_dwBufferRows = _ttoi(OLE2T(varAttValue.bstrVal));
										m_bUseTextBuffer = TRUE;
									} 
								else
									{
										m_dwBufferRows = m_dwRows;
									}
								if (m_dwBufferRows < m_dwRows) m_dwBufferRows = m_dwRows;
							}
						SAFERELEASE(pAppearanaceSubelement);
						
						pAppearanaceSubelement = GetElement(pAppearanceColl, _T("transparency"));
						if (pAppearanaceSubelement) 
							{
								GetAttribute(pAppearanaceSubelement, _T("alpha"), m_byAlpha);
								GetAttribute(pAppearanaceSubelement, _T("inactive_alpha"), m_byInactiveAlpha);
								
								pAppearanaceSubelement->get_text(strText.Out());
								strTempText = OLE2T(strText);
								
								if (!_tcsicmp(strTempText.c_str(), _T("none"))) 
									{
										m_dwTransparency = TRANSPARENCY_NONE;
									} 
								else if (!_tcsicmp(strTempText.c_str(), _T("alpha"))) 
									{
										m_dwTransparency = TRANSPARENCY_ALPHA;
									} 
								else if (!_tcsicmp(strTempText.c_str(), _T("color key"))) 
									{
										m_dwTransparency = TRANSPARENCY_COLORKEY;
									} 
								else if (!_tcsicmp(strTempText.c_str(), _T("fake"))) 
									{
										m_dwTransparency = TRANSPARENCY_FAKE;
									}
							}
						SAFERELEASE(pAppearanaceSubelement);
						
				// get background settings
				IXMLElementCollection*	pBackgroundColl = NULL;
				
				pBackgroundElement = GetElement(pAppearanceColl, _T("background"));
				if (pBackgroundElement) 
					{
						pBackgroundColl = GetCollection(pBackgroundElement);
						if (pBackgroundColl) 
							{
								IXMLElement* pBackgroundSubelement = NULL;
								
								pBackgroundSubelement = GetElement(pBackgroundColl, _T("color"));
								if (pBackgroundSubelement) 
									{
										BYTE r = 0;
										BYTE g = 0;
										BYTE b = 0;
										
										GetAttribute(pBackgroundSubelement, _T("r"), r);
										GetAttribute(pBackgroundSubelement, _T("g"), g);
										GetAttribute(pBackgroundSubelement, _T("b"), b);
										
										m_crBackground = RGB(r, g, b);
										m_bBkColorSet = TRUE;								
									}
								SAFERELEASE(pBackgroundSubelement);
								
								pBackgroundSubelement = GetElement(pBackgroundColl, _T("tint"));
								if (pBackgroundSubelement) 
									{
										GetAttribute(pBackgroundSubelement, _T("r"), m_byTintR);
										GetAttribute(pBackgroundSubelement, _T("g"), m_byTintG);
										GetAttribute(pBackgroundSubelement, _T("b"), m_byTintB);
										
										GetAttribute(pBackgroundSubelement, _T("opacity"), m_byTintOpacity);
										
										if (m_byTintOpacity > 100) m_byTintOpacity = 50;
										m_bTintSet = TRUE;								
									}
								SAFERELEASE(pBackgroundSubelement);
								
								pBackgroundSubelement = GetElement(pBackgroundColl, _T("image"));
								if (pBackgroundSubelement) 
									{
										pBackgroundSubelement->getAttribute(CComBSTR(_T("style")), varAttValue.Out());
										if (varAttValue.vt == VT_BSTR) 
											{
												if (!_tcsicmp(OLE2T(varAttValue.bstrVal), _T("resize"))) 
													{
														m_dwBackgroundStyle = BACKGROUND_STYLE_RESIZE;
													} 
												else if (!_tcsicmp(OLE2T(varAttValue.bstrVal), _T("center"))) 
													{
														m_dwBackgroundStyle = BACKGROUND_STYLE_CENTER;
													} 
												else if (!_tcsicmp(OLE2T(varAttValue.bstrVal), _T("tile"))) 
													{
														m_dwBackgroundStyle = BACKGROUND_STYLE_TILE;
													}
											}
										
										pBackgroundSubelement->getAttribute(CComBSTR(_T("relative")), varAttValue.Out());
										if (varAttValue.vt == VT_BSTR)
											{
												if (!_tcsicmp(OLE2T(varAttValue.bstrVal), _T("true"))) {
													m_bRelativeBackground = TRUE;
												} 
												else
													{
														m_bRelativeBackground = FALSE;
													}
											}
										
										pBackgroundSubelement->getAttribute(CComBSTR(_T("extend")), varAttValue.Out());
										if (varAttValue.vt == VT_BSTR)
											{
												if (!_tcsicmp(OLE2T(varAttValue.bstrVal), _T("true"))) 
													{
														m_bExtendBackground = TRUE;
													} 
												else
													{
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
				
				pCursorElement = GetElement(pAppearanceColl, _T("cursor"));
				if (pCursorElement) 
					{
						pCursorColl = GetCollection(pCursorElement);
						
						if (pCursorColl) 
							{
								IXMLElement* pCursorSubelement = NULL;
								
								pCursorSubelement = GetElement(pCursorColl, _T("color"));
								if (pCursorSubelement) 
									{
										BYTE r = 0;
										BYTE g = 0;
										BYTE b = 0;
										
										GetAttribute(pCursorSubelement, _T("r"), r);
										GetAttribute(pCursorSubelement, _T("g"), g);
										GetAttribute(pCursorSubelement, _T("b"), b);
										
										m_crCursorColor = RGB(r, g, b);
									}
								SAFERELEASE(pCursorSubelement);
								
								pCursorSubelement = GetElement(pCursorColl, _T("style"));
								if (pCursorSubelement) 
									{
										pCursorSubelement->get_text(strText.Out());
										strTempText = OLE2T(strText);
										
										if (!_tcsicmp(strTempText.c_str(), _T("none"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_NONE;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("XTerm"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_XTERM;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("block"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_BLOCK;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("noblink block"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_NBBLOCK;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("pulse block"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_PULSEBLOCK;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("bar"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_BAR;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("console"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_CONSOLE;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("noblink line"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_NBHLINE;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("horizontal line"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_HLINE;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("vertical line"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_VLINE;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("rect"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_RECT;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("noblink rect"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_NBRECT;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("pulse rect"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_PULSERECT;
											} 
										else if (!_tcsicmp(strTempText.c_str(), _T("fading block"))) 
											{
												m_dwCursorStyle = CURSOR_STYLE_FADEBLOCK;
											}
									}
								SAFERELEASE(pCursorSubelement);
							}
						SAFERELEASE(pCursorColl);
					}
				
				pAppearanaceSubelement = GetElement(pAppearanceColl, _T("icon"));
				if (pAppearanaceSubelement) 
					{
						pAppearanaceSubelement->get_text(strText.Out());
						if (strText.Length() > 0) m_strIconFilename = OLE2T(strText);
					}
				SAFERELEASE(pAppearanaceSubelement);
				
					}
				SAFERELEASE(pAppearanceColl);
			}
		
		// get behaviour settings
		IXMLElementCollection*	pBehaviorColl = NULL;
		
		pBehaviorElement = GetElement(pColl, _T("behaviour"));
		if (! pBehaviorElement)
			{
				pBehaviorElement = GetElement(pColl, _T("behavior"));
			}
		
		if (pBehaviorElement) 
			{
				pBehaviorColl = GetCollection(pBehaviorElement);
				
				if (pBehaviorColl) 
					{
						IXMLElement* pBehaviourSubelement = NULL;
						
						pBehaviourSubelement = GetElement(pBehaviorColl, _T("mouse_drag"));
						if (pBehaviourSubelement) 
							{
								pBehaviourSubelement->get_text(strText.Out());
								if (!_tcsicmp(OLE2T(strText), _T("true"))) 
									{
										m_bMouseDragable = true;
									} 
								else 
									{
										m_bMouseDragable = false;
									}
							}
						SAFERELEASE(pBehaviourSubelement);
						
						pBehaviourSubelement = GetElement(pBehaviorColl, _T("copy_on_select"));
						if (pBehaviourSubelement) 
							{
								pBehaviourSubelement->get_text(strText.Out());
								if (!_tcsicmp(OLE2T(strText), _T("true"))) 
									{
										m_bCopyOnSelect = TRUE;
									} 
								else
									{
										m_bCopyOnSelect = FALSE;
									}
							}
						SAFERELEASE(pBehaviourSubelement);
						
						pBehaviourSubelement = GetElement(pBehaviorColl, _T("inverse_shift"));
						if (pBehaviourSubelement) 
							{
								pBehaviourSubelement->get_text(strText.Out());
								if (!_tcsicmp(OLE2T(strText), _T("true"))) 
									{
										m_bInverseShift = TRUE;
									} 
								else
									{
										m_bInverseShift = FALSE;
									}
							}
						SAFERELEASE(pBehaviourSubelement);
						
						pBehaviourSubelement = GetElement(pBehaviorColl, _T("reload_new_config"));
						if (pBehaviourSubelement) 
							{
								pBehaviourSubelement->get_text(strText.Out());
								if (!_tcsicmp(OLE2T(strText), _T("yes")))
									{
										m_dwReloadNewConfig = RELOAD_NEW_CONFIG_YES;
									} 
								else if (!_tcsicmp(OLE2T(strText), _T("no"))) 
									{
										m_dwReloadNewConfig = RELOAD_NEW_CONFIG_NO;
									} 
								else
									{
										m_dwReloadNewConfig = RELOAD_NEW_CONFIG_PROMPT;
									}
							}
						SAFERELEASE(pBehaviourSubelement);
						
						pBehaviourSubelement = GetElement(pBehaviorColl, _T("disable_menu"));
						if (pBehaviourSubelement) 
							{
								pBehaviourSubelement->get_text(strText.Out());
								if (!_tcsicmp(OLE2T(strText), _T("true"))) 
									{
										m_bPopupMenuDisabled = TRUE;
									} 
								else
									{
										m_bPopupMenuDisabled = FALSE;
									}
							}
						SAFERELEASE(pBehaviourSubelement);
						
					}
				SAFERELEASE(pBehaviorColl);
			}
		
		bRet = TRUE;
		
	} 
	catch (const XmlException& e) 
		{
			bRet = e.m_bRet == TRUE;
		}
	catch (XmlException* e)
		{
			bRet = e->m_bRet == TRUE;
			delete e;
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

void ConfigSettings::_setDefaults()
{
 	// set default values
	
	/* set the shell to comspec or cmd.exe
	 * depending on what we find.
	 */
	m_strShell.clear();
	{
		TCHAR	szComspec[MAX_PATH];
		if (::GetEnvironmentVariable(_T("COMSPEC"), szComspec, MAX_PATH) > 0) 
			{ m_strShell = szComspec; }
		else { m_strShell = _T("cmd.exe"); }
	}
	
	m_strConfigEditor		= _T("notepad.exe");
	m_strConfigEditorParams	= _T("");
	m_dwReloadNewConfig		= RELOAD_NEW_CONFIG_PROMPT;
	m_dwMasterRepaintInt	= 500;
	m_dwChangeRepaintInt	= 50;
	m_strIconFilename		= _T("");
	m_bPopupMenuDisabled	= FALSE;
	m_strWindowTitle		= _T("console");
	m_strFontName			= _T("Lucida Console");
	m_dwFontSize			= 8;
	m_bBold					= FALSE;
	m_bItalic				= FALSE;
	m_bUseFontColor			= FALSE;
	m_crFontColor			= RGB(0, 0, 0);
	m_nX					= 0;
	m_nY					= 0;

	m_dwWindowBorder		= BORDER_NONE;
	m_nScrollbarStyle		= FSB_REGULAR_MODE;
	m_crScrollbarColor		= ::GetSysColor(COLOR_3DHILIGHT);
	m_nScrollbarWidth		= ::GetSystemMetrics(SM_CXVSCROLL);
	m_nScrollbarButtonHeight= ::GetSystemMetrics(SM_CYVSCROLL);
	m_nScrollbarThunmbHeight= ::GetSystemMetrics(SM_CYVTHUMB);

	m_dwTaskbarButton		= TASKBAR_BUTTON_NORMAL;
	m_bMouseDragable		= true;
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
}

/////////////////////////////////////////////////////////////////////////////

void Console::ReloadSettings() 
{
	m_bInitializing = TRUE;
	
	_settings->setReloading(true);
	
	// suspend console monitor thread
	::SuspendThread(m_hMonitorThread);
	::KillTimer(m_hWnd, TIMER_REPAINT_CHANGE);
	if (_settings->masterRepaintInt()) ::KillTimer(m_hWnd, TIMER_REPAINT_MASTER);

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
	if (_settings->scrollbarStyle() != FSB_REGULAR_MODE) ::UninitializeFlatSB(m_hWnd);

	// destory window
	::DestroyWindow(m_hWnd);
	if (m_hwndInvisParent) ::DestroyWindow(m_hwndInvisParent);
	
	//SetDefaultConsoleColors();

	_settings->load();
	m_strWindowTitleCurrent = _settings->windowTitle();

	CreateConsoleWindow();

	SetupMenus();

	CreateNewFont();
	CreateNewBrush();
	
	SetWindowTransparency();
	SetWindowIcons();
	
	CreateCursor();

	RefreshStdOut();
	InitConsoleWndSize(_settings->columns());
	ResizeConsoleWindow();
	ShowHideConsole();
	
	m_bInitializing = FALSE;
	
	_settings->setReloading(false);
	
	RefreshScreenBuffer();
	::CopyMemory(m_pScreenBuffer, m_pScreenBufferNew, 
							 sizeof(CHAR_INFO) * _settings->rows() * _settings->columns());
	RepaintWindow();

	if (_settings->masterRepaintInt())
		{
			::SetTimer(m_hWnd, TIMER_REPAINT_MASTER, _settings->masterRepaintInt(), NULL);
		}
	
	::ResumeThread(m_hMonitorThread);

	if (_settings->startMinimized())
		{
			if (_settings->taskbarButton() > TASKBAR_BUTTON_NORMAL) 
				{
					m_bHideWindow = TRUE;
				} 
			else 
				{
					::ShowWindow(m_hWnd, SW_MINIMIZE);
				}
		} 
	else
		{
			::ShowWindow(m_hWnd, SW_SHOW);
		}
	
	::SetForegroundWindow(m_hWnd);
}

/////////////////////////////////////////////////////////////////////////////

