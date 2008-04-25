/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2006 by SIL International
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

/* additional declarations we want to slip in for xetex */

#define	native_node_size	6
#define native_node_text(p)	((unsigned short*)(&(mem[(p) + native_node_size])))

#define getnativechar(p,i)		native_node_text(p)[i]
#define setnativechar(p,i,v)	native_node_text(p)[i] = v

/* p is native_word node; g is XeTeX_use_glyph_metrics flag */
#define setnativemetrics(p,g)					measure_native_node(&(mem[p]), g)

#define setnativeglyphmetrics(p,g)				measure_native_glyph(&(mem[p]), g)

#define setjustifiednativeglyphs(p)				store_justified_native_glyphs(&(mem[p]))

#define getnativeitaliccorrection(p)			get_native_ital_corr(&(mem[p]))
#define getnativeglyphitaliccorrection(p)		get_native_glyph_ital_corr(&(mem[p]))

#define getnativeglyph(p,i)						get_native_glyph_id(&(mem[p]), i)

#define makexdvglypharraydata(p)				makeXDVGlyphArrayData(&(mem[p]))
#define xdvbufferbyte(i)						xdvbuffer[i]

void* getotassemblyptr(int f, int g, int horiz); /* function in XeTeXOTMath.cpp */

#define pic_node_size		8

#define deref(p)				(*(p))

#define findpicfile(a,b,c,d)	find_pic_file(a, b, c, d)

#define picpathbyte(p,i)		((unsigned char*)&(mem[p+pic_node_size]))[i]

#define dviopenout(f)			open_dvi_output(&(f))

#define nullptr				(NULL)
#define glyphinfobyte(p,k)	((unsigned char*)p)[k]
#define casttoushort(x)		(unsigned short)(x)

/* easier to do the bit-twiddling here than in Pascal */
/* read fields from a 32-bit math code */
#define mathfamfield(x)     (((unsigned)(x) >> 24) & 0xFF)
#define mathclassfield(x)   (((unsigned)(x) >> 21) & 0x07)
#define mathcharfield(x)    ((unsigned)(x) & 0x1FFFFF)
/* calculate pieces to assign to a math code */
#define setfamilyfield(x)   (((unsigned)(x) & 0xFF) << 24)
#define setclassfield(x)    (((unsigned)(x) & 0x07) << 21)

/* Unicode file reading modes */
#define AUTO		0	/* default: will become one of 1..3 at file open time, after sniffing */
#define UTF8		1
#define UTF16BE		2
#define UTF16LE		3
#define RAW			4
#define ICUMAPPING	5

/* we don't use xchr, so change the cpascal.h definition of this... */
#undef Xchr
#define Xchr(x)		(x)

#ifdef XETEX_MAC
#include <Carbon/Carbon.h>	/* for Mac OS X, it's handy to have the Carbon APIs available */
#endif

#include "trans.h"	/* functions for affine transform operations */
#define Byte my_Byte /* hack to work around typedef conflict with zlib */
#include "TECkit_Common.h" /* include this before XeTeX_ext.h */
#undef Byte
#include "XeTeX_ext.h" /* other extension functions */
