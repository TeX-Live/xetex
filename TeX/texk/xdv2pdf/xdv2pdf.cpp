/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

/*
	xdv2pdf
	Convert xdv file from XeTeX to PDF format for viewing/printing

	usage: xdv2pdf [-d debug] [-m mag] [-p papersize] [-v] xdvfile [-o pdffile]

		-m	override magnification from xdv file
		-v	show progress messages (page counters)
		-d  set kpathsea_debug values
        -p	papersize [default: from Mac OS X printing system]
        
        known paperSize values:
            a0-a10
			b0-b10
			c0-c10
			jb0-jb10
            letter
            legal
            ledger
            tabloid
		can append ":landscape"
		or use "x,y" where x and y are dimensions in "big points" or with units
        
		output file defaults to <xdvfile>.pdf

	also permissible to omit xdvfile but specify pdffile; then input is from stdin
*/

#define MAC_OS_X_VERSION_MIN_REQUIRED	1020
#define MAC_OS_X_VERSION_MAX_ALLOWED	1030

#include <ApplicationServices/ApplicationServices.h>
#include <Quicktime/Quicktime.h>
#include <string>
#include <vector>
#include <list>
#include <map>

#include <sys/stat.h>

#include "DVIops.h"

#define aat_font_flag	0xffff

struct papersizerec {
	const char*	name;
	double_t	width;
	double_t	height;	// dimensions in BP ("big" or PostScript/CG points)
};

static papersizerec	papersizes[] =
{
	{ "a0",		2383.937008,	3370.393701	},
	{ "a1",		1683.779528,	2383.937008	},
	{ "a2",		1190.551181,	1683.779528	},
	{ "a3",		841.8897638,	1190.551181	},
	{ "a4",		595.2755906,	841.8897638	},
	{ "a5",		419.5275591,	595.2755906	},
	{ "a6",		297.6377953,	419.5275591	},
	{ "a7",		209.7637795,	297.6377953	},
	{ "a8",		147.4015748,	209.7637795	},
	{ "a9",		104.8818898,	147.4015748	},
	{ "a10",	73.7007874,		104.8818898	},
	
	{ "b0",		2834.645669,	4008.188976	},
	{ "b1",		2004.094488,	2834.645669	},
	{ "b2",		1417.322835,	2004.094488	},
	{ "b3",		1000.629921,	1417.322835	},
	{ "b4",		708.6614173,	1000.629921	},
	{ "b5",		498.8976378,	708.6614173	},
	{ "b6",		354.3307087,	498.8976378	},
	{ "b7",		249.4488189,	354.3307087	},
	{ "b8",		175.7480315,	249.4488189	},
	{ "b9",		124.7244094,	175.7480315	},
	{ "b10",	87.87401575,	124.7244094	},
	
	{ "c0",		2599.370079,	3676.535433	},
	{ "c1",		1836.850394,	2599.370079	},
	{ "c2",		1298.267717,	1836.850394	},
	{ "c3",		918.4251969,	1298.267717	},
	{ "c4",		649.1338583,	918.4251969	},
	{ "c5",		459.2125984,	649.1338583	},
	{ "c6",		323.1496063,	459.2125984	},
	{ "c7",		229.6062992,	323.1496063	},
	{ "c8",		161.5748031,	229.6062992	},
	{ "c9",		113.3858268,	161.5748031	},
	{ "c10",	79.37007874,	113.3858268	},
	
	{ "jb0",	2919.685039,	4127.244094	},
	{ "jb1",	2063.622047,	2919.685039	},
	{ "jb2",	1459.84252,		2063.622047	},
	{ "jb3",	1031.811024,	1459.84252	},
	{ "jb4",	728.503937,		1031.811024	},
	{ "jb5",	515.9055118,	728.503937	},
	{ "jb6",	362.8346457,	515.9055118	},
	{ "jb7",	257.9527559,	362.8346457	},
	{ "jb8",	181.4173228,	257.9527559	},
	{ "jb9",	127.5590551,	181.4173228	},
	{ "jb10",	90.70866142,	127.5590551	},

	{ "letter",		612.0,	792.0	},
	{ "legal",		612.0,	1008.0	},
	{ "tabloid",	792.0,	1224.0	},
	{ "ledger",		1224.0,	792.0	},

	{ 0, 0, 0 }
};

struct dimenrec {
	const char*	name;
	double_t	factor;	// multiplier to convert to BP
};

static dimenrec dimenratios[] =
{
	{ "in", 72.0 },
	{ "mm", 72.0 / 25.4 },
	{ "cm", 72.0 / 2.54 },
	{ "bp", 1.0 },
	{ "pt", 72.0 / 72.27 },
	{ "pc", 12.0 * 72.0 / 72.27 },
	{ "dd", 1238.0 * 72.0 / 72.27 / 1157.0 },
	{ "cc", 14856.0 * 72.0 / 72.27 / 1157.0 },
	{ 0, 0 }
};

struct TEX_FONT {
				TEX_FONT()
                    : cgFont(0)
                    , size(Long2Fix(10))
					, charMap(NULL)
					, hasMetrics(false)
						{
						}
	CGFontRef			cgFont;
    Fixed				size;
	std::vector<Fixed>	widths;
	std::vector<Fixed>	heights;
	std::vector<Fixed>	depths;
	std::vector<UInt16>*charMap;
	bool				hasMetrics;
	bool				hasMap;
};

static std::map<UInt32,TEX_FONT>	tex_fonts;

struct NATIVE_FONT {
				NATIVE_FONT()
					: atsuStyle(0)
                    , cgFont(0)
                    , size(Long2Fix(10))
                    , isColored(false)
						{
						}
	ATSUStyle			atsuStyle;
	CGFontRef			cgFont;
	Fixed				size;
	ATSURGBAlphaColor	color;
	bool				isColored;
};

static std::map<UInt32,NATIVE_FONT>	native_fonts;

static ATSUTextLayout	layout;

static double_t scr2dvi = (72.27 * 65536.0) / 72.0;
static double_t dvi2scr = 1.0 / scr2dvi;

struct dvi_vars {
	SInt32	h;
	SInt32	v;
	SInt32	w;
	SInt32	x;
	SInt32	y;
	SInt32	z;
};
static dvi_vars		dvi;
static UInt32			f, cur_f;
static const UInt32	kUndefinedFont = 0x80000000;

static const ATSURGBAlphaColor	constBlackColor = { 0.0, 0.0, 0.0, 1.0 };

struct colorStateRec {
	ATSURGBAlphaColor	color;
	bool				override;
};
static std::list<colorStateRec>	textColorStack;
static std::list<colorStateRec>	ruleColorStack;

static colorStateRec	textColor = { { 0.0, 0.0, 0.0, 1.0 }, false };
static colorStateRec	ruleColor = { { 0.0, 0.0, 0.0, 1.0 }, false };

static double_t	pageHt = 0;
static double_t	pageWd = 0;

static UInt32	mag = 0;
static double_t	mag_scale = 1.0;

static CGContextRef		graphicsContext = 0;

static long	page_index = 0;

static bool	verbose = false;

static CFURLRef	saveURL = 0;

static FILE*	pdfmarks = NULL;
static char		pdfmarkPath[_POSIX_PATH_MAX+1];

static std::map<ATSFontRef,CGFontRef>	cg_fonts;

static CGFontRef
getCGFontForATSFont(ATSFontRef fontRef)
{
	std::map<ATSFontRef,CGFontRef>::const_iterator	i = cg_fonts.find(fontRef);
	if (i != cg_fonts.end()) {
		return i->second;
	}
	CGFontRef	newFont;
	if (fontRef == 0) {
		ATSFontRef	tmpRef = ATSFontFindFromPostScriptName(CFSTR("Helvetica"), kATSOptionFlagsDefault);
		newFont = CGFontCreateWithPlatformFont(&tmpRef);
	}
	else
		newFont = CGFontCreateWithPlatformFont(&fontRef);
	cg_fonts[fontRef] = newFont;
	return newFont;
}

struct box {
	float	llx;
	float	lly;
	float	urx;
	float	ury;
};

static box	annot_box;
static int	tag_level = -1;
static int	dvi_level = 0;
static bool	tracking_boxes = false;

static void
init_annot_box()
{
	annot_box.llx = pageWd;
	annot_box.lly = 0;
	annot_box.urx = 0;
	annot_box.ury = pageHt;
}

static void
merge_box(const box& b)
{
	if (b.llx < annot_box.llx)
		annot_box.llx = b.llx;
	if (b.lly > annot_box.lly)
		annot_box.lly = b.lly;	// NB positive is downwards!
	if (b.urx > annot_box.urx)
		annot_box.urx = b.urx;
	if (b.ury < annot_box.ury)
		annot_box.ury = b.ury;
}

long
read_signed(FILE* f, int k)
{
	long	s = (long)(signed char)getc(f);
	while (--k > 0) {
		s <<= 8;
		s += (getc(f) & 0xff);
	}
	return s;
}

unsigned long
read_unsigned(FILE* f, int k)
{
	unsigned long	u = getc(f);
	while (--k > 0) {
		u <<= 8;
		u += (getc(f) & 0xff);
	}
	return u;
}

#define MAX_BUFFERED_GLYPHS	1024
long		gGlyphCount = 0;
CGGlyph		gGlyphs[MAX_BUFFERED_GLYPHS];
CGSize		gAdvances[MAX_BUFFERED_GLYPHS];
CGPoint		gStartLoc;
CGPoint		gPrevLoc;

void
flushGlyphs()
{
	if (gGlyphCount > 0) {
		CGContextSetTextPosition(graphicsContext, gStartLoc.x, gStartLoc.y);
		gAdvances[gGlyphCount].width = 0;
		gAdvances[gGlyphCount].height = 0;
		if (textColor.override) {
			CGContextSaveGState(graphicsContext);
			CGContextSetFillColor(graphicsContext, &textColor.color.red);
		}
		CGContextShowGlyphsWithAdvances(graphicsContext, gGlyphs, gAdvances, gGlyphCount);
		if (textColor.override)
			CGContextRestoreGState(graphicsContext);
		gGlyphCount = 0;
	}
}

void
bufferGlyph(CGGlyph g)
{
	CGPoint	curLoc;
	curLoc.x = dvi2scr * dvi.h;
	curLoc.y = dvi2scr * (pageHt - dvi.v);
	if (gGlyphCount == 0)
		gStartLoc = curLoc;
	else {
		gAdvances[gGlyphCount-1].width = curLoc.x - gPrevLoc.x;
		gAdvances[gGlyphCount-1].height = curLoc.y - gPrevLoc.y;
	}
	gPrevLoc = curLoc;
	gGlyphs[gGlyphCount] = g;
	if (++gGlyphCount == MAX_BUFFERED_GLYPHS)
		flushGlyphs();
}

void
setchar(UInt32 c, bool adv)
{
	OSStatus	status;

    if (f != cur_f) {
		flushGlyphs();
        CGContextSetFont(graphicsContext, tex_fonts[f].cgFont);
        CGContextSetFontSize(graphicsContext, Fix2X(tex_fonts[f].size) * mag_scale);
        cur_f = f;
	}

    CGGlyph	g;
	if (tex_fonts[f].charMap != NULL) {
		if (c < tex_fonts[f].charMap->size())
			g = (*tex_fonts[f].charMap)[c];
		else
			g = c;
	}
	else
		// default: glyph ID = char + 1, works for the majority of the CM PS fonts
	    g = c + 1;
	    
	bufferGlyph(g);

	if (tracking_boxes) {
		if (tex_fonts[f].hasMetrics) {
			box	b = {
				dvi.h,
				dvi.v + tex_fonts[f].depths[c],
				dvi.h + tex_fonts[f].widths[c],
				dvi.v - tex_fonts[f].heights[c] };
			merge_box(b);
		}
	}

	if (adv && tex_fonts[f].hasMetrics)
		dvi.h += tex_fonts[f].widths[c];
}

