/*
 * Copyright (c) 1987, 1989 University of Maryland
 * Department of Computer Science.  All rights reserved.
 * Permission to copy for any purpose is hereby granted
 * so long as this copyright notice remains intact.
 */

#ifndef lint
static char rcsid[] = "$Header: /usr/src/local/tex/local/mctex/dvi/RCS/dviselect.c,v 3.1 89/08/22 17:16:13 chris Exp $";
#endif

/*
 * DVI page selection program
 *
 * Reads DVI version 2 files and selects pages, writing a new DVI
 * file.  The new DVI file is technically correct, though we do not
 * adjust the tallest and widest page values, nor the DVI stack size.
 * This is all right since the values will never become too small,
 * but it might be nice to fix them up.  Perhaps someday . . . .
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#ifdef KPATHSEA
#include <kpathsea/c-fopen.h>
#include <kpathsea/getopt.h>
#else
#define FOPEN_RBIN_MODE  "rb"
#define FOPEN_RBIN_MODE  "wb"
#define SET_BINARY(x)
extern char *optarg;
extern int optind;
#endif
 
#include "types.h"
#include "dviclass.h"
#include "dvicodes.h"
#include "error.h"
#include "fio.h"
#include "gripes.h"
#include "search.h"
#include <stdio.h>
#include <ctype.h>

#define white(x) ((x) == ' ' || (x) == '\t' || (x) == ',')

char  *ProgName;
/* Globals */
char	serrbuf[BUFSIZ];	/* buffer for stderr */

/*
 * We will try to keep output lines shorter than MAXCOL characters.
 */
#define MAXCOL	75

/*
 * We use the following structure to keep track of fonts we have seen.
 * The final DVI file lists only the fonts it uses.
 */
struct fontinfo {
	i32	fi_newindex;	/* font number in output file */
	int	fi_reallyused;	/* true => used on a page we copied */
	i32	fi_checksum;	/* the checksum */
	i32	fi_mag;		/* the magnification */
	i32	fi_designsize;	/* the design size */
	short	fi_n1;		/* the name header length */
	short	fi_n2;		/* the name body length */
	char	*fi_name;	/* the name itself */
};

/*
 * We need to remember which pages the user would like.  We build a linked
 * list that allows us to decide (for any given page) whether it should
 * be included in the output file.  Each page has ten \count variables
 * associated with it.  We put a bound on the values allowed for each, and
 * keep a linked list of alternatives should any be outside the allowed
 * range.  For example, `dviselect *.3,10-15' would generate a two-element
 * page list, with the first allowing any value for \count0 (and \counts 2 to
 * 9) but \count1 restricted to the range 3-3, and the second restricting
 * \count0 to the range 10-15 but leaving \counts 1 to 9 unrestrained.
 *
 * We remember whether a bound is specified, so that we need not fix
 * some `large' number as a maximum value.
 *
 * We also allow `absolute' page references, where the first page is
 * page 1, the second 2, and so forth.  These are specified with an
 * equal sign: `dviselect =4:10' picks up the fourth through tenth
 * sequential pages, irrespective of \count values.
 *
 * There is also an even/odd switch (which can be applied to absolute
 * or relative pages): dviselect =even, even.3, etc.
 */
enum pstype {
	pt_lu,			/* have both lower and upper bounds */
	pt_l,			/* lower bound only */
	pt_u,			/* upper bound only */
	pt_unbounded,		/* any value at all */
	pt_even,		/* even values only */
	pt_odd			/* odd values only */
};
struct pagesel {
	enum pstype ps_type;	/* type of selector */
	i32	ps_low;		/* lower bound */
	i32	ps_high;	/* upper bound */
};
struct pagelist {
	struct	pagelist *pl_alt;	/* next in a series of alternates */
	int	pl_len;			/* number of ranges to check */
	int	pl_abs;			/* true iff absolute page ref */
	struct	pagesel pl_pages[10];	/* one for each \count variable */
};

