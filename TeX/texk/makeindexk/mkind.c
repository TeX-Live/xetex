/*
 *
 *  This file is part of
 *	MakeIndex - A formatter and format independent index processor
 *
 *  Copyright (C) 1989 by Chen & Harrison International Systems, Inc.
 *  Copyright (C) 1988 by Olivetti Research Center
 *  Copyright (C) 1987 by Regents of the University of California
 *
 *  Author:
 *	Pehong Chen
 *	Chen & Harrison International Systems, Inc.
 *	Palo Alto, California
 *	USA
 *	(phc@renoir.berkeley.edu or chen@orc.olivetti.com)
 *
 *  Contributors:
 *	Please refer to the CONTRIB file that comes with this release
 *	for a list of people who have contributed to this and/or previous
 *	release(s) of MakeIndex.
 *
 *  All rights reserved by the copyright holders.  See the copyright
 *  notice distributed with this software for a complete description of
 *  the conditions under which it is made available.
 *
 */

#define MKIND_C	1
#include    "mkind.h"
#undef MKIND_C

int     letter_ordering = FALSE;
int     compress_blanks = FALSE;
int     merge_page = TRUE;
int     init_page = FALSE;
int     even_odd = -1;
int     verbose = TRUE;
int     german_sort = FALSE;
int     thai_sort = FALSE;
int     locale_sort = FALSE;
int     fn_no = -1;		       /* total number of files */
int     idx_dot = TRUE;		       /* flag which shows dot in ilg being
					* active */
int     idx_tt = 0;		       /* total entry count (all files) */
int     idx_et = 0;		       /* erroneous entry count (all files) */
int     idx_gt = 0;		       /* good entry count (all files) */

FIELD_PTR *idx_key;
FILE   *log_fp;
FILE   *sty_fp;
FILE   *idx_fp;
FILE   *ind_fp;
FILE   *ilg_fp;

char   *pgm_fn;
char    sty_fn[LINE_MAX];
char   *idx_fn;
char    ind[STRING_MAX];
char   *ind_fn;
char    ilg[STRING_MAX];
char   *ilg_fn;
char    pageno[NUMBER_MAX];

static char log_fn[STRING_MAX];
static char base[STRING_MAX];
static int need_version = TRUE;

static	void	check_all ARGS((char *fn,int ind_given,int ilg_given,
			int log_given));
static	void	check_idx ARGS((char *fn,int open_fn));
static	void	find_pageno ARGS((void));
static	void	open_sty ARGS((char *fn));
static	void	prepare_idx ARGS((void));
static	void	process_idx ARGS((char * *fn,int use_stdin,int sty_given,
			int ind_given,int ilg_given,int log_given));

#ifdef DEBUG
long totmem = 0L;			/* for debugging memory usage */
#endif /* DEBUG */

