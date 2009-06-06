/* cpascal.h: implement various bits of standard and other Pascal that
   we use in the change files.  Public domain.
   
   This is the top-level include file for all the web2c-generated C
   programs except TeX and Metafont themselves, which use texmf.h.  It's
   not included by the web2c programs, though.  */

#ifndef CPASCAL_H
#define CPASCAL_H

#ifdef WIN32
#pragma warning( disable : 4018 4244 )  
#endif

/* We must include this first, to resolve many C issues.  */
#include "config.h"

/* We only use getopt in the applications, not in web2c itself.  */
#include <kpathsea/getopt.h>

/* Almost everybody needs path searching.  May as well always include
   them and simplify the change files.  */
#include <kpathsea/progname.h>
#include <kpathsea/proginit.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/variable.h>

/* Help messages.  */
#include "help.h"

/* Allow translation files.  */
#define	Xchr(x) xchr[x]

/* Pieces of predefined Pascal web2c doesn't convert.  */

/* Absolute value.  Without the casts to integer here, the Ultrix and
   AIX compilers (at least) produce bad code (or maybe it's that I don't
   understand all the casting rules in C) for tests on memory fields. 
   Specifically, a test in diag_round (in Metafont) on a quarterword
   comes out differently without the cast, thus causing the trap test to
   fail.  (A path at line 86 is constructed slightly differently).  */
/* If the system had an abs #define already, get rid of it.  */
#undef abs
#define abs(x) ((integer)(x) >= 0 ? (integer)(x) : (integer)-(x))

#define chr(x)		(x)
#define ord(x)		(x)
#define odd(x)		((x) & 1)
#define round(x)	zround ((double) (x))
#define trunc(x)	((integer) (x))
#undef floor /* MacOSX */
#define floor(x)	((integer)floor((double)(x)))
#define input stdin
#define output stdout
#define maxint INTEGER_MAX
#define nil NULL

#define floorunscaled(i) ((i)>>16)
#define floorscaled(i) ((i)&(-65536))
#define roundunscaled(i) (((i>>15)+1)>>1)
#define roundfraction(i) (((i>>11)+1)>>1)
#ifndef TeX
/* In TeX, the half routine is always applied to positive integers.
   In MF and MP, it isn't; therefore, we can't portably use the C shift
   operator -- whether zeros or the sign bit will be shifted in on
   negative left operands is implementation-defined.
   
   It might be worth going through MF and using halfp where possible, as
   in MP.  */
#define half(i) ( ((i)<0) ? (((i)+1)>>1) : ((i)>>1) )
#endif
#define halfp(i) ((i) >> 1)

/* Standard Pascal file routines.  These are used for both binary and
   text files, but binary is more common.  If you want it 100% right,
   fix the change files to pass the fopen mode to reset in all cases and
   send me the changes; it doesn't matter for Unix, so I'm not going to
   spend any more time on it.  */
#define reset(f,n) f = xfopen (n, FOPEN_R_MODE)
#define rewrite(f,n) f = xfopen (n, FOPEN_W_MODE)
#define resetbin(f,n) f = xfopen (n, FOPEN_RBIN_MODE)
#define rewritebin(f,n) f = xfopen (n, FOPEN_WBIN_MODE)

#if defined(read)
#undef read
#endif
#define read(f,b) ((b) = getc (f))

/* We hope this will be efficient than the `x = x - 1' that decr would
   otherwise be translated to.  Likewise for incr.  */
#define decr(x) --(x)
#define incr(x) ++(x)

/* `real' is used for noncritical floating-point stuff.  */
typedef double real;

/* C doesn't need to distinguish between text files and other files.  */
typedef FILE *text;

/* Extra stuff used in various change files for various reasons.  */

/* Pascal has no address-of operator, and we need pointers to integers
   to set up the option table.  */
#define addressof(x) (&(x))

/* So dvicopy can use stdin/stdout.  */
#if defined (__DJGPP__) || defined (WIN32)
#include <io.h>
/* Don't set console device to binary.  */
#define makebinaryfile(arg) ((!isatty(fileno(arg)) && setmode(fileno(arg), O_BINARY)), arg)
#else
#define makebinaryfile(arg) (arg)
#endif