int	SFlag;			/* true => -s, silent operation */

struct	search *FontFinder;	/* maps from input indicies to fontinfo */
i32	NextOutputFontIndex;	/* generates output indicies */
i32	CurrentFontIndex;	/* current (old) index in input */
i32	OutputFontIndex;	/* current (new) index in ouput */

struct	pagelist *PageList;	/* the list of allowed pages */

char	*DVIFileName;		/* name of input DVI file */
FILE	*inf;			/* the input file itself */
FILE	*outf;			/* the output DVI file */

long	StartOfLastPage;	/* The file position just before we started
				   the last page (this is later written to
				   the output file as the previous page
				   pointer). */
long	CurrentPosition;	/* The current position of the file */

int	UseThisPage;		/* true => current page is selected */

i32	InputPageNumber;	/* current absolute page in old DVI file */
int	NumberOfOutputPages;	/* number of pages in new DVI file */

i32	Numerator;		/* numerator from DVI file */
i32	Denominator;		/* denominator from DVI file */
i32	DVIMag;			/* magnification from DVI file */

i32	Count[10];		/* the 10 \count variables */

/* save some string space: we use this a lot */
char	writeerr[] = "error writing DVI file";

#ifdef NeedFunctionPrototypes
#include <string.h>
void WriteFont(struct fontinfo *);
int evenodd(char *);
int ParsePages(char *);
void HandleDVIFile();
void PutFontSelector(i32);

#ifdef _AMIGA
#define bcmp(s1, s2, len) memcmp(s1, s2, len)
#define bzero(s, len) memset(s, '\0', len)
#define index(s, c) strchr(s, c)
#endif /* _AMIGA */

#ifndef KPATHSEA
char	*malloc(), *realloc();
#endif /* not WIN32 */
#endif
/*
 * You may get lint warnings about sprintf's return value.
 * Older versions of 4BSD have `char *sprintf()'.  ANSI and
 * SysV use `int sprintf()'; so ignore the warnings.
 */

/*
 * Lint gets somewhat confused over putc.
 */
#ifdef lint
#undef putc
#ifdef ultrix /* grr */
#define putc(c, f) fputc((char)(c), f)
#else
#define putc(c, f) fputc((int)(c), f)
#endif
#endif

/*
 * Return true iff the 10 \counts are one of the desired output pages.
 */
DesiredPageP()
{
	register struct pagelist *pl;

	for (pl = PageList; pl != NULL; pl = pl->pl_alt) {
		register struct pagesel *ps = pl->pl_pages;
		register int i;
		register i32 *pagep;

		pagep = pl->pl_abs ? &InputPageNumber : &Count[0];
		for (i = 0; i < pl->pl_len; i++, ps++, pagep++) {
			switch (ps->ps_type) {

			case pt_lu:	/* check upper, fall through */
				if (*pagep > ps->ps_high)
					goto no_good;
				/* FALLTHROUGH */

			case pt_l:	/* only if not too low */
				if (*pagep < ps->ps_low)
					goto no_good;
				break;

			case pt_u:	/* only if not too high */
				if (*pagep > ps->ps_high)
					goto no_good;
				break;

			case pt_unbounded:
				break;

			case pt_even:	/* only if even */
				if (*pagep & 1)
					goto no_good;
				break;

			case pt_odd:	/* only if odd */
				if ((*pagep & 1) == 0)
					goto no_good;
				break;
			}
		}
		return (1);	/* success */
	no_good:
		continue;
	}
	return (0);
}

/*
 * Print a message to stderr, with an optional leading space, and handling
 * long line wraps.
 */
void message(space, str, len)
	int space;
	register char *str;
	register int len;
{
	static int beenhere;
	static int col;

	if (!beenhere)
		space = 0, beenhere++;
	if (len == 0)
		len = strlen(str);
	col += len;
	if (space) {
		if (col >= MAXCOL)
			(void) putc('\n', stderr), col = len;
		else
			(void) putc(' ', stderr), col++;
	}
	while (--len >= 0)
		(void) putc(*str++, stderr);
	(void) fflush(stderr);
}

