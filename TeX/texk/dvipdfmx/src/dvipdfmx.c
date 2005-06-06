/*  $Header: /home/cvsroot/dvipdfmx/src/dvipdfmx.c,v 1.26 2004/02/10 09:49:07 hirata Exp $
    
    This is DVIPDFMx, an eXtended version of DVIPDFM by Mark A. Wicks.

    Copyright (C) 2002-2003 by Jin-Hwan Cho and Shunsaku Hirata,
    the DVIPDFMx project team <dvipdfmx@project.ktug.or.kr>
    
    Copyright (C) 1998, 1999 by Mark A. Wicks <mwicks@kettering.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

#include "system.h"
#include "mem.h"
#include "mfileio.h"
#include "dvi.h"
#include "pdfdoc.h"
#include "pdfdev.h"
#include "encodings.h"
#include "unicode.h"
#include "agl.h"
#include "type1.h"
#include "type1c.h"
#include "type0.h"
#include "cid.h"
#include "cmap.h"
#include "ttf.h"
#include "colorsp.h"
#include "pdfspecial.h"
#include "pdfparse.h"
#include "vf.h"
#include "pkfont.h"
#include "thumbnail.h"
#include "psimage.h"
#include "tfm.h"
#include "fontmap.h"

#include "pdfencrypt.h"

static int verbose = 0;

static char ignore_colors = 0;
static double mag = 1.0;
static int font_dpi = 600;
static int really_quiet = 0;
static int pdfdecimaldigits = 3;

/* Encryption */
static int do_encryption = 0;
static unsigned key_bits   = 40;
static unsigned permission = 0x003C;

/* Page device */
static double paper_width = 595.0, paper_height = 842.0;
static double x_offset = 72.0, y_offset = 72.0;
static int  default_papersize = 1;
static char landscape_mode    = 0;

struct rectangle {
  double llx, lly, urx, ury;
};

#ifdef HAVE_LIBPAPER
#include <paper.h>
#else /* !HAVE_LIBPAPER */
struct paper {
  const char* name;
  double pswidth, psheight;
};

#ifndef DEFAULT_PAPER_NAME
#define DEFAULT_PAPER_NAME "a4"
#endif


static const struct paper paperspecs[] = {
  {"letter",  612.00,  792.00},
  {"legal" ,  612.00, 1008.00},
  {"ledger", 1224.00,  792.00},
  {"tabloid", 792.00, 1224.00},
  {"a6",      297.64,  420.91},
  {"a5",      420.91,  595.27},
  {"a4",      595.27,  841.82},
  {"a3",      841.82, 1190.16},
  /*
   * The B series paper formats were first introduced in Japanese
   * patch (jpatch). The size of b6, ..., b5var paper is JIS paper
   * size for this reason. Please modify the following line or use
   * libpaper if you want to use ISO paper sizes.
   */
  {"b6",      364.25,  515.91},
  {"b5",      515.91,  728.50},
  {"b4",      728.50, 1031.81},
  {"b3",     1031.81, 1457.00},
  {"b5var",   515.91,  651.97},
  /* -- */
  {NULL, 0, 0},
};

#define paperinit()
#define paperdone()
#define paperpswidth(p)    (((p) && (p)->name) ? p->pswidth  : 0.0)
#define paperpsheight(p)   (((p) && (p)->name) ? p->psheight : 0.0)
#define papername(p)       (((p) && (p)->name) ? p->name : NULL)
#define paperfirst()       &(paperspecs[0])
#define papernext(p)       ((((p)+1) && ((p)+1)->name) ? (p+1) : NULL)

#define defaultpapername() DEFAULT_PAPER_NAME
#define systempapername()  DEFAULT_PAPER_NAME

#define defaultpapersizefile() NULL
#define systempapersizefile()  NULL

