/* 
Copyright (c) 2008 jerome DOT laurens AT u-bourgogne DOT fr

This file is part of the SyncTeX package.

License:
--------
Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE

Except as contained in this notice, the name of the copyright holder  
shall not be used in advertising or otherwise to promote the sale,  
use or other dealings in this Software without prior written  
authorization from the copyright holder.

Important notice:
-----------------
This file is named "synctex.c", it may or may not have a header counterpart
depending on its use.  It aims to provide basic components useful for the
input/output synchronization technology for TeX.
The purpose of the implementation is threefold
- firstly, it defines a new input/output synchronization technology named
  "synchronize texnology", "SyncTeX" or "synctex"
- secondly, it defines the naming convention and format of the auxiliary file
  used by this technology
- thirdly, it defines the API of a controller and a controller, used in
  particular by the pdfTeX and XeTeX programs to prepare synchronization.

All these are up to a great extent de facto definitions, which means that they
are partly defined by the implementation itself.

This technology was first designed for pdfTeX, an extension of TeX managing the
pdf output file format, but it can certainly be adapted to other programs built
from TeX as long as the extensions do not break too much the core design.
Moreover, the synchronize texnology only relies on code concept and not
implementation details, so it can be ported to other TeX systems.  In order to
support SyncTeX, one can start reading the dedicated section in synctex.ch,
sync-pdftex.ch and sync-xetex.ch. Actually, support is provided for TeX, e-TeX,
pdfTeX and XeTeX.

Other existing public synchronization technologies are defined by srcltx.sty -
also used by source specials - and pdfsync.sty.  Like them, the synchronize
texnology is meant to be shared by various text editors, viewers and TeX
engines.  A centralized reference and source of information is available in TeX-Live.

Versioning:
-----------
As synctex is embedded into different TeX implementation, there is an independent
versionning system.
For TeX implementations, the actual version is: 1
For .synctex file format, the actual version is SYNCTEX_VERSION below

Please, do not remove these explanations.

Acknowledgments:
----------------
The author received useful remarks from the pdfTeX developers, especially Hahn The Thanh,
and significant help from XeTeX developer Jonathan Kew

Nota Bene:
----------
If you include or use a significant part of the synctex package into a software,
I would appreciate to be listed as contributor and see "SyncTeX" highlighted.

Version 1
Tue Jul 1 15:23:00 UTC 2008

*/

#   define SYNCTEX_VERSION 1

#   define SYNCTEX_DEBUG 0

/*  Debugging: define the next macro to "return;" in order to disable the synctex code
 *  only suplemental function calls will be used. The compiler may optimize them. */
#   define SYNCTEX_RETURN_IF_DISABLED ;

#   define SYNCTEX_NOERR 0

#   define EXTERN extern

#   ifdef xfree
#       define SYNCTEX_FREE xfree
#   else
#       define SYNCTEX_FREE(x) free(x)
#   endif

EXTERN char *gettexstring(int n);

/*  the macros defined below do the same job than their almost eponym
 *  counterparts of *tex.web, the memory access is sometimes more direct
 *  because *tex.web won't share its own constants the main purpose is to
 *  maintain very few hook points into *tex.web in order both to ensure
 *  portability and not modifying to much the original code.  see texmfmem.h
 *  and *tex.web for details, the synctex_ prefix prevents name conflicts, it
 *  is some kind of namespace
*/
#   warning These structures MUST be kept in synchronization with the main program
/*  synctexoption is a global integer variable defined in *tex.web
 *  it is set to 1 by texmfmp.c if the command line has the '-synctex=1'
 *  option.  */
#   define synctex_options synctexoption
#   define SYNCTEX_NO_OPTION INT_MAX
/*  if synctex_options is set to SYNCTEX_NO_OPTION, no command line option was provided.  */

/*  glue code: really define the main memory,
 *  this is exactly the same "mem" as in *tex.web.  */
#   define mem zmem
/*  glue code: synctexoffset is a global integer variable defined in *tex.web
 *  it is set to the offset where the primitive \synctex reads and writes its
 *  value.  */
#   define SYNCTEX_VALUE zeqtb[synctexoffset].cint
/*  if there were a mean to share the value of synctex_code between *tex.web
 *  and this file, it would be great.  */

#   define SYNCTEX_UNIT_FACTOR 1
#   define UNIT / synctex_ctxt.unit
/*  UNIT is the scale. TeX coordinates are very accurate and client won't need
 *  that, at leat in a first step.  1.0 <-> 2^16 = 65536. 
 *  The TeX unit is sp (scaled point) or pt/65536 which means that the scale
 *  factor to retrieve a bp unit (a postscript) is 72/72.27/65536 =
 *  1/4096/16.06 = 1/8192/8.03
 *  Here we use 1/SYNCTEX_UNIT_FACTOR as scale factor, then we can limit ourselves to
 *  integers. This default value assumes that TeX magnification factor is 1000.
 *  The real TeX magnification factor is used to fine tune the synctex context
 *  scale in the synctex_dot_open function.
 *  IMPORTANT: We can say that the natural unit of .synctex files is SYNCTEX_UNIT_FACTOR sp.
 *  To retrieve the proper bp unit, we'll have to divide by 8.03.  To reduce
 *  rounding errors, we'll certainly have to add 0.5 for non negative integers
 *  and ±0.5 for negative integers.  This trick is mainly to gain speed and
 *  size. A binary file would be more appropriate in that respect, but I guess
 *  that some clients like auctex would not like it very much.  we cannot use
 *  "<<13" instead of "/SYNCTEX_UNIT_FACTOR" because the integers are signed and we do not
 *  want the sign bit to be propagated.  The origin of the coordinates is at
 *  the top left corner of the page.  For pdf mode, it is straightforward, but
 *  for dvi mode, we'll have to record the 1in offset in both directions,
 *  eventually modified by the magnification.
*/

/*  WARNING:
    The 9 definitions below must be in sync with their eponym declarations in
    the proper synctex-*.ch* file.
*/
#   define synchronization_field_size 2
/*  The default value is 2, it is suitable for original TeX and alike,
 *  but it is too big for XeTeX.
 *  The tag and the line are just the two last words of the node.  This is a
 *  very handy design but this is not strictly required by the concept.  If
 *  really necessary, one can define other storage rules.
 *  XeTeX redefines synchronization_field_size,
 *  SYNCTEX_TAG_MODEL and SYNCTEX_LINE_MODEL
 *  All the default values are targeted to TeX or e-TeX.  */
