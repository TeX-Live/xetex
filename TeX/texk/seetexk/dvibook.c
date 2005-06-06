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
 * DVI page rearrangement program
 *
 * Reads DVI version 2 files and rearranges pages into signatures,
 * writing a new DVI file.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef KPATHSEA
#include <kpathsea/c-fopen.h>
#include <kpathsea/getopt.h>
#else
#define FOPEN_RBIN_MODE  "r"
#define FOPEN_RBIN_MODE  "w"
#define SET_BINARY(x)
extern char *optarg;
extern int   optind;
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
#include "seek.h"

#define white(x) ((x) == ' ' || (x) == '\t' || (x) == ',')

#define MAXDVIPAGES 1000 /* max (absolute) pages in DVI file */

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

int     Signature;              /* #pages per signature (multiple of 4) */

int	SFlag;			/* true => -s, silent operation */

struct	search *FontFinder;	/* maps from input indicies to fontinfo */
i32	NextOutputFontIndex;	/* generates output indicies */
i32	CurrentFontIndex;	/* current (old) index in input */
i32	OutputFontIndex;	/* current (new) index in ouput */

char	*DVIFileName;		/* name of input DVI file */
FILE	*inf;			/* the input file itself */
FILE	*outf;			/* the output DVI file */

long	StartOfPage[MAXDVIPAGES];	/* The file positions of the
					   input pages */

long	StartOfLastPage;	/* The file position just before we
				   started the last page */
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

#ifndef KPATHSEA
char *malloc(), *realloc();
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

#ifdef HAVE_PROTOTYPES
void message(int space,char *str,int len);
void BeginPage(void);
void DviEndPage(void);
void PostAmbleFontEnumerator(char *addr,long key);
void HandlePostAmble(void);
void WriteFont(struct fontinfo *fi);
void HandlePreAmble(void);
int main(int argc,char **argv );
void HandleFontDef(long strchr);
void HandleSpecial(int c,int l,long p);
void ReallyUseFont(void);
void PutFontSelector(long strchr);
void ScanDVIFile(void);
void HandleDVIFile(void);
int HandlePage(void);
void PutEmptyPage(void);
#else
void message();
void BeginPage();
void DviEndPage();
void PostAmbleFontEnumerator();
void HandlePostAmble();
void WriteFont();
void HandlePreAmble();
int main();
void HandleFontDef();
void HandleSpecial();
void ReallyUseFont();
void PutFontSelector();
void ScanDVIFile();
void HandleDVIFile();
int HandlePage();
void PutEmptyPage();
#endif

/*
 * Print a message to stderr, with an optional leading space, and handling
 * long line wraps.
 */
void
message(space, str, len)
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
void
BeginPage()
{
	register i32 *i;

	OutputFontIndex = -1;	/* new page requires respecifying font */
	for (i = Count; i < &Count[10]; i++)
		fGetLong(inf, *i);
	(void) GetLong(inf);	/* previous page pointer */

	if (!UseThisPage)
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
void
EndPage()
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

void
HandlePostAmble()
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
void
WriteFont(fi)
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
void
HandlePreAmble()
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

	Signature = 0;

	ProgName = *argv;
	setbuf(stderr, serrbuf);

	while ((c = getopt(argc, argv, "i:o:s:q")) != EOF) {
		switch (c) {

		case 'q':	/* silent */
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

		case 's':
			if (Signature != 0)
				goto usage;
			Signature = atoi(optarg);
			if (Signature <= 0)
			   error(1, -1, "-s parameter must be > 0");
			if (Signature % 4 != 0)
			   error(1, -1,
				 "-s parameter must be a multiple of 4");
			break;

		case '?':
usage:
			(void) fprintf(stderr, "\
Usage: %s [-s signature] [-q] [-i infile] [-o outfile] [infile [outfile]]\n",
				ProgName);
			(void) fflush(stderr);
			exit(1);
		}
	}

	while (optind < argc) {
		s = argv[optind++];
		c = *s;
		if (DVIFileName == NULL)
			DVIFileName = s;
		else if (outname == NULL)
			outname = s;
		else
			goto usage;
	}
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
	}
	else if ((outf = fopen(outname, FOPEN_WBIN_MODE)) == 0)
		error(1, -1, "cannot write %s", outname);

	if ((FontFinder = SCreate(sizeof(struct fontinfo))) == 0)
		error(1, 0, "cannot create font finder (out of memory?)");

	/* copy inf to TEMP file if not seekable */
	if ((inf = SeekFile(inf)) == NULL) {
	        error(1, 0, "can't seek file");
	}
	InputPageNumber = 0;
	StartOfLastPage = -1;
	HandlePreAmble();
	ScanDVIFile();
	HandleDVIFile();
	HandlePostAmble();
	if (!SFlag)
		(void) fprintf(stderr, "\nWrote %d page%s, %ld bytes\n",
		    NumberOfOutputPages, NumberOfOutputPages == 1 ? "" : "s",
		    (long)CurrentPosition);
	exit(0);
	/* NOTREACHED */
}