static const struct paper *
paperinfo (const char *ppspec)
{
  const struct paper *ppinfo;

  ppinfo = paperfirst();
  while (ppinfo && papername(ppinfo)) {
    if (!strcasecmp(ppspec, papername(ppinfo)))
      break;
    ppinfo = papernext(ppinfo);
  }
  
  return ((ppinfo && papername(ppinfo)) ? ppinfo : NULL);
}

#endif /* HAVE_LIBPAPER */

static void paperconf (void)
{
  const struct paper *ppinfo;

  fprintf(stdout, "Supported paper format:\n");
  ppinfo = paperfirst();
  while (ppinfo && papername(ppinfo)) {
    fprintf(stdout, "  %s: %g %g\n",
	    papername(ppinfo), paperpswidth(ppinfo), paperpsheight(ppinfo));
    ppinfo = papernext(ppinfo);
  }
  if (systempapername() != NULL)
    fprintf(stdout, "System default: %s\n", systempapername());
  else if (defaultpapername() != NULL)
    fprintf(stdout, "System default: %s\n", defaultpapername());

  return;
}

static int get_mediasize (struct rectangle *rect, const char *ppspec)
{
  const struct paper *ppinfo;

  ppinfo = paperinfo(ppspec);
  if (ppinfo && papername(ppinfo)) {
    rect->urx = paperpswidth(ppinfo);
    rect->ury = paperpsheight(ppinfo);
  } else {
    char  *start, *end, *number;
    double u2bp;

    start = (char *) ppspec;
    end   = start + strlen(ppspec);
    /* page width */
    {
      number = parse_number(&start, end);
      if (!number)
	return -1;
      rect->urx = atof(number);
      RELEASE(number);
      if (start < end && *start != ',') {
	u2bp = parse_one_unit(&start, end);
	if (u2bp <= 0.0)
	  return -1;
	rect->urx *= u2bp;
      }
    }
    if (start >= end || *start != ',' ||
	++start >= end)
      return -1;
    /* page height */
    {
      number = parse_number(&start, end);
      if (!number)
	return -1;
      rect->ury = atof(number);
      RELEASE(number);
      if (start < end) {
	u2bp = parse_one_unit(&start, end);
	if (u2bp <= 0.0)
	  return -1;
	rect->ury *= u2bp;
      }
    }
  }

  rect->llx = rect->lly = 0.0;
  return ((rect->urx <= 0.0 || rect->ury <= 0.0) ? -1 : 0);
}

char *dvi_filename = NULL, *pdf_filename = NULL;
static void set_default_pdf_filename(void)
{
  const char *dvi_base;
#ifdef HAVE_XBASENAME
  dvi_base = xbasename (dvi_filename);
#else   
  dvi_base = basename (dvi_filename);
#endif   
  if (strlen (dvi_base) < 5 || strncmp (".dvi", dvi_base+strlen(dvi_base)-4, 4)) 
  {
    pdf_filename = NEW (strlen(dvi_base)+5, char);
    strcpy (pdf_filename, dvi_base);
    strcat (pdf_filename, ".pdf");
  } else
  {
    pdf_filename = NEW (strlen(dvi_base)+1, char);
    strncpy (pdf_filename, dvi_base, strlen(dvi_base)-4);
    strcpy (pdf_filename+strlen(dvi_base)-4, ".pdf");
  }
}