#   define SYNCTEX_TAG_MODEL(NODE,SIZE)\
                    mem[NODE+SIZE-synchronization_field_size].cint
#   define SYNCTEX_LINE_MODEL(NODE,SIZE)\
                    mem[NODE+SIZE-synchronization_field_size+1].cint
/*  SYNCTEX_TAG_MODEL and SYNCTEX_LINE_MODEL are used to define
 *  SYNCTEX_TAG and SYNCTEX_LINE in a model independant way
 *  Both are tag and line accessors */
#   define box_node_size (7+synchronization_field_size)
/*  see: @d box_node_size=...  
 *  There should be an automatic process here because these definitions
 *  are redundant. However, this process would certainly be overcomplicated
 *  (building then parsing the *tex.web file would be a pain) */
#   define width_offset 1
/*  see: @d width_offset=...  */
#   define depth_offset 2
/*  see: @d depth_offset=...  */
#   define height_offset 3
/*  see: @d height_offset=...  */

/*  Now define the local version of width(##), height(##) and depth(##) macros
        These only depend on the 3 macros above.  */
#   define SYNCTEX_TYPE(NODE) mem[NODE].hh.b0
#   define rule_node 2
#   define glue_node 10
#   define kern_node 11
#   define SYNCTEX_SUBTYPE(NODE) mem[NODE].hh.b1
#   define SYNCTEX_WIDTH(NODE) mem[NODE+width_offset].cint
#   define SYNCTEX_DEPTH(NODE) mem[NODE+depth_offset].cint
#   define SYNCTEX_HEIGHT(NODE) mem[NODE+height_offset].cint

/*  When an hlist ships out, it can contain many different kern/glue nodes with
 *  exactly the same sync tag and line.  To reduce the size of the .synctex
 *  file, we only display a kern node sync info when either the sync tag or the
 *  line changes.  Also, we try ro reduce the distance between the chosen nodes
 *  in order to improve accuracy.  It means that we display information for
 *  consecutive nodes, as far as possible.  This tricky part uses a "recorder",
 *  which is the address of the routine that knows how to write the
 *  synchronization info to the .synctex file.  It also uses criteria to detect
 *  a change in the context, this is the macro SYNCTEX_CONTEXT_DID_CHANGE. The
 *  SYNCTEX_IGNORE macro is used to detect unproperly initialized nodes.  See
 *  details in the implementation of the functions below.  */
#   define SYNCTEX_IGNORE(NODE) SYNCTEX_IS_OFF || !SYNCTEX_VALUE || !SYNCTEX_FILE

/*  Some parts of the code may differ depending on the ouput mode,
 *  dvi or xdv vs pdf, in particular the management of magnification.
 *  The default is dvi mode.
 *  Also, if pdftex is used, the origin of the coordinates is at 0, not at 1in
 *  Default values are suitable for TeX  */
#   define SYNCTEX_OUTPUT "dvi"
#   define SYNCTEX_OFFSET_IS_PDF 0

#   define SYNCTEX_YES (-1)
#   define SYNCTEX_NO  (0)
#   define SYNCTEX_NO_ERROR  (0)

#   include "synctex-TEX-OR-MF-OR-MP.h"

#   if defined(__SyncTeX__)

#   include <stdio.h>
#   include <stdarg.h>
#   include "zlib.h"

typedef void (*synctex_recorder_t)(halfword); /* recorders know how to record a node */
typedef int (*synctex_fprintf_t)(void *, const char * , ...); /* print formatted to either FILE * or gzFile */

#   define SYNCTEX_BITS_PER_BYTE 8

/*  Here are all the local variables gathered in one "synchronization context"  */
static struct {
    void *file;                 /*  the foo.synctex or foo.synctex.gz I/O identifier  */
	synctex_fprintf_t fprintf;  /*  either fprintf or gzprintf */
    char *busy_name;            /*  the real "foo.synctex(busy)" or "foo.synctex.gz(busy)" name  */
    char *root_name;            /*  in general jobname.tex  */
    integer count;              /*  The number of interesting records in "foo.synctex"  */
    /*  next concern the last sync record encountered  */
    halfword node;              /*  the last synchronized node, must be set 
                                 *  before the recorder */
    synctex_recorder_t recorder;/*  the recorder of the node above, the
                                 *  routine that knows how to record the 
                                 *  node to the .synctex file */
    integer tag, line;          /*  current tag and line  */
    integer curh, curv;         /*  current point  */
    integer magnification;      /*  The magnification as given by \mag */
    integer unit;               /*  The unit, defaults to 1, use 8192 to produce shorter but less accurate info */
    integer total_length;       /*  The total length of the bytes written since the last check point  */
    struct _flags {
        unsigned int option_read:1; /*  Command line option read (in case of problem or at the end) */
        unsigned int off:1;         /*  Definitely turn off synctex, corresponds to cli option -synctex=0 */
        unsigned int no_gz:1;       /*  Whether zlib is used or not */
        unsigned int not_void:1;    /*  Whether it really contains synchronization material */
        unsigned int warn:1;        /*  One shot warning flag */
        unsigned int reserved:SYNCTEX_BITS_PER_BYTE*sizeof(int)-5; /* Align */
	} flags;
} synctex_ctxt = {
NULL, NULL, NULL, NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, {0,0,0,0,0,0}};

#define SYNCTEX_FILE synctex_ctxt.file
#define SYNCTEX_IS_OFF (synctex_ctxt.flags.off)
#define SYNCTEX_NO_GZ (synctex_ctxt.flags.no_gz)
#define SYNCTEX_NOT_VOID (synctex_ctxt.flags.not_void)
#define SYNCTEX_WARNING_DISABLE (synctex_ctxt.flags.warn)
#define SYNCTEX_fprintf (*synctex_ctxt.fprintf)

/*  Initialize the options, synchronize the variables.
 *  This is sent by *tex.web before any TeX macro is used.
 *  */
void synctexinitcommand(void)
{
	/*  This is a one shot function, any subsequent call is void */
	if (synctex_ctxt.flags.option_read) {
		return;
	}
    if (SYNCTEX_NO_OPTION == synctex_options) {
        /*  No option given from the command line  */
        SYNCTEX_VALUE = 0;
    } else if (synctex_options == 0) {
        /*  -synctex=0 was given: SyncTeX must be definitely disabled,
		 *  any subsequent \synctex=1 will have no effect at all */
        SYNCTEX_IS_OFF = SYNCTEX_YES;
        SYNCTEX_VALUE = 0;
    } else {
        /*  the command line options are not ignored  */
		if(synctex_options < 0) {
			SYNCTEX_NO_GZ = SYNCTEX_YES;
		}
		/*  Initialize the content of the \synctex primitive */
		SYNCTEX_VALUE = synctex_options;
    }
	synctex_ctxt.flags.option_read = SYNCTEX_YES;
	return;
}