/*
 * Handle a font definition.
 */
void
HandleFontDef(index)
	i32 index;
{
	register struct fontinfo *fi;
	register int i;
	register char *s;
	int def = S_CREATE | S_EXCL;

	if (!UseThisPage) {
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
	} else {
	        (void) GetLong(inf);
	        (void) GetLong(inf);
	        (void) GetLong(inf);
		i = UnSign8(GetByte(inf));
		i += UnSign8(GetByte(inf));
		while (--i >= 0)
			(void) GetByte(inf);
	}
}

/*
 * Handle a \special.
 */
void
HandleSpecial(c, l, p)
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

void
ReallyUseFont()
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
void
PutFontSelector(index)
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
 * Here we scan the input DVI file and record pointers to the pages.
 */
void
ScanDVIFile()
{
	UseThisPage = 0;

	StartOfPage[InputPageNumber] = ftell(inf);
	while (HandlePage()) {  /* scan DVI file */
	        StartOfPage[++InputPageNumber] = ftell(inf);
	}
}

/*
 * Here we read the input DVI file and write relevant pages to the
 * output DVI file. We also keep track of font changes, handle font
 * definitions, and perform some other housekeeping.
 */
void
HandleDVIFile()
{
        int CurrentPage, ActualPage, MaxPage;

	UseThisPage = 1;

	MaxPage = InputPageNumber + (4-InputPageNumber%4)%4;

	if (!Signature)
	        Signature = MaxPage;
	for (CurrentPage = 0; CurrentPage < MaxPage; CurrentPage++) {
	        ActualPage = CurrentPage - CurrentPage%Signature;
	        switch (CurrentPage%4) {
		case 0:
		case 3:
		   ActualPage += Signature-1-(CurrentPage%Signature)/2;
		   break;
		case 1:
		case 2:
		   ActualPage += (CurrentPage%Signature)/2;
		   break;
		}
	        if (ActualPage < InputPageNumber) {
		        if (fseek(inf, StartOfPage[ActualPage], 0) == -1)
		               error(1, -1,
				     "can't seek page %d", ActualPage+1);
		        HandlePage();
		} else
    	                PutEmptyPage();
	}
	if (fseek(inf, StartOfPage[InputPageNumber]+1, 0) == -1)
	        error(1, -1, "can't seek last page");
}

int
HandlePage()
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
			break;

		case DVI_EOP:
			if (!doingpage)
				GripeUnexpectedOp("EOP (outside page)");
			EndPage();
			doingpage = 0;
			return(1);

		case DVI_PRE:
			GripeUnexpectedOp("PRE");
			/* NOTREACHED */

		case DVI_POST:
			if (doingpage)
				GripeUnexpectedOp("POST (inside page)");
			return(0);

		case DVI_POSTPOST:
			GripeUnexpectedOp("POSTPOST");
			/* NOTREACHED */

		default:
			GripeUndefinedOp(c);
			/* NOTREACHED */
		}
	}
}

/* write an empty page to fill out space */
void
PutEmptyPage()
{
        int i;

	putbyte(outf, DVI_BOP);
	PutLong(outf, -1L);
	for (i = 1; i < 10; i++)     /* set all sub counts to 0 */
		PutLong(outf, 0L);
	PutLong(outf, StartOfLastPage);
	putbyte(outf, DVI_EOP);
	if (!SFlag) {		/* write nice page usage messages */
		char *msg = "[*]";
		message(1, msg, strlen(msg));
	}
	if (ferror(outf))
		error(1, -1, writeerr);

	StartOfLastPage = CurrentPosition;
	CurrentPosition += 46;	/* we just wrote this much */
	NumberOfOutputPages++;
}