static void usage (void)
{
  fprintf (stdout, "\nThis is %s-%s by the DVIPDFMx project team,\n", PACKAGE, VERSION);
  fprintf (stdout, "an extended version of dvipdfm-0.13.2c developed by Mark A. Wicks.\n");
  fprintf (stdout, "\nCopyright (C) 2002-2003 by the DVIPDFMx project team\n");
  fprintf (stdout, "\nThis is free software; you can redistribute it and/or modify\n");
  fprintf (stdout, "it under the terms of the GNU General Public License as published by\n");
  fprintf (stdout, "the Free Software Foundation; either version 2 of the License, or\n");
  fprintf (stdout, "(at your option) any later version.\n");
  fprintf (stdout, "\nUsage: dvipdfmx [options] dvifile\n");
  fprintf (stdout, "-c      \tIgnore color specials (for B&W printing)\n");
  fprintf (stdout, "-f filename\tSet font map file name [t1fonts.map]\n");
  fprintf (stdout, "-o filename\tSet output file name [dvifile.pdf]\n");
  fprintf (stdout, "-l \t\tLandscape mode\n");
  fprintf (stdout, "-m number\tSet additional magnification\n");
  fprintf (stdout, "-p papersize\tSet papersize [a4]\n");
  fprintf (stdout, "-r resolution\tSet resolution (in DPI) for raster fonts [600]\n");
  fprintf (stdout, "-s pages\tSelect page ranges (-)\n");
  fprintf (stdout, "-t      \tEmbed thumbnail images\n");
  fprintf (stdout, "-T      \tRemove thumbnail images when finished\n");
  fprintf (stdout, "-x dimension\tSet horizontal offset [1.0in]\n");
  fprintf (stdout, "-y dimension\tSet vertical offset [1.0in]\n");
  fprintf (stdout, "-e          \tDisable partial font embedding [default is enabled]\n");
  fprintf (stdout, "-z number\tSet compression level (0-9) [default is 9]\n");
  fprintf (stdout, "-d number\tSet PDF decimal digits (0-4) [default is 3]\n");
  fprintf (stdout, "-v          \tBe verbose\n");
  fprintf (stdout, "-vv         \tBe more verbose\n");
  fprintf (stdout, "-S          \tEnable PDF encryption\n");
  fprintf (stdout, "\nAll dimensions entered on the command line are \"true\" TeX dimensions.\n");
  fprintf (stdout, "Argument of \"-s\" lists physical page ranges separated by commas, e.g., \"-s 1-3,5-6\"\n");
  fprintf (stdout, "Papersize is specified by paper format (e.g., \"a4\") or by w<unit>,h<unit> (e.g., \"20cm,30cm\").\n");
  fprintf (stdout, "Please type \"dvipdfmx -vh\" for the list of available paper format.\n\n");
  if (verbose) {
    paperconf();
  }
  fprintf (stdout, "\n");

  exit(1);
}

struct page_range 
{
  unsigned int first, last;
} *page_ranges = NULL;
int max_page_ranges = 0, num_page_ranges = 0;