static void
do_set_glyph(UInt16 g, Fixed a)
{
	// NOTE that this is separate from the glyph-buffering done by setchar()
	// as \XeTeXglyph deals with native fonts, while setchar() and bufferGlyph()
	// are used to work with legacy TeX fonts
	
	if (f != cur_f) {
		CGContextSetFont(graphicsContext, native_fonts[f].cgFont);
		CGContextSetFontSize(graphicsContext, Fix2X(native_fonts[f].size) * mag_scale);
		cur_f = f;
	}
	
	bool	resetColor = false;
    if (textColor.override) {
    	CGContextSaveGState(graphicsContext);
        CGContextSetFillColor(graphicsContext, &textColor.color.red);
		resetColor = true;
	}
	else if (native_fonts[f].isColored) {
    	CGContextSaveGState(graphicsContext);
        CGContextSetFillColor(graphicsContext, &native_fonts[f].color.red);
       	resetColor = true;
	}
    CGContextShowGlyphsAtPoint(graphicsContext, dvi2scr * dvi.h, dvi2scr * (pageHt - dvi.v), &g, 1);
    if (resetColor)
    	CGContextRestoreGState(graphicsContext);

	if (tracking_boxes) {
		box	b = {
			dvi.h,
			dvi.v + (native_fonts[f].size / 3),
			dvi.h + a,
			dvi.v - (2 * native_fonts[f].size / 3) };
		merge_box(b);
	}

	dvi.h += a;
}

void
do_set_native(FILE* xdv)
{
	OSStatus	status = noErr;
	Boolean	dir = read_unsigned(xdv, 1);
	UInt32	wid = read_unsigned(xdv, 4);
	UInt32	len = read_unsigned(xdv, 2);
    
	static UniChar*	text = 0;
    static UniCharCount	textBufLen = 0;
    if (len > textBufLen) {
        if (text != 0)
            delete[] text;
        textBufLen = (len & 0xFFFFFFF0) + 32;
        text = new UniChar[textBufLen];
    }

	UInt32	i;
	for (i = 0; i < len; ++i)
		text[i] = read_unsigned(xdv, 2);

	if (native_fonts[f].atsuStyle != 0) {
		status = ATSUClearLayoutCache(layout, 0);
		status = ATSUSetTextPointerLocation(layout, &text[0], 0, len, len);
	
		ATSUAttributeTag		tag[3] = { kATSULineWidthTag, kATSULineJustificationFactorTag, kATSULineDirectionTag };
		ByteCount				valueSize[3] = { sizeof(Fixed), sizeof(Fract), sizeof(Boolean) };
		ATSUAttributeValuePtr	value[3];
		Fixed	scaledWid = X2Fix(dvi2scr * wid);
		value[0] = &scaledWid;
		Fract	just = len > 2 ? fract1 : 0;
		value[1] = &just;
		value[2] = &dir;
		status = ATSUSetLayoutControls(layout, 3, &tag[0], &valueSize[0], &value[0]);
	
		ATSUStyle	tmpStyle;
		if (textColor.override) {
			ATSUAttributeTag		tag = kATSURGBAlphaColorTag;
			ByteCount				valueSize = sizeof(ATSURGBAlphaColor);
			ATSUAttributeValuePtr	valuePtr = &textColor.color;
			status = ATSUCreateAndCopyStyle(native_fonts[f].atsuStyle, &tmpStyle);
			status = ATSUSetAttributes(tmpStyle, 1, &tag, &valueSize, &valuePtr);
			status = ATSUSetRunStyle(layout, tmpStyle, kATSUFromTextBeginning, kATSUToTextEnd);
			cur_f = kUndefinedFont;
		}
		else if (f != cur_f) {
			status = ATSUSetRunStyle(layout, native_fonts[f].atsuStyle, kATSUFromTextBeginning, kATSUToTextEnd);
			cur_f = f;
		}
	
		CGContextTranslateCTM(graphicsContext, dvi2scr * dvi.h, dvi2scr * (pageHt - dvi.v));
		status = ATSUDrawText(layout, 0, len, 0, 0);
		CGContextTranslateCTM(graphicsContext, -dvi2scr * dvi.h, -dvi2scr * (pageHt - dvi.v));
	
		if (textColor.override)
			status = ATSUDisposeStyle(tmpStyle);

		if (tracking_boxes) {
			Rect	rect;
			status = ATSUMeasureTextImage(layout, 0, len, 0, 0, &rect);
			box	b = {
				dvi.h,
				dvi.v + scr2dvi * rect.bottom,
				dvi.h + wid,
				dvi.v + scr2dvi * rect.top };
			merge_box(b);
		}
	}

	dvi.h += wid;
}

#define native_glyph_data_size	(sizeof(CGGlyph) + sizeof(CGSize))
static void
do_glyph_array(FILE* xdv)
{
	static char*	glyphInfoBuf = 0;
	static int		maxGlyphs = 0;

	Fixed	wid = read_unsigned(xdv, 4);
	int	glyphCount = read_unsigned(xdv, 2);
	
	if (glyphCount >= maxGlyphs) {
		maxGlyphs = glyphCount + 100;
		if (glyphInfoBuf != 0)
			delete[] glyphInfoBuf;
		glyphInfoBuf = new char[maxGlyphs * native_glyph_data_size];
	}
	
	fread(glyphInfoBuf, native_glyph_data_size, glyphCount, xdv);
	CGPoint*	locations = (CGPoint*)glyphInfoBuf;
	CGGlyph*	glyphs = (CGGlyph*)(locations + glyphCount);
	
	if (f != cur_f) {
		CGContextSetFont(graphicsContext, native_fonts[f].cgFont);
		CGContextSetFontSize(graphicsContext, Fix2X(native_fonts[f].size) * mag_scale);
		cur_f = f;
	}

	CGContextSetTextPosition(graphicsContext, dvi2scr * dvi.h + locations[0].x,
												dvi2scr * (pageHt - dvi.v) - locations[0].y);

	CGSize*		advances = (CGSize*)locations;
	for (int i = 0; i < glyphCount - 1; ++i) {
		advances[i].width = locations[i+1].x - locations[i].x;
		advances[i].height = - (locations[i+1].y - locations[i].y);
	}
	advances[glyphCount-1].width = 0;
	advances[glyphCount-1].height = 0;

	bool	resetColor = false;
    if (textColor.override) {
    	CGContextSaveGState(graphicsContext);
        CGContextSetFillColor(graphicsContext, &textColor.color.red);
		resetColor = true;
	}
	else if (native_fonts[f].isColored) {
    	CGContextSaveGState(graphicsContext);
        CGContextSetFillColor(graphicsContext, &native_fonts[f].color.red);
		resetColor = true;
	}

	CGContextShowGlyphsWithAdvances(graphicsContext, glyphs, advances, glyphCount);

	if (resetColor)
		CGContextRestoreGState(graphicsContext);

	if (tracking_boxes) {
		box	b = {
			dvi.h,
			dvi.v + (native_fonts[f].size / 3),
			dvi.h + wid,
			dvi.v - (2 * native_fonts[f].size / 3) };
		merge_box(b);
	}

	dvi.h += wid;
}

/* code lifted almost verbatim from Apple sample "QTtoCG" */
typedef struct {
	size_t width;
	size_t height;
	size_t bitsPerComponent;
	size_t bitsPerPixel;
	size_t bytesPerRow;
	size_t size;
	CGImageAlphaInfo ai;
	CGColorSpaceRef cs;
	unsigned char *data;
	CMProfileRef prof;
} BitmapInfo;

void readBitmapInfo(GraphicsImportComponent gi, BitmapInfo *bi)
{
	ComponentResult result;
	ImageDescriptionHandle imageDescH = NULL;
	ImageDescription *desc;
	Handle profile = NULL;
	
	result = GraphicsImportGetImageDescription(gi, &imageDescH);
	if( noErr != result || imageDescH == NULL ) {
		fprintf(stderr, "Error while retrieving image description");
		exit(1);
	}
	
	desc = *imageDescH;
	
	bi->width = desc->width;
	bi->height = desc->height;
	bi->bitsPerComponent = 8;
	bi->bitsPerPixel = 32;
	bi->bytesPerRow = (bi->bitsPerPixel * bi->width + 7)/8;
	bi->ai = (desc->depth == 32) ? kCGImageAlphaFirst : kCGImageAlphaNoneSkipFirst;
	bi->size = bi->bytesPerRow * bi->height;
	bi->data = (unsigned char*)malloc(bi->size);
	
	bi->cs = NULL;
	bi->prof = NULL;
	GraphicsImportGetColorSyncProfile(gi, &profile);
	if( NULL != profile ) {
		CMError err;
		CMProfileLocation profLoc;
		Boolean bValid, bPreferredCMMNotFound;

		profLoc.locType = cmHandleBasedProfile;
		profLoc.u.handleLoc.h = profile;
		
		err = CMOpenProfile(&bi->prof, &profLoc);
		if( err != noErr ) {
			fprintf(stderr, "Cannot open profile");
			exit(1);
		}
		
		/* Not necessary to validate profile, but good for debugging */
		err = CMValidateProfile(bi->prof, &bValid, &bPreferredCMMNotFound);
		if( err != noErr ) {
			fprintf(stderr, "Cannot validate profile : Valid: %d, Preferred CMM not found : %d", bValid, 
				  bPreferredCMMNotFound);
			exit(1);
		}
		
		bi->cs = CGColorSpaceCreateWithPlatformColorSpace( bi->prof );

		if( bi->cs == NULL ) {
			fprintf(stderr, "Error creating cg colorspace from csync profile");
			exit(1);
		}
		DisposeHandle(profile);
	}	
	
	if( imageDescH != NULL)
		DisposeHandle((Handle)imageDescH);
}

void getBitmapData(GraphicsImportComponent gi, BitmapInfo *bi)
{
	GWorldPtr gWorld;
	QDErr err = noErr;
	Rect boundsRect = { 0, 0, bi->height, bi->width };
	ComponentResult result;
	
	if( bi->data == NULL ) {
		fprintf(stderr, "no bitmap buffer available");
		exit(1);
	}
	
	err = NewGWorldFromPtr( &gWorld, k32ARGBPixelFormat, &boundsRect, NULL, NULL, 0, 
							(char*)bi->data, bi->bytesPerRow );
	if (noErr != err) {
		fprintf(stderr, "error creating new gworld - %d", err);
		exit(1);
	}
	
	if( (result = GraphicsImportSetGWorld(gi, gWorld, NULL)) != noErr ) {
		fprintf(stderr, "error while setting gworld");
		exit(1);
	}
	
	if( (result = GraphicsImportDraw(gi)) != noErr ) {
		fprintf(stderr, "error while drawing image through qt");
		exit(1);
	}
	
	DisposeGWorld(gWorld);	
}
/* end of code from "QTtoCG" */

struct imageRec { CGImageRef ref; CGRect bounds; };
std::map<std::string,imageRec>			cgImages;
std::map<std::string,CGPDFDocumentRef>	pdfDocs;