/* It's not worth fixing fixwrites to handle Pascal-style n:m write
   specifiers for reals, so the change files call print_real instead.  */
#define printreal(r,n,m) fprintreal (stdout, r, n, m)

/* Write the byte X to the file F.  */
#define putbyte(x,f) \
 do { if (putc ((char) (x) & 255, f) == EOF) \
        FATAL1 ("putbyte(%ld) failed", (long) x); } while (0)

/* To work around casting problems.  */
#define intcast(x) ((integer) (x))
#define stringcast(x) ((string) (x))
#define conststringcast(x) ((const_string) (x))

/* For throwing away input from the file F.  */
#define vgetc(f) (void) getc (f)

/* The fixwrites program outputs this, for diagnostics and such, that
   aren't worth checking the return value on.  */
#define Fputs(f,s) (void) fputs (s, f)

/* `aopenin' is used for all kinds of input text files, so it
   needs to know what path to use.  Used by BibTeX, MF, TeX.  */
#define aopenin(f,p) open_input (&(f), p, FOPEN_RBIN_MODE)
#define aopenout(f)  open_output (&(f), FOPEN_W_MODE)
#define aclose close_file

/* For faking arrays in tftopl.  */
typedef unsigned char *pointertobyte;
#define casttobytepointer(e) ((pointertobyte) e)

/* Write out elements START through END of BUF to the file F.  For gftodvi.  */
#define writechunk(f, buf, start, end) \
  (void) fwrite (&buf[start], sizeof (buf[start]), end - start + 1, f)

/* PatGen 2 uses this.  */
#define input2ints(a,b) zinput2ints (&a, &b)

/* We need this only if TeX is being debugged.  */
#define input3ints(a,b,c) zinput3ints (&a, &b, &c)

/* Allocate an array of a given type. Add 1 to size to account for the
   fact that Pascal arrays are used from [1..size], unlike C arrays which
   use [0..size). */
#define xmallocarray(type,size) ((type*)xmalloc((size+1)*sizeof(type)))
/* Same for reallocating an array. */
#define xreallocarray(ptr,type,size) ((type*)xrealloc(ptr,(size+1)*sizeof(type)))

/* BibTeX needs this to dynamically reallocate arrays.  Too bad we can't
   rely on stringification, or we could avoid the ARRAY_NAME arg.
   Actually allocate one more than requests, so we can index the last
   entry, as Pascal wants to do.  */
#define BIBXRETALLOC(array_name, array_var, type, size_var, new_size) do { \
  fprintf (logfile, "Reallocated %s (elt_size=%d) to %ld items from %ld.\n", \
           array_name, (int) sizeof (type), new_size, size_var); \
  XRETALLOC (array_var, new_size + 1, type); \
  size_var = new_size; \
} while (0)
  
/* Need precisely int for getopt, etc. */
#define cinttype int

/* Need this because web2c doesn't translate `var1,var2:^char' correctly
   -- var2 lacks the *.  */
#define cstring string

#define constcstring const_string

/* Not all C libraries have fabs, so we'll roll our own.  */
#undef fabs
#define fabs(x) ((x) >= 0.0 ? (x) : -(x))

/* TeX et al. have a variable free, but we also need the C routine.  */
#define libcfree free

/* We have a system-dependent prompt in tex.ch.  We don't want it in the
   string pool, since (now that the pools are compiled into the
   binaries), that would make the .fmt unsharable.  So go through this
   circumlotion to print a C string.  The lack of the closing ) is
   intentional, since the code adds more text sometimes.  Although the
   eof character can be changed with stty or whatever, we're certainly
   not going to try to extract the actual value from a terminal struct.
   Anyone who is savvy enough to change it will not be confused.  */
#ifdef WIN32
#define promptfilenamehelpmsg "(Press Enter to retry, or Control-Z to exit"
#else
#define promptfilenamehelpmsg "(Press Enter to retry, or Control-D to exit"
#endif

/* We use this rather than a simple fputs so that the string will end up
   in the .log file, too.  */
#define printcstring(STR)        \
  do {                           \
    const_string ch_ptr = (STR); \
    while (*ch_ptr)              \
      printchar(*(ch_ptr++));    \
  } while (0)


