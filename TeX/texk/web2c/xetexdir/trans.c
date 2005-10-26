/****************************************************************************\
 Part of the XeTeX typesetting system
 copyright (c) 1994-2005 by SIL International
 written by Jonathan Kew

 This software is distributed under the terms of the Common Public License,
 version 1.0.
 For details, see <http://www.opensource.org/licenses/cpl1.0.php> or the file
 cpl1.0.txt included with the software.
\****************************************************************************/

#include "trans.h"

void makeidentity(transform* t)
{
#ifdef XETEX_MAC
	*t = CGAffineTransformMake(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
#else
	t->a = 1.0;
	t->b = 0.0;
	t->c = 0.0;
	t->d = 1.0;
	t->x = 0.0;
	t->y = 0.0;
#endif
}

void makescale(transform* t, double xscale, double yscale)
{
#ifdef XETEX_MAC
	*t = CGAffineTransformMakeScale(xscale, yscale);
#else
	t->a = xscale;
	t->b = 0.0;
	t->c = 0.0;
	t->d = yscale;
	t->x = 0.0;
	t->y = 0.0;
#endif
}

void maketranslation(transform* t, double dx, double dy)
{
#ifdef XETEX_MAC
	*t = CGAffineTransformMakeTranslation(dx, dy);
#else
	t->a = 1.0;
	t->b = 0.0;
	t->c = 0.0;
	t->d = 1.0;
	t->x = dx;
	t->y = dy;
#endif
}

void makerotation(transform* t, double a)
{
#ifdef XETEX_MAC
	*t = CGAffineTransformMakeRotation(a);
#else
	t->a = cos(a);
	t->b = sin(a);
	t->c = -sin(a);
	t->d = cos(a);
	t->x = 0.0;
	t->y = 0.0;
#endif
}

void transformpoint(realpoint* p, const transform* t)
{
#ifdef XETEX_MAC
	CGPoint r = CGPointApplyAffineTransform(*(CGPoint*)p, *t);
	*p = *(realpoint*)&r;
#else
	realpoint	r;
	r.x = t->a * p->x + t->c * p->y + t->x;
	r.y = t->b * p->x + t->d * p->y + t->y;
	*p = r;
#endif
}

void transformconcat(transform* t1, const transform* t2)
{
#ifdef XETEX_MAC
	*t1 = CGAffineTransformConcat(*t1, *t2);
#else
	transform	r;
	r.a = t1->a * t2->a + t1->b * t2->c + 0.0 * t2->x;
	r.b = t1->a * t2->b + t1->b * t2->d + 0.0 * t2->y;
	r.c = t1->c * t2->a + t1->d * t2->c + 0.0 * t2->x;
	r.d = t1->c * t2->b + t1->d * t2->d + 0.0 * t2->y;
	r.x = t1->x * t2->a + t1->y * t2->c + 1.0 * t2->x;
	r.y = t1->x * t2->b + t1->y * t2->d + 1.0 * t2->y;
	*t1 = r;
#endif
}
