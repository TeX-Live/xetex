/****************************************************************
Copyright 1990 - 1995 by AT&T Bell Laboratories.

Permission to use, copy, modify, and distribute this software
and its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the names of AT&T Bell Laboratories or
any of its entities not be used in advertising or publicity
pertaining to distribution of the software without specific,
written prior permission.

AT&T disclaims all warranties with regard to this software,
including all implied warranties of merchantability and fitness.
In no event shall AT&T be liable for any special, indirect or
consequential damages or any damages whatsoever resulting from
loss of use, data or profits, whether in an action of contract,
negligence or other tortious action, arising out of or in
connection with the use or performance of this software.
****************************************************************/

/* This program transforms a MetaPost input file into a TeX or LaTeX input
 * file by stripping out btex...etex and verbatimtex...etex sections.
 * Leading and trailing spaces and tabs are removed from the extracted
 * material and it is surrounded by the preceding and following strings
 * defined immediately below.  The input file should be given as argument 1
 * and the resulting TeX file is written on standard output.
 */

/* Or we translate to troff.  Just a matter of different boilerplate.  */

#include "c-auto.h"	/* For WEB2CVERSION */
#include "cpascal.h"
#include <stdio.h>
#include <stdlib.h>
#include <kpathsea/config.h>
#include <kpathsea/variable.h>

#ifdef WIN32
#include <string.h>
#endif


/* MetaPost itself has a configurable max line length, copy the limits
   from mp.ch */
#define INF_BUF_SIZE  500
#define SUP_BUF_SIZE  300000
#define BUF_SIZE_NAME "buf_size"

char*	tex_predoc = "";
char*	tex_postdoc = "\\end{document}\n";
char*	tex_pretex1 = "\\shipout\\hbox{\\smash{\\hbox{\\hbox{%% line %d %s\n";
char*	tex_pretex = "\\shipout\\hbox{\\smash{\\hbox{\\hbox{%% line %d %s\n";
char*	tex_posttex = "}\\vrule width1sp}}}\n";
char*	tex_preverb1 = "";			/* if very first instance */
char*	tex_preverb = "%% line %d %s\n";	/* all other instances */
char*	tex_postverb = "%\n";

/* According to CSTR #54 the ".lf" directive should be ".lf %d %s",
 * not ".lf line %d %s" as used in the original code.  This affects
 * troff_pretex1, troff_pretex, troff_preverb1, troff_preverb.
 */
char*	troff_predoc = ".po 0\n";
char*	troff_postdoc = "";
char*	troff_pretex1 = ".lf %d %s\n";	/* first instance */
char*	troff_pretex = ".bp\n.lf %d %s\n";	/* subsequent instances */
char*	troff_posttex = "\n";
char*	troff_preverb1 = ".lf %d %s\n";
char*	troff_preverb = ".lf %d %s\n";
char*	troff_postverb = "\n";

char*	predoc;
char*	postdoc;
char*	pretex1;
char*	pretex;
char*	posttex;
char*	preverb1;
char*	preverb;
char*	postverb;

char *mpname;
FILE *mpfile;
int lnno = 0;		/* current line number */
int texcnt = 0;		/* btex..etex blocks so far */
int verbcnt = 0;	/* verbatimtex..etex blocks so far */
char *buf;		/* the input line */
long bufsize = 0;       /* the buffer size */
char *bb, *tt, *aa;	/* start of before, token, and after strings */

void err P1C(char *, msg)
{
	fprintf(stderr, "mpto: %s:%d: %s\n",
		mpname, lnno, msg);
	exit(1);
}

char *getline()	/* returns NULL on EOF or error, otherwise buf */
{
	int c;
	unsigned loc = 0;
	while ((c = getc (mpfile)) != EOF && c != '\n' && c != '\r') {
		buf[loc++] = c;
		if (loc == bufsize)
			err("Line is too long");
	}
	if (c == EOF) return NULL;
	buf[loc] = 0;
	if (c == '\r') {
		c = getc(mpfile);
		if (c != '\n') ungetc(c, mpfile);
	}
	lnno++;
	return buf;
}