/* Tangle removes underscores from names.  Put them back for things that
   are defined in C with _'s.  */
#define extendfilename	extend_filename
#define findsuffix	find_suffix
#define FOPENRBINMODE	FOPEN_RBIN_MODE
#define FOPENRMODE	FOPEN_R_MODE
#define getoptlongonly	getopt_long_only
#define hasarg		has_arg
#define ISDIRSEP	IS_DIR_SEP
#define kpsebibformat	kpse_bib_format
#define kpsebstformat	kpse_bst_format
#define kpsefindfile	kpse_find_file
#define kpsefindmf	kpse_find_mf
#define kpsefindmft	kpse_find_mft
#define kpsefindofm    kpse_find_ofm
#define kpsefindovf    kpse_find_ovf
#define kpsefindtex	kpse_find_tex
#define kpsefindtfm	kpse_find_tfm
#define kpsefindvf	kpse_find_vf
#define kpseinitprog	kpse_init_prog
#define kpsesetprogname kpse_set_progname
#define kpsesetprogramname kpse_set_program_name
#define kpseresetprogramname kpse_reset_program_name
#define kpsegfformat	kpse_gf_format
#define kpselastformat	kpse_last_format
#define kpsemaketexdiscarderrors kpse_make_tex_discard_errors
#define kpsemfformat	kpse_mf_format
#define kpsemftformat	kpse_mft_format
#define kpsempformat	kpse_mp_format
#define kpseocpformat	kpse_ocp_format
#define kpseofmformat	kpse_ofm_format
#define kpseoplformat	kpse_opl_format
#define kpseotpformat	kpse_otp_format
#define kpseovpformat	kpse_ovp_format
#define kpseovfformat	kpse_ovf_format
#define kpseopenfile	kpse_open_file
#define kpsepkformat	kpse_pk_format
#define kpsetfmformat	kpse_tfm_format
#define kpsevfformat	kpse_vf_format
#define kpsewebformat	kpse_web_format
#define kpsevarvalue	kpse_var_value
#define kpsesetprogramenabled	kpse_set_program_enabled
#define kpsesrccmdline	kpse_src_cmdline
#define makesuffix	make_suffix
#define recorderchangefilename	recorder_change_filename
#define recorderenabled	recorder_enabled
#define removesuffix	remove_suffix

/* We need a new type for the argument parsing, too.  */
typedef struct option getoptstruct;

/* We never need the `link' system call, which may be declared in
   <unistd.h>, but we do have variables named `link' in the webs.  */
#undef link
#define link link_var

/* Throw away VMS' library routine `getname', as WEB uses that name.  */
#ifdef VMS
#undef getname
#define getname vms_getname
#endif

/* Apparently POSIX 2008 has getline and glibc 2.9.90 exports it.
   tangle, weave, et al. use that symbol; try to define it away so
   something that a standard won't usurp.  */
#ifdef getline
#undef getline
#endif
#define getline web2c_getline


/* Declarations for the routines we provide ourselves in lib/.  */

extern string basenamechangesuffix P3H(const_string,const_string,const_string);
extern string chartostring P1H(char);
extern boolean eof P1H(FILE *);
extern boolean eoln P1H(FILE *);
extern void readln P1H(FILE *);
extern void fprintreal P4H(FILE *, double, int, int);
extern integer inputint P1H(FILE *);
extern int loadpoolstrings P1H(integer);
extern void printversionandexit P4H(const_string, const_string, const_string, char*);
extern void zinput2ints P2H(integer *, integer *);
extern void zinput3ints P3H(integer *, integer *, integer *);
extern integer zround P1H(double);

/* main.c */
extern int argc;
extern string *argv;
extern string cmdline P1H(int);
extern TEXDLL void mainbody P1H(void); /* generated by web2c */

/* openclose.c */
extern boolean open_input P3H(FILE **, int, const_string fopen_mode);
extern boolean open_output P2H(FILE **, const_string fopen_mode);
extern void close_file P1H(FILE *);
extern void recorder_change_filename P1H(string);
extern boolean recorder_enabled;
extern string output_directory;
extern void recorder_record_input P1H(const_string);
extern void recorder_record_output P1H(const_string);

/* version.c */
extern string versionstring;

#endif /* not CPASCAL_H */