void
do_pic_file(FILE* xdv, bool isPDF)	// t[4][6] p[2] l[2] a[l]
{
    CGAffineTransform	t;
    t.a = Fix2X(read_signed(xdv, 4));
    t.b = Fix2X(read_signed(xdv, 4));
    t.c = Fix2X(read_signed(xdv, 4));
    t.d = Fix2X(read_signed(xdv, 4));
    t.tx = Fix2X(read_signed(xdv, 4));
    t.ty = Fix2X(read_signed(xdv, 4));
	if (mag != 1000)
		t = CGAffineTransformConcat(t, CGAffineTransformMakeScale(mag_scale, mag_scale));

    int		p = read_signed(xdv, 2);
    int		l = read_unsigned(xdv, 2);
    Handle	alias = NewHandle(l);
    if (alias != 0) {
        for (int i = 0; i < l; ++i)
            (*alias)[i] = read_unsigned(xdv, 1);
        FSSpec	spec;
        Boolean	changed;
        OSErr	result = ResolveAlias(0, (AliasHandle)alias, &spec, &changed);
        DisposeHandle(alias);
        
        if (result == noErr) {
			std::string	specString((char*)&spec, sizeof(spec));	// key for our map<>s of already-used images

            // is it a \pdffile instance?
			CGPDFDocumentRef	document = NULL;
			CGImageRef			image = NULL;
			CGRect				bounds;
            if (isPDF) {
                std::map<std::string,CGPDFDocumentRef>::const_iterator	i = pdfDocs.find(specString);
				if (i != pdfDocs.end())
					document = i->second;
				else {
					FSRef	fsRef;
					result = FSpMakeFSRef(&spec, &fsRef);
					CFURLRef	url = CFURLCreateFromFSRef(kCFAllocatorDefault, &fsRef);
					document = CGPDFDocumentCreateWithURL(url);
	                CFRelease(url);
					pdfDocs[specString] = document;
				}
                if (document != NULL) {
                    int	nPages = CGPDFDocumentGetNumberOfPages(document);
                    if (p < 0)			p = nPages + 1 + p;
                    if (p > nPages)		p = nPages;
                    if (p < 1)			p = 1;
                    bounds = CGPDFDocumentGetMediaBox(document, p);
				}
            }

            // otherwise use GraphicsImport
            else {
                std::map<std::string,imageRec>::const_iterator	i = cgImages.find(specString);
            	if (i != cgImages.end()) {
            		image = i->second.ref;
            		bounds = i->second.bounds;
				}
            	else {
					ComponentInstance	gi;
					result = GetGraphicsImporterForFile(&spec, &gi);
					if (result == noErr) {
						BitmapInfo	bi;
						readBitmapInfo(gi, &bi);
						getBitmapData(gi, &bi);
		
						if (bi.cs == NULL)
							bi.cs = CGColorSpaceCreateDeviceRGB();
						CGDataProviderRef	provider = CGDataProviderCreateWithData(NULL, bi.data, bi.size, NULL);
						image = CGImageCreate(bi.width, bi.height, bi.bitsPerComponent, bi.bitsPerPixel,
												bi.bytesPerRow, bi.cs, bi.ai, provider, NULL, 0, kCGRenderingIntentDefault);
						CGColorSpaceRelease(bi.cs);

						ImageDescriptionHandle	desc = NULL;
						result = GraphicsImportGetImageDescription(gi, &desc);
						bounds.origin.x = 0;
						bounds.origin.y = 0;
						bounds.size.width = (*desc)->width * 72.0 / Fix2X((*desc)->hRes);
						bounds.size.height = (*desc)->height * 72.0 / Fix2X((*desc)->vRes);
						DisposeHandle((Handle)desc);
						result = CloseComponent(gi);
             		}
					imageRec	ir = { image, bounds };
               		cgImages[specString] = ir;
               	}
            }

			CGContextSaveGState(graphicsContext);

			CGContextTranslateCTM(graphicsContext, dvi2scr * dvi.h, dvi2scr * (pageHt - dvi.v));
			CGContextConcatCTM(graphicsContext, t);

			if (document != NULL) {
                bounds.origin.x = bounds.origin.y = 0.0;
				CGContextDrawPDFDocument(graphicsContext, bounds, document, p);
			}
			else if (image != NULL)
				CGContextDrawImage(graphicsContext, bounds, image);

			CGContextRestoreGState(graphicsContext);

        }
    }
}

static int
readHexDigit(const char*& cp)
{
    if (*cp >= '0' && *cp <= '9')
        return *cp++ - '0';
    if (*cp >= 'A' && *cp <= 'F')
        return *cp++ - 'A' + 10;
    if (*cp >= 'a' && *cp <= 'f')
        return *cp++ - 'a' + 10;
    return 0;
}

static void
readColorValue(const char* s, ATSURGBAlphaColor& c)
{
    int	x;
    x = 0;
    x = readHexDigit(s);
    x <<= 4;
    x += readHexDigit(s);
    c.red = (double)x / 255.0;
    x = 0;
    x = readHexDigit(s);
    x <<= 4;
    x += readHexDigit(s);
    c.green = (double)x / 255.0;
    x = 0;
    x = readHexDigit(s);
    x <<= 4;
    x += readHexDigit(s);
    c.blue = (double)x / 255.0;
    c.alpha = 1.0;
}

static void
readColorValueWithAlpha(const char* s, ATSURGBAlphaColor& c)
{
    int	x;
    x = 0;
    x = readHexDigit(s);
    x <<= 4;
    x += readHexDigit(s);
    c.red = (double)x / 255.0;
    x = 0;
    x = readHexDigit(s);
    x <<= 4;
    x += readHexDigit(s);
    c.green = (double)x / 255.0;
    x = 0;
    x = readHexDigit(s);
    x <<= 4;
    x += readHexDigit(s);
    c.blue = (double)x / 255.0;
    x = 0;
    x = readHexDigit(s);
    x <<= 4;
    x += readHexDigit(s);
    c.alpha = (double)x / 255.0;
}

/* declarations of KPATHSEARCH functions we use for finding TFMs and OTFs */
extern "C" {
    UInt8* kpse_find_file(const UInt8* name, int type, int must_exist);
    UInt8* uppercasify(const UInt8* s);
};
extern	unsigned kpathsea_debug;

#include "xdv_kpse_formats.h"

static void
load_metrics(struct TEX_FONT& font, UInt8* name, Fixed d, Fixed s)
{
	UInt8*	pathname = kpse_find_file(name, xdv_kpse_tfm_format, true);
    if (pathname) {
        FILE*	tfmFile = fopen((char*)pathname, "rb");
        if (tfmFile != 0) {
            enum { lf = 0, lh, bc, ec, nw, nh, nd, ni, nl, nk, ne, np };
            SInt16	directory[12];
            fread(&directory[0], 2, 12, tfmFile);
            fseek(tfmFile, directory[lh] * 4, SEEK_CUR);
            int	nChars = directory[ec] - directory[bc] + 1;
            double_t	factor = Fix2X(d) / 16.0;
            if (s != d)
                factor = factor * Fix2X(s) / Fix2X(d);
            if (nChars > 0) {
                struct s_charinfo {
                    UInt8	widthIndex;
                    UInt8	heightDepth;
                    UInt8	italicIndex;
                    UInt8	remainder;
                };
                s_charinfo*	charInfo = new s_charinfo[nChars];
                fread(&charInfo[0], 4, nChars, tfmFile);
                Fixed*	widths = new Fixed[directory[nw]];
                fread(&widths[0], 4, directory[nw], tfmFile);
                Fixed*	heights = new Fixed[directory[nh]];
                fread(&heights[0], 4, directory[nh], tfmFile);
                Fixed*	depths = new Fixed[directory[nd]];
                fread(&depths[0], 4, directory[nd], tfmFile);
                
                font.widths.reserve(directory[ec] + 1);
                font.heights.reserve(directory[ec] + 1);
                font.depths.reserve(directory[ec] + 1);
                font.charMap = new std::vector<UInt16>;
                font.charMap->reserve(directory[ec] + 1);
                for (int i = 0; i < directory[bc]; ++i) {
                    font.widths.push_back(0);
                    font.heights.push_back(0);
                    font.depths.push_back(0);
                    font.charMap->push_back(0);
                }
                int	g = 0;
                for (int i = 0; i < nChars; ++i) {
                	if (charInfo[i].widthIndex == 0) {
						font.widths.push_back(0);
						font.heights.push_back(0);
						font.depths.push_back(0);
						font.charMap->push_back(0);
					}
					else {
						double_t	wid = Fix2X(widths[charInfo[i].widthIndex]) * factor;
						font.widths.push_back(X2Fix(wid));

						int	heightIndex = (charInfo[i].heightDepth >> 4);
						double_t	ht = Fix2X(heights[heightIndex]) * factor;
						font.heights.push_back(X2Fix(ht));
						
						int	depthIndex = (charInfo[i].heightDepth & 0x0f);
						double_t	dp = Fix2X(depths[depthIndex]) * factor;
						font.depths.push_back(X2Fix(dp));

						font.charMap->push_back(++g);
					}
                }
                
                delete[] widths;
                delete[] heights;
                delete[] depths;
                delete[] charInfo;
            }
    
            font.hasMetrics = true;
            fclose(tfmFile);
        }
        free(pathname);
    }
}

typedef std::vector<std::string>	encoding;
std::map<std::string,encoding>		encodings;

class fontMapRec {
public:
				fontMapRec()
					: cgFont(0)
					, cmap(NULL)
					, loaded(false)
					{ }

	std::string	psName;
	std::string	encName;
	std::string	pfbName;

	CGFontRef			cgFont;
	std::vector<UInt16>*cmap;

	bool		loaded;
};

std::map<std::string,fontMapRec>	psFontsMap;

static void
clearFontMap()
{
	std::map<std::string,fontMapRec>::iterator	i;
	for (i = psFontsMap.begin(); i != psFontsMap.end(); ++i) {
		if (i->second.cmap != NULL)
			delete i->second.cmap;
	}
	psFontsMap.clear();
}

static void
doPdfMapLine(const char* line, char mode)
{
	if (*line == '%')
		return;
	while (*line == ' ' || *line == '\t')
		++line;
	if (*line < ' ')
		return;

	if (mode == 0) {
		switch (*line) {
			case '+':
			case '-':
			case '=':
				mode = *line;
				++line;
				while (*line == ' ' || *line == '\t')
					++line;
				if (*line < ' ')
					return;
				break;
			default:
				clearFontMap();
				mode = '+';
				break;
		}
	}

	const char*	b = line;
	const char*	e = b;
	while (*e > ' ')
		++e;
	std::string	tfmName(b, e - b);
	
	if (mode == '-') {
		psFontsMap.erase(tfmName);
		return;
			// erase existing entry
	}
	
	if ((mode == '+') && (psFontsMap.find(tfmName) != psFontsMap.end()))
		return;
			// don't add if entry already exists
	
	while (*e && *e <= ' ')
		++e;
	b = e;
	while (*e > ' ')
		++e;
	if (e > b) {
		fontMapRec	fr;
		fr.psName.assign(b, e - b);
		while (*e && *e <= ' ')
			++e;
		if (*e == '"') {	// skip quoted string; we don't do oblique, stretch, etc. (yet)
			++e;
			while (*e && *e != '"')
				++e;
			if (*e == '"')
				++e;
			while (*e && *e <= ' ')
				++e;
		}
		while (*e == '<') {
			++e;
			b = e;
			while (*e > ' ')
				++e;
			if (strncmp(e - 4, ".enc", 4) == 0) {
				fr.encName.assign(b, e - b);
			}
			else if (strncmp(e - 4, ".pfb", 4) == 0) {
				fr.pfbName.assign(b, e - b);
			}
/*
			else if (strncmp(e - 4, ".pfa", 4) == 0) {
				fr.otfName.assign(b, e - b - 4);
			}
*/
			while (*e && *e <= ' ')
				++e;
		}
		psFontsMap[tfmName] = fr;
	}
}

static bool	sMapFileLoaded = false;