/*
 * Start a page (process a DVI_BOP).
 */
void BeginPage()
{
	register i32 *i;

	OutputFontIndex = -1;	/* new page requires respecifying font */
	InputPageNumber++;	/* count it */
	for (i = Count; i < &Count[10]; i++)
		fGetLong(inf, *i);
	(void) GetLong(inf);	/* previous page pointer */

	if ((UseThisPage = DesiredPageP()) == 0)
		return;

	putbyte(outf, DVI_BOP);
	for (i = Count; i < &Count[10]; i++)
		PutLong(outf, *i);
	PutLong(outf, StartOfLastPage);
	if (ferror(outf))
		error(1, -1, writeerr);

	StartOfLastPage = CurrentPosition;
	CurrentPosition += 45;	/* we just wrote this much */

	if (!SFlag) {		/* write nice page usage messages */
		register int z = 0;
		register int mlen = 0;
		char msg[80];

		(void) sprintf(msg, "[%ld", (long)Count[0]);
		mlen = strlen(msg);
		for (i = &Count[1]; i < &Count[10]; i++) {
			if (*i == 0) {
				z++;
				continue;
			}
			while (--z >= 0)
				msg[mlen++] = '.', msg[mlen++] = '0';
			z = 0;
			(void) sprintf(msg + mlen, ".%ld", (long)*i);
			mlen += strlen(msg + mlen);
		}
		message(1, msg, mlen);
	}
}

/*
 * End a page (process a DVI_EOP).
 */
void EndPage()
{

	if (!UseThisPage)
		return;
	if (!SFlag)
		message(0, "]", 1);
	putbyte(outf, DVI_EOP);
	if (ferror(outf))
		error(1, -1, writeerr);
	CurrentPosition++;
	NumberOfOutputPages++;
}

/*
 * For each of the fonts used in the new DVI file, write out a definition.
 */
/* ARGSUSED */
void
PostAmbleFontEnumerator(addr, key)
	char *addr;
	i32 key;
{

	if (((struct fontinfo *)addr)->fi_reallyused)
		WriteFont((struct fontinfo *)addr);
}

void HandlePostAmble()
{
	register i32 c;

	(void) GetLong(inf);	/* previous page pointer */
	if (GetLong(inf) != Numerator)
		GripeMismatchedValue("numerator");
	if (GetLong(inf) != Denominator)
		GripeMismatchedValue("denominator");
	if (GetLong(inf) != DVIMag)
		GripeMismatchedValue("\\magnification");

	putbyte(outf, DVI_POST);
	PutLong(outf, StartOfLastPage);
	PutLong(outf, Numerator);
	PutLong(outf, Denominator);
	PutLong(outf, DVIMag);
	c = GetLong(inf);
	PutLong(outf, c);	/* tallest page height */
	c = GetLong(inf);
	PutLong(outf, c);	/* widest page width */
	c = GetWord(inf);
	PutWord(outf, c);	/* DVI stack size */
	PutWord(outf, NumberOfOutputPages);
	StartOfLastPage = CurrentPosition;	/* point at post */
	CurrentPosition += 29;	/* count all those `put's */
#ifdef notdef
	(void) GetWord(inf);	/* skip original number of pages */
#endif

	/*
	 * just ignore all the incoming font definitions; we are done with
	 * input file 
	 */

	/*
	 * run through the FontFinder table and dump definitions for the
	 * fonts we have used. 
	 */
	SEnumerate(FontFinder, PostAmbleFontEnumerator);

	putbyte(outf, DVI_POSTPOST);
	PutLong(outf, StartOfLastPage);	/* actually start of postamble */
	putbyte(outf, DVI_VERSION);
	putbyte(outf, DVI_FILLER);
	putbyte(outf, DVI_FILLER);
	putbyte(outf, DVI_FILLER);
	putbyte(outf, DVI_FILLER);
	CurrentPosition += 10;
	while (CurrentPosition & 3) {
		putbyte(outf, DVI_FILLER);
		CurrentPosition++;
	}
	if (ferror(outf))
		error(1, -1, writeerr);
}