/*  Free all memory used and close the file,
 *  It is sent locally when there is a problem with synctex output. */
void synctex_abort(void)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctex_abort\n");
#endif
    if (SYNCTEX_FILE) {
		if (SYNCTEX_NO_GZ) {
			xfclose((FILE *)SYNCTEX_FILE, synctex_ctxt.busy_name);
		} else {
	        gzclose((gzFile)SYNCTEX_FILE);
		}
        SYNCTEX_FREE(synctex_ctxt.busy_name);
        synctex_ctxt.busy_name = NULL;
    }
    SYNCTEX_FREE(synctex_ctxt.root_name);
    synctex_ctxt.root_name = NULL;
    SYNCTEX_IS_OFF = SYNCTEX_YES; /* disable synctex */
}

static inline int synctex_record_preamble(void);
static inline int synctex_record_input(integer tag, char *name);

static char *synctex_suffix = ".synctex";
static char *synctex_suffix_gz = ".gz";
static char *synctex_suffix_busy = "(busy)";

/*  synctex_dot_open ensures that the foo.synctex file is open.
 *  In case of problem, it definitely disables synchronization.
 *  Now all the output synchronization info is gathered in only one file.
 *  It is possible to split this info into as many different output files as sheets
 *  plus 1 for the control but the overall benefits are not so clear.
 *     For example foo-i.synctex would contain input synchronization
 *     information for page i alone.
*/
static void *synctex_dot_open(void)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nwarning: Synchronize DEBUG: synctex_dot_open\n");
    printf("\nwarning: SYNCTEX_VALUE=%0X\n", SYNCTEX_VALUE);
    printf("\nwarning: synctex_options=%0X\n", synctex_options);
#   endif
    if (SYNCTEX_IS_OFF || !SYNCTEX_VALUE) {
        return NULL;      /*  synchronization is disabled: do nothing  */
    }
    if (SYNCTEX_FILE) {
        return SYNCTEX_FILE;      /*  synchronization is alerady enabled  */
    }
#if SYNCTEX_DEBUG
    printf("\nwarning: Synchronize DEBUG: synctex_dot_open 1\n");
#   endif
	/*  this is the first time we are asked to open the file
		this part of code is executed only once:
		either SYNCTEX_FILE is nonnegative or synchronization is
		definitely disabled. */
	{
		char *tmp = gettexstring(jobname);
		/*  jobname was set by the \jobname command on the *TeX side  */
		char * the_busy_name = xmalloc(strlen(tmp) + strlen(synctex_suffix) + strlen(synctex_suffix_gz) + strlen(synctex_suffix_busy) + 1);
		if(!the_busy_name) {
			SYNCTEX_FREE(tmp);
			synctex_abort();
			return NULL;
		}
		strcpy(the_busy_name, tmp);
		SYNCTEX_FREE(tmp);
		tmp = NULL;
		strcat(the_busy_name, synctex_suffix);
		/*  Initialize SYNCTEX_NO_GZ with the content of \synctex to let the user choose the format. */
		SYNCTEX_NO_GZ = SYNCTEX_VALUE<0?SYNCTEX_YES:SYNCTEX_NO;
		if (!SYNCTEX_NO_GZ) {
			strcat(the_busy_name, synctex_suffix_gz);
		}
		strcat(the_busy_name, synctex_suffix_busy);
		if (SYNCTEX_NO_GZ) {
			SYNCTEX_FILE = xfopen(the_busy_name, FOPEN_WBIN_MODE);
			synctex_ctxt.fprintf = (synctex_fprintf_t)(&fprintf);
		} else {
			SYNCTEX_FILE = gzopen(the_busy_name, FOPEN_WBIN_MODE);
			synctex_ctxt.fprintf = (synctex_fprintf_t)(&gzprintf);
		}
#if SYNCTEX_DEBUG
		printf("\nwarning: Synchronize DEBUG: synctex_dot_open 2\n");
#endif
		if (SYNCTEX_FILE) {
			if(SYNCTEX_NO_ERROR != synctex_record_preamble()) {
				synctex_abort();
				return NULL;
			}
			/*  Initialization of the context */
			synctex_ctxt.magnification = 1000;
			synctex_ctxt.unit = SYNCTEX_UNIT_FACTOR;
			/*  synctex_ctxt.busy_name was NULL before, it now owns the_busy_name */
			synctex_ctxt.busy_name = the_busy_name;
			the_busy_name = NULL;
			/*  print the preamble, this is an quite an UTF8 file  */
			if (NULL != synctex_ctxt.root_name) {
				synctex_record_input(1,synctex_ctxt.root_name);
				SYNCTEX_FREE(synctex_ctxt.root_name);
				synctex_ctxt.root_name = NULL;
			}
			synctex_ctxt.count = 0;
#if SYNCTEX_DEBUG
			fprintf(stdout,
					"\nwarning: Synchronize DEBUG: synctex_dot_open SYNCTEX AVAILABLE\n");
#endif
		} else {
			/*  no .synctex file available, so disable synchronization  */
			SYNCTEX_IS_OFF = SYNCTEX_YES;
			SYNCTEX_VALUE = 0;
			printf("\nSyncTeX warning: no synchronization, problem with %s\n",the_busy_name);
			/* and free the_busy_name */
			SYNCTEX_FREE(the_busy_name);
			the_busy_name = NULL;
#if SYNCTEX_DEBUG
			fprintf(stdout,
					"\nwarning: Synchronize DEBUG: synctex_dot_open SYNCTEX DISABLED\n");
#endif
		}
	}
    return SYNCTEX_FILE;
}