int
#if STDC
main(int argc, char *argv[])
#else
main(argc, argv)
int     argc;
char   *argv[];
#endif
{
    char   *fns[ARRAY_MAX];
    char   *ap;
    int     use_stdin = FALSE;
    int     sty_given = FALSE;
    int     ind_given = FALSE;
    int     ilg_given = FALSE;
    int     log_given = FALSE;

    /* determine program name */
    pgm_fn = strrchr(*argv, DIR_DELIM);
#ifdef ALT_DIR_DELIM
    {
      char *alt_delim = strrchr(*argv, ALT_DIR_DELIM);

      if (pgm_fn == NULL || (alt_delim != NULL && alt_delim > pgm_fn))
	pgm_fn = alt_delim;
    }
#endif
    if (pgm_fn == NULL)
	pgm_fn = *argv;
    else
	pgm_fn++;

#if USE_KPATHSEA
    kpse_set_progname (pgm_fn);  /* use the same name as the intro message */
#endif


    /* process command line options */
    while (--argc > 0) {
	if (**++argv == SW_PREFIX) {
	    if (*(*argv + 1) == NUL)
		break;
	    for (ap = ++*argv; *ap != NUL; ap++)
		switch (*ap) {
		    /* use standard input */
		case 'i':
		    use_stdin = TRUE;
		    break;

		    /* enable letter ordering */
		case 'l':
		    letter_ordering = TRUE;
		    break;

		    /* disable range merge */
		case 'r':
		    merge_page = FALSE;
		    break;

		    /* supress progress message -- quiet mode */
		case 'q':
		    verbose = FALSE;
		    break;

		    /* compress blanks */
		case 'c':
		    compress_blanks = TRUE;
		    break;

		    /* style file */
		case 's':
		    argc--;
		    if (argc <= 0)
			FATAL("Expected -s <stylefile>\n","");
		    open_sty(*++argv);
		    sty_given = TRUE;
		    break;

		    /* output index file name */
		case 'o':
		    argc--;
		    if (argc <= 0)
			FATAL("Expected -o <ind>\n","");
		    ind_fn = *++argv;
		    ind_given = TRUE;
		    break;

		    /* transcript file name */
		case 't':
		    argc--;
		    if (argc <= 0)
			FATAL("Expected -t <logfile>\n","");
		    ilg_fn = *++argv;
		    ilg_given = TRUE;
		    break;

		    /* initial page */
		case 'p':
		    argc--;
		    if (argc <= 0)
			FATAL("Expected -p <num>\n","");
		    strcpy(pageno, *++argv);
		    init_page = TRUE;
		    if (STREQ(pageno, EVEN)) {
			log_given = TRUE;
			even_odd = 2;
		    } else if (STREQ(pageno, ODD)) {
			log_given = TRUE;
			even_odd = 1;
		    } else if (STREQ(pageno, ANY)) {
			log_given = TRUE;
			even_odd = 0;
		    }
		    break;

		    /* enable german sort */
		case 'g':
		    german_sort = TRUE;
		    break;

#ifdef HAVE_SETLOCALE
		    /* enable locale-based sort */
		case 'L':
		    locale_sort = TRUE;
		    break;

		    /* enable Thai sort */
		case 'T':
		    thai_sort = locale_sort = TRUE;
		    break;
#endif

		    /* bad option */
		default:
		    FATAL("Unknown option -%c.\n", *ap);
		    break;
		}
	} else {
	    if (fn_no < ARRAY_MAX) {
		check_idx(*argv, FALSE);
		fns[++fn_no] = *argv;
	    } else {
		FATAL("Too many input files (max %d).\n", ARRAY_MAX);
	    }
	}
    }

	if (fn_no == 0 && !sty_given)
	{
		char tmp[STRING_MAX + 5];
		
		/* base set by last call to check_idx */
		sprintf (tmp, "%s%s", base, INDEX_STY);
		if (0 == access(tmp, R_OK)) {
			open_sty (tmp);
			sty_given = TRUE;
		}
	}
 
    process_idx(fns, use_stdin, sty_given, ind_given, ilg_given, log_given);
    idx_gt = idx_tt - idx_et;
    ALL_DONE;
    if (idx_gt > 0) {
	prepare_idx();
	sort_idx();
	gen_ind();
	MESSAGE("Output written in %s.\n", ind_fn);
    } else
	MESSAGE("Nothing written in %s.\n", ind_fn);

    MESSAGE("Transcript written in %s.\n", ilg_fn);
    CLOSE(ind_fp);
    CLOSE(ilg_fp);
    EXIT(0);

    return (0);			       /* never executed--avoids complaints */
    /* about no return value */
}


static void
prepare_idx(VOID_ARG)
{
    NODE_PTR ptr = head;
    int     i = 0;

#ifdef DEBUG
    totmem += idx_gt * sizeof(FIELD_PTR);
    (void)fprintf(stderr,"prepare_idx(): calloc(%d,%d)\ttotmem = %ld\n",
	idx_gt,sizeof(FIELD_PTR),totmem);
#endif /* DEBUG */

    if (head == (NODE_PTR)NULL)
	FATAL("No valid index entries collected.\n", "");

    if ((idx_key = (FIELD_PTR *) calloc(idx_gt, sizeof(FIELD_PTR))) == NULL) {
	FATAL("Not enough core...abort.\n", "");
    }
    for (i = 0; i < idx_gt; i++) {
	idx_key[i] = &(ptr->data);
	ptr = ptr->next;
    }
}


