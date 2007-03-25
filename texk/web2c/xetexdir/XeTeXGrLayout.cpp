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

#include "XeTeX_ext.h"
#include "XeTeXswap.h"

#include "XeTeXGrLayout.h"

#include "GrDebug.h"
#include "GrClient.h"
#include "GrData.h"
#include "GraphiteProcess.h"

/* internal Graphite headers used here until the Font API gets cleaned up */
typedef wchar_t OLECHAR;
#include "../src/segment/GrEngine.h"
#include "../src/segment/FontCache.h"
#include "../src/segment/FontFace.h"


/* XeTeXGrFont class */

XeTeXGrFont::XeTeXGrFont(const XeTeXFontInst* inFont, const char* name)
	: Font()
	, fXeTeXFont(inFont)
	, fName(NULL)
{
	fName = strdup(name);

	int	len = strlen(name);
	wchar_t*	wname = new wchar_t[len + 1];
	mbstowcs(wname, name, len);

	m_pfface = gr::FontFace::GetFontFace(this, wname, false, false);
	delete[] wname;

	gr::FontFace::SetFlushMode(gr::kflushManual);
}

XeTeXGrFont::~XeTeXGrFont()
{
	delete[] fName;
}

XeTeXGrFont *
XeTeXGrFont::copyThis()
{
	XeTeXGrFont*	rval = new XeTeXGrFont(fXeTeXFont, fName);
	return rval;
}

const void *
XeTeXGrFont::getTable(gr::fontTableId32 tableID, size_t * pcbSize)
{
	le_uint32	length;
	LETag		tag = SWAP32(tableID);
	const void *rval = fXeTeXFont->readTable(tag, &length);
	*pcbSize = length;
	return rval;
}

void
XeTeXGrFont::getFontMetrics(float * pAscent, float * pDescent, float * pEmSquare)
{
	if (pAscent)
		*pAscent = fXeTeXFont->getExactAscent();
	if (pDescent)
		*pDescent = - fXeTeXFont->getExactDescent();
	if (pEmSquare)
		*pEmSquare = fXeTeXFont->getXPixelsPerEm();
}


/* XeTeXGrTextSource class */

void
XeTeXGrTextSource::setText(const UniChar* iText, size_t iLen, bool iRTL)
{
	fTextBuffer = iText;
	fTextLength = iLen;
	fRightToLeft = iRTL;
}

void
XeTeXGrTextSource::setFeatures(int nFeatures, const int* featureIDs, const int* featureValues)
{
	if (fFeatureSettings != NULL)
		delete[] fFeatureSettings;
	
	fNumFeatures = nFeatures;
	if (nFeatures == 0)
		fFeatureSettings = NULL;
	else {
		gr::FeatureSetting* newFeatures = new gr::FeatureSetting[nFeatures];
		for (int i = 0; i < nFeatures; ++i) {
			newFeatures[i].id = featureIDs[i];
			newFeatures[i].value = featureValues[i];
		}
		fFeatureSettings = newFeatures;
	}
}

void
XeTeXGrTextSource::setFeatures(int nFeatures, const gr::FeatureSetting* features)
{
	if (fFeatureSettings != NULL)
		delete[] fFeatureSettings;
	
	fNumFeatures = nFeatures;
	if (nFeatures == 0)
		fFeatureSettings = NULL;
	else {
		gr::FeatureSetting* newFeatures = new gr::FeatureSetting[nFeatures];
		for (int i = 0; i < nFeatures; ++i)
			newFeatures[i] = features[i];
		fFeatureSettings = newFeatures;
	}
}

size_t
XeTeXGrTextSource::fetch(toffset startChar, size_t n, gr::utf16* buffer)
{
	for (size_t i = 0; i < n; ++i)
		buffer[i] = fTextBuffer[i];
	return n;
}

bool
XeTeXGrTextSource::getRightToLeft(toffset charIndex)
{
	return false; // FIXME
}

unsigned int
XeTeXGrTextSource::getDirectionDepth(toffset charIndex)
{
	return 0; // FIXME
}

size_t
XeTeXGrTextSource::getFontFeatures(toffset charIndex, gr::FeatureSetting properties[64])
{
	for (int i = 0; i < fNumFeatures; ++i)
		properties[i] = fFeatureSettings[i];
	return fNumFeatures;
}

const isocode XeTeXGrTextSource::kUnknownLanguage = { 0, 0, 0, 0 };