static void
doPdfMapFile(const char* fileName)
{
	char	mode = 0;

	while (*fileName == ' ' || *fileName == '\t')
		++fileName;
	if (*fileName < ' ')
		return;

	switch (*fileName) {
		case '+':
		case '-':
		case '=':
			mode = *fileName;
			++fileName;
			break;
		default:
			clearFontMap();
			mode = '+';
			break;
	}
	while (*fileName == ' ' || *fileName == '\t')
		++fileName;
	if (*fileName < ' ')
		return;

	bool	loaded = false;
	UInt8*	pathname = kpse_find_file((UInt8*)fileName, xdv_kpse_dvips_config_format, true);
	if (pathname) {
		FILE*	f = fopen((char*)pathname, "r");
		if (f != NULL) {
			char	line[1000];
			while (!feof(f)) {
				if (fgets(line, 999, f) == 0)
					break;
				doPdfMapLine(line, mode);

			}
			loaded = true;
			fclose(f);
			if (verbose)
				fprintf(stderr, "\n{fontmap: %s} ", pathname);
		}
		free(pathname);
	}
	if (!loaded)
		fprintf(stderr, "\n*** fontmap %s not found; texmf.cnf may be broken\n", fileName);
	else
		sMapFileLoaded = true;
}

// return a pointer to the encoding vector with the given name, loading it if necessary
// we don't really "parse" the .enc file
static const encoding*
getEncoding(const std::string& name)
{
	std::map<std::string,encoding>::iterator	i = encodings.find(name);
	if (i == encodings.end()) {
		encoding	enc;
		UInt8*	pathname = kpse_find_file((UInt8*)(name.c_str()), xdv_kpse_tex_ps_header_format, true);
		if (pathname != NULL) {
			FILE*	f = fopen((char*)pathname, "r");
			if (f != NULL) {
				int	c;
				bool	inVector = false;
				std::string	str;
				while ((c = getc(f)) != EOF) {
				got_ch:
					if (c == '%') {	// comment: skip rest of line
						while ((c = getc(f)) != EOF && c != '\r' && c != '\n')
							;
						goto got_ch;
					}
					if (c == '[') {
						inVector = true;
					}
					else if (c == '/' || c <= ' ' || c == ']' || c == EOF) {
						if (inVector && str.length() > 0)
							enc.push_back(str);
						str.clear();
					}
					else if (inVector && c != EOF)
						str.append(1, (char)c);
				}
				if (verbose)
					fprintf(stderr, "\n{encoding: %s} ", pathname);
				fclose(f);
			}
			free(pathname);
		}
		encodings[name] = enc;
		return &(encodings[name]);
	}

	return &(i->second);
}

static ATSFontRef
activateFromPath(const char* pathName)
{
	FSRef		fontFileRef;
	FSSpec		fontFileSpec;
	OSStatus	status = FSPathMakeRef((UInt8*)pathName, &fontFileRef, 0);
	if (status == noErr)
		status = FSGetCatalogInfo(&fontFileRef, 0, 0, 0, &fontFileSpec, 0);
	if (status == noErr) {
		ATSFontContainerRef containerRef;
		status = ATSFontActivateFromFileSpecification(&fontFileSpec, kATSFontContextLocal,
						kATSFontFormatUnspecified, 0, kATSOptionFlagsDefault, &containerRef);
		ATSFontRef	fontRef;
		ItemCount	count;
		status = ATSFontFindFromContainer(containerRef, 0, 1, &fontRef, &count);
		if ((status == noErr) && (count == 1))
			return fontRef;
		// failed, or container contained multiple fonts(!) -- confused
		ATSFontDeactivate(containerRef, 0, kATSOptionFlagsDefault);
	}
	return kATSUInvalidFontID;
}

struct postTable {
	Fixed	format;
	Fixed	italicAngle;
	SInt16	underlinePosition;
	SInt16	underlineThickness;
	UInt16	isFixedPitch;
	UInt16	reserved;
	UInt32	minMemType42;
	UInt32	maxMemType42;
	UInt32	minMemType1;
	UInt32	maxMemType1;
};

#include "appleGlyphNames.c"

#define	sfntCacheDir	"/Library/Caches/Type1-sfnt-fonts/"
#define	sfntWrapCommand	"T1Wrap"
#define sfntWrapSuffix	"-sfnt.otf"

static ATSFontRef
activatePFB(const char* pfbName)
{
	ATSFontRef fontRef = kATSUInvalidFontID;
	OSStatus	status = noErr;

	static int firstTime = 1;
	if (firstTime) {
		firstTime = 0;
		status = mkdir(sfntCacheDir, S_IRWXU | S_IRWXG | S_IRWXO);
		if (status != 0) {
			if (errno == EEXIST) {
				// clean up possible residue from past failed conversions
				system("/usr/bin/find " sfntCacheDir " -maxdepth 1 -empty -type f -delete");
				status = 0;
			}
			else
				fprintf(stderr, "*** failed to create sfnt cache directory %s (errno = %d), cannot activate .pfb fonts\n",
						sfntCacheDir, errno);
		}
	}
	
	char*	sfntName = new char[strlen(sfntCacheDir) + strlen(pfbName) + strlen(sfntWrapSuffix) + 1];
	strcpy(sfntName, sfntCacheDir);
	strcat(sfntName, pfbName);
	strcat(sfntName, sfntWrapSuffix);

	FSRef	ref;
	status = FSPathMakeRef((const UInt8*)sfntName, &ref, NULL);
	if (status == fnfErr) {
		char*	pathName = (char*)kpse_find_file((UInt8*)pfbName, xdv_kpse_pfb_format, true);
		if (pathName != NULL) {
			char* cmd = new char[strlen(sfntWrapCommand) + strlen(pathName) + strlen(sfntName) + 6];
			strcpy(cmd, sfntWrapCommand " ");
			strcat(cmd, pathName);
			strcat(cmd, " > ");
			strcat(cmd, sfntName);
			status = system(cmd);
			delete[] cmd;
			free(pathName);
		}
	}
	
	if (status == noErr)
		fontRef = activateFromPath(sfntName);

	delete[] sfntName;

	if (fontRef == kATSUInvalidFontID) {
		// try removing extension (.pfb) and looking for an .otf font...
		char*	baseName = new char[strlen(pfbName) + 1];
		strcpy(baseName, pfbName);
		char*	dot = strrchr(baseName, '.');
		if (dot != NULL)
			*dot = 0;
		char*	pathName = (char*)kpse_find_file((UInt8*)baseName, xdv_kpse_otf_format, true);
		delete[] baseName;
		if (pathName != NULL) {
			fontRef = activateFromPath(pathName);
			free(pathName);
		}
	}
	
	if (fontRef == kATSUInvalidFontID)
		fprintf(stderr, "\n*** font activation failed (status=%d): %s\n", status, pfbName);

	return fontRef;
}

static std::vector<UInt16>*
readMacRomanCmap(ATSFontRef fontRef)
{
	std::vector<UInt16>*	cmap = NULL;
	ByteCount	size;
	OSStatus	status = ATSFontGetTable(fontRef, 'cmap', 0, 0, 0, &size);
	if (status == noErr) {
		UInt8*	buffer = new UInt8[size];
		ATSFontGetTable(fontRef, 'cmap', 0, size, buffer, &size);

		struct cmapHeader {
			UInt16	version;
			UInt16	numSubtables;
		};
		struct subtableHeader {
			UInt16	platform;
			UInt16	encoding;
			UInt32	offset;
		};
		struct format0 {
			UInt16	format;
			UInt16	length;
			UInt16	language;
			UInt8	glyphIndex[256];
		};
		struct format6 {
			UInt16	format;
			UInt16	length;
			UInt16	language;
			UInt16	firstCode;
			UInt16	entryCount;
			UInt16	glyphIndex[1];
		};
		
		struct cmapHeader*	h = (struct cmapHeader*)buffer;
		struct subtableHeader*	sh = (struct subtableHeader*)(buffer + sizeof(struct cmapHeader));
		int	subtable = 0;
		cmap = new std::vector<UInt16>;
		cmap->reserve(256);
		while (subtable < h->numSubtables) {
			if ((sh->platform == 1) && (sh->encoding == 0)) {
				struct format0*	f0 = (struct format0*)(buffer + sh->offset);
				if (f0->format == 0) {
					for (int ch = 0; ch < 256; ++ch) {
						cmap->push_back(f0->glyphIndex[ch]);
					}
				}
				else if (f0->format == 6) {
					struct format6*	f6 = (struct format6*)f0;
					for (int ch = 0; ch < 256; ++ch) {
						if ((ch < f6->firstCode) || (ch >= f6->firstCode + f6->entryCount))
							cmap->push_back(0);
						else
							cmap->push_back(f6->glyphIndex[ch - f6->firstCode]);
					}
				}
				else {
					// unsupported cmap subtable format
					fprintf(stderr, "\n*** unsupported 'cmap' subtable format (%d)\n", f0->format);
				}
				break;
			}
			++subtable;
			++sh;
		}
		
		delete[] buffer;
	}
	
	return cmap;
}

static const fontMapRec*
getFontRec(const std::string& name)
{
	if (!sMapFileLoaded)
		doPdfMapFile("psfonts.map");

	std::map<std::string,fontMapRec>::iterator	i = psFontsMap.find(name);
	if (i == psFontsMap.end())
		return NULL;
	if (i->second.loaded)
		return &(i->second);

	fontMapRec	fr = i->second;

	ATSFontRef	fontRef = kATSUInvalidFontID;
	// if a filename is known, try to find and activate it

	if (fr.pfbName.length() > 0)
		fontRef = activatePFB(fr.pfbName.c_str());

	// if no downloadable file, see if it's installed in the OS
	if (fontRef == kATSUInvalidFontID)
	    fontRef = ATSFontFindFromPostScriptName(CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, fr.psName.c_str(),
    	                                        CFStringGetSystemEncoding(), kCFAllocatorNull), kATSOptionFlagsDefault);

	if (fontRef == kATSUInvalidFontID)
		fprintf(stderr, "\n*** font %s (%s: file '%s') not found\n", name.c_str(), fr.psName.c_str(), fr.pfbName.c_str());

	if (fontRef != kATSUInvalidFontID) {
		// if re-encoding was called for, load the encoding vector and build a new cmap
		if (fr.encName.length() > 0) {
			const encoding* enc = getEncoding(fr.encName);
			if (enc != 0) {
				ByteCount	size;
				OSStatus	status = ATSFontGetTable(fontRef, 'post', 0, 0, 0, &size);
				if (status == noErr) {
					UInt8*	buffer = new UInt8[size];
					ATSFontGetTable(fontRef, 'post', 0, size, buffer, &size);
					postTable*	p = (postTable*)&buffer[0];
					std::map<std::string,UInt16>	name2gid;
					UInt16	g = 0;
					switch (p->format) {
						case 0x00010000:
							{
								char*	cp;
								while ((cp = appleGlyphNames[g]) != 0) {
									name2gid[cp] = g;
									++g;
								}
							}
							break;
						
						case 0x00020000:
							{
								UInt16*	n = (UInt16*)(p + 1);
								UInt16	numGlyphs = *n++;
								UInt8*	ps = (UInt8*)(n + numGlyphs);
								std::vector<std::string>	newNames;
								while (ps < buffer + size) {
									newNames.push_back(std::string((char*)ps + 1, *ps));
									ps += *ps + 1;
								}
								for (g = 0; g < numGlyphs; ++g) {
									if (*n < 258)
										name2gid[appleGlyphNames[*n]] = g;
									else
										name2gid[newNames[*n - 258]] = g;
									++n;
								}
							}
							break;
						
						case 0x00028000:
							fprintf(stderr, "\n*** format 2.5 'post' table not supported\n");
							break;
						
						case 0x00030000:
							// TODO: see if it's a CFF OpenType font, and if so, get the glyph names from the CFF data
							fprintf(stderr, "\n*** format 3 'post' table; cannot reencode font %s\n", name.c_str());
							break;
						
						case 0x00040000:
							fprintf(stderr, "\n*** format 4 'post' table not supported\n");
							break;
						
						default:
							fprintf(stderr, "\n*** unknown 'post' table format %08x\n");
							break;
					}
					if (fr.cmap != NULL)
						delete fr.cmap;
					fr.cmap = new std::vector<UInt16>;
					for (encoding::const_iterator i = enc->begin(); i != enc->end(); ++i) {
						std::map<std::string,UInt16>::const_iterator	g = name2gid.find(*i);
						if (g == name2gid.end())
							fr.cmap->push_back(0);
						else
							fr.cmap->push_back(g->second);
					}
				}
				else {
					fprintf(stderr, "\n*** no 'post' table found, unable to re-encode font %s\n", name.c_str());
				}
			}
		}
		else {
			// read the MacOS/Roman cmap, if available
			std::vector<UInt16>*	cmap = readMacRomanCmap(fontRef);
			if (fr.cmap != NULL)
				delete fr.cmap;
			fr.cmap = cmap;
		}
	}
	
	if (fontRef == kATSUInvalidFontID) {
		fprintf(stderr, "\n*** font %s (%s) not found, will substitute Helvetica glyphs\n", name.c_str(), fr.pfbName.c_str());
		fontRef = ATSFontFindFromPostScriptName(CFSTR("Helvetica"), kATSOptionFlagsDefault);
		if (fr.cmap != NULL)
			delete fr.cmap;
		fr.cmap = readMacRomanCmap(fontRef);
	}

	fr.cgFont = getCGFontForATSFont(fontRef);
	fr.loaded = true;
	
	psFontsMap[name] = fr;
	return &(psFontsMap[name]);
}