static void
#if STDC
process_idx(char *fn[], int use_stdin, int sty_given, int ind_given,
	    int ilg_given, int log_given)
#else
process_idx(fn, use_stdin, sty_given, ind_given, ilg_given, log_given)
char   *fn[];
int     use_stdin;
int     sty_given;
int     ind_given;
int     ilg_given;
int     log_given;
#endif
{
    int     i;

    if (fn_no == -1)
	/* use stdin if no input files specified */
	use_stdin = TRUE;
    else {
	check_all(fn[0], ind_given, ilg_given, log_given);
	PUT_VERSION;
	if (sty_given)
	    scan_sty();
	if (german_sort && (idx_quote == '"'))
FATAL("Option -g invalid, quote character must be different from '%c'.\n",
'"');
	scan_idx();
	ind_given = TRUE;
	ilg_given = TRUE;
	for (i = 1; i <= fn_no; i++) {
	    check_idx(fn[i], TRUE);
	    scan_idx();
	}
    }

    if (use_stdin) {
	idx_fn = "stdin";
	idx_fp = stdin;

	if (ind_given) {
	    if (!ind_fp && ((ind_fp = OPEN_OUT(ind_fn)) == NULL))
		FATAL("Can't create output index file %s.\n", ind_fn);
	} else {
	    ind_fn = "stdout";
	    ind_fp = stdout;
	}

	if (ilg_given) {
	    if (!ilg_fp && ((ilg_fp = OPEN_OUT(ilg_fn)) == NULL))
		FATAL("Can't create transcript file %s.\n", ilg_fn);
	} else {
	    ilg_fn = "stderr";
	    ilg_fp = stderr;
            /* This is already equivalent to verbose mode... */
            verbose = FALSE;
	}

	if ((fn_no == -1) && (sty_given))
	    scan_sty();
	if (german_sort && (idx_quote == '"'))
FATAL("Option -g ignored, quote character must be different from '%c'.\n",
'"');

	if (need_version) {
	    PUT_VERSION;
	}
	scan_idx();
	fn_no++;
    }
}


static void
#if STDC
check_idx(char *fn, int open_fn)
#else
check_idx(fn, open_fn)
char   *fn;
int     open_fn;
#endif
{
    char   *ptr = fn;
    char   *ext;
    int     with_ext = FALSE;
    int     i = 0;

    ext = strrchr(fn, EXT_DELIM);
    if ((ext != NULL) && (ext != fn) && (strchr(ext + 1, DIR_DELIM) == NULL)
#ifdef ALT_DIR_DELIM
	&& (strchr(ext + 1, ALT_DIR_DELIM) == NULL)
#endif
	) {
	with_ext = TRUE;
	while ((ptr != ext) && (i < STRING_MAX))
	    base[i++] = *ptr++;
    } else
	while ((*ptr != NUL) && (i < STRING_MAX))
	    base[i++] = *ptr++;

    if (i < STRING_MAX)
	base[i] = NUL;
    else
	FATAL2("Index file name %s too long (max %d).\n",
	       base, STRING_MAX);

    idx_fn = fn;

    if ( ( open_fn && 
	 ((idx_fp = OPEN_IN(idx_fn)) == NULL)
	 ) ||
	((!open_fn) && (access(idx_fn, R_OK) != 0)))
	if (with_ext) {
	    FATAL("Input index file %s not found.\n", idx_fn);
	} else {

#ifdef DEBUG
	    totmem += STRING_MAX;
	    (void)fprintf(stderr,"check_idx()-2: malloc(%d)\ttotmem = %ld\n",
		    STRING_MAX,totmem);
#endif /* DEBUG */

	    if ((idx_fn = (char *) malloc(STRING_MAX)) == NULL)
		FATAL("Not enough core...abort.\n", "");
	    sprintf(idx_fn, "%s%s", base, INDEX_IDX);
	    if ((open_fn && 
	 ((idx_fp = OPEN_IN(idx_fn)) == NULL)
	) ||
		((!open_fn) && (access(idx_fn, R_OK) != 0))) {
		FATAL2("Couldn't find input index file %s nor %s.\n", base,
		       idx_fn);
	    }
	}
}