#define pop_arg() {argv += 1; argc -= 1;}
static void do_args (int argc, char *argv[])
{
  char *flag, *ptr;
  while (argc > 0 && *argv[0] == '-') {
    for (flag=argv[0]+1; *flag != 0; flag++) {
      switch (*flag) {
      case 'D':
	if (argc < 2) {
	  fprintf (stderr, "PS->PDF conversion command line template missing\n\n");
	  usage();
	}
	set_distiller_template(argv[1]);
	pop_arg();
	break;
      case 'r':
	if (argc < 2) {
	  fprintf (stderr, "\nResolution specification missing a number\n\n");
	  usage();
	}
	font_dpi = (int) strtol (argv[1], &ptr, 0);
	if (ptr == argv[1]) {
	  fprintf (stderr, "\nError in number following resolution specification\n\n");
	  usage();
	}
	pop_arg();
	break;
      case 'm':
	if (argc < 2) {
	  fprintf (stderr, "\nMagnification specification missing a number\n\n");
	  usage();
	}
	{
	  char *result, *end, *start = argv[1];
	  end = start + strlen(argv[1]);
	  result = parse_number (&start, end);
	  if (result != NULL && start == end) {
	    mag = atof (result);
	  }
	  else {
	    fprintf (stderr, "\nError in number following magnification specification\n\n");
	    usage();
	  }
	  if (result != NULL) {
	    RELEASE (result);
	  }
	}
	pop_arg();
	break;
      case 'g':
	if (argc < 2) {
	  fprintf (stderr, "\nAnnotation \"grow\" specification missing a number\n\n");
	  usage();
	}
	{
	  char *result, *end, *start = argv[1];
	  end = start + strlen(argv[1]);
	  result = parse_number (&start, end);
	  if (result != NULL && start == end) {
	    pdf_special_set_grow (atof (result));
	  }
	  else {
	    fprintf (stderr, "\nError in number following magnification specification\n\n");
	    usage();
	  }
	  if (result != NULL) {
	    RELEASE (result);
	  }
	}
	pop_arg();
	break;
      case 'x':
	if (argc < 2) {
	  fprintf (stderr, "\nX Offset specification missing a number\n\n");
	  usage();
	}
	{
	  char *result, *end, *start = argv[1];
	  double unit;
	  end = start + strlen(argv[1]);
	  result = parse_number (&start, end);
	  if (result != NULL) {
	    x_offset = atof (result);
	  }
	  else {
	    fprintf (stderr, "\nError in number following xoffset specification\n\n");
	    usage();
	  }
	  if (result != NULL) {
	    RELEASE (result);
	  }
	  unit = parse_one_unit(&start, end);
	  if (unit > 0.0) {
	    x_offset *= unit;
	  }
	  else {
	    fprintf (stderr, "\nError in dimension specification following xoffset\n\n");
	    usage();
	  }
	}
	pop_arg();
	break;
      case 'y':
	if (argc < 2) {
	  fprintf (stderr, "\nY offset specification missing a number\n\n");
	  usage();
	}
	{
	  char *result, *end, *start = argv[1];
	  double unit;
	  end = start + strlen(argv[1]);
	  result = parse_number (&start, end);
	  if (result != NULL) {
	    y_offset = atof (result);
	  }
	  else {
	    fprintf (stderr, "\nError in number following yoffset specification\n\n");
	    usage();
	  }
	  if (result != NULL) {
	    RELEASE (result);
	  }
	  unit = parse_one_unit(&start, end);
	  if (unit > 0.0) {
	    y_offset *= unit;
	  }
	  else {
	    fprintf (stderr, "\nError in dimension specification following yoffset\n\n");
	    usage();
	  }
	}
	pop_arg();
	break;
      case 'o':  
	if (argc < 2)
	  ERROR ("Missing output file name");
	pdf_filename = NEW (strlen(argv[1])+1,char);
	strcpy (pdf_filename, argv[1]);
	pop_arg();
	break;
      case 's':
	{
	  char *result, *end, *start = argv[1];
	  if (argc < 2)
	    ERROR ("Missing page selection specification");
	  end = start + strlen (argv[1]);
	  while (start < end) {
	    /* Enlarge page range table if necessary */
	    if (num_page_ranges >= max_page_ranges) {
	      max_page_ranges += 4;
	      page_ranges = RENEW (page_ranges, max_page_ranges,
				   struct page_range);
	    }
	    skip_white (&start, end);
	    page_ranges[num_page_ranges].first = 0;
	    if ((result = parse_unsigned (&start, end))) {
	      page_ranges[num_page_ranges].first = atoi(result)-1;
	      page_ranges[num_page_ranges].last =
		page_ranges[num_page_ranges].first;
	      RELEASE(result);
	    }
	    skip_white (&start, end);
	    if (*start == '-') {
	      start += 1;
	      page_ranges[num_page_ranges].last = UINT_MAX;
	      skip_white (&start, end);
	      if (start < end &&
		  ((result = parse_unsigned (&start, end)))) {
		page_ranges[num_page_ranges].last = atoi(result)-1;
		RELEASE (result);
	      }
	    }
	    num_page_ranges += 1;
	    skip_white (&start, end);
	    if (start < end && *start == ',') {
	      start += 1;
	      continue;
	    }
	    skip_white (&start, end);
	    if (start < end) {
	      fprintf (stderr, "Page selection? %s", start);
	      ERROR ("Bad page range specification");
	    }
	  }
	  pop_arg();
	}
	break;
      case 't':
	{
#ifdef HAVE_LIBPNG
	  pdf_doc_enable_thumbnails ();
#else
	  ERROR ("The thumbnail option requires libpng, which you apparently don't have.");
#endif /* HAVE_LIBPNG */
	}
	break;
      case 'T':
	{
#ifdef HAVE_LIBPNG
	  thumb_remove ();
#else
	  ERROR ("The thumbnail option requires libpng, which you apparently don't have.");
#endif /* HAVE_LIBPNG */
	}
	break;
      case 'p':
	{
	  struct rectangle rect;
	  if (argc < 2)
	    ERROR ("Missing paper size");
	  if (get_mediasize (&rect, argv[1]) < 0) {
	    ERROR("Unknown papersize specification format: \"%s\"", argv[1]);
	  }
	  paper_width  = rect.urx;
	  paper_height = rect.ury;
	  default_papersize = 0;
	  pop_arg();
	}
	break;
      case 'c':
	ignore_colors = 1;
	break;
      case 'l':
	landscape_mode = 1;
	break;
      case 'f':
	read_mapfile (argv[1]);
	pop_arg();
	break;
      case 'e':
	type1_disable_partial();
	break;
      case 'q':
	really_quiet = 1;
	break;
      case 'v':
	dvi_set_verbose();
	fontmap_set_verbose();
	pdf_dev_set_verbose();
	pdf_doc_set_verbose();
	pdf_enc_set_verbose();
	pdf_obj_set_verbose();
	pdf_special_set_verbose();
	pk_set_verbose();
	tfm_set_verbose();
	type1_set_verbose();
	vf_set_verbose();
	Encoding_set_verbose();
	AGL_set_verbose();
	UC_set_verbose();
	CMap_set_verbose();
	Type1CFont_set_verbose();
	TTFont_set_verbose();
	Type0Font_set_verbose();
	CIDFont_set_verbose();
	verbose++;
	break;
      case 'V':
	{
	  unsigned level = 3;
	  if (isdigit (*(flag+1)))
	    level = *(++flag) - '0';
	  else {
	    if (argc < 2) {
	      fprintf (stderr, "\nVersion specification missing number (3-4)\n\n");
	      usage();
	    }
	    level = (int) strtol (argv[1], &ptr, 0);
	    if (ptr == argv[1]) {
	      fprintf (stderr, "\nError in number following version specification\n\n");
	      usage();
	    }
	    pop_arg();
	  }
	  if (level >= 3 && level <= 4)
	    pdf_set_version(level);
	  else if (level == 2)
	    fprintf (stderr, "\nDVIPDFMx does not support the PDF version 1.2 so that 1.3 will be assumed\n\n");
	  else
	    fprintf (stderr, "\nNumber following version specification is out of range\n\n");
	}
	break;
      case 'z': 
	{
	  int level = 9;
#ifndef HAVE_ZLIB
	  fprintf (stderr, "\nYou don't have compression compiled in.  Possibly libz wasn't found by configure.\nCompression specification will be ignored.\n\n");
#endif  /* HAVE_ZLIB */
	  if (isdigit (*(flag+1)))
	    level = *(++flag) - '0';
	  else {
	    if (argc < 2) {
	      fprintf (stderr, "\nCompression specification missing number for level\n\n");
	      usage();
	    }
	    level = (int) strtol (argv[1], &ptr, 0);
	    if (ptr == argv[1]) {
	      fprintf (stderr, "\nError in number following compression specification\n\n");
	      usage();
	    }
	    pop_arg();
	  }
	  if (level >= 0 && level <= 9)
	    pdf_obj_set_compression(level);
	  else
	    fprintf (stderr, "\nNumber following compression specification is out of range\n\n");
	}
	break;
      case 'd': 
	{
	  int digits;
	  if (isdigit(*(flag+1)))
	    digits = *(++flag) - '0';
	  else {
	    if (argc < 2) {
	      fprintf(stderr, "\nMissing number for pdfdecimaldigits\n\n");
	      usage();
	    }
	    digits = (int)strtol(argv[1], &ptr, 0);
	    if (ptr == argv[1]) {
	      fprintf (stderr, "\nError in pdfdecimaldigits\n\n");
	      usage();
	    }
	    pop_arg();
	  }
	  if (digits >= 0 && digits <= 4)
	    pdfdecimaldigits = digits;
	  else
	    fprintf (stderr, "\npdfdecimaldigits is out of range (0-4). Use the default value 3\n\n");
	}
	break;
      case 'S':
        do_encryption = 1;
	break;
      case 'K': 
	if (argc < 2) {
	  fprintf (stderr, "\nEncryption key bits specification missing number\n\n");
	  usage();
	}
	key_bits = (unsigned) strtol (argv[1], &ptr, 0);
	if (ptr == argv[1]) {
	  fprintf (stderr, "\nError in number following encryption key bits specification\n\n");
	  usage();
	}
	pop_arg();
	if (key_bits < 40 || key_bits > 128)
	  fprintf (stderr, "\nNumber following encryption key bits specification is out of range\n\n");
	break;
      case 'P': 
	if (argc < 2) {
	  fprintf (stderr, "\nEncryption permission specification missing number\n\n");
	  usage();
	}
	permission = (unsigned) strtol (argv[1], &ptr, 0);
	if (ptr == argv[1]) {
	  fprintf (stderr, "\nError in number following encryption permission specification\n\n");
	  usage();
	}
	pop_arg();
	break;
      default:
	usage();
      }
    }
    argc -= 1 ;
    argv += 1;
  }
  if (argc > 1) {
    fprintf (stderr, "\nMultiple dvi filenames?\n\n");
    usage();
  }
  /* The only legitimate way to have argc == 0 here is
     is do_args was called from config file.  In that case, there is
     no dvi file name.  Check for that case  */
  if (argc > 0) {
    if (strncmp (".dvi", argv[0]+strlen(argv[0])-4, 4)) {
      dvi_filename = NEW (strlen (argv[0])+1+4, char);
      strcpy (dvi_filename, argv[0]);
      strcat (dvi_filename, ".dvi");
    }
    else {
      dvi_filename = NEW (strlen (argv[0])+1, char);
      strcpy (dvi_filename, argv[0]);
    }
  }
}