/*  Each time TeX opens a file, it sends a synctexstartinput message and enters
 *  this function.  Here, a new synchronization tag is created and stored in
 *  the synctex_tag_field of the TeX current input context.  Each synchronized
 *  TeX node will record this tag instead of the file name.  synctexstartinput
 *  writes the mapping synctag <-> file name to the .synctex (or .synctex.gz) file.  A client
 *  will read the .synctex file and retrieve this mapping, it will be able to
 *  open the correct file just knowing its tag.  If the same file is read
 *  multiple times, it might be associated to different tags.  Synchronization
 *  controllers, either in viewers, editors or standalone should be prepared to
 *  handle this situation and take the appropriate action if they want to
 *  optimize memory.  No two different files will have the same positive tag.
 *  It is not advisable to definitely store the file names here.  If the file
 *  names ever have to be stored, it should definitely be done at the TeX level
 *  just like src-specials do, such that other components of the program can use
 *  it.  This function does not make any difference between the files, it
 *  treats the same way .tex, .aux, .sty ... files, even if many of them do not
 *  contain any material meant to be typeset.
*/
void synctexstartinput(void)
{
    static unsigned int synctex_tag_counter = 0;

	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nwarning: Synchronize DEBUG: synctexstartinput %i",
            synctex_tag_counter);
    printf("\nwarning: SYNCTEX_VALUE=%i", SYNCTEX_VALUE);
    printf("\nwarning: synctex_options=%0X", synctex_options);
#endif

    if (SYNCTEX_IS_OFF) {
        return;
    }
    /*  synctex_tag_counter is a counter uniquely identifying the file actually
     *  open.  Each time tex opens a new file, synctexstartinput will increment this
     *  counter  */
    if (~synctex_tag_counter > 0) {
        ++synctex_tag_counter;
    } else {
        /*  we have reached the limit, subsequent files will be softly ignored
         *  this makes a lot of files... even in 32 bits
         *  Maybe we will limit this to 16bits and
         *  use the 16 other bits to store the column number */
        curinput.synctextagfield = 0;
        return;
    }
    curinput.synctextagfield = synctex_tag_counter;     /*  -> *TeX.web  */
    if (synctex_tag_counter == 1) {
        /*  this is the first file TeX ever opens, in general \jobname.tex we
         *  do not know yet if synchronization will ever be enabled so we have
         *  to store the file name, because we will need it later.
		 *  This is necessary because \jobname can be different */
        synctex_ctxt.root_name = gettexstring(curinput.namefield);
		/* we could initialize the unit field to 1 to avoid floating point exception
		 * when accidentaly dividing by the unit.
		 * This occurs when some SYNCTEX_IGNORE macro is not used.
		 * But this must not happen unexpectedly, so we leave the unit to 0 */
#if SYNCTEX_DEBUG
        printf("\nwarning: Synchronize DEBUG: synctexstartinput first END\n");
#   endif
        return;
    }
    if (SYNCTEX_FILE || (SYNCTEX_VALUE && (SYNCTEX_NO_ERROR != synctex_dot_open()))) {
        char *tmp = gettexstring(curinput.namefield);
        synctex_record_input(curinput.synctextagfield,tmp);
        SYNCTEX_FREE(tmp);
    }
#if SYNCTEX_DEBUG
    printf("\nwarning: Synchronize DEBUG: synctexstartinput END\n");
#   endif
    return;
}

/*  All the synctex... functions below have the smallest set of parameters.  It
 *  appears to be either the address of a node, or nothing at all.  Using zmem,
 *  which is the place where all the nodes are stored, one can retrieve every
 *  information about a node.  The other information is obtained through the
 *  global context variable.
 */

static inline int synctex_record_postamble(void);


/*  Free all memory used and close the file,
 *  sent by close_files_and_terminate in tex.web.
 *  synctexterminate() is called when the TeX run terminates.
 *  If synchronization was active, the working synctex file is moved to
 *  the final synctex file name.
 *  If synchronization was not active of if there is no output,
 *  the synctex file is removed if any.
 *  That way we can be sure that any synctex file is in sync with a tex run.
 *  However, it does not mean that it will be in sync with the pdf, especially
 *  when the output is dvi or xdv and the dvi (or xdv) to pdf driver has not been applied.
 */
void synctexterminate(boolean log_opened)
{
	char *tmp = NULL;
	char * the_real_syncname = NULL;
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexterminate\n");
#endif
	tmp = gettexstring(jobname);
	the_real_syncname = xmalloc(strlen(tmp) + strlen(synctex_suffix) + strlen(synctex_suffix_gz) + 1);
	if(!the_real_syncname) {
		SYNCTEX_FREE(tmp);
		synctex_abort();
		return;
	}
	strcpy(the_real_syncname, tmp);
	strcat(the_real_syncname, synctex_suffix);
	if (!SYNCTEX_NO_GZ) {
		/*  Remove any uncompressed synctex file, from a previous build. */
		remove(the_real_syncname);
		strcat(the_real_syncname, synctex_suffix_gz);
	}
	/* allways remove the synctex output file before renaming it, windows requires it. */
	if(0 != remove(the_real_syncname) && errno == EACCES) {
		fprintf(stderr,"SyncTeX: Can't remove %s (file is open or read only)\n",the_real_syncname);		
	}
    if (SYNCTEX_FILE) {
		if (SYNCTEX_NOT_VOID) {
			synctex_record_postamble();
			/* close the synctex file*/
			if (SYNCTEX_NO_GZ) {
				xfclose((FILE *)SYNCTEX_FILE, synctex_ctxt.busy_name);
			} else {
				gzclose((gzFile)SYNCTEX_FILE);
			}
			/*  renaming the working synctex file */
			if(0 == rename(synctex_ctxt.busy_name,the_real_syncname)) {
				if(log_opened) {
					printf("\nSyncTeX written on %s",the_real_syncname); /* SyncTeX also refers to the contents */
				}
			} else {
				fprintf(stderr,"SyncTeX: Can't rename %s to %s\n",synctex_ctxt.busy_name,the_real_syncname);
				remove(synctex_ctxt.busy_name);
			}
		} else {
			/* close and remove the synctex file because there are no pages of output */
			if (SYNCTEX_NO_GZ) {
				xfclose((FILE *)SYNCTEX_FILE, synctex_ctxt.busy_name);
			} else {
				gzclose((gzFile)SYNCTEX_FILE);
			}
			remove(synctex_ctxt.busy_name);
		}
		SYNCTEX_FILE = NULL;
	}
	if (SYNCTEX_NO_GZ) {
		/*  Remove any compressed synctex file, from a previous build. */
		strcat(the_real_syncname, synctex_suffix_gz);
		remove(the_real_syncname);
	}
	SYNCTEX_FREE(synctex_ctxt.busy_name);
	synctex_ctxt.busy_name = NULL;
	SYNCTEX_FREE(the_real_syncname);
	SYNCTEX_FREE(tmp);
	synctex_abort();
}

