/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

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
	realpoint	origin;
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