static void
do_font_def(FILE* xdv, int k)
{
    OSStatus	status;

    UInt32	f = read_unsigned(xdv, k);	// font number we're defining
    UInt32	c = read_unsigned(xdv, 4);	// TFM checksum
    Fixed	s = read_unsigned(xdv, 4);	// at size
    Fixed	d = read_unsigned(xdv, 4);	// design size
    
    UInt16	alen = read_unsigned(xdv, 1);	// area length
    UInt16	nlen = read_unsigned(xdv, 1);	// name length

    UInt8*	name = new UInt8[alen + nlen + 2];
    if (alen > 0) {
        fread(name, 1, alen, xdv);
        name[alen] = '/';
        fread(name + alen + 1, 1, nlen, xdv);
        nlen += alen + 1;
    }
    else
        fread(name, 1, nlen, xdv);
    name[nlen] = 0;

    TEX_FONT	font;
    load_metrics(font, name, d, s);

	if (alen > 0)
		name = name + alen + 1;	// point past the area to get the name by itself for the remaining operations

    ATSFontRef	fontRef;

	// look for a map entry for this font name
	std::string	nameStr((char*)name);
	const fontMapRec*	fr = getFontRec(nameStr);
	if (fr != NULL) {
		font.cgFont = fr->cgFont;
		if (fr->cmap->size() > 0) {
			if (font.charMap != NULL)
				delete font.charMap;
			font.charMap = fr->cmap;	// replacing map that was synthesized from the tfm coverage
		}
	}
	else {
		/* try to find the font without the benefit of psfonts.map...
			first try the name as a pfb or otf filename
			and then as the PS name of an installed font
		*/
		
		// ****** FIXME ****** this needs re-working to read the 'cmap' properly
		
		fontRef = ATSFontFindFromPostScriptName(CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, (char*)name,
												CFStringGetSystemEncoding(), kCFAllocatorNull), kATSOptionFlagsDefault);
		UInt8*	ucname = 0;
		if (fontRef == kATSUInvalidFontID) {
			ucname = uppercasify(name);
			fontRef = ATSFontFindFromPostScriptName(CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, (char*)ucname,
												CFStringGetSystemEncoding(), kCFAllocatorNull), kATSOptionFlagsDefault);
		}
		
		if (ucname != 0)
			free(ucname);

		if (fontRef == kATSUInvalidFontID) {
			fprintf(stderr, "\n*** font %s not found in psfonts.map or host system; will substitute Helvetica glyphs\n", (char*)name);
			fontRef = ATSFontFindFromPostScriptName(CFSTR("Helvetica"), kATSOptionFlagsDefault);
			if (font.charMap != NULL)
				delete font.charMap;
			font.charMap = readMacRomanCmap(fontRef);
		}
		
		font.cgFont = getCGFontForATSFont(fontRef);

		delete[] name;
	}

    font.size = s;
    tex_fonts.insert(std::pair<const UInt32,TEX_FONT>(f, font));
}

inline bool operator==(const ATSURGBAlphaColor& a, const ATSURGBAlphaColor& b)
{
	return (a.red == b.red
		&& a.green == b.green
		&& a.blue == b.blue
		&& a.alpha == b.alpha);
}

static void
do_native_font_def(FILE* xdv)
{
	static ATSURGBAlphaColor	sRed = { 1.0, 0.0, 0.0, 1.0 };

    UInt32	f = read_unsigned(xdv, 4);	// font ID
    Fixed	s = read_unsigned(xdv, 4);	// size
    if (mag != 1000)
    	s = X2Fix(mag_scale * Fix2X(s));
	UInt16	technologyFlag = read_unsigned(xdv, 2);
	if (technologyFlag == aat_font_flag) {
		// AAT font
		ATSUStyle	style;
		OSStatus	status = ATSUCreateStyle(&style);
		int		n = read_unsigned(xdv, 2);	// number of features
		if (n > 0) {
			UInt16*	types = new UInt16[n];
			UInt16*	selectors = new UInt16[n];
			fread(types, 2, n, xdv);
			fread(selectors, 2, n, xdv);
			status = ATSUSetFontFeatures(style, n, types, selectors);
			delete[] selectors;
			delete[] types;
		}
		n = read_unsigned(xdv, 2); // number of variations
		if (n > 0) {
			UInt32*	axes = new UInt32[n];
			SInt32*	values = new SInt32[n];
			fread(axes, 4, n , xdv);
			fread(values, 4, n, xdv);
			status = ATSUSetVariations(style, n, axes, values);
			delete[] values;
			delete[] axes;
		}
		ATSURGBAlphaColor	rgba;
		fread(&rgba, 1, sizeof(ATSURGBAlphaColor), xdv);
	
		n = read_unsigned(xdv, 2);	// name length
		char*	name = new char[n+1];
		fread(name, 1, n, xdv);
		name[n] = 0;
	
		ATSUFontID	fontID;
		status = ATSUFindFontFromName(name, n, kFontPostscriptName,
			kFontNoPlatformCode, kFontNoScriptCode, kFontNoLanguageCode, &fontID);
	
		delete[] name;
		
		int numTags = 4;
		ATSUAttributeTag		tag[4] = { kATSUHangingInhibitFactorTag, kATSUSizeTag, kATSURGBAlphaColorTag, kATSUFontTag };
		ByteCount				valueSize[4] = { sizeof(Fract), sizeof(Fixed), sizeof(ATSURGBAlphaColor), sizeof(ATSUFontID) };
		ATSUAttributeValuePtr	value[4];
		Fract					inhibit = fract1;
		value[0] = &inhibit;
		value[1] = &s;
		value[2] = &rgba;
		value[3] = &fontID;
		if (fontID == kATSUInvalidFontID) {
			value[2] = &sRed;
			numTags--;
		}
		status = ATSUSetAttributes(style, numTags, &tag[0], &valueSize[0], &value[0]);
	
		NATIVE_FONT	fontRec;
		fontRec.atsuStyle = style;
		fontRec.cgFont = getCGFontForATSFont(FMGetATSFontRefFromFont(fontID));
		fontRec.size = s;
		fontRec.color = rgba;
		fontRec.isColored = !(rgba == constBlackColor);
		native_fonts.insert(std::pair<const UInt32,NATIVE_FONT>(f, fontRec));
	}
	else {
		// OT font
		ATSURGBAlphaColor	rgba;
		fread(&rgba, 1, sizeof(ATSURGBAlphaColor), xdv);

		int	n = read_unsigned(xdv, 2);	// name length
		char*	name = new char[n+1];
		fread(name, 1, n, xdv);
		name[n] = 0;

		ATSUFontID	fontID;
		OSStatus	status = ATSUFindFontFromName(name, n, kFontPostscriptName,
			kFontNoPlatformCode, kFontNoScriptCode, kFontNoLanguageCode, &fontID);
		
		if (fontID == kATSUInvalidFontID)
			rgba = sRed;
		
		NATIVE_FONT	fontRec;
		fontRec.cgFont = getCGFontForATSFont(FMGetATSFontRefFromFont(fontID));
		fontRec.size = s;
		fontRec.color = rgba;
		fontRec.isColored = !(rgba == constBlackColor);
		native_fonts.insert(std::pair<const UInt32,NATIVE_FONT>(f, fontRec));
	}
}

static	CGRect			mediaBox;

static float
readFloat(const char*& cp)
{
	float	result = 0.0;
	bool	negate = false;
	if (*cp == '-') {
		negate = true;
		++cp;
	}
	while (*cp >= '0' && *cp <= '9')
		result = result * 10 + *cp++ - '0';
	if (*cp == '.') {
		++cp;
		float	dec = 1.0;
		while (*cp >= '0' && *cp <= '9')
			result = result + (dec /= 10.0) * (*cp++ - '0');
	}
	if (negate)
		result = -result;
	return result;
}

static void	doPDFspecial(const char* special);

static void
flush_annot_box()
{
	char	buf[200];
	sprintf(buf, "ABOX [%f %f %f %f]",
					dvi2scr * annot_box.llx + 72.0,
					dvi2scr * (pageHt - annot_box.lly) - 72.0,
					dvi2scr * annot_box.urx + 72.0,
					dvi2scr * (pageHt - annot_box.ury) - 72.0);
	doPDFspecial(buf);
	init_annot_box();
}

static void
doPDFspecial(const char* special)
{
	if (pdfmarks == NULL) {
		static bool firstTime = true;
		if (firstTime) {
			firstTime = false;
			CFURLRef	markURL = CFURLCreateCopyAppendingPathExtension(kCFAllocatorDefault, saveURL, CFSTR("marks"));
			Boolean	gotPath = CFURLGetFileSystemRepresentation(markURL, true, (UInt8*)pdfmarkPath, _POSIX_PATH_MAX);
			if (gotPath) {
				pdfmarks = fopen(pdfmarkPath, "w");
				if (pdfmarks == NULL)
					fprintf(stderr, "*** unable to write to PDFmark file \"%s\"\n", pdfmarkPath);
			}
			else
				fprintf(stderr, "*** unable to get pathname for PDFmark file\n");
		}
	}
	if (pdfmarks != NULL) {
		if (strncmp(special, "bann", 4) == 0) {
			tag_level = dvi_level;
			init_annot_box();
			tracking_boxes = true;
		}
		else if (strncmp(special, "eann", 4) == 0) {
			flush_annot_box();
			tracking_boxes = false;
			tag_level = -1;
		}
		fprintf(pdfmarks, "%d\t%f\t%f\t%s\n",
			page_index, dvi2scr * dvi.h + 72.0, dvi2scr * (pageHt - dvi.v) - 72.0, special);
	}
}

static void
pushTextColor()
{
	textColorStack.push_back(textColor);
}

static void
popTextColor()
{
	if (textColorStack.size() == 0) {
		fprintf(stderr, "\n*** text color stack underflow\n");
	}
	else {
		textColor = textColorStack.back();
		textColorStack.pop_back();
	}
}

static void
pushRuleColor()
{
	ruleColorStack.push_back(ruleColor);
}

static void
popRuleColor()
{
	if (ruleColorStack.size() == 0) {
		fprintf(stderr, "\n*** rule color stack underflow\n");
	}
	else {
		ruleColor = ruleColorStack.back();
		ruleColorStack.pop_back();
	}
}

static void
setRuleColor(const ATSURGBAlphaColor& color)
{
	ruleColor.color = color;
	ruleColor.override = true;
}

