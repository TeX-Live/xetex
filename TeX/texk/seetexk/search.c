/*
 * Copyright (c) 1987, 1989 University of Maryland
 * Department of Computer Science.  All rights reserved.
 * Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

#ifndef lint
static char rcsid[] = "$Header: /usr/src/local/tex/local/mctex/lib/RCS/search.c,v 3.1 89/08/22 21:58:14 chris Exp $";
#endif

/*
 * Key search routines (for a 32 bit key)
 *
 * SCreate initializes the search control area.
 *
 * SSearch returns the address of the data area (if found or created)
 * or a null pointer (if not).  The last argument controls the disposition
 * in various cases, and is a ``value-result'' parameter.
 *
 * SEnumerate calls the given function on each data object within the
 * search table.  Note that no ordering is specified (though currently
 * it runs in increasing-key-value sequence).
 */

#include "types.h"
#include "search.h"

#if vax || mc68000 || ns32000 || pyr
#define	HARD_ALIGNMENT	4
#else
#define	HARD_ALIGNMENT	16	/* should suffice for most everyone */
#endif

static int DOffset;		/* part of alignment code */

#ifndef KPATHSEA
char	*malloc(), *realloc();
#endif

struct search *
SCreate(dsize)
	register unsigned int dsize;
{
	register struct search *s;

	if ((s = (struct search *) malloc(sizeof *s)) == 0)
		return (0);

	if (DOffset == 0) {
#ifndef HARD_ALIGNMENT
		DOffset = sizeof(i32);
#else
		DOffset = (sizeof(i32) + HARD_ALIGNMENT - 1) &
			~(HARD_ALIGNMENT - 1);
#endif
	}
	dsize += DOffset;	/* tack on space for keys */

#ifdef HARD_ALIGNMENT
	/*
	 * For machines with strict alignment constraints, it may be
	 * necessary to align the data at a multiple of some positive power
	 * of two.  In general, it would suffice to make dsize a power of
	 * two, but this could be very space-wasteful, so instead we align it
	 * to HARD_ALIGNMENT.  64 bit machines might ``#define HARD_ALIGNMENT
	 * 8'', for example.  N.B.:  we assume that HARD_ALIGNMENT is a power
	 * of two.
	 */

	dsize = (dsize + HARD_ALIGNMENT - 1) & ~(HARD_ALIGNMENT - 1);
#endif

	s->s_dsize = dsize;	/* save data object size */
	s->s_space = 10;	/* initially, room for 10 objects */
	s->s_n = 0;		/* and none in the table */
	if ((s->s_data = malloc(s->s_space * dsize)) == 0) {
		free((char *)s);
		return (0);
	}
	return (s);
}

/*
 * We actually use a binary search right now - this may change.
 */
char *
SSearch(s, key, disp)
	register struct search *s;
	register i32 key;
	int *disp;
{
	register char *keyaddr;
	int itemstomove;

	*disp &= S_CREATE | S_EXCL;	/* clear return codes */
	if (s->s_n) {		/* look for the key */
		register int h, l, m;

		h = s->s_n - 1;
		l = 0;
		while (l <= h) {
			m = (l + h) >> 1;
			keyaddr = s->s_data + m * s->s_dsize;
			if (*(i32 *) keyaddr > key)
				h = m - 1;
			else if (*(i32 *) keyaddr < key)
				l = m + 1;
			else {	/* found it, now what? */
				if (*disp & S_EXCL) {
					*disp |= S_COLL;
					return (0);	/* collision */
				}
				*disp |= S_FOUND;
				return (keyaddr + DOffset);
			}
		}
		keyaddr = s->s_data + l * s->s_dsize;
	} else
		keyaddr = s->s_data;

	/* keyaddr is now where the key should have been found, if anywhere */
	if ((*disp & S_CREATE) == 0)
		return (0);	/* not found */

	/* avoid using realloc so as to retain old data if out of memory */
	if (s->s_space <= 0) {	/* must expand; double it */
		register char *new;

		if ((new = malloc((s->s_n << 1) * s->s_dsize)) == 0) {
			*disp |= S_ERROR;	/* no space */
			return (0);
		}
		keyaddr = (keyaddr - s->s_data) + new;	/* relocate */
		bcopy(s->s_data, new, s->s_n * s->s_dsize);
		free(s->s_data);
		s->s_data = new;
		s->s_space = s->s_n;
	}
	/* now move any keyed data that is beyond keyaddr down */
	itemstomove = s->s_n - (keyaddr - s->s_data) / s->s_dsize;
	if (itemstomove) {
#ifndef BLOCK_COPY
		register char *from, *to;

		from = s->s_data + s->s_n * s->s_dsize;
		to = from + s->s_dsize;
		while (from > keyaddr)
			*--to = *--from;
#else
		BLOCK_COPY(keyaddr, keyaddr + s->s_dsize,
		    itemstomove * s->s_dsize);
#endif
	}
	*disp |= S_NEW;
	s->s_n++;
	s->s_space--;
	*(i32 *) keyaddr = key;
	keyaddr += DOffset;	/* now actually dataaddr */
	/* the bzero is just a frill... */
	bzero(keyaddr, s->s_dsize - DOffset);
	return (keyaddr);
}

/*
 * Call function `f' for each element in the search table `s'.
 */
void
SEnumerate(s, f)
	register struct search *s;
	register void (*f)();
{
	register int n;
	register char *p;

	n = s->s_n;
	p = s->s_data;
	while (--n >= 0) {
		(*f)(p + DOffset, *(i32 *)p);
		p += s->s_dsize;
	}
}
