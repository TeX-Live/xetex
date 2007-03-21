/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2007 by SIL International
 written by Jonathan Kew

Permission is hereby granted, free of charge, to any person obtaining  
a copy of this software and associated documentation files (the  
"Software"), to deal in the Software without restriction, including  
without limitation the rights to use, copy, modify, merge, publish,  
distribute, sublicense, and/or sell copies of the Software, and to  
permit persons to whom the Software is furnished to do so, subject to  
the following conditions:

The above copyright notice and this permission notice shall be  
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,  
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF  
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND  
NONINFRINGEMENT. IN NO EVENT SHALL SIL INTERNATIONAL BE LIABLE FOR  
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION  
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of SIL International  
shall not be used in advertising or otherwise to promote the sale,  
use or other dealings in this Software without prior written  
authorization from SIL International.
\****************************************************************************/

#ifndef __XETEX_GR_LAYOUT__
#define __XETEX_GR_LAYOUT__

#include "XeTeXFontInst.h"

#include "GrClient.h"

#include "Font.h"
#include "Segment.h"
#include "ITextSource.h"

class XeTeXGrFont
	: public gr::Font
{
public:
			XeTeXGrFont(const XeTeXFontInst* inFont, const char* name);

	virtual ~XeTeXGrFont();

	/**
	* Returns a copy of the recipient. Specifically needed to store the 
	* Font in a segment. 
	* @internal
	* @return pointer to copy
	*/
	virtual XeTeXGrFont * copyThis();

	/**
	* Return wether the font is bold.
	* @return true when bold
	*/
	virtual bool bold() { return false; }

	/**
	* Return wether the font is italic.
	* @return true when italic
	*/
	virtual bool italic() { return false; }

	/**
	* Returns the font ascent.
	* Value is the same as that returned by getFontMetrics()
	* @return the font ascent in device co-ordinates
	*/
	virtual float ascent() { return fXeTeXFont->getExactAscent(); }

	/**
	* Returns the font descent.
	* Value is the same as that returned by getFontMetrics()
	* @return the font descent in device co-ordinates
	*/
	virtual float descent() { return fXeTeXFont->getExactDescent(); }

	/**
	* Returns the total height of the font. 
	* @return font height in device co-ordinates
	*/
	virtual float height() { return ascent() + descent(); }

	/** 
	* Returns the x and y resolution of the device co-ordinate space.
	*/
	virtual unsigned int getDPIx() { return 72.27; }
	virtual unsigned int getDPIy() { return 72.27; }

	/**
	* Returns a pointer to the start of a table in the font.
	* If the Font class cannot easily determine the length of the table, 
	* it may set 0 as the length (while returning a non-NULL pointer to 
	* the table). This means that certain kinds of error checking cannot 
	* be done by the Graphite engine.
	* Throws an exception if there is some other error in reading the 
	* table, or if the table asked for is not in the font.
	* @param tableID the TTF ID of the table.
	* @param pcbSize pointer to a size_t to hold the table size.
	* @return address of the buffer containing the table or 0
	*/
	virtual const void * getTable(gr::fontTableId32 tableID, size_t * pcbSize);

	/**
	* Fetches the basic metrics of the font in device co-ordinates
	* (normaly pixels).
	* @param pAscent pointer to hold font ascent.
	* @param pDescent pointer to hold font descent.
	* @param pEmSquare pointer to hold font EM square.
	*/
	virtual void getFontMetrics(float * pAscent, float * pDescent = NULL, float * pEmSquare = NULL);

protected:
	const XeTeXFontInst*	fXeTeXFont;
	const char*				fName;
};

class XeTeXGrTextSource
	: public gr::ITextSource
{
public:
							XeTeXGrTextSource(const UniChar* iText = NULL, size_t iLen = 0, bool iRTL = false,
									size_t iNumF = 0, const gr::FeatureSetting* iFeatures = NULL)
								: fTextBuffer(iText)
								, fTextLength(iLen)
								, fRightToLeft(iRTL)
								, fNumFeatures(iNumF)
								, fFeatureSettings(iFeatures)
									{ }

	virtual					~XeTeXGrTextSource()
								{ }

	void					setText(const UniChar* iText, size_t iLen, bool iRTL = false,
									size_t iNumF = 0, const gr::FeatureSetting* iFeatures = NULL);

	virtual gr::UtfType		utfEncodingForm()
								{ return gr::kutf16; }

	virtual size_t			fetch(toffset startChar, size_t n, gr::utf32* buffer)
								{ throw; }

	virtual size_t			fetch(toffset startChar, size_t n, gr::utf8* buffer)
								{ throw; }

	virtual size_t			fetch(toffset startChar, size_t n, gr::utf16* buffer);

	virtual size_t			getLength()
								{ return fTextLength; }
	
	virtual std::pair<toffset,toffset>	propertyRange(toffset charIndex)
								{ return std::pair<toffset,toffset>(0, fTextLength); }
	
	virtual bool			sameSegment(toffset firstChar, toffset secondChar)
								{ return true; }

	virtual bool			getRightToLeft(toffset charIndex);

	virtual unsigned int	getDirectionDepth(toffset charIndex);

	virtual float			getVerticalOffset(toffset charIndex)
								{ return 0.0; }

	virtual isocode			getLanguage(toffset charIndex)
								{ return kUnknownLanguage; }

	virtual size_t			getFontFeatures(toffset charIndex, gr::FeatureSetting properties[64]);

protected:
	const UInt16*				fTextBuffer;
	size_t						fTextLength;
	bool						fRightToLeft;
	size_t						fNumFeatures;
	const gr::FeatureSetting*	fFeatureSettings;
	
	static const isocode		kUnknownLanguage;
};

#endif /* __XETEX_GR_LAYOUT__ */