static void
resetRuleColor()
{
	ruleColor.color = constBlackColor;
	ruleColor.override = false;
}

static void
setTextColor(const ATSURGBAlphaColor& color)
{
	textColor.color = color;
	textColor.override = true;
}

static void
resetTextColor()
{
	textColor.color = constBlackColor;
	textColor.override = false;
}

static bool
readColorValues(const char*& s, int n, float* v)
{
	while (n-- > 0) {
		while (*s == ' ')
			++s;
		if ( !( ( (*s >= '0') && (*s <= '9') )		// unless we have a digit
				|| (*s == '.') || (*s == ',') ) )	// or decimal sign, break
			return false;
		float val = 0.0;
		while ((*s >= '0') && (*s <= '9')) {
			val = val * 10.0 + *s - '0';
			++s;
		}
		if ((*s == '.') || (*s == ',')) {
			++s;
			float	dec = 10.0;
			while ((*s >= '0') && (*s <= '9')) {
				val += (*s - '0') / dec;
				dec *= 10;
				++s;
			}
		}
		*v++ = val;
	}
	return true;
}

struct color_by_name {
  char *name;
  float cmyk[4];
} colors_by_name[] = {
  {"GreenYellow", {0.15, 0, 0.69, 0}},
  {"Yellow", {0, 0, 1, 0}},
  {"Goldenrod", {0, 0.10, 0.84, 0}},
  {"Dandelion", {0, 0.29, 0.84, 0}},
  {"Apricot", {0, 0.32, 0.52, 0}},
  {"Peach", {0, 0.50, 0.70, 0}},
  {"Melon", {0, 0.46, 0.50, 0}},
  {"YellowOrange", {0, 0.42, 1, 0}},
  {"Orange", {0, 0.61, 0.87, 0}},
  {"BurntOrange", {0, 0.51, 1, 0}},
  {"Bittersweet", {0, 0.75, 1, 0.24}},
  {"RedOrange", {0, 0.77, 0.87, 0}},
  {"Mahogany", {0, 0.85, 0.87, 0.35}},
  {"Maroon", {0, 0.87, 0.68, 0.32}},
  {"BrickRed", {0, 0.89, 0.94, 0.28}},
  {"Red", {0, 1, 1, 0}},
  {"OrangeRed", {0, 1, 0.50, 0}},
  {"RubineRed", {0, 1, 0.13, 0}},
  {"WildStrawberry", {0, 0.96, 0.39, 0}},
  {"Salmon", {0, 0.53, 0.38, 0}},
  {"CarnationPink", {0, 0.63, 0, 0}},
  {"Magenta", {0, 1, 0, 0}},
  {"VioletRed", {0, 0.81, 0, 0}},
  {"Rhodamine", {0, 0.82, 0, 0}},
  {"Mulberry", {0.34, 0.90, 0, 0.02}},
  {"RedViolet", {0.07, 0.90, 0, 0.34}},
  {"Fuchsia", {0.47, 0.91, 0, 0.08}},
  {"Lavender", {0, 0.48, 0, 0}},
  {"Thistle", {0.12, 0.59, 0, 0}},
  {"Orchid", {0.32, 0.64, 0, 0}},
  {"DarkOrchid", {0.40, 0.80, 0.20, 0}},
  {"Purple", {0.45, 0.86, 0, 0}},
  {"Plum", {0.50, 1, 0, 0}},
  {"Violet", {0.79, 0.88, 0, 0}},
  {"RoyalPurple", {0.75, 0.90, 0, 0}},
  {"BlueViolet", {0.86, 0.91, 0, 0.04}},
  {"Periwinkle", {0.57, 0.55, 0, 0}},
  {"CadetBlue", {0.62, 0.57, 0.23, 0}},
  {"CornflowerBlue", {0.65, 0.13, 0, 0}},
  {"MidnightBlue", {0.98, 0.13, 0, 0.43}},
  {"NavyBlue", {0.94, 0.54, 0, 0}},
  {"RoyalBlue", {1, 0.50, 0, 0}},
  {"Blue", {1, 1, 0, 0}},
  {"Cerulean", {0.94, 0.11, 0, 0}},
  {"Cyan", {1, 0, 0, 0}},
  {"ProcessBlue", {0.96, 0, 0, 0}},
  {"SkyBlue", {0.62, 0, 0.12, 0}},
  {"Turquoise", {0.85, 0, 0.20, 0}},
  {"TealBlue", {0.86, 0, 0.34, 0.02}},
  {"Aquamarine", {0.82, 0, 0.30, 0}},
  {"BlueGreen", {0.85, 0, 0.33, 0}},
  {"Emerald", {1, 0, 0.50, 0}},
  {"JungleGreen", {0.99, 0, 0.52, 0}},
  {"SeaGreen", {0.69, 0, 0.50, 0}},
  {"Green", {1, 0, 1, 0}},
  {"ForestGreen", {0.91, 0, 0.88, 0.12}},
  {"PineGreen", {0.92, 0, 0.59, 0.25}},
  {"LimeGreen", {0.50, 0, 1, 0}},
  {"YellowGreen", {0.44, 0, 0.74, 0}},
  {"SpringGreen", {0.26, 0, 0.76, 0}},
  {"OliveGreen", {0.64, 0, 0.95, 0.40}},
  {"RawSienna", {0, 0.72, 1, 0.45}},
  {"Sepia", {0, 0.83, 1, 0.70}},
  {"Brown", {0, 0.81, 1, 0.60}},
  {"Tan", {0.14, 0.42, 0.56, 0}},
  {"Gray", {0, 0, 0, 0.50}},
  {"Black", {0, 0, 0, 1}},
  {"White", {0, 0, 0, 0}}
};

inline bool
kwmatch(const char*& s, const char* k)
{
	while (*s == ' ')
		++s;
	int kwlen = strlen(k);
	if ( (s[kwlen] == 0) || (s[kwlen] == ' ') ) {
		if (strncmp(s, k, kwlen) == 0) {
			s += kwlen;
			return true;
		}
	}
	return false;
}

static float
readAlphaIfPresent(const char*& s)
{
	if (kwmatch(s, "alpha")) {
		float	alpha;
		if (readColorValues(s, 1, &alpha))
			return alpha;
	}
	return 1.0;
}

static void
doColorSpecial(const char* s)
{
	while (*s != 0) {
		if (kwmatch(s, "push")) {
			pushRuleColor();
			pushTextColor();
			continue;
		}
		
		if (kwmatch(s, "pop")) {
			popRuleColor();
			popTextColor();
			continue;
		}
		
		if (kwmatch(s, "rgb")) {
			float	rgb[3] = { 0.0, 0.0, 0.0 };
			if (readColorValues(s, 3, rgb)) {
				ATSURGBAlphaColor	c;
				c.red = rgb[0];
				c.green = rgb[1];
				c.blue = rgb[2];
				c.alpha = readAlphaIfPresent(s);
				setRuleColor(c);
				setTextColor(c);
			}
			break;
		}
		
		if (kwmatch(s, "cmyk")) {
			float	cmyk[4] = { 0.0, 0.0, 0.0, 1.0 };
			if (readColorValues(s, 4, cmyk)) {
				ATSURGBAlphaColor	c;
	
				c.red = (1.0 - cmyk[0]) * (1.0 - cmyk[3]);	// cmyk->rgb conversion a la ghostscript
				c.green = (1.0 - cmyk[1]) * (1.0 - cmyk[3]);
				c.blue = (1.0 - cmyk[2]) * (1.0 - cmyk[3]);
	/*
	#define min(a,b)	((a) < (b) ? (a) : (b))
				c.red = 1.0 - min(1.0, cmyk[0] + cmyk[3]);	// cmyk->rgb conversion a la adobe
				c.green = 1.0 - min(1.0, cmyk[1] + cmyk[3]);
				c.blue = 1.0 - min(1.0, cmyk[2] + cmyk[3]);
	*/
				c.alpha = readAlphaIfPresent(s);
				setRuleColor(c);
				setTextColor(c);
			}
			break;
		}
		
		if (kwmatch(s, "hsb") || kwmatch(s, "hsv")) {
			float	hsb[3] = { 0.0, 1.0, 0.0 };
			if (readColorValues(s, 3, hsb)) {
				CMColor	cm;
				cm.hsv.hue = (UInt16)(hsb[0] * 65535);
				cm.hsv.saturation = (UInt16)(hsb[1] * 65535);
				cm.hsv.value = (UInt16)(hsb[2] * 65535);
				CMConvertHSVToRGB(&cm, &cm, 1);
				ATSURGBAlphaColor	c;
				c.red = cm.rgb.red / 65535.0;
				c.green = cm.rgb.green / 65535.0;
				c.blue = cm.rgb.blue / 65535.0;
				c.alpha = readAlphaIfPresent(s);
				setRuleColor(c);
				setTextColor(c);
			}
			break;
		}
		
		if (kwmatch(s, "hls")) {
			float	hls[3] = { 0.0, 1.0, 0.0 };
			if (readColorValues(s, 3, hls)) {
				CMColor	cm;
				cm.hls.hue = (UInt16)(hls[0] * 65535);
				cm.hls.lightness = (UInt16)(hls[1] * 65535);
				cm.hls.saturation = (UInt16)(hls[2] * 65535);
				CMConvertHLSToRGB(&cm, &cm, 1);
				ATSURGBAlphaColor	c;
				c.red = cm.rgb.red / 65535.0;
				c.green = cm.rgb.green / 65535.0;
				c.blue = cm.rgb.blue / 65535.0;
				c.alpha = readAlphaIfPresent(s);
				setRuleColor(c);
				setTextColor(c);
			}
			break;
		}
		
		if (kwmatch(s, "gray")) {
			float	gray = 0.0;
			if (readColorValues(s, 1, &gray)) {
				ATSURGBAlphaColor	c;
				c.red = gray;
				c.green = gray;
				c.blue = gray;
				c.alpha = readAlphaIfPresent(s);
				setRuleColor(c);
				setTextColor(c);
			}
			break;
		}
		
		const color_by_name*	nc = colors_by_name + sizeof(colors_by_name) / sizeof(color_by_name);
		while (nc-- > colors_by_name) {
			if (kwmatch(s, nc->name)) {
				ATSURGBAlphaColor	c;
				c.red = (1.0 - nc->cmyk[0]) * (1.0 - nc->cmyk[3]);
				c.green = (1.0 - nc->cmyk[1]) * (1.0 - nc->cmyk[3]);
				c.blue = (1.0 - nc->cmyk[2]) * (1.0 - nc->cmyk[3]);
				c.alpha = readAlphaIfPresent(s);
				setRuleColor(c);
				setTextColor(c);
				break;
			}
		}
		break;		
	}
}

static bool
prefix_eq(const char* str, const char* prefix, const char*& remainder)
{
	int	len = strlen(prefix);
	if (strncmp(str, prefix, len) == 0) {
		remainder = str + len;
		return true;
	}
	return false;
}

inline bool
str_eq(const char* s, const char* t)
{
	return (strcmp(s, t) == 0);
}