static inline int synctex_record_content(void);
static inline int synctex_record_settings(void);
static inline int synctex_record_sheet(integer sheet);

/*  Recording the "{..." line.  In *tex.web, use synctex_sheet(pdf_output) at
 *  the very beginning of the ship_out procedure.
*/
void synctexsheet(integer mag)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexsheet %i\n",mag);
#endif
    if (SYNCTEX_IS_OFF) {
		if(SYNCTEX_VALUE && !SYNCTEX_WARNING_DISABLE) {
			SYNCTEX_WARNING_DISABLE = SYNCTEX_YES;
			printf("\nSyncTeX warning: Synchronization was disabled from\nthe command line with -synctex=0\nChanging the value of \\synctex has no effect.");
		}
        return;
    }
    if (SYNCTEX_FILE || (SYNCTEX_VALUE && (SYNCTEX_NO_ERROR != synctex_dot_open()))) {
        /*  First possibility: the .synctex file is already open because SyncTeX was activated on the CLI
		 *  or it was activated with the \synctex macro and the first page is already shipped out.
		 *  Second possibility: tries to open the .synctex, useful if synchronization was enabled
         *  from the source file and not from the CLI.
         *  totalpages is defined in tex.web   */
        if (totalpages == 0) {
            /*  Now it is time to properly set up the scale factor. */
            if(mag>0) {
                synctex_ctxt.magnification = mag;
            }
			if(SYNCTEX_NO_ERROR != synctex_record_settings()
					|| SYNCTEX_NO_ERROR != synctex_record_content()) {
				synctex_abort();
				return;
			}
        }
        synctex_record_sheet(totalpages+1);
    }
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexsheet END\n");
#endif
    return;
}

static inline int synctex_record_teehs(integer sheet);

/*  Recording the "}..." line.  In *tex.web, use synctex_teehs at
 *  the very end of the ship_out procedure.
*/
void synctexteehs(void)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexteehs\n");
#endif
    if (SYNCTEX_IS_OFF || !SYNCTEX_FILE ) {
        return;
    }
    synctex_record_teehs(totalpages);/* not totalpages+1*/
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexteehs END\n");
#endif
    return;
}

static inline void synctex_record_vlist(halfword p);

/*  This message is sent when a vlist will be shipped out, more precisely at
 *  the beginning of the vliSYNCTEX_out procedure in *TeX.web.  It will be balanced
 *  by a synctex_tsilv, sent at the end of the vliSYNCTEX_out procedure.  p is the
 *  address of the vlist We assume that p is really a vlist node! */
void synctexvlist(halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexhlist\n");
#endif
    if (SYNCTEX_IGNORE(this_box)) {
        return;
    }
    synctex_ctxt.node = this_box;   /*  0 to reset  */
    synctex_ctxt.recorder = NULL;   /*  reset  */
    synctex_ctxt.tag = SYNCTEX_TAG_MODEL(this_box,box_node_size);
    synctex_ctxt.line = SYNCTEX_LINE_MODEL(this_box,box_node_size);
	synctex_ctxt.curh = curh;
	synctex_ctxt.curv = curv;
    synctex_record_vlist(this_box);
}

static inline void synctex_record_tsilv(halfword p);

/*  Recording a "f" line ending a vbox: this message is sent whenever a vlist
 *  has been shipped out. It is used to close the vlist nesting level. It is
 *  sent at the end of the vliSYNCTEX_out procedure in *TeX.web to balance a former
 *  synctex_vlist sent at the beginning of that procedure.    */
void synctextsilv(halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctextsilv\n");
#endif
    if (SYNCTEX_IGNORE(this_box)) {
        return;
    }
    /*  Ignoring any pending info to be recorded  */
    synctex_ctxt.node = this_box; /*  0 to reset  */
    synctex_ctxt.tag = SYNCTEX_TAG_MODEL(this_box,box_node_size);
    synctex_ctxt.line = SYNCTEX_LINE_MODEL(this_box,box_node_size);
	synctex_ctxt.curh = curh;
	synctex_ctxt.curv = curv;
    synctex_ctxt.recorder = NULL;
    synctex_record_tsilv(this_box);
}

static inline void synctex_record_void_vlist(halfword p);

/*  This message is sent when a void vlist will be shipped out.
 *  There is no need to balance a void vlist.  */
void synctexvoidvlist(halfword p, halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexvoidvlist\n");
#endif
    if (SYNCTEX_IGNORE(p)) {
        return;
    }
    synctex_ctxt.node = p;          /*  reset  */
    synctex_ctxt.tag = SYNCTEX_TAG_MODEL(p,box_node_size);
    synctex_ctxt.line = SYNCTEX_LINE_MODEL(p,box_node_size);
	synctex_ctxt.curh = curh;
	synctex_ctxt.curv = curv;
    synctex_ctxt.recorder = NULL;   /*  reset  */
    synctex_record_void_vlist(p);
}

static inline void synctex_record_hlist(halfword p);

/*  This message is sent when an hlist will be shipped out, more precisely at
 *  the beginning of the hliSYNCTEX_out procedure in *TeX.web.  It will be balanced
 *  by a synctex_tsilh, sent at the end of the hliSYNCTEX_out procedure.  p is the
 *  address of the hlist We assume that p is really an hlist node! */
void synctexhlist(halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexhlist\n");
#endif
    if (SYNCTEX_IGNORE(this_box)) {
        return;
    }
    synctex_ctxt.node = this_box;   /*  0 to reset  */
    synctex_ctxt.tag = SYNCTEX_TAG_MODEL(this_box,box_node_size);
    synctex_ctxt.line = SYNCTEX_LINE_MODEL(this_box,box_node_size);
	synctex_ctxt.curh = curh;
	synctex_ctxt.curv = curv;
    synctex_ctxt.recorder = NULL;   /*  reset  */
    synctex_record_hlist(this_box);
}

static inline void synctex_record_tsilh(halfword p);

/*  Recording a ")" line ending an hbox this message is sent whenever an hlist
 *  has been shipped out it is used to close the hlist nesting level. It is
 *  sent at the end of the hliSYNCTEX_out procedure in *TeX.web to balance a former
 *  synctex_hlist sent at the beginning of that procedure.    */