/*
 * Write a font definition to the output file
 */
void WriteFont(fi)
	register struct fontinfo *fi;
{
	register int l;
	register char *s;

	if (fi->fi_newindex < 256) {
		putbyte(outf, DVI_FNTDEF1);
		putbyte(outf, fi->fi_newindex);
		CurrentPosition += 2;
	} else if (fi->fi_newindex < 65536) {
		putbyte(outf, DVI_FNTDEF2);
		PutWord(outf, fi->fi_newindex);
		CurrentPosition += 3;
	} else if (fi->fi_newindex < 16777216) {
		putbyte(outf, DVI_FNTDEF3);
		Put3Byte(outf, fi->fi_newindex);
		CurrentPosition += 4;
	} else {
		putbyte(outf, DVI_FNTDEF4);
		PutLong(outf, fi->fi_newindex);
		CurrentPosition += 5;
	}
	PutLong(outf, fi->fi_checksum);
	PutLong(outf, fi->fi_mag);
	PutLong(outf, fi->fi_designsize);
	putbyte(outf, fi->fi_n1);
	putbyte(outf, fi->fi_n2);
	l = fi->fi_n1 + fi->fi_n2;
	CurrentPosition += 14 + l;
	s = fi->fi_name;
	while (--l >= 0)
		putbyte(outf, *s++);
}

/*
 * Handle the preamble.  Someday we should update the comment field.
 */
void HandlePreAmble()
{
	register int n, c;

	c = getc(inf);
	if (c == EOF)
		GripeUnexpectedDVIEOF();
	if (c != DVI_PRE)
		GripeMissingOp("PRE");
	if (getc(inf) != DVI_VERSION)
		error(1, 0, "%s is not a DVI version %d file",
		    DVIFileName, DVI_VERSION);
	Numerator = GetLong(inf);
	Denominator = GetLong(inf);
	DVIMag = GetLong(inf);
	putbyte(outf, DVI_PRE);
	putbyte(outf, DVI_VERSION);
	PutLong(outf, Numerator);
	PutLong(outf, Denominator);
	PutLong(outf, DVIMag);

	n = UnSign8(GetByte(inf));
	CurrentPosition = 15 + n;	/* well, almost */
	putbyte(outf, n);
	while (--n >= 0) {
		c = GetByte(inf);
		putbyte(outf, c);
	}
}

main(argc, argv)
	int argc;
	register char **argv;
{
	register int c;
	register char *s;
	char *outname = NULL;

	ProgName = *argv;
	setbuf(stderr, serrbuf);

	while ((c = getopt(argc, argv, "i:o:s")) != EOF) {
		switch (c) {

		case 's':	/* silent */
			SFlag++;
			break;

		case 'i':
			if (DVIFileName != NULL)
				goto usage;
			DVIFileName = optarg;
			break;

		case 'o':
			if (outname != NULL)
				goto usage;
			outname = optarg;
			break;

		case '?':
usage:
			(void) fprintf(stderr, "\
Usage: %s [-s] [-i infile] [-o outfile] pages [...] [infile [outfile]]\n",
				ProgName);
			(void) fflush(stderr);
			exit(1);
		}
	}

	while (optind < argc) {
		s = argv[optind++];
		c = *s;
		if (!isalpha(c) && c != '/' ||
		    (c == 'e' || c == 'o') && evenodd(s)) {
			if (ParsePages(s))
				goto usage;
		} else if (DVIFileName == NULL)
			DVIFileName = s;
		else if (outname == NULL)
			outname = s;
		else
			goto usage;
	}
	if (PageList == NULL)
		goto usage;
	if (DVIFileName == NULL) {
		DVIFileName = "`stdin'";
		inf = stdin;
		if (!isatty(fileno(inf)))
		  SET_BINARY(fileno(inf));
	} else if ((inf = fopen(DVIFileName, FOPEN_RBIN_MODE)) == 0)
		error(1, -1, "cannot read %s", DVIFileName);
	if (outname == NULL) {
		outf = stdout;
		if (!isatty(fileno(outf)))
		  SET_BINARY(fileno(outf));
	} else if ((outf = fopen(outname, FOPEN_WBIN_MODE)) == 0)
		error(1, -1, "cannot write %s", outname);

	if ((FontFinder = SCreate(sizeof(struct fontinfo))) == 0)
		error(1, 0, "cannot create font finder (out of memory?)");

	StartOfLastPage = -1;
	HandlePreAmble();
	HandleDVIFile();
	HandlePostAmble();
	if (!SFlag)
		(void) fprintf(stderr, "\nWrote %d page%s, %ld bytes\n",
		    NumberOfOutputPages, NumberOfOutputPages == 1 ? "" : "s",
		    (long)CurrentPosition);
	exit(0);
	/* NOTREACHED */
}

