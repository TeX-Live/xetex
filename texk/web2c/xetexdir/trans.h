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

#ifndef _TRANS_H_
#define _TRANS_H_

#ifdef XETEX_MAC

#include <Carbon/Carbon.h>

typedef CGAffineTransform	transform;

#else

#include <math.h>

typedef struct {
	double	a;
	double	b;
	double	c;
	double	d;
	double	x;
	double	y;
} transform;

#endif

typedef struct {
	float	x;
	float	y;
} realpoint;

typedef struct {
	float	x;
	float	y;
	float	wd;
	float	ht;
} realrect;

#define xCoord(p)				p.x
#define yCoord(p)				p.y

#define wdField(r)				r.wd
#define htField(r)				r.ht

#define aField(t)				t.a
#define bField(t)				t.b
#define cField(t)				t.c
#define dField(t)				t.d
#ifdef XETEX_MAC
#define xField(t)				t.tx
#define yField(t)				t.ty
#else
#define xField(t)				t.x
#define yField(t)				t.y
#endif

#define setPoint(P,X,Y)			do { P.x = X; P.y = Y; } while (0)

void makeidentity(transform* t);
void makescale(transform* t, double xscale, double yscale);
void maketranslation(transform* t, double dx, double dy);
void makerotation(transform* t, double a);
void transformpoint(realpoint* p, const transform* t);
void transformconcat(transform* t1, const transform* t2);

#endif /* _TRANS_H_ */