void synctextsilh(halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctextsilh\n");
#endif
    if (SYNCTEX_IGNORE(this_box)) {
        return;
    }
    /*  Ignoring any pending info to be recorded  */
    synctex_ctxt.node = this_box;     /*  0 to force next node to be recorded!  */
    synctex_ctxt.tag = SYNCTEX_TAG_MODEL(this_box,box_node_size);
    synctex_ctxt.line = SYNCTEX_LINE_MODEL(this_box,box_node_size);
	synctex_ctxt.curh = curh;
	synctex_ctxt.curv = curv;
    synctex_ctxt.recorder = NULL;   /*  reset  */
    synctex_record_tsilh(this_box);
}

static inline void synctex_record_void_hlist(halfword p);

/*  This message is sent when a void hlist will be shipped out.
 *  There is no need to balance a void hlist.  */
void synctexvoidhlist(halfword p, halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexvoidhlist\n");
#endif
    if (SYNCTEX_IGNORE(p)) {
        return;
    }
	/*  the sync context has changed  */
	if (synctex_ctxt.recorder != NULL) {
		/*  but was not yet recorded  */
		(*synctex_ctxt.recorder) (synctex_ctxt.node);
	}
    synctex_ctxt.node = p;          /*  0 to reset  */
    synctex_ctxt.tag = SYNCTEX_TAG_MODEL(p,box_node_size);
    synctex_ctxt.line = SYNCTEX_LINE_MODEL(p,box_node_size);
	synctex_ctxt.curh = curh;
	synctex_ctxt.curv = curv;
    synctex_ctxt.recorder = NULL;   /*  reset  */
    synctex_record_void_hlist(p);
}

/*  glue code: these only work with nodes of size medium_node_size  */
#   define small_node_size 2
/*  see: @d small_node_size=2 {number of words to allocate for most node types}  */
#   define medium_node_size (small_node_size+synchronization_field_size)
/*  see: @d rule_node_size=4  */
#   define rule_node_size (4+synchronization_field_size)

/* IN THE SEQUEL, ALL NODE ARE medium_node_size'd, UNTIL THE CONTRARY IS MENTIONNED */
#   undef SYNCTEX_IGNORE
#   define SYNCTEX_IGNORE(NODE) SYNCTEX_IS_OFF || !SYNCTEX_VALUE \
                                || (0 >= SYNCTEX_TAG_MODEL(NODE,medium_node_size)) \
                                || (0 >= SYNCTEX_LINE_MODEL(NODE,medium_node_size))

/*  This macro will detect a change in the synchronization context.  As long as
 *  the synchronization context remains the same, there is no need to write
 *  synchronization info: it would not help more.  The synchronization context
 *  has changed when either the line number or the file tag has changed.  */
#   define SYNCTEX_CONTEXT_DID_CHANGE(NODE) ((0 == synctex_ctxt.node)\
                || (SYNCTEX_TAG_MODEL(NODE,medium_node_size) != synctex_ctxt.tag)\
                || (SYNCTEX_LINE_MODEL(NODE,medium_node_size) != synctex_ctxt.line))

void synctex_math_recorder(halfword p);

/*  glue code this message is sent whenever an inline math node will ship out
        See: @ @<Output the non-|char_node| |p| for...  */
void synctexmath(halfword p, halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexmath\n");
#endif
    if (SYNCTEX_IGNORE(p)) {
        return;
    }
    if ((synctex_ctxt.recorder != NULL) && SYNCTEX_CONTEXT_DID_CHANGE(p)) {
        /*  the sync context did change  */
        (*synctex_ctxt.recorder) (synctex_ctxt.node);
    }
    synctex_ctxt.node = p;
    synctex_ctxt.tag = SYNCTEX_TAG_MODEL(p,medium_node_size);
    synctex_ctxt.line = SYNCTEX_LINE_MODEL(p,medium_node_size);
	synctex_ctxt.curh = curh;
	synctex_ctxt.curv = curv;
    synctex_ctxt.recorder = NULL;/*  no need to record once more  */
    synctex_math_recorder(p);/*  always record synchronously  */
}

static inline void synctex_record_glue(halfword p);
static inline void synctex_record_kern(halfword p);
static inline void synctex_record_rule(halfword p);

/*  this message is sent whenever an horizontal glue node or rule node ships out
        See: move_past:...    */
#   define SYNCTEX_IGNORE_RULE(NODE) SYNCTEX_IS_OFF || !SYNCTEX_VALUE \
                                || (0 >= SYNCTEX_TAG_MODEL(NODE,rule_node_size)) \
                                || (0 >= SYNCTEX_LINE_MODEL(NODE,rule_node_size))
void synctexhorizontalruleorglue(halfword p, halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexglue\n");
#endif
	if (SYNCTEX_TYPE(p) == rule_node) { /* not medium_node_size so we can't use SYNCTEX_IGNORE */
		if (SYNCTEX_IGNORE_RULE(p)) {
			return;
		}
	}
	else {
		if (SYNCTEX_IGNORE(p)) {
			return;
		}
	}
	synctex_ctxt.node = p;
	synctex_ctxt.curh = curh;
	synctex_ctxt.curv = curv;
	synctex_ctxt.recorder = NULL;
	switch(SYNCTEX_TYPE(p)) {
		case rule_node:
			synctex_ctxt.tag = SYNCTEX_TAG_MODEL(p,rule_node_size);
			synctex_ctxt.line = SYNCTEX_LINE_MODEL(p,rule_node_size);
			synctex_record_rule(p);/*  always record synchronously: maybe some text is outside the box  */
		break;
		case glue_node:
			synctex_ctxt.tag = SYNCTEX_TAG_MODEL(p,medium_node_size);
			synctex_ctxt.line = SYNCTEX_LINE_MODEL(p,medium_node_size);
			synctex_record_glue(p);/*  always record synchronously: maybe some text is outside the box  */
		break;
		case kern_node:
			synctex_ctxt.tag = SYNCTEX_TAG_MODEL(p,medium_node_size);
			synctex_ctxt.line = SYNCTEX_LINE_MODEL(p,medium_node_size);
			synctex_record_kern(p);/*  always record synchronously: maybe some text is outside the box  */
		break;
		default:
			printf("\nSynchronize ERROR: unknown node type %i\n",SYNCTEX_TYPE(p));
	}
}

void synctex_kern_recorder(halfword p);

/*  this message is sent whenever a kern node ships out
        See: @ @<Output the non-|char_node| |p| for...    */