struct pagelist *
InstallPL(ps, n, absolute)
	register struct pagesel *ps;
	register int n;
	int absolute;
{
	register struct pagelist *pl;

	pl = (struct pagelist *)malloc(sizeof *pl);
	if (pl == NULL)
		GripeOutOfMemory(sizeof *pl, "page list");
	pl->pl_alt = PageList;
	PageList = pl;
	pl->pl_len = n;
	while (--n >= 0)
		pl->pl_pages[n] = ps[n];
	pl->pl_abs = absolute;
	return pl;
}

/*
 * Return true iff the string `s' is the word `even' or `odd',
 * followed by `.' or `white' characters.
 */
int
evenodd(s)
	char *s;
{
	register int c;

	if (strncmp(s, "even", 4) == 0)
		c = s[4];
	else if (strncmp(s, "odd", 3) == 0)
		c = s[3];
	else
		return (0);
	if (c == 0 || c == '.' || white(c))
		return (1);
	return (0);
}

/*
 * Parse a string representing a list of pages.  Return 0 iff ok.  As a
 * side effect, the page selection(s) is (are) prepended to PageList.
 */
int ParsePages(s)
	register char *s;
{
	register struct pagesel *ps;
	register int c;		/* current character */
	register i32 n;		/* current numeric value */
	register int innumber;	/* true => gathering a number */
	int i;			/* next index in page select list */
	int range;		/* true => saw a range indicator */
	int negative;		/* true => number being built is negative */
	int absolute;		/* true => absolute, not \count */
	int lb, ub, even, odd;	/* flags for lower,upper,even,odd */
	struct pagesel pagesel[10];

	range = 0;
	innumber = 0;
	absolute = 0;
	i = 0;
	ps = pagesel;
	lb = ub = even = odd = 0;
	/*
	 * Talk about ad hoc!  (Not to mention convoluted.)
	 */
	for (;;) {
		c = *s++;
		if (i == 0 && !innumber && !range) {
			/* nothing special going on */
			if (c == 0)
				return 0;
			if (white(c))
				continue;
		}
		if ((c == 'e' || c == 'o') && evenodd(s - 1)) {
			if (innumber || range)
				return (-1);
			if (c == 'e') {
				s += 3;
				even = 1;
			} else {
				s += 2;
				odd = 1;
			}
			c = *s++;
			goto finishnum;
		}
		if (c == '_') {
			/* kludge: should be '-' for negatives */
			if (innumber || absolute)
				return (-1);
			innumber = 1;
			negative = 1;
			n = 0;
			continue;
		}
		if (c == '=') {
			/* absolute page */
			if (innumber || range || i > 0)
				return (-1);
			absolute = 1;
			negative = 0;
			n = 0;
			continue;
		}
		if (isdigit(c)) {
			/* accumulate numeric value */
			if (!innumber) {
				innumber = 1;
				negative = 0;
				n = c - '0';
				continue;
			}
			n *= 10;
			n += negative ? '0' - c : c - '0';
			continue;
		}
		if (c == '-' || c == ':') {
			/* here is a range */
			if (range)
				return (-1);
			if (innumber) {	/* have a lower bound */
				ps->ps_low = n;
				lb = 1;
			}
			range = 1;
			innumber = 0;
			continue;
		}
		if (c == '*') {
			/* no lower bound, no upper bound */
			c = *s++;
			if (innumber || range || (c && c != '.' && !white(c)))
				return (-1);
			goto finishnum;
		}
		if (c == 0 || c == '.' || white(c)) {
			/* end of this range */
			if (innumber) {		/* have an upper bound */
				ps->ps_high = n;
				ub = 1;
				if (!range) {
					/* no range => lower bound == upper */
					ps->ps_low = ps->ps_high;
					lb = 1;
				}
			}
finishnum:
			if (even)
				ps->ps_type = pt_even;
			else if (odd)
				ps->ps_type = pt_odd;
			else if (lb)
				ps->ps_type = ub ? pt_lu : pt_l;
			else
				ps->ps_type = ub ? pt_u : pt_unbounded;
			if (c == '.') {
				if (absolute)
					return (-1);
				if (++i >= 10)	/* too many specifiers */
					return (-1);
				ps++;
			} else {
				InstallPL(pagesel, i + 1, absolute);
				ps = pagesel;
				i = 0;
				absolute = 0;
			}
			if (c == 0)
				return (0);
			range = 0;
			innumber = 0;
			lb = ub = even = odd = 0;
			continue;
		}
		/* illegal character */
		return (-1);
	}
}