static void
#if STDC
check_all(char *fn, int ind_given, int ilg_given, int log_given)
#else
check_all(fn, ind_given, ilg_given, log_given)
char   *fn;
int     ind_given;
int     ilg_given;
int     log_given;
#endif
{
    check_idx(fn, TRUE);

    /* index output file */
    if (!ind_given) {
	sprintf(ind, "%s%s", base, INDEX_IND);
	ind_fn = ind;
    }
    if ((ind_fp = OPEN_OUT(ind_fn)) == NULL)
	FATAL("Can't create output index file %s.\n", ind_fn);

    /* index transcript file */
    if (!ilg_given) {
	sprintf(ilg, "%s%s", base, INDEX_ILG);
	ilg_fn = ilg;
    }
    if ((ilg_fp = OPEN_OUT(ilg_fn)) == NULL)
	FATAL("Can't create transcript file %s.\n", ilg_fn);

    if (log_given) {
	sprintf(log_fn, "%s%s", base, INDEX_LOG);
	if ((log_fp = OPEN_IN(log_fn)) == NULL) {
	    FATAL("Source log file %s not found.\n", log_fn);
	} else {
	    find_pageno();
	    CLOSE(log_fp);
	}
    }
}


static void
find_pageno(VOID_ARG)
{
    int     i = 0;
    int     p, c;

    fseek(log_fp, -1L, 2);
    p = GET_CHAR(log_fp);
    fseek(log_fp, -2L, 1);
    do {
	c = p;
	p = GET_CHAR(log_fp);
    } while (!(((p == LSQ) && isdigit(c)) || (fseek(log_fp, -2L, 1) != 0)));
    if (p == LSQ) {
	while ((c = GET_CHAR(log_fp)) == SPC);
	do {
	    pageno[i++] = (char) c;
	    c = GET_CHAR(log_fp);
	} while (isdigit(c));
	pageno[i] = NUL;
    } else {
	fprintf(ilg_fp, "Couldn't find any page number in %s...ignored\n",
		log_fn);
	init_page = FALSE;
    }
}

static void
#if STDC
open_sty(char *fn)
#else
open_sty(fn)
char   *fn;
#endif
{
    char   *path;
    char   *ptr;
    int     i;
    int     len;
#if USE_KPATHSEA
    char   *found;

  if ((found = kpse_find_file (fn, kpse_ist_format, 1)) == NULL) {
     FATAL("Index style file %s not found.\n", fn);
  } else {
    strcpy(sty_fn,found);
    if ((sty_fp = OPEN_IN(sty_fn)) == NULL) {
      FATAL("Could not open style file %s.\n", sty_fn);
    }
  }
#else
    if ((path = getenv(STYLE_PATH)) == NULL) {
	/* style input path not defined */
	strcpy(sty_fn, fn);
	sty_fp = OPEN_IN(sty_fn);
    } else {
	len = ARRAY_MAX - strlen(fn) - 1;
	while (*path != NUL) {
	    ptr = strchr(path, ENV_SEPAR);
	    i = 0;
	    if (ptr == (char*)NULL)
	    {
		int j = strlen(path);

		while (i < j)
		    sty_fn[i++] = *path++;
	    }
	    else
	    {
		while ((path != ptr) && (i < len))
		    sty_fn[i++] = *path++;
	    }
	    if (i == len) {
		FATAL2("Path %s too long (max %d).\n", sty_fn, ARRAY_MAX);
	    } else {
		sty_fn[i++] = DIR_DELIM;
		sty_fn[i] = NUL;
		strcat(sty_fn, fn);
		if ((sty_fp = OPEN_IN(sty_fn)) == NULL)
		    path++;
		else
		    break;
	    }
	}
    }

    if (sty_fp == NULL)
	FATAL("Index style file %s not found.\n", fn);
#endif
}


int
#if STDC
strtoint(char *str)
#else
strtoint(str)
char   *str;
#endif
{
    int     val = 0;

    while (*str != NUL) {
	val = 10 * val + *str - 48;
	str++;
    }
    return (val);
}
