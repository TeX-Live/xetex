/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

/* additional declarations we want to slip in for xetex */

#define	native_node_size	6
#define native_node_text(p)	((unsigned short*)(&(mem[(p) + native_node_size])))

#define getnativechar(p,i)		native_node_text(p)[i]
#define setnativechar(p,i,v)	native_node_text(p)[i] = v
#define setnativemetrics(p)		measure_native_node(&(mem[p]))
#define setnativeglyphmetrics(p)	measure_native_glyph(&(mem[p]))

#define pic_node_size		8

#define deref(p)				(*(p))

#define findpicfile(a,b,c,d)	find_pic_file(a, b, c, d)

#define picpathbyte(p,i)		((unsigned char*)&(mem[p+pic_node_size]))[i]

#define dviopenout(f)			open_dvi_output(&(f))

#define readcint1(x)		(x).cint1
#define setcint1(x,y)		(x).cint1 = (y)

#define delcode1(x)			readcint1(eqtb[xetexdelcodebase+(x)])
#define setdelcode1(x,y)	setcint1(eqtb[xetexdelcodebase+(x)],(y))

#define casttoptr(x)		(void*)(x)
#define casttointeger(x)	(long)(x)
#define glyphinfobyte(p,k)	((unsigned char*)p)[k]

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