/* Return nonzero if a prefix of string s matches the null-terminated string t
 * and the next character is not a letter of an underscore.
 */
int match_str P2C(char*, s, char *, t)
{
	while (*t!=0) {
		if (*s!=*t) return 0;
		s++; t++;
	}
	switch (*s) {
	case 'a': case 'c': case 'd': case 'f': case 'g': case 'h':
	case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
	case 'o': case 'p': case 'q': case 'r': case 's': case 't':
	case 'u': case 'w': case 'x': case 'y': case 'z': case '_':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
	case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
	case 'Y': case 'Z': case 'b': case 'e': case 'v':
		return 0;
	default:
		return 1;
	}
}


/* This function tries to express s as the concatenation of three strings
 * b, t, a, with the global pointers bb, tt, and aa set to the start of the
 * corresponding strings.  String t is either a quote mark, a percent sign,
 * or an alphabetic token "btex", "etex", or "verbatimtex".  (An alphabetic
 * token is a maximal sequence of letters and underscores.)  If there are
 * several possible substrings t, we choose the leftmost one.  If there is
 * no such t, we set b=s and return 0.
 */
int getbta P1C(char *, s)
{
	int ok=1;	/* zero if last character was a-z, A-Z, or _ */

	bb = s;
	for (tt=bb; *tt!=0; tt++)
		switch (*tt) {
		case '"': case '%':
			aa = tt+1;
			return 1;
		case 'b':
			if (ok && match_str(tt,"btex")) {
				aa = tt+4;
				return 1;
			} else ok=0;
			break;
		case 'e':
			if (ok && match_str(tt,"etex")) {
				aa = tt+4;
				return 1;
			} else ok=0;
			break;
		case 'v':
			if (ok && match_str(tt,"verbatimtex")) {
				aa = tt+11;
				return 1;
			} else ok=0;
			break;
		case 'a': case 'c': case 'd': case 'f': case 'g': case 'h':
		case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
		case 'o': case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'w': case 'x': case 'y': case 'z': case '_':
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
		case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
		case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z':
			ok = 0;
			break;
		default:
			ok = 1;
		}
	aa = tt;
	return 0;
}


void copytex()
{
	char *s;	/* where a string to print stops */
	char c;

	while (*aa==' ' || *aa=='\t') aa++;
	if (*aa==0)
		if ((aa=getline())==NULL)
			err("btex section does not end");
	do {	if (*aa==0)
			if ((aa=getline())==NULL)
				err("btex section does not end");
			else printf("\n");
		if (getbta(aa) && *tt=='e') {
			s = tt-1;
			while (s>=bb && (*s==' ' || *s=='\t'))
				s--;
			s++;
		} else {
			if (*tt=='b') err("btex in TeX mode");
			if (*tt=='v') err("verbatimtex in TeX mode");
			s = aa;
		}
		c = *s;
		*s = 0;
		printf("%s", bb);
		*s = c;
	} while (*tt!='e');
}


void do_line()
{
	aa = buf;
	while (getbta(aa))
		if (*tt=='%') break;
		else if (*tt=='"') {
			do if (!getbta(aa)) err("string does not end");
			while (*tt!='"');
		} else if (*tt=='b') {
			if (texcnt++ ==0) printf(pretex1,lnno,mpname);
			else printf(pretex,lnno,mpname);
			copytex();
			printf("%s",posttex);
		} else if (*tt=='v') {
			if (verbcnt++ ==0 && texcnt==0)
				printf(preverb1,lnno,mpname);
			else printf(preverb,lnno,mpname);
			copytex();
			printf("%s",postverb);
		} else err("unmatched etex");
}

static const_string MPTOHELP[] = {
    "Usage: mpto [-tex|-troff] MPFILE",
    "  Strip btex..etex and verbatimtex...etex parts from MetaPost input",
    "  file MPFILE, converting to either TeX or troff (TeX by default).",
    "",
    "--help      display this help and exit",
    "--version   output version information and exit",
#ifdef AMIGA
    "-E <errlog-file>",
#endif
    NULL
};
  
