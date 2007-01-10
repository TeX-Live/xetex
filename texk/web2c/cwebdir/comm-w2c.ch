% Kpathsea changes for CWEB by Wlodek Bzyl and Olaf Weber
% Copyright 2002 Wlodek Bzyl and Olaf Weber
% This file is in the Public Domain.

@x l.20
\def\title{Common code for CTANGLE and CWEAVE (Version 3.64)}
\def\topofcontents{\null\vfill
  \centerline{\titlefont Common code for {\ttitlefont CTANGLE} and
    {\ttitlefont CWEAVE}}
  \vskip 15pt
  \centerline{(Version 3.64)}
  \vfill}
@y
\def\Kpathsea/{{\mc KPATHSEA\spacefactor1000}}
\def\title{Common code for CTANGLE and CWEAVE (Version 3.64k)}
\def\topofcontents{\null\vfill
  \centerline{\titlefont Common code for {\ttitlefont CTANGLE} and
    {\ttitlefont CWEAVE}}
  \vskip 15pt
  \centerline{(Version 3.64k)}
  \vfill}
@z

This change can not be applied when `tie' is  used
(TOC file can not be typeset).

%@x l.42
%\let\maybe=\iftrue
%@y
%\let\maybe=\iffalse % print only changed modules
%@z


Section 2. 
We use the definition from `kpathsea/types.h':

  typedef enum { false = 0, true = 1 } boolean;

Note that this definition also occurs in common.h.
@x l.74
typedef short boolean;
@y
@z


Section 4.

@x l.91
common_init()
@y
common_init P1H(void)
@z

@x l.93
  @<Initialize pointers@>;
@y
  @<Initialize pointers@>;
  @<Set up |PROGNAME| feature and initialize the search path mechanism@>;
@z

Section 9.

@x l.173
int input_ln(fp) /* copies a line into |buffer| or returns 0 */
FILE *fp; /* what file to read from */
@y
int input_ln P1C(FILE *, fp) /* copies a line into |buffer| or returns 0 */
@z

Section 10.

@x l.207 - max_file_name_length is way too small.
@d max_file_name_length 60
@y
@d max_file_name_length 1024
@z

@x l.221 - no alt_web_file_name needed.
char alt_web_file_name[max_file_name_length]; /* alternate name to try */
@y
@z

Section 12.

@x l.254
prime_the_change_buffer()
@y
prime_the_change_buffer P1H(void)
@z

Section 16.

@x l.322
check_change() /* switches to |change_file| if the buffers match */
@y
check_change P1H(void) /* switches to |change_file| if the buffers match */
@z

Section 18.

@x l.380
reset_input()
@y
reset_input P1H(void)
@z

Section 19.

@x l.394
if ((web_file=fopen(web_file_name,"r"))==NULL) {
  strcpy(web_file_name,alt_web_file_name);
  if ((web_file=fopen(web_file_name,"r"))==NULL)
       fatal("! Cannot open input file ", web_file_name);
}
@y
if ((found_filename=kpse_find_cweb(web_file_name))==NULL ||
    (web_file=fopen(found_filename,"r"))==NULL) {
  fatal("! Cannot open input file ", web_file_name);
} else if (strlen(found_filename) < max_file_name_length) {
  strcpy(web_file_name, found_filename);
  free(found_filename);
}
@z

@x l.402
if ((change_file=fopen(change_file_name,"r"))==NULL)
       fatal("! Cannot open change file ", change_file_name);
@y
if ((found_filename=kpse_find_cweb(change_file_name))==NULL ||
    (change_file=fopen(found_filename,"r"))==NULL) {
  fatal("! Cannot open change file ", change_file_name);
} else if (strlen(found_filename) < max_file_name_length) {
  strcpy(change_file_name, found_filename);
  free(found_filename);
}
@z

Section 21.

@x l.427
int get_line() /* inputs the next line */
@y
int get_line P1H(void) /* inputs the next line */
@z

Section 22.

@x l.472
#include <stdlib.h> /* declaration of |getenv| and |exit| */
@y
#include <stdlib.h> /* declaration of |getenv| and |exit| */
#define CWEB
#include "cpascal.h"
#include <kpathsea/kpathsea.h> /* include every \Kpathsea/ header */
#include "help.h"