static void
process_one_page(FILE* xdv)
{
	/* enter here having just read BOP from xdv file */
	++page_index;

	OSStatus		status;
	int				i;

	std::list<dvi_vars>	stack;
	dvi_level = 0;

	CGContextSaveGState(graphicsContext);
    CGContextBeginPage(graphicsContext, &mediaBox);

	static CGAffineTransform	initTextMatrix;
	if (page_index == 1)
		initTextMatrix = CGContextGetTextMatrix(graphicsContext);
	else
		CGContextSetTextMatrix(graphicsContext, initTextMatrix);

	CGColorSpaceRef	userColorSpace = 
		(&CGColorSpaceCreateWithName) != NULL
			? CGColorSpaceCreateWithName(kCGColorSpaceUserRGB)
			: CGColorSpaceCreateDeviceRGB();
	CGContextSetFillColorSpace(graphicsContext, userColorSpace);

	CGContextTranslateCTM(graphicsContext, 72.0, -72.0);

    pageWd = scr2dvi * mediaBox.size.width;
    pageHt = scr2dvi * mediaBox.size.height;
    
    int	j = 0;
    int	count[10];
    for (i = 0; i < 10; ++i) {	// read counts
		count[i] = read_signed(xdv, 4);
        if (count[i] != 0)
            j = i;
    }
	if (verbose) {
		fprintf(stderr, "[");
		for (i = 0; i < j; ++i)
			fprintf(stderr, "%d.", count[i]);
		fprintf(stderr, "%d", count[j]);
	}

    (void)read_unsigned(xdv, 4);	// skip prev-BOP pointer
	
	cur_f = kUndefinedFont;
	dvi.h = dvi.v = 0;
	dvi.w = dvi.x = dvi.y = dvi.z = 0;
	f = 0;
	
	unsigned int	cmd = DVI_BOP;
	while (cmd != DVI_EOP) {
		UInt32	u;
		SInt32	s;
		SInt32	ht, wd;
		int		k = 1;

		cmd = read_unsigned(xdv, 1);
		switch (cmd) {
			default:
				if (cmd < DVI_SET1)
					setchar(cmd, true);
				else if (cmd >= DVI_FNTNUM0 && cmd <= DVI_FNTNUM0 + 63)
					f = cmd - DVI_FNTNUM0;
				else
					goto ABORT_PAGE;
				break;
			
			case DVI_EOP:
				flushGlyphs();
				break;
			
			case DVI_NOP:
				break;
			
			case DVI_FNT4:
				++k;
			case DVI_FNT3:
				++k;
			case DVI_FNT2:
				++k;
			case DVI_FNT1:
				f = read_unsigned(xdv, k);	// that's it: just set |f|
				break;

			case DVI_XXX4:
				++k;
			case DVI_XXX3:
				++k;
			case DVI_XXX2:
				++k;
			case DVI_XXX1:
				flushGlyphs();
                u = read_unsigned(xdv, k);
                if (u > 0) {
                    char* special = new char[u+1];
                    fread(special, 1, u, xdv);
                    special[u] = '\0';
                    const char* specialArg;
                    
                    if (prefix_eq(special, "pdf:", specialArg))
                    	doPDFspecial(specialArg);
					
					else if (prefix_eq(special, "color ", specialArg))
						doColorSpecial(specialArg);
					
					else if (prefix_eq(special, "x:fontmapfile", specialArg))
						doPdfMapFile(specialArg);
					else if (prefix_eq(special, "x:fontmapline", specialArg))
						doPdfMapLine(specialArg, 0);
					
					else if (str_eq(special, "x:textcolorpush"))
						pushTextColor();
					else if (str_eq(special, "x:textcolorpop"))
						popTextColor();
					else if (str_eq(special, "x:rulecolorpush"))
						pushRuleColor();
					else if (str_eq(special, "x:rulecolorpop"))
						popRuleColor();

                    else if (prefix_eq(special, "x:rulecolor", specialArg)) {
                        // set rule color
                        if (*specialArg != '=' || u < 18)
                            // not long enough for a proper color spec, so set it to black
                            resetRuleColor();
                        else {
                        	++specialArg;
							ATSURGBAlphaColor	color;
                        	if (u < 20)
	                            readColorValue(specialArg, color);
							else
	                            readColorValueWithAlpha(specialArg, color);
                            setRuleColor(color);
                        }
                    }

                    else if (prefix_eq(special, "x:textcolor", specialArg)) {
                        // set text color (overriding color of the font style)
                        if (*specialArg != '=' || u < 18)
                        	resetTextColor();
                        else {
                        	++specialArg;
							ATSURGBAlphaColor	color;
                        	if (u < 20)
	                            readColorValue(specialArg, color);
                        	else
	                            readColorValueWithAlpha(specialArg, color);
                            setTextColor(color);
                        }
                    }

                    else if (str_eq(special, "x:gsave")) {
                    	CGContextSaveGState(graphicsContext);
                    }
                    else if (str_eq(special, "x:grestore")) {
                    	CGContextRestoreGState(graphicsContext);
						if (tex_fonts[cur_f].cgFont != NULL) {
							CGContextSetFont(graphicsContext, tex_fonts[cur_f].cgFont);
							CGContextSetFontSize(graphicsContext, Fix2X(tex_fonts[cur_f].size) * mag_scale);
						}
                    }
                    else if (prefix_eq(special, "x:scale", specialArg)) {
                    	while (*specialArg && *specialArg <= ' ')
                    		++specialArg;
                    	float	xscale = atof(specialArg);
                    	while (*specialArg && (((*specialArg >= '0') && (*specialArg <= '9')) || *specialArg == '.' || *specialArg == '-'))
                    		++specialArg;
                    	while (*specialArg && *specialArg <= ' ')
                    		++specialArg;
						float	yscale = atof(specialArg);
						if (xscale != 0.0 && yscale != 0.0) {
							CGContextTranslateCTM(graphicsContext, dvi2scr * dvi.h, dvi2scr * (pageHt - dvi.v));
	                    	CGContextScaleCTM(graphicsContext, xscale, yscale);
							CGContextTranslateCTM(graphicsContext, -dvi2scr * dvi.h, -dvi2scr * (pageHt - dvi.v));
						}
                    }
                    else if (prefix_eq(special, "x:rotate", specialArg)) {
                    	while (*specialArg && *specialArg <= ' ')
                    		++specialArg;
                    	float	rotation = atof(specialArg);
						CGContextTranslateCTM(graphicsContext, dvi2scr * dvi.h, dvi2scr * (pageHt - dvi.v));
                    	CGContextRotateCTM(graphicsContext, rotation * M_PI / 180.0);
						CGContextTranslateCTM(graphicsContext, -dvi2scr * dvi.h, -dvi2scr * (pageHt - dvi.v));
                    }
                    else if (prefix_eq(special, "x:backgroundcolor", specialArg)) {
                        if (*specialArg != '=' || u < 24) {
                        	// ignore incorrect special
                        }
                        else {
                        	++specialArg;
                        	ATSURGBAlphaColor	bg;
                        	if (u < 26)
	                            readColorValue(specialArg, bg);
	                        else
	                            readColorValueWithAlpha(specialArg, bg);
	                        if (bg.alpha == 0.0)
	                        	bg.alpha = 0.0001;	// avoid an apparent Quartz bug
							CGContextSaveGState(graphicsContext);
                        	CGContextSetFillColor(graphicsContext, &bg.red);
                        	CGContextTranslateCTM(graphicsContext, -72.0, 72.0);
		                    CGContextFillRect(graphicsContext, mediaBox);
							CGContextRestoreGState(graphicsContext);
                       }
                    }
                    else if (prefix_eq(special, "x:shadow", specialArg)) {
                    	// syntax: \special{x:shadow(x,y),b}
                    	do {
							CGSize	offset;
							float	blur;
							
							if (*specialArg++ != '(')	break;
							offset.width = readFloat(specialArg);
							
							if (*specialArg++ != ',')	break;
							offset.height = readFloat(specialArg);
							
							if (*specialArg++ != ')')	break;
							if (*specialArg++ != ',')	break;
							blur = readFloat(specialArg);
							
							// NB: API only available on 10.3 and later
							if (&CGContextSetShadow != NULL)
								CGContextSetShadow(graphicsContext, offset, blur);
						} while (false);
                    }
                    else if (prefix_eq(special, "x:colorshadow", specialArg)) {
                    	// syntax: \special{x:colorshadow(x,y),b,c}
                    	do {
							CGSize				offset;
							float				blur;
							ATSURGBAlphaColor	color;
							
							if (*specialArg++ != '(')	break;
							offset.width = readFloat(specialArg);
							
							if (*specialArg++ != ',')	break;
							offset.height = readFloat(specialArg);
							
							if (*specialArg++ != ')')	break;
							if (*specialArg++ != ',')	break;
							blur = readFloat(specialArg);
							
							if (*specialArg++ != ',')	break;
                            if (special + u < specialArg + 6)	break;
                            if (special + u < specialArg + 8)
	                            readColorValue(specialArg, color);
	                        else
	                       		readColorValueWithAlpha(specialArg, color);
                            
							// NB: APIs only available on 10.3 and later
							if (&CGContextSetShadowWithColor != NULL) {
								CGColorRef		colorRef = CGColorCreate(userColorSpace, &color.red);
								CGContextSetShadowWithColor(graphicsContext, offset, blur, colorRef);
								CGColorRelease(colorRef);
							}
						} while (false);
                    }
                    
                    delete[] special;
                }
				break;
			
			case DVI_SETRULE:
			case DVI_PUTRULE:
				ht = read_signed(xdv, 4);
				wd = read_signed(xdv, 4);
				if (ht > 0 && wd > 0) {
					CGRect	r = CGRectMake(dvi2scr * dvi.h, dvi2scr * (pageHt - dvi.v), dvi2scr * wd, dvi2scr * ht);
                    if (ruleColor.override) {
                    	CGContextSaveGState(graphicsContext);
                        CGContextSetFillColor(graphicsContext, &ruleColor.color.red);
					}
                    CGContextFillRect(graphicsContext, r);
                    if (ruleColor.override)
                        CGContextRestoreGState(graphicsContext);
                    if (tracking_boxes) {
						box	b = {
							dvi.h,
							dvi.v,
							dvi.h + wd,
							dvi.v - ht };
						merge_box(b);
                    }
				}
				if (cmd == DVI_SETRULE)
					dvi.h += wd;
				break;
			
			case DVI_SET4:
				++k;
			case DVI_SET3:
				++k;
			case DVI_SET2:
				++k;
			case DVI_SET1:
				u = read_unsigned(xdv, k);
				setchar(u, true);
				break;
			
			case DVI_PUT4:
				++k;
			case DVI_PUT3:
				++k;
			case DVI_PUT2:
				++k;
			case DVI_PUT1:
				u = read_unsigned(xdv, k);
				setchar(u, false);
				break;
			
			case DVI_PUSH:
				stack.push_back(dvi);
				++dvi_level;
				break;

			case DVI_POP:
				if (dvi_level == tag_level)
					flush_annot_box();
				--dvi_level;
				dvi = stack.back();
				stack.pop_back();
				break;
			
			case DVI_RIGHT4:
				++k;
			case DVI_RIGHT3:
				++k;
			case DVI_RIGHT2:
				++k;
			case DVI_RIGHT1:
				s = read_signed(xdv, k);
				dvi.h += s;
				break;
			
			case DVI_DOWN4:
				++k;
			case DVI_DOWN3:
				++k;
			case DVI_DOWN2:
				++k;
			case DVI_DOWN1:
				s = read_signed(xdv, k);
				dvi.v += s;
				break;
			
			case DVI_W4:
				++k;
			case DVI_W3:
				++k;
			case DVI_W2:
				++k;
			case DVI_W1:
				s = read_signed(xdv, k);
				dvi.w = s;
			case DVI_W0:
				dvi.h += dvi.w;
				break;
				
			case DVI_X4:
				++k;
			case DVI_X3:
				++k;
			case DVI_X2:
				++k;
			case DVI_X1:
				s = read_signed(xdv, k);
				dvi.x = s;
			case DVI_X0:
				dvi.h += dvi.x;
				break;
				
			case DVI_Y4:
				++k;
			case DVI_Y3:
				++k;
			case DVI_Y2:
				++k;
			case DVI_Y1:
				s = read_signed(xdv, k);
				dvi.y = s;
			case DVI_Y0:
				dvi.v += dvi.y;
				break;
				
			case DVI_Z4:
				++k;
			case DVI_Z3:
				++k;
			case DVI_Z2:
				++k;
			case DVI_Z1:
				s = read_signed(xdv, k);
				dvi.z = s;
			case DVI_Z0:
				dvi.v += dvi.z;
				break;
			
			case DVI_GLYPH:
				flushGlyphs();
				u = read_unsigned(xdv, 2);
				s = read_signed(xdv, 4);
				do_set_glyph(u, s);
				break;
			
			case DVI_NATIVE:
				flushGlyphs();
				do_set_native(xdv);
				break;
			
			case DVI_GLYPH_ARRAY:
				flushGlyphs();
				do_glyph_array(xdv);
				break;
			
            case DVI_PIC_FILE:
				flushGlyphs();
                do_pic_file(xdv, false);
                break;
                            
            case DVI_PDF_FILE:
				flushGlyphs();
                do_pic_file(xdv, true);
                break;
                        
			case DVI_FNTDEF4:
				++k;
			case DVI_FNTDEF3:
				++k;
			case DVI_FNTDEF2:
				++k;
			case DVI_FNTDEF1:
				do_font_def(xdv, k);
				break;
				
			case DVI_NATIVE_FONT_DEF:
                do_native_font_def(xdv);
				break;
		}
	}

	if (verbose) {
		static int	sPageIndex = 0;
		fprintf(stderr, "]%s", (++sPageIndex % 10) == 0 ? "\n" : "");
	}

ABORT_PAGE:
    
	CGColorSpaceRelease(userColorSpace);

    CGContextEndPage(graphicsContext);
    CGContextRestoreGState(graphicsContext);
}