int main P2C(int, argc, char **, argv)
{
        int mode;
        char *buf_size_str;
        
        kpse_set_program_name(argv[0], NULL);
	
	if (argc == 1) {
	  fputs ("mpto: Need exactly one file argument.\n", stderr);
	  fputs ("Try `mpto --help' for more information.\n", stderr);
	  exit(1);
	} else if (argc > 1 && strcmp (argv[1], "--help") == 0) {
            usagehelp (MPTOHELP, NULL);
	} else if (argc > 1 && strcmp (argv[1], "--version") == 0) {
          printf ("mpto%s 0.63\n\
Copyright (C) 1996 AT&T Bell Laboratories.\n\
There is NO warranty.  You may redistribute this software\n\
under the terms of the GNU General Public License\n\
and the mpto copyright.\n\
For more information about these matters, see the files\n\
named COPYING and the mpto source.\n\
Primary author of mpto: John Hobby.\n",
WEB2CVERSION);
	  exit (0);
	} else if (argc == 2) {
	  mpname = argv[1];
	  mode = 0;
#ifdef AMIGA
	} else if (argc == 3) {
	  if (strcmp (argv[1], "-tex") == 0) {
	    mpname = argv[2];
	    mode = 0;
	  } else if (strcmp (argv[1], "-troff") == 0) {
	    mpname = argv[2];
	    mode = 1;
	  } else if (strncmp(argv[2], "-E", 2) || (argv[2]+2 == NULL)) {
	    usage(argv[0]);
	  } else {
	    mpname = argv[1];
	    freopen(argv[2]+2, "w", stderr);
	  }
	} else if (argc == 4) {
	  if (strcmp (argv[1], "-tex") == 0) {
	    mode = 0;
	  } else if (strcmp (argv[1], "-troff") == 0) {
	    mode = 1;
	  } else {
	    usage(argv[0]);
	  }
	  if (strncmp(argv[3], "-E", 2) || (argv[3]+2 == NULL)) {
	    usage(argv[0]);
	  } else {
	    mpname = argv[2];
	    freopen(argv[3]+2, "w", stderr);
	  }
	} else {
	  usage(argv[0]);
	}
#else /* not AMIGA */
	} else if (argc == 3) {
	  if (strcmp (argv[1], "-tex") == 0) {
	    mode = 0;
	  } else if (strcmp (argv[1], "-troff") == 0) {
	    mode = 1;
	  } else {
	    usage (argv[0]);
	  }
	  mpname = argv[2];
	} else {
	  usage(argv[0]);
	}
#endif /* not AMIGA */
	mpfile = fopen(mpname, "r");
	if (mpfile==NULL) {
 	  fprintf (stderr, "%s: ", argv[0]);
	  perror (mpname);
	  exit (1);
	}

        buf_size_str = kpse_var_value(BUF_SIZE_NAME);
        if (buf_size_str)
           bufsize = atoi(buf_size_str);
        if (bufsize < INF_BUF_SIZE)
           bufsize = INF_BUF_SIZE;
        if (bufsize > SUP_BUF_SIZE)
           bufsize = SUP_BUF_SIZE;
        buf = (char*)xmalloc(bufsize);
        if (buf_size_str) free(buf_size_str);

	/* This is far from elegant, but life is short.  */
	if (mode == 0) {
          predoc = tex_predoc;
          postdoc = tex_postdoc;
          pretex1 = tex_pretex1;
          pretex = tex_pretex;
          posttex = tex_posttex;
          preverb1 = tex_preverb1;
          preverb = tex_preverb;
          postverb = tex_postverb;
	} else {
          predoc = troff_predoc;
          postdoc = troff_postdoc;
          pretex1 = troff_pretex1;
          pretex = troff_pretex;
          posttex = troff_posttex;
          preverb1 = troff_preverb1;
          preverb = troff_preverb;
          postverb = troff_postverb;
	}
	printf("%s",predoc);
	while (getline()!=NULL)
		do_line();
	printf("%s",postdoc);
        free(buf);
	exit(0);
}