void synctexkern(halfword p, halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexkern\n");
#endif
    if (SYNCTEX_IGNORE(p)) {
        return;
    }
    if (SYNCTEX_CONTEXT_DID_CHANGE(p)) {
        /*  the sync context has changed  */
        if (synctex_ctxt.recorder != NULL) {
            /*  but was not yet recorded  */
            (*synctex_ctxt.recorder) (synctex_ctxt.node);
       }
        if(synctex_ctxt.node == this_box) {
            /* first node in the list */
            synctex_ctxt.node = p;
            synctex_ctxt.tag = SYNCTEX_TAG_MODEL(p,medium_node_size);
            synctex_ctxt.line = SYNCTEX_LINE_MODEL(p,medium_node_size);
            synctex_ctxt.recorder = &synctex_kern_recorder;
        } else {
            synctex_ctxt.node = p;
            synctex_ctxt.tag = SYNCTEX_TAG_MODEL(p,medium_node_size);
            synctex_ctxt.line = SYNCTEX_LINE_MODEL(p,medium_node_size);
            synctex_ctxt.recorder = NULL;
            /*  always record when the context has just changed
             *  and when not the first node  */
            synctex_kern_recorder(p);
        }
    } else {
            /*  just update the geometry and type (for future improvements)  */
        synctex_ctxt.node = p;
        synctex_ctxt.tag = SYNCTEX_TAG_MODEL(p,medium_node_size);
        synctex_ctxt.line = SYNCTEX_LINE_MODEL(p,medium_node_size);
        synctex_ctxt.recorder = &synctex_kern_recorder;
    }
}

/*  This last part is used as a tool to infer TeX behaviour,
 *  but not for direct synchronization. */
#   undef SYNCTEX_IGNORE
#   define SYNCTEX_IGNORE(NODE) SYNCTEX_IS_OFF || !SYNCTEX_VALUE || !SYNCTEX_FILE \
                                || (synctex_ctxt.count>2000)

void synctex_char_recorder(halfword p);

/*  this message is sent whenever a char node ships out    */
void synctexchar(halfword p, halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexchar\n");
#endif
    if (SYNCTEX_IGNORE(p)) {
        return;
    }
    if (synctex_ctxt.recorder != NULL) {
        /*  but was not yet recorded  */
        (*synctex_ctxt.recorder) (synctex_ctxt.node);
    }
    synctex_ctxt.node = p;
    synctex_ctxt.tag = 0;
    synctex_ctxt.line = 0;
    synctex_ctxt.recorder = NULL;
    /*  always record when the context has just changed  */
    synctex_char_recorder(p);
}

void synctex_node_recorder(halfword p);

#   undef SYNCTEX_IGNORE
#   define SYNCTEX_IGNORE(NODE) (SYNCTEX_IS_OFF || !SYNCTEX_VALUE || !SYNCTEX_FILE)

/*  this message should be sent to record information
         for a node of an unknown type    */
void synctexnode(halfword p, halfword this_box)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexnode\n");
#endif
	if (SYNCTEX_IGNORE(p)) {
        return;
    }
    /*  always record, not very usefull yet  */
    synctex_node_recorder(p);
}

/*  this message should be sent to record information
         synchronously for the current location    */
void synctexcurrent(void)
{
	SYNCTEX_RETURN_IF_DISABLED;
#if SYNCTEX_DEBUG
    printf("\nSynchronize DEBUG: synctexcurrent\n");
#endif
	if (SYNCTEX_IGNORE(nothing)) {
        return;
    } else {
		size_t len = SYNCTEX_fprintf(SYNCTEX_FILE,"x%i,%i:%i,%i\n",
			synctex_ctxt.tag,synctex_ctxt.line,
			curh UNIT,curv UNIT);
		if(len>0) {
			synctex_ctxt.total_length += len;
			return;
		}
	}
    synctex_abort();
	return;
}

#pragma mark -
#pragma mark Glue code Recorders

/*  Recording the settings  */
static inline int synctex_record_settings(void)
{
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_settings\n");
#endif
    if(NULL == SYNCTEX_FILE) {
        return SYNCTEX_NOERR;
    }
	if(SYNCTEX_FILE) {
		size_t len = SYNCTEX_fprintf(SYNCTEX_FILE,"Output:%s\nMagnification:%i\nUnit:%i\nX Offset:%i\nY Offset:%i\n",
			SYNCTEX_OUTPUT,synctex_ctxt.magnification,synctex_ctxt.unit,
			((SYNCTEX_OFFSET_IS_PDF != 0) ? 0 : 4736287 UNIT),
			((SYNCTEX_OFFSET_IS_PDF != 0) ? 0 : 4736287 UNIT));
		if(len>0) {
			synctex_ctxt.total_length += len;
			return SYNCTEX_NOERR;
		}
	}
	synctex_abort();
	return -1;
}

/*  Recording a "SyncTeX..." line  */
static inline int synctex_record_preamble(void)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_preamble\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"SyncTeX Version:%i\n",SYNCTEX_VERSION);
	if(len>0) {
		synctex_ctxt.total_length = len;
		return SYNCTEX_NOERR;
	}
    synctex_abort();
	return -1;
}

/*  Recording a "Input:..." line  */
static inline int synctex_record_input(integer tag, char *name)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_input\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"Input:%i:%s\n",tag,name);
	if(len>0) {
		synctex_ctxt.total_length += len;
		return SYNCTEX_NOERR;
	}
	synctex_abort();
	return -1;
}

/*  Recording a "!..." line  */
static inline int synctex_record_anchor(void)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_anchor\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"!%i\n",synctex_ctxt.total_length);
	if(len>0) {
		synctex_ctxt.total_length = len;
		++synctex_ctxt.count;
		return SYNCTEX_NOERR;
	}
	synctex_abort();
	return -1;
}

/*  Recording a "Content" line  */
static inline int synctex_record_content(void)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_content\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"Content:\n");
	if(len>0) {
		synctex_ctxt.total_length += len;
		return SYNCTEX_NOERR;
	}
	synctex_abort();
	return -1;
}

/*  Recording a "{..." line  */
static inline int synctex_record_sheet(integer sheet)
{
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_sheet\n");
#endif
	if(SYNCTEX_NOERR == synctex_record_anchor()) {
		size_t len = SYNCTEX_fprintf(SYNCTEX_FILE,"{%i\n",sheet);
		if(len>0) {
			synctex_ctxt.total_length += len;
			++synctex_ctxt.count;
			return SYNCTEX_NOERR;
		}
	}
	synctex_abort();
	return -1;
}