static void
process_pages(FILE* xdv)
{
	page_index = 0;
	unsigned int	cmd;
	while ((cmd = read_unsigned(xdv, 1)) != DVI_POST) {
		switch (cmd) {
			case DVI_BOP:
				process_one_page(xdv);
				break;
				
			case DVI_NOP:
				break;
			
			default:
				fprintf(stderr, "\n*** unexpected DVI command: %d\n", cmd);
                exit(1);
		}
	}
}

static double_t
readDimen(const char* arg)	// currently reads unsigned dimens only; no negative paper sizes!
{
	double_t	rval = 0.0;
	while ((*arg >= '0') && (*arg <= '9')) {
		rval *= 10.0;
		rval += *arg - '0';
		++arg;
	}
	if (*arg == '.') {
		++arg;
		double_t	frac = 10.0;
		while ((*arg >= '0') && (*arg <= '9')) {
			rval += (*arg - '0') / frac;
			frac *= 10.0;
			++arg;
		}
	}
	const dimenrec*	dim = &dimenratios[0];
	while (dim->name != 0) {
		if (strcasecmp(arg, dim->name) == 0) {
			rval *= dim->factor;
			break;
		}
		++dim;
	}
	return rval;
}

static bool
setPaperSize(const char* arg)
{
	pageHt = pageWd = 0.0;

	char*   s1 = strdup(arg);
	char*   s2 = strchr(s1, ':');
	if (s2 != NULL)
		*s2++ = 0;

	const papersizerec*	ps = &papersizes[0];
	while (ps->name != 0) {
		if (strcasecmp(s1, ps->name) == 0) {
			pageWd = ps->width;
			pageHt = ps->height;
			break;
		}
		++ps;
	}
	if (ps->name == 0) {
		char*   s3 = strchr(s1, ',');
		if (s3 != NULL) {
			*s3++ = 0;
			pageWd = readDimen(s1);
			pageHt = readDimen(s3);
		}
	}
	if (s2 != NULL && strcasecmp(s2, "landscape") == 0) {
		double_t  t = pageHt;
		pageHt = pageWd;
		pageWd = t;
	}
	
	free(s1);
	
	return (pageHt > 0.0) && (pageWd > 0.0);
}

const char* progName;
static void
usage()
{
    fprintf(stderr, "usage: %s [-m mag] [-p papersize[:landscape]] [-v] [-o pdfFile] xdvFile\n", progName);
	fprintf(stderr, "    papersize values: ");
	papersizerec*	ps = &papersizes[0];
	while (ps->name != 0) {
		fprintf(stderr, "%s/", ps->name);
		++ps;
	}
	fprintf(stderr, "wd,ht [in 'big' points or with explicit units]\n");
}

extern "C" {
	int	xdv2pdf(int argc, char** argv);
};

int
xdv2pdf(int argc, char** argv)
{
	OSStatus			status;
    
    progName = argv[0];
    
    int	ch;
    while ((ch = getopt(argc, argv, "o:p:m:d:hv" /*r:*/)) != -1) {
        switch (ch) {
            case 'o':
                {
                    CFStringRef	outFileName = CFStringCreateWithCString(kCFAllocatorDefault, optarg, kCFStringEncodingUTF8);
                    saveURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, outFileName, kCFURLPOSIXPathStyle, NULL);
                    CFRelease(outFileName);
                }
                break;
            
            case 'p':
				if (!setPaperSize(optarg)) {
                    fprintf(stderr, "*** unknown paper name: %s\n", optarg);
                    exit(1);
                }
                break;
                
            case 'm':
            	mag = atoi(optarg);
            	break;
            
            case 'd':
            	kpathsea_debug |= atoi(optarg);
            	break;
            
            case 'v':
            	verbose = true;
            	break;
            
            case 'h':
                usage();
                exit(0);
        }
    }

    // get default paper size from printing system
    PMRect				paperRect = { 0, 0, 792, 612 };
	PMPrintSession		printSession;
	PMPrintSettings		printSettings;
	PMPageFormat		pageFormat;
    status = PMCreateSession(&printSession);
    if (status == noErr) {
        status = PMCreatePrintSettings(&printSettings);
        if (status == noErr) {
            status = PMSessionDefaultPrintSettings(printSession, printSettings);
            status = PMCreatePageFormat(&pageFormat);
            if (status == noErr) {
                status = PMSessionDefaultPageFormat(printSession, pageFormat);
                PMGetUnadjustedPaperRect(pageFormat, &paperRect);
                status = PMRelease(pageFormat);
            }
            status = PMRelease(printSettings);
        }
        status = PMRelease(printSession);
    }

    // modify page size if specific request passed
    if (pageHt != 0)
        paperRect.bottom = paperRect.top + pageHt;
    if (pageWd != 0)
        paperRect.right = paperRect.left + pageWd;
    
    // set the media box for PDF generation
    mediaBox = CGRectMake(0, 0, paperRect.right - paperRect.left, paperRect.bottom - paperRect.top);
    
    argc -= optind;
    argv += optind;
    if (argc == 1 && saveURL == 0) {
        CFStringRef	inFileName = CFStringCreateWithCString(kCFAllocatorDefault, argv[0], CFStringGetSystemEncoding());
        CFURLRef	inURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inFileName, kCFURLPOSIXPathStyle, NULL);
        CFRelease(inFileName);
        CFURLRef	tmpURL = CFURLCreateCopyDeletingPathExtension(kCFAllocatorDefault, inURL);
        CFRelease(inURL);
        saveURL = CFURLCreateCopyAppendingPathExtension(kCFAllocatorDefault, tmpURL, CFSTR("pdf"));
        CFRelease(tmpURL);
    }

    if (argc > 1 || saveURL == 0) {
        usage();
        exit(1);
    }
    
	ATSUCreateTextLayout(&layout);
//	ATSUFontFallbacks fallbacks;
//	status = ATSUCreateFontFallbacks(&fallbacks);
//	status = ATSUSetObjFontFallbacks(fallbacks, 0, 0, /*kATSULastResortOnlyFallback*/kATSUDefaultFontFallbacks);
//	ATSUAttributeTag		tag = kATSULineFontFallbacksTag;
//	ByteCount				valueSize = sizeof(fallbacks);
//	ATSUAttributeValuePtr	value = &fallbacks;
//	status = ATSUSetLayoutControls(layout, 1, &tag, &valueSize, &value);
	status = ATSUSetTransientFontMatching(layout, true);

	FILE*	xdv;
    if (argc == 1)
        xdv = fopen(argv[0], "r");
    else
        xdv = stdin;

	if (xdv != NULL) {
		// read the preamble
		unsigned	b = read_unsigned(xdv, 1);
		if (b != DVI_PRE) { fprintf(stderr, "*** bad XDV file: DVI_PRE not found, b=%d\n", b); exit(1); }
		b = read_unsigned(xdv, 1);
		if (b != 4) { fprintf(stderr, "*** bad XDV file: version=%d, expected 4\n", b); exit(1); }
		(void)read_unsigned(xdv, 4);	// num
		(void)read_unsigned(xdv, 4);	// den
		if (mag == 0)
			mag = read_unsigned(xdv, 4);	// mag
		else
			(void)read_unsigned(xdv, 4);

		unsigned numBytes = read_unsigned(xdv, 1);	// length of output comment
        UInt8* bytes = new UInt8[numBytes];
        fread(bytes, 1, numBytes, xdv);

        CFStringRef	keys[2] =   { CFSTR("Creator") , CFSTR("Title") /*, CFSTR("Author")*/ };
        CFStringRef values[2] = { CFSTR("xdv2pdf") , 0 /*, 0*/ };
        values[1] = CFStringCreateWithBytes(kCFAllocatorDefault, bytes, numBytes, CFStringGetSystemEncoding(), false);
        delete[] bytes;
//        values[2] = CFStringCreateWithCString(kCFAllocatorDefault, getlogin(), CFStringGetSystemEncoding());
        CFDictionaryRef	auxInfo = CFDictionaryCreate(kCFAllocatorDefault, (const void**)keys, (const void**)values, sizeof(keys) / sizeof(keys[0]),
                &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFRelease(values[1]);
//        CFRelease(values[2]);

        // create PDF graphics context
        graphicsContext = CGPDFContextCreateWithURL(saveURL, &mediaBox, auxInfo);

		// set up the TextLayout to use this graphics context
		ByteCount				iSize = sizeof(CGContextRef);
		ATSUAttributeTag		iTag = kATSUCGContextTag;
		ATSUAttributeValuePtr	iValuePtr = &graphicsContext;
		ATSUSetLayoutControls(layout, 1, &iTag, &iSize, &iValuePtr); 
	
		// handle magnification
		if (mag != 1000) {
			mag_scale = mag / 1000.0;
			scr2dvi /= mag_scale;
			dvi2scr *= mag_scale;
			CGContextScaleCTM(graphicsContext, mag_scale, mag_scale);
		}

		// draw all the pages
        process_pages(xdv);

        CGContextRelease(graphicsContext);

		while (getc(xdv) != EOF)
			;
		fclose(xdv);
	}
	
    ATSUDisposeTextLayout(layout);
    
	if (pdfmarks != NULL) {
		fclose(pdfmarks);
		char	pdfPath[_POSIX_PATH_MAX+1];
		Boolean	gotPath = CFURLGetFileSystemRepresentation(saveURL, true, (UInt8*)pdfPath, _POSIX_PATH_MAX);
		CFRelease(saveURL);
#if 0
		if (gotPath) {
			char*	mergeMarks = "xdv2pdf_mergemarks";
			execlp(mergeMarks, mergeMarks, pdfPath, pdfmarkPath, 0);	// should not return!
			status = errno;
		}
		fprintf(stderr, "*** failed to run xdv2pdf_mergemarks: status = %d\n", status);
#else
		if (gotPath) {
			char*	mergeMarks = "xdv2pdf_mergemarks";
			char	cmd[_POSIX_PATH_MAX*2 + 100];
			sprintf(cmd, "%s \"%s\" \"%s\"", mergeMarks, pdfPath, pdfmarkPath);
			status = system(cmd);
		}
		else
			status = fnfErr;
		if (status != 0)
			fprintf(stderr, "*** failed to run xdv2pdf_mergemarks: status = %d\n", status);
#endif
	}
	else
		CFRelease(saveURL);

	return status;
}