/*
 * Handle a font definition.
 */
void HandleFontDef(index)
	i32 index;
{
	register struct fontinfo *fi;
	register int i;
	register char *s;
	int def = S_CREATE | S_EXCL;

	if ((fi = (struct fontinfo *)SSearch(FontFinder, index, &def)) == 0)
		if (def & S_COLL)
			error(1, 0, "font %ld already defined", (long)index);
		else
			error(1, 0, "cannot stash font %ld (out of memory?)",
				(long)index);
	fi->fi_reallyused = 0;
	fi->fi_checksum = GetLong(inf);
	fi->fi_mag = GetLong(inf);
	fi->fi_designsize = GetLong(inf);
	fi->fi_n1 = UnSign8(GetByte(inf));
	fi->fi_n2 = UnSign8(GetByte(inf));
	i = fi->fi_n1 + fi->fi_n2;
	if ((s = malloc((unsigned)i)) == 0)
		GripeOutOfMemory(i, "font name");
	fi->fi_name = s;
	while (--i >= 0)
		*s++ = GetByte(inf);
}

/*
 * Handle a \special.
 */
void HandleSpecial(c, l, p)
	int c;
	register int l;
	register i32 p;
{
	register int i;

	if (UseThisPage) {
		putbyte(outf, c);
		switch (l) {

		case DPL_UNS1:
			putbyte(outf, p);
			CurrentPosition += 2;
			break;

		case DPL_UNS2:
			PutWord(outf, p);
			CurrentPosition += 3;
			break;

		case DPL_UNS3:
			Put3Byte(outf, p);
			CurrentPosition += 4;
			break;

		case DPL_SGN4:
			PutLong(outf, p);
			CurrentPosition += 5;
			break;

		default:
			panic("HandleSpecial l=%d", l);
			/* NOTREACHED */
		}
		CurrentPosition += p;
		while (--p >= 0) {
			i = getc(inf);
			putbyte(outf, i);
		}
		if (feof(inf))
			GripeUnexpectedDVIEOF();
		if (ferror(outf))
			error(1, -1, writeerr);
	} else
		while (--p >= 0)
			(void) getc(inf);
}