@ The \.{ctangle} and \.{cweave} programs from the original \.{CWEB}
package use the compile-time default directory or the value of the
environment variable \.{CWEBINPUTS} as an alternative place to be
searched for files, if they could not be found in the current
directory.

This version uses the \Kpathsea/ mechanism for searching files. 
The directories to be searched for come from three sources:

 (a)~a user-set environment variable \.{CWEBINPUTS}
    (overriden by \.{CWEBINPUTS\_cweb});\par
 (b)~a line in \Kpathsea/ configuration file \.{texmf.cnf},\hfil\break
    e.g. \.{CWEBINPUTS=.:$TEXMF/texmf/cweb//}
    or \.{CWEBINPUTS.cweb=.:$TEXMF/texmf/cweb//};\hangindent=2\parindent\par
 (c)~compile-time default directories \.{.:$TEXMF/texmf/cweb//}
    (specified in \.{texmf.in}).


@d kpse_find_cweb(name) kpse_find_file(name,kpse_cweb_format,true)

@ The simple file searching is replaced by `path searching' mechanism
that \Kpathsea/ library provides.

We set |kpse_program_name| to a |"cweb"|.  This means if the
variable |CWEBINPUTS.cweb| is present in \.{texmf.cnf} (or |CWEBINPUTS_cweb|
in the environment) its value will be used as the search path for
filenames.  This allows different flawors of \.{CWEB} to have
different search paths.

FIXME: Not sure this is the best way to go about this.

@<Set up |PROGNAME| feature and initialize the search path mechanism@>=
kpse_set_program_name(argv[0], "cweb"); 
@z


Section 23.

@x l.475
  char temp_file_name[max_file_name_length];
  char *cur_file_name_end=cur_file_name+max_file_name_length-1;
  char *k=cur_file_name, *kk;
  int l; /* length of file name */
@y
  char *cur_file_name_end=cur_file_name+max_file_name_length-1;
  char *k=cur_file_name;
@z

@x l.489
  if ((cur_file=fopen(cur_file_name,"r"))!=NULL) {
@y
  if ((found_filename=kpse_find_cweb(cur_file_name))!=NULL &&
      (cur_file=fopen(found_filename,"r"))!=NULL) {
    /* Copy name for #line directives. */
    if (strlen(found_filename) < max_file_name_length) {
      strcpy(cur_file_name, found_filename);
      free(found_filename);
    }
@z

Replaced by Kpathsea `kpse_find_file'

@x l.493
  kk=getenv("CWEBINPUTS");
  if (kk!=NULL) {
    if ((l=strlen(kk))>max_file_name_length-2) too_long();
    strcpy(temp_file_name,kk);
  }
  else {
#ifdef CWEBINPUTS
    if ((l=strlen(CWEBINPUTS))>max_file_name_length-2) too_long();
    strcpy(temp_file_name,CWEBINPUTS);
#else
    l=0;
#endif /* |CWEBINPUTS| */
  }
  if (l>0) {
    if (k+l+2>=cur_file_name_end)  too_long();
@.Include file name ...@>
    for (; k>= cur_file_name; k--) *(k+l+1)=*k;
    strcpy(cur_file_name,temp_file_name);
    cur_file_name[l]='/'; /* \UNIX/ pathname separator */
    if ((cur_file=fopen(cur_file_name,"r"))!=NULL) {
      cur_line=0; print_where=1;
      goto restart; /* success */
    }
  }
@y
@z

Section 26.

@x l.571
check_complete(){
@y
check_complete P1H(void) {
@z

Section 33.

@x l.651
extern int names_match();
@y
extern int names_match P4H(name_pointer, char*, int, char);
@z

Section 35.

@x l.661
id_lookup(first,last,t) /* looks up a string in the identifier table */
char *first; /* first character of string */
char *last; /* last character of string plus one */
char t; /* the |ilk|; used by \.{CWEAVE} only */
@y
/* looks up a string in the identifier table */
id_lookup P3C(char*,first, char*,last, char,t)
@z

Section 38.

@x l.704
void init_p();
@y
extern void init_p P2C(name_pointer,p, char,t);
@z

Section 42.

@x l.766
print_section_name(p)
name_pointer p;
@y
print_section_name P1C(name_pointer, p)
@z

Section 43.

@x l.785
sprint_section_name(dest,p)
  char*dest;
  name_pointer p;
@y
sprint_section_name P2C(char*,dest, name_pointer,p)
@z

Section 44.

@x l.806
print_prefix_name(p)
name_pointer p;
@y
print_prefix_name P1C(name_pointer,p)
@z

Section 45.

@x l.826
int web_strcmp(j,j_len,k,k_len) /* fuller comparison than |strcmp| */
  char *j, *k; /* beginning of first and second strings */
  int j_len, k_len; /* length of strings */
@y
/* fuller comparison than |strcmp| */
int web_strcmp P4C(char*,j, int,j_len, char*,k, int,k_len)
@z

Section 46.

@x l.853
extern void init_node();
@y
extern void init_node P1C(name_pointer,node);
@z

Section 47.

@x l.857
add_section_name(par,c,first,last,ispref) /* install a new node in the tree */
name_pointer par; /* parent of new node */
int c; /* right or left? */
char *first; /* first character of section name */
char *last; /* last character of section name, plus one */
int ispref; /* are we adding a prefix or a full name? */
@y
/* install a new node in the tree */
add_section_name P5C(name_pointer,par, int,c, char*,first, char*,last,
                     int,ispref)
@z

Section 48.

@x l.886
extend_section_name(p,first,last,ispref)
name_pointer p; /* name to be extended */
char *first; /* beginning of extension text */
char *last; /* one beyond end of extension text */
int ispref; /* are we adding a prefix or a full name? */
@y
extend_section_name P4C(name_pointer,p, char*,first, char*,last, int,ispref)
@z

Section 49.

@x l.914
section_lookup(first,last,ispref) /* find or install section name in tree */
char *first, *last; /* first and last characters of new name */
int ispref; /* is the new name a prefix or a full name? */
@y
/* find or install section name in tree */
section_lookup P3C(char*,first, char*,last, int,ispref)
@z

Section 53.

@x l.1018
int section_name_cmp();
@y
int section_name_cmp P3H(char**, int, name_pointer);
@z

Section 54.

@x l.1021
int section_name_cmp(pfirst,len,r)
char **pfirst; /* pointer to beginning of comparison string */
int len; /* length of string */
name_pointer r; /* section name being compared */
@y
int section_name_cmp P3C(char**,pfirst, int,len, name_pointer,r)
@z

Section 57.

@x l.1093
void  err_print();
@y
void  err_print P1H(char*);
@z

Section 58.

@x l.1098
err_print(s) /* prints `\..' and location of error message */
char *s;
@y
err_print P1C(char*,s) /* prints `\..' and location of error message */
@z

Section 60.

@x l.1141
int wrap_up();
extern void print_stats();
@y
int wrap_up P1H(void);
extern void print_stats P1H(void);
@z

Section 61.

@x l.1151
int wrap_up() {
@y
int wrap_up P1H(void) {
@z

Section 63.

@x l.1174
void fatal(), overflow();
@y
void fatal P2H(char*,char*);
void overflow(char*);
@z

Section 64.

@x l.1180
fatal(s,t)
  char *s,*t;
@y
fatal P2C(char*,s, char*,t)
@z

Section 65.

@x l.1191
overflow(t)
  char *t;
@y
overflow P1C(char*,t)
@z

Section 67.

@x l.1212
the names of those files. Most of the 128 flags are undefined but available
for future extensions.
@y
the names of those files. Most of the 128 flags are undefined but available
for future extensions.

We use `kpathsea' library functions to find literate sources and
NLS configuration files. When the files you expect are not
being found, the thing to do is to enable `kpathsea' runtime
debugging by assigning to |kpathsea_debug| variable a small number
via `\.{-d}' option. The meaning of number is shown below. To set
more than one debugging options sum the corresponding numbers.
$$\halign{\hskip5em\tt\hfil#&&\qquad\tt#\cr
 1&report `\.{stat}' calls\cr
 2&report lookups in all hash tables\cr
 4&report file openings and closings\cr
 8&report path information\cr
16&report directory list\cr
32&report on each file search\cr
64&report values of variables being looked up\cr}$$
Debugging output is always written to |stderr|, and begins with the string
`\.{kdebug:}'.
@z

@x l.1218
@d show_happiness flags['h'] /* should lack of errors be announced? */
@y
@d show_happiness flags['h'] /* should lack of errors be announced? */
@d show_kpathsea_debug flags['d']
  /* should results of file searching be shown? */
@z

@x l.1234
show_banner=show_happiness=show_progress=1;
@y
show_banner=show_happiness=show_progress=1;
@z

Section 69.

@x l.1252
void scan_args();
@y
void scan_args P1H(void);
@z


Section 70.

@x l.1257
scan_args()
@y
scan_args P1H(void)
@z


Section 71.

@x l.1282 - use a define for /dev/null
  if (found_change<=0) strcpy(change_file_name,"/dev/null");
@y
  if (found_change<=0) strcpy(change_file_name,DEV_NULL);
@z

@x l.1302 - no alt_web_file_name
  sprintf(alt_web_file_name,"%s.web",*argv);
@y
@z


Section 74.

@x l.1344
@ @<Handle flag...@>=
{
@y
@ @<Handle flag...@>=
{
  if (strcmp("-help",*argv)==0 || strcmp("--help",*argv)==0)
    @<Display help message and exit@>;
  if (strcmp("-version",*argv)==0 || strcmp("--version",*argv)==0)
    @<Display version information and exit@>;
@z

@x l.1347
  else flag_change=1;
@y
  else flag_change=1;
  if (*(*argv+1)=='d')
    if (sscanf(*argv+2,"%u",&kpathsea_debug)!=1) @<Print usage error...@>;
@z

@x l.1349
    flags[*dot_pos]=flag_change;
@y
    flags[(unsigned char)*dot_pos]=flag_change;
@z

Section 75.

@x l.1354
if (program==ctangle)
  fatal(
"! Usage: ctangle [options] webfile[.w] [{changefile[.ch]|-} [outfile[.c]]]\n"
   ,"");
@.Usage:@>
else fatal(
"! Usage: cweave [options] webfile[.w] [{changefile[.ch]|-} [outfile[.tex]]]\n"
   ,"");
@y
if (program==ctangle) {
  fprintf(stderr, "ctangle: Need one to three file arguments.\n");
  usage("ctangle");
} else {
  fprintf(stderr, "cweave: Need one to three file arguments.\n");
  usage("cweave");
}
@z

Section 77.

@x l.1375
FILE *active_file; /* currently active file for \.{CWEAVE} output */
@y
FILE *active_file; /* currently active file for \.{CWEAVE} output */
char *found_filename; /* filename found by |kpse_find_file| */
@z


Section 81. (removed)

@x l.1403
@ We predeclare several standard system functions here instead of including
their system header files, because the names of the header files are not as
standard as the names of the functions. (For example, some \CEE/ environments
have \.{<string.h>} where others have \.{<strings.h>}.)

@<Predecl...@>=
extern int strlen(); /* length of string */
extern int strcmp(); /* compare strings lexicographically */
extern char* strcpy(); /* copy one string to another */
extern int strncmp(); /* compare up to $n$ string characters */
extern char* strncpy(); /* copy up to $n$ string characters */
@y
@z

@x
@** Index.
@y
@** System dependent changes.

@ Modules for dealing with help messages and version info.

@<Display help message and exit@>=
usagehelp(program==ctangle ? CTANGLEHELP : CWEAVEHELP, NULL);
@.--help@>

@ Will have to change these if the version numbers change (ouch).

@d ctangle_banner "This is CTANGLE, Version 3.64"
@d cweave_banner "This is CWEAVE, Version 3.64"

@<Display version information and exit@>=
printversionandexit((program==ctangle ? ctangle_banner : cweave_banner),
  "Silvio Levy and Donald E. Knuth", NULL, NULL);
@.--version@>

@** Index.
@z