static void cleanup(void)
{
  RELEASE (dvi_filename);
  RELEASE (pdf_filename);
  if (page_ranges)
    RELEASE (page_ranges);
  psimage_close();
  release_dpx_images();
}

static const char *default_config_file = "dvipdfmx.cfg";

static void read_config_file (const char *config)
{
  char *fullname;
  char *start, *end, *option;
  FILE *fp;
  
#ifdef MIKTEX
  if (!miktex_find_app_input_file("dvipdfm", config, fullname = work_buffer))
    return;
#else
  if ((fullname = kpse_find_file(config, kpse_program_text_format, 1)) == NULL)
    return;
#endif

  if (!(fp = MFOPEN (fullname, FOPEN_R_MODE))) {
    fprintf (stderr, "\nCould not open config file \"%s\".\n", fullname);
    return;
  }
  while ((start = mfgets (work_buffer, WORK_BUFFER_SIZE, fp)) != NULL) {
    char *argv[2];
    int   argc;

    argc = 0;
    end = work_buffer + strlen(work_buffer);
    skip_white (&start, end);
    if (start >= end)
      continue;
    /* Build up an argument list as if it were passed on the command
       line */
    if ((option = parse_ident (&start, end))) {
      argc = 1;
      argv[0] = NEW (strlen(option)+2, char);
      strcpy (argv[0]+1, option);
      RELEASE (option);
      *argv[0] = '-';
      skip_white (&start, end);
      if (start < end) {
	argc += 1;
	if (*start == '"') {
	  argv[1] = parse_c_string (&start, end);
	}
	else
	  argv[1] = parse_ident (&start, end);
      }
    }
    do_args (argc, argv);
    while (argc > 0) {
      RELEASE (argv[--argc]);
    }
  }
  if (fp)
    MFCLOSE(fp);
}