void ReallyUseFont()
{
	register struct fontinfo *fi;
	int look = S_LOOKUP;

	fi = (struct fontinfo *)SSearch(FontFinder, CurrentFontIndex, &look);
	if (fi == NULL)
		error(1, 0, "DVI file requested font %ld without defining it",
		    (long)CurrentFontIndex);
	if (fi->fi_reallyused == 0) {
		fi->fi_reallyused++;
		fi->fi_newindex = NextOutputFontIndex++;
		WriteFont(fi);
	}
	if (fi->fi_newindex != OutputFontIndex) {
		PutFontSelector(fi->fi_newindex);
		OutputFontIndex = fi->fi_newindex;
	}
}

/*
 * Write a font selection command to the output file
 */
void PutFontSelector(index)
	i32 index;
{

	if (index < 64) {
		putbyte(outf, index + DVI_FNTNUM0);
		CurrentPosition++;
	} else if (index < 256) {
		putbyte(outf, DVI_FNT1);
		putbyte(outf, index);
		CurrentPosition += 2;
	} else if (index < 65536) {
		putbyte(outf, DVI_FNT2);
		PutWord(outf, index);
		CurrentPosition += 3;
	} else if (index < 16777216) {
		putbyte(outf, DVI_FNT3);
		Put3Byte(outf, index);
		CurrentPosition += 4;
	} else {
		putbyte(outf, DVI_FNT4);
		PutLong(outf, index);
		CurrentPosition += 5;
	}
}

/*
 * The following table describes the length (in bytes) of each of the DVI
 * commands that we can simply copy, starting with DVI_SET1 (128).
 */
char	oplen[128] = {
	0, 0, 0, 0,		/* DVI_SET1 .. DVI_SET4 */
	9,			/* DVI_SETRULE */
	0, 0, 0, 0,		/* DVI_PUT1 .. DVI_PUT4 */
	9,			/* DVI_PUTRULE */
	1,			/* DVI_NOP */
	0,			/* DVI_BOP */
	0,			/* DVI_EOP */
	1,			/* DVI_PUSH */
	1,			/* DVI_POP */
	2, 3, 4, 5,		/* DVI_RIGHT1 .. DVI_RIGHT4 */
	1,			/* DVI_W0 */
	2, 3, 4, 5,		/* DVI_W1 .. DVI_W4 */
	1,			/* DVI_X0 */
	2, 3, 4, 5,		/* DVI_X1 .. DVI_X4 */
	2, 3, 4, 5,		/* DVI_DOWN1 .. DVI_DOWN4 */
	1,			/* DVI_Y0 */
	2, 3, 4, 5,		/* DVI_Y1 .. DVI_Y4 */
	1,			/* DVI_Z0 */
	2, 3, 4, 5,		/* DVI_Z1 .. DVI_Z4 */
	0,			/* DVI_FNTNUM0 (171) */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 172 .. 179 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 180 .. 187 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 188 .. 195 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 196 .. 203 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 204 .. 211 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 212 .. 219 */
	0, 0, 0, 0, 0, 0, 0, 0,	/* 220 .. 227 */
	0, 0, 0, 0, 0, 0, 0,	/* 228 .. 234 */
	0, 0, 0, 0,		/* DVI_FNT1 .. DVI_FNT4 */
	0, 0, 0, 0,		/* DVI_XXX1 .. DVI_XXX4 */
	0, 0, 0, 0,		/* DVI_FNTDEF1 .. DVI_FNTDEF4 */
	0,			/* DVI_PRE */
	0,			/* DVI_POST */
	0,			/* DVI_POSTPOST */
	0, 0, 0, 0, 0, 0,	/* 250 .. 255 */
};

/*
 * Here we read the input DVI file and write relevant pages to the
 * output DVI file. We also keep track of font changes, handle font
 * definitions, and perform some other housekeeping.
 */