/*  Recording a "}..." line  */
static inline int synctex_record_teehs(integer sheet)
{
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_teehs\n");
#endif
	if(SYNCTEX_NOERR == synctex_record_anchor()) {
		size_t len = SYNCTEX_fprintf(SYNCTEX_FILE,"}%i\n",sheet);
		if(len>0) {
			synctex_ctxt.total_length += len;
			++synctex_ctxt.count;
			return SYNCTEX_NOERR;
		}
	}
	synctex_abort();
	return -1;
}

/*  Recording a "v..." line  */
static inline void synctex_record_void_vlist(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_void_vlist\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"v%i,%i:%i,%i:%i,%i,%i\n",
				SYNCTEX_TAG_MODEL(p,box_node_size),
				SYNCTEX_LINE_MODEL(p,box_node_size),
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT,
				SYNCTEX_WIDTH(p) UNIT,
				SYNCTEX_HEIGHT(p) UNIT,
				SYNCTEX_DEPTH(p) UNIT);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a "[..." line  */
static inline void synctex_record_vlist(halfword p)
{
	size_t len = 0;
	SYNCTEX_NOT_VOID = SYNCTEX_YES;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_vlist\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"[%i,%i:%i,%i:%i,%i,%i\n",
				SYNCTEX_TAG_MODEL(p,box_node_size),
				SYNCTEX_LINE_MODEL(p,box_node_size),
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT,
				SYNCTEX_WIDTH(p) UNIT,
				SYNCTEX_HEIGHT(p) UNIT,
				SYNCTEX_DEPTH(p) UNIT);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a "]..." line  */
static inline void synctex_record_tsilv(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_tsilv\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"]\n");
	if(len>0) {
		synctex_ctxt.total_length += len;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a "h..." line  */
static inline void synctex_record_void_hlist(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_void_hlist\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"h%i,%i:%i,%i:%i,%i,%i\n",
				SYNCTEX_TAG_MODEL(p,box_node_size),
				SYNCTEX_LINE_MODEL(p,box_node_size),
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT,
				SYNCTEX_WIDTH(p) UNIT,
				SYNCTEX_HEIGHT(p) UNIT,
				SYNCTEX_DEPTH(p) UNIT);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a "(..." line  */
static inline void synctex_record_hlist(halfword p)
{
	size_t len = 0;
	SYNCTEX_NOT_VOID = SYNCTEX_YES;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_hlist\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"(%i,%i:%i,%i:%i,%i,%i\n",
				SYNCTEX_TAG_MODEL(p,box_node_size),
				SYNCTEX_LINE_MODEL(p,box_node_size),
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT,
				SYNCTEX_WIDTH(p) UNIT,
				SYNCTEX_HEIGHT(p) UNIT,
				SYNCTEX_DEPTH(p) UNIT);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a ")..." line  */
static inline void synctex_record_tsilh(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_tsilh\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,")\n");
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a "Count..." line  */
static inline int synctex_record_count(void) {
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_count\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"Count:%i\n",synctex_ctxt.count);
	if(len>0) {
		synctex_ctxt.total_length += len;
		return SYNCTEX_NOERR;
	}
	synctex_abort();
	return -1;
}

/*  Recording a "Postamble" section  */
static inline int synctex_record_postamble(void)
{
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_postamble\n");
#endif
	if(SYNCTEX_NOERR == synctex_record_anchor()) {
		size_t len = SYNCTEX_fprintf(SYNCTEX_FILE,"Postamble:\n");
		if(len>0) {
			synctex_ctxt.total_length += len;
			if(synctex_record_count() || synctex_record_anchor()) {
			} else {
				len = SYNCTEX_fprintf(SYNCTEX_FILE,"Post scriptum:\n");
				if(len>0) {
					synctex_ctxt.total_length += len;
					return SYNCTEX_NOERR;
				}
			}
		}
	}
	synctex_abort();
	return -1;
}

/*  Recording a "g..." line  */
static inline void synctex_record_glue(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_glue_recorder\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"g%i,%i:%i,%i\n",
				SYNCTEX_TAG_MODEL(p,medium_node_size),
				SYNCTEX_LINE_MODEL(p,medium_node_size),
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a "k..." line  */
static inline void synctex_record_kern(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_kern_recorder\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"k%i,%i:%i,%i:%i\n",
				SYNCTEX_TAG_MODEL(p,medium_node_size),
				SYNCTEX_LINE_MODEL(p,medium_node_size),
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT,
				SYNCTEX_WIDTH(p) UNIT);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a "r..." line  */
static inline void synctex_record_rule(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_record_tsilh\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"r%i,%i:%i,%i:%i,%i,%i\n",
				SYNCTEX_TAG_MODEL(p,rule_node_size),
				SYNCTEX_LINE_MODEL(p,rule_node_size),
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT,
				rulewd UNIT,
				ruleht UNIT,
				ruledp UNIT);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}


#pragma mark -
#pragma mark Recorders

/*  Recording a "$..." line  */
void synctex_math_recorder(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_math_recorder\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"$%i,%i:%i,%i\n",
				SYNCTEX_TAG_MODEL(p,medium_node_size),
				SYNCTEX_LINE_MODEL(p,medium_node_size),
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a "k..." line  */
void synctex_kern_recorder(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_kern_recorder\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"k%i,%i:%i,%i:%i\n",
				SYNCTEX_TAG_MODEL(p,medium_node_size),
				SYNCTEX_LINE_MODEL(p,medium_node_size),
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT,
				SYNCTEX_WIDTH(p) UNIT);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a "c..." line  */
void synctex_char_recorder(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_char_recorder\n");
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"c%i,%i\n",
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

/*  Recording a "?..." line, type, subtype and position  */
void synctex_node_recorder(halfword p)
{
	size_t len = 0;
#if SYNCTEX_DEBUG > 999
    printf("\nSynchronize DEBUG: synctex_node_recorder(0x%x)\n",p);
#endif
	len = SYNCTEX_fprintf(SYNCTEX_FILE,"?%i,%i:%i,%i\n",
				synctex_ctxt.curh UNIT, synctex_ctxt.curv UNIT,
				mem[p].hh.b0,mem[p].hh.b1);
	if(len>0) {
		synctex_ctxt.total_length += len;
        ++synctex_ctxt.count;
		return;
	}
	synctex_abort();
	return;
}

#   else
#       warning "SyncTeX is disabled"
#   endif