static void set_system_default (void)
{
  struct rectangle rect;

  if (systempapername() != NULL &&
      get_mediasize(&rect, systempapername()) >= 0) {
    paper_width  = rect.urx;
    paper_height = rect.ury;
  } else if (defaultpapername() != NULL &&
	     get_mediasize(&rect, defaultpapername())) {
    paper_width  = rect.urx;
    paper_height = rect.ury;
  }
}

void error_cleanup (void)
{
  pdf_error_cleanup();
  remove (pdf_filename);
  fprintf (stderr, "\nOutput file removed.\n");
}

int CDECL main (int argc, char *argv[]) 
{
  int i;
  int at_least_one_page = 0;

#ifdef MIKTEX
  miktex_initialize ();
#endif
  if (argc < 2) {
    fprintf (stderr, "\nNo dvi filename specified.\n\n");
    usage();
    return 1;
  }
#ifndef MIKTEX
  kpse_set_program_name(argv[0], "dvipdfm");
#endif

  argv+=1;
  argc-=1;

  paperinit();
  set_system_default();

  /* Process config file, if any */
  if (default_config_file)
    read_config_file(default_config_file);

  default_papersize = 1; /* reset */
  do_args (argc, argv);

#ifndef MIKTEX
  kpse_init_prog("", font_dpi, NULL, NULL);
  pk_set_dpi(font_dpi);
  kpse_set_program_enabled(kpse_pk_format, true, kpse_src_texmf_cnf);
#endif

  if (!dvi_filename) {
    fprintf (stderr, "\nNo dvi filename specified.\n\n");
    usage();
  }

  /* Check for ".dvi" at end of argument name */
  if (pdf_filename == NULL)
    set_default_pdf_filename();
  
  if (!really_quiet)
    fprintf (stdout, "\n%s -> %s\n", dvi_filename, pdf_filename);

  if (do_encryption) {
    pdf_enc_set_passwd(key_bits, permission, dvi_filename, pdf_filename);
    if (key_bits > 40)
      pdf_set_version(4);
  }

  /* dev_set_precision (pdfdecimaldigits); */
  init_pdf_numbers(pdfdecimaldigits);

  dvi_init (dvi_filename, pdf_filename, mag, x_offset, y_offset);

  if (ignore_colors) {
    color_special_ignore_colors();
    pdf_special_ignore_colors();
  }

  if (default_papersize)
    dvi_paper_size(&paper_width, &paper_height, &landscape_mode);
  if (landscape_mode) {
    double swap = paper_width;
    paper_width = paper_height; paper_height = swap;
  }
  dev_set_page_size(paper_width, paper_height);

  if ((num_page_ranges))
    for (i=0; i<num_page_ranges; i++) {
      unsigned j;
      if (page_ranges[i].first <= page_ranges[i].last)
	for (j=page_ranges[i].first; j<=page_ranges[i].last && j<dvi_npages(); j++) {
 	  fprintf (stderr, "[%d", j+1);
   	  dvi_do_page (j);
	  at_least_one_page = 1;
	  fprintf (stderr, "]");
	}
      else
	for (j=page_ranges[i].first;
	     j>=page_ranges[i].last && j<dvi_npages(); j--) {
	  fprintf (stderr, "[%d", j+1);
	  dvi_do_page (j);
	  at_least_one_page = 1;
	  fprintf (stderr, "]");
	}
    }
  if (!at_least_one_page && num_page_ranges) {
    fprintf (stderr, "No pages fall in range!\nFalling back to entire document.\n");
  }
  if (!at_least_one_page) /* Fall back to entire document */
    for (i=0; i<dvi_npages(); i++) {
      fprintf (stderr, "[%d", i+1);
      dvi_do_page (i);
      fprintf (stderr, "]");
    }
  dvi_close();
  fprintf (stderr, "\n");
  cleanup();

  paperdone();
#ifdef MIKTEX
  miktex_uninitialize ();
#endif

  return 0;
}