void HandleDVIFile()
{
	register int c, l;
	register i32 p;
	register int CurrentFontOK = 0;
	int doingpage = 0;

	/* Only way out is via "return" statement */
	for (;;) {
		c = getc(inf);	/* getc() returns unsigned values */
		if (DVI_IsChar(c)) {
			/*
			 * Copy chars, note font usage, but ignore if
			 * page is not interesting.
			 */
			if (!UseThisPage)
				continue;
			if (!CurrentFontOK) {
				ReallyUseFont();
				CurrentFontOK++;
			}
			putbyte(outf, c);
			CurrentPosition++;
			continue;
		}
		if (DVI_IsFont(c)) {	/* note font change */
			CurrentFontIndex = c - DVI_FNTNUM0;
			CurrentFontOK = 0;
			continue;
		}
		if (c == EOF)
			GripeUnexpectedDVIEOF();
		if ((l = (oplen - 128)[c]) != 0) {	/* simple copy */
			if (!UseThisPage) {
				while (--l > 0)
					(void) getc(inf);
				continue;
			}
			CurrentPosition += l;
			putbyte(outf, c);
			while (--l > 0) {
				c = getc(inf);
				putbyte(outf, c);
			}
			if (ferror(outf))
				error(1, -1, writeerr);
			continue;
		}
		if ((l = DVI_OpLen(c)) != 0) {
			/*
			 * Handle other generics.
			 * N.B.: there should only be unsigned parameters
			 * here (save SGN4), for commands with negative
			 * parameters have been taken care of above.
			 */
			switch (l) {

			case DPL_UNS1:
				p = getc(inf);
				break;

			case DPL_UNS2:
				fGetWord(inf, p);
				break;

			case DPL_UNS3:
				fGet3Byte(inf, p);
				break;

			case DPL_SGN4:
				fGetLong(inf, p);
				break;

			default:
				panic("HandleDVIFile l=%d", l);
			}

			/*
			 * Now that we have the parameter, perform the
			 * command.
			 */
			switch (DVI_DT(c)) {

			case DT_SET:
			case DT_PUT:
				if (!UseThisPage)
					continue;
				if (!CurrentFontOK) {
					ReallyUseFont();
					CurrentFontOK++;
				}
				putbyte(outf, c);
				switch (l) {

				case DPL_UNS1:
					putbyte(outf, p);
					CurrentPosition += 2;
					continue;

				case DPL_UNS2:
					PutWord(outf, p);
					CurrentPosition += 3;
					continue;

				case DPL_UNS3:
					Put3Byte(outf, p);
					CurrentPosition += 4;
					continue;

				case DPL_SGN4:
					PutLong(outf, p);
					CurrentPosition += 5;
					continue;
				}

			case DT_FNT:
				CurrentFontIndex = p;
				CurrentFontOK = 0;
				continue;

			case DT_XXX:
				HandleSpecial(c, l, p);
				continue;

			case DT_FNTDEF:
				HandleFontDef(p);
				continue;

			default:
				panic("HandleDVIFile DVI_DT(%d)=%d",
				      c, DVI_DT(c));
			}
			continue;
		}

		switch (c) {	/* handle the few remaining cases */

		case DVI_BOP:
			if (doingpage)
				GripeUnexpectedOp("BOP (during page)");
			BeginPage();
			doingpage = 1;
			CurrentFontOK = 0;
			break;

		case DVI_EOP:
			if (!doingpage)
				GripeUnexpectedOp("EOP (outside page)");
			EndPage();
			doingpage = 0;
			break;

		case DVI_PRE:
			GripeUnexpectedOp("PRE");
			/* NOTREACHED */

		case DVI_POST:
			if (doingpage)
				GripeUnexpectedOp("POST (inside page)");
			return;

		case DVI_POSTPOST:
			GripeUnexpectedOp("POSTPOST");
			/* NOTREACHED */

		default:
			GripeUndefinedOp(c);
			/* NOTREACHED */
		}
	}
}
