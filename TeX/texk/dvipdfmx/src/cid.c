/*  $Header: /home/cvsroot/dvipdfmx/src/cid.c,v 1.13 2004/02/05 16:14:22 hirata Exp $
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>
    
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

/*
 * CID-keyed font support:
 *
 *  See also, cidtype0, and cidtype2
 */

#include <string.h>

#include "system.h"
#include "mem.h"
#include "error.h"
#include "dpxutil.h"

#include "pdfobj.h"

#include "cidtype0.h"
#include "cidtype2.h"
#include "cid_p.h"
#include "cid.h"

#define CIDFONT_DEBUG     3
#define CIDFONT_DEBUG_STR "CIDFont"

#define PDF_CID_SUPPORT_MIN 2
#define PDF_CID_SUPPORT_MAX 5

/*
 * Unicode and Standard Character Collections.
 *
 *  Dvipdfmx does not rely on any character collections/character sets.
 *  Here is the minimal information required for PDF.
 *
 */
static struct {
  const char *registry;
  const char *ordering;
  /* Heighest Supplement values supported by PDF-1.0, 1.1, ... */
  int   supplement[PDF_CID_SUPPORT_MAX+1];
} CIDFont_stdcc_def[] = {
  {"Adobe", "UCS",    {-1, -1, 0, 0, 0, 0}}, 
  {"Adobe", "GB1",    {-1, -1, 0, 2, 4, 4}}, 
  {"Adobe", "CNS1",   {-1, -1, 0, 0, 3, 4}},
  {"Adobe", "Japan1", {-1, -1, 2, 2, 4, 5}},
  {"Adobe", "Korea1", {-1, -1, 1, 1, 2, 2}},
  {"Adobe", "Identity", {-1, -1, 0, 0, 0, 0}},
  {NULL, NULL, {0,0,0,0,0,0}}
};
#define UCS_CC    0
#define ACC_START 1

/* Optional supplement after alias name. */
static struct {
  const char *name;
  int   index;
} CIDFont_stdcc_alias[] = {
  {"AU",     0}, {"AG1",    1}, {"AC1",    2}, {"AJ1",    3}, {"AK1",    4}, {"AI", 5},
  {"UCS",    0}, {"GB1",    1}, {"CNS1",   2}, {"Japan1", 3}, {"Korea1", 4}, {"Identity", 5},
  {"U",      0}, {"G",      1}, {"C",      2}, {"J",      3}, {"K",      4}, {"I", 5},
  {NULL,     0}
};

static char *substr        (char **str, char stop);
static void  release_opt   (cid_opt *opt);
static char *strip_options (const char *map_name, cid_opt *opt);

static int __verbose = 0;

void
CIDFont_set_verbose (void)
{
  CIDFont_type0_set_verbose();
  CIDFont_type2_set_verbose();
  __verbose++;
}

int
CIDFont_require_version (void)
{
  return PDF_CID_SUPPORT_MIN;
}

CIDFont *
CIDFont_new (void)
{
  CIDFont *font = NULL;

  font = NEW(1, struct CIDFont);

  font->name     = NULL;
  font->fontname = NULL;
  font->ident   = NULL;

  /*
   * CIDFont
   */
  font->subtype = -1;
  font->flags   = FONT_FLAG_NONE;
  font->csi     = NULL;
  font->options = NULL;
  (font->parent)[0] = -1; /* Horizontal */
  (font->parent)[1] = -1; /* Vertical   */

  /*
   * PDF Font Resource
   */
  font->indirect = NULL;
  font->fontdict = NULL;
  font->descriptor = NULL;

  return font;
}

/* It does write PDF objects. */
void
CIDFont_flush (CIDFont *font)
{
  if (font) {
    if (font->indirect)   pdf_release_obj(font->indirect);
    font->indirect = NULL;
    if (font->fontdict)   pdf_release_obj(font->fontdict);
    font->fontdict = NULL;
    if (font->descriptor) pdf_release_obj(font->descriptor);
    font->descriptor = NULL;
  }
}

void
CIDFont_release (CIDFont *font)
{
  if (font) {
    switch(font->subtype) {
    case CIDFONT_TYPE0:
      CIDFont_type0_release(font);
      break;
    case CIDFONT_TYPE2:
      CIDFont_type2_release(font);
      break;
    default:
      ERROR("%s: Unknown type.", CIDFONT_DEBUG_STR);
      break;
    }
    if (font->indirect)   ERROR("%s: Object not flushed.", CIDFONT_DEBUG_STR);
    if (font->fontdict)   ERROR("%s: Object not flushed.", CIDFONT_DEBUG_STR);
    if (font->descriptor) ERROR("%s: Object not flushed.", CIDFONT_DEBUG_STR);

    if (font->fontname) RELEASE(font->fontname);
    if (font->name)     RELEASE(font->name);
    if (font->ident)    RELEASE(font->ident);
    if (font->csi) {
      if (font->csi->registry)
	RELEASE(font->csi->registry);
      if (font->csi->ordering)
	RELEASE(font->csi->ordering);
      RELEASE(font->csi);
    }
    if (font->options)
      release_opt(font->options);
  }
}

char *
CIDFont_get_fontname (CIDFont *font)
{
  ASSERT(font);
  return font->fontname;
}

int
CIDFont_get_subtype (CIDFont *font)
{
  ASSERT(font);
  return font->subtype;
}

int
CIDFont_get_embedding (CIDFont *font)
{
  ASSERT(font);
  return font->options->embed;
}

CIDSysInfo *
CIDFont_get_CIDSysInfo (CIDFont *font)
{
  ASSERT(font);

  return font->csi;
}

/*
 * Returns ID of parent Type0 font
 *  wmode: 0 for horizontal, 1 for vertical
 */
int
CIDFont_get_parent_id (CIDFont *font, int wmode)
{
  ASSERT(font);

  if (wmode < 0 || wmode > 1)
    ERROR("%s: Invalid wmode value.", CIDFONT_DEBUG_STR);

  return (font->parent)[wmode];
}

pdf_obj *
CIDFont_get_resource (CIDFont *font)
{
  ASSERT(font);

  if (!font->indirect)
    font->indirect = pdf_ref_obj(font->fontdict);

  return pdf_link_obj(font->indirect);
}

/*
 * Set parent Type0 font.
 */
void
CIDFont_attach_parent (CIDFont *font, int parent_id, int wmode)
{
  ASSERT(font);

  if (wmode < 0 || wmode > 1)
    ERROR("%s: Invalid wmode value.", CIDFONT_DEBUG_STR);

  if (font->parent[wmode] >= 0)
    WARN("%s: CIDFont already have a parent Type1 font.", CIDFONT_DEBUG_STR);

  font->parent[wmode] = parent_id;
}

int
CIDFont_is_ACCFont (CIDFont *font)
{
  ASSERT(font);

  if (!font->csi)
    ERROR("%s: CIDSystemInfo undefined.", CIDFONT_DEBUG_STR);
  {
    int i;
    for (i = ACC_START; CIDFont_stdcc_def[i].registry; i++) {
      if (!strcmp(font->csi->registry, CIDFont_stdcc_def[i].registry) ||
	  !strcmp(font->csi->ordering, CIDFont_stdcc_def[i].ordering))
	return 1;
    }
  }

  return 0;
}

int
CIDFont_is_UCSFont (CIDFont *font)
{
  ASSERT(font);
  if (!strcmp(font->csi->ordering, "UCS") ||
      !strcmp(font->csi->ordering, "UCS2"))
    return 1;
  else
    return 0;
}

static void
CIDFont_dofont (CIDFont *font)
{
  if (!font || !font->indirect)
    return;

  if (__verbose)
    MESG(":%s", font->ident);
  if (__verbose > 1) {
    if (font->filename)
      MESG("[%s]", font->filename);
    if (font->fontname)
      MESG("[%s]", font->fontname);
  }

  switch (font->subtype) {
  case CIDFONT_TYPE0:
    if(__verbose)
      MESG("[CIDFontType0]");
    CIDFont_type0_dofont(font);
    break;
  case CIDFONT_TYPE2:
    if(__verbose)
      MESG("[CIDFontType2]");
    CIDFont_type2_dofont(font);
    break;
  default:
    ERROR("%s: Unknown CIDFontType %d.", CIDFONT_DEBUG_STR, font->subtype);
    break;
  }
}

/****************************** BASIC FONT ********************************/

int
CIDFont_is_BaseFont (CIDFont *font)
{
  ASSERT(font);
  return (font->flags & FONT_FLAG_BASEFONT) ? 1 : 0;
}

#include "pdfparse.h"
#include "cid_basefont.h"

static int CIDFont_base_open (CIDFont *font, const char *name,
			      CIDSysInfo *cmap_csi, cid_opt *opt);

static int
CIDFont_base_open (CIDFont *font, const char *name, CIDSysInfo *cmap_csi, cid_opt *opt)
{
  pdf_obj    *tmp, *fontdict, *descriptor;
  char       *fontname = NULL;
  char       *registry, *ordering;
  int         supplement, idx;

  ASSERT(font);

  for (idx=0; cid_basefont[idx].fontname != NULL; idx++) {
    if (!strcmp(name, cid_basefont[idx].fontname) ||
	(strlen(name) == strlen(cid_basefont[idx].fontname) - strlen("-Acro") &&
	 !strncmp(name, cid_basefont[idx].fontname,
		  strlen(cid_basefont[idx].fontname)-strlen("-Acro")))
	)
      break;
  }

  if (cid_basefont[idx].fontname == NULL)
    return -1;

  fontname = NEW(strlen(name)+12, char);
  memset(fontname, 0, strlen(name)+12);
  strcpy(fontname, name);

  switch (opt->style) {
  case FONT_STYLE_BOLD:
    strcat(fontname, ",Bold");
    break;
  case FONT_STYLE_ITALIC:
    strcat(fontname, ",Italic");
    break;
  case FONT_STYLE_BOLDITALIC:
    strcat(fontname, ",BoldItalic");
    break;
  }
  {
    const char *start;
    const char *end;

    start = cid_basefont[idx].fontdict;
    end   = start + strlen(start);
    fontdict   = parse_pdf_dict((char **)&start, (char *)end);
    start = cid_basefont[idx].descriptor;
    end   = start + strlen(start);
    descriptor = parse_pdf_dict((char **)&start, (char *)end);

    ASSERT(fontdict && descriptor);
  }

  tmp = pdf_lookup_dict(fontdict, "CIDSystemInfo");
  if (!tmp)
    ERROR("%s: Undefined CIDSystemInfo.", CIDFONT_DEBUG_STR);
  registry   = pdf_string_value(pdf_lookup_dict(tmp, "Registry"));
  ordering   = pdf_string_value(pdf_lookup_dict(tmp, "Ordering"));
  supplement = pdf_number_value(pdf_lookup_dict(tmp, "Supplement"));
  if (cmap_csi) { /* NULL for accept any */
    if (strcmp(registry, cmap_csi->registry) ||
	strcmp(ordering, cmap_csi->ordering))
      ERROR("%s: Inconsistent CMap applied to CID-keyed font %s.",
	    CIDFONT_DEBUG_STR< cid_basefont[idx].fontname);
    else if (supplement < cmap_csi->supplement) {
      WARN("%s: CMap have higher supplement number.", CIDFONT_DEBUG_STR);
      WARN("%s: Some chracters may not be displayed or printed.", CIDFONT_DEBUG_STR);
    }
  }

  font->fontname = fontname;
  font->filename = NULL;
  font->flags   |= FONT_FLAG_BASEFONT;
  tmp = pdf_lookup_dict(fontdict, "Subtype");
  if (!tmp)
    ERROR("%s: Undefined CIDFontType.", CIDFONT_DEBUG_STR);
  if (!strcmp(pdf_name_value(tmp), "CIDFontType0"))
    font->subtype = CIDFONT_TYPE0;
  else if (!strcmp(pdf_name_value(tmp), "CIDFontType2"))
    font->subtype = CIDFONT_TYPE2;
  else
    ERROR("%s: Unknown CIDFontType %s.", CIDFONT_DEBUG_STR, pdf_name_value(tmp));
  font->csi = NEW(1, CIDSysInfo);
  font->csi->registry   = strdup(registry);
  font->csi->ordering   = strdup(ordering);
  font->csi->supplement = supplement;

  pdf_add_dict(fontdict,   pdf_new_name("Type"),     pdf_new_name("Font"));
  pdf_add_dict(fontdict,   pdf_new_name("BaseFont"), pdf_new_name(fontname));
  pdf_add_dict(descriptor, pdf_new_name("Type"),     pdf_new_name("FontDescriptor"));
  pdf_add_dict(descriptor, pdf_new_name("FontName"), pdf_new_name(fontname));

  font->fontdict   = fontdict;
  font->descriptor = descriptor;

  opt->embed = 0;

  return 0;
}

static const struct {
  const char *registry;
  const char *ordering;
  const char *fontname;
} cid_default[] = {
  {"Adobe", "CNS1",   "AdobeMingStd-Light-Acro"},
  {"Adobe", "GB1",    "AdobeSongStd-Light-Acro"},
  {"Adobe", "Japan1", "KozMinPro-Regular-Acro"},
  {"Adobe", "Korea1", "AdobeMyungjoStd-Medium-Acro"},
  {NULL, NULL, NULL}
};

static int
try_default (CIDFont *font, const char *map_name, CIDSysInfo *cmap_csi, cid_opt *opt)
{
  int i;
  char *registry = NULL, *ordering = NULL;

  ASSERT(font);
  WARN("%s: No usable CID-keyed font found for \"%s\".", CIDFONT_DEBUG_STR, map_name);
  if (opt && opt->csi) {
    registry = opt->csi->registry;
    ordering = opt->csi->ordering;
  } else if (cmap_csi) {
    registry = cmap_csi->registry;
    ordering = cmap_csi->ordering;
  }
  WARN("%s: Trying default font for \"%s-%s\".", CIDFONT_DEBUG_STR, registry, ordering);
  for (i = 0; cid_default[i].fontname; i++) {
    if ((registry && !strcmp(registry, cid_default[i].registry)) &&
	(ordering && !strcmp(ordering, cid_default[i].ordering))) {
      WARN("%s: \"%s\" will be used.", CIDFONT_DEBUG_STR, cid_default[i].fontname);
      return CIDFont_base_open(font, cid_default[i].fontname, cmap_csi, opt);
    }
  }

  return -1;
}


/******************************** CACHE *******************************/

#define CACHE_ALLOC_SIZE  16u

struct FontCache {
  int       num;
  int       max;
  CIDFont **fonts;
};

static struct FontCache *__cache   = NULL;

#define CHECK_ID(n) do {\
                        if (! __cache)\
                           ERROR("%s: CIDFont cache not initialized.", CIDFONT_DEBUG_STR);\
                        if ((n) < 0 || (n) >= __cache->num)\
                           ERROR("%s: Invalid ID %d", CIDFONT_DEBUG_STR, (n));\
                    } while (0)

void
CIDFont_cache_init (void)
{
  if (__cache)
    ERROR("%s: Already initialized.", CIDFONT_DEBUG_STR);

  if (__verbose > CIDFONT_DEBUG)
    MESG("%s: Initialize\n", CIDFONT_DEBUG_STR);

  __cache = NEW(1, struct FontCache);

  __cache->max  = CACHE_ALLOC_SIZE;
  __cache->fonts = NEW(__cache->max, struct CIDFont *);
  __cache->num  = 0;
}

CIDFont *
CIDFont_cache_get (int font_id)
{
  CHECK_ID(font_id);
  return __cache->fonts[font_id];
}

/*
 * cmap_csi is NULL if CMap is Identity.
 */
int
CIDFont_cache_find (const char *map_name, CIDSysInfo *cmap_csi)
{
  int      font_id = -1;
  CIDFont *font   = NULL;
  char    *name;
  cid_opt *opt;

  if (!__cache)
    CIDFont_cache_init();

  opt  = NEW(1, cid_opt);
  name = strip_options(map_name, opt);
  if (!opt->csi && cmap_csi) {
    /*
     * No CIDSystemInfo supplied explicitly. Copy from CMap's one if available.
     * It is not neccesary for CID-keyed fonts. But TrueType requires them.
     */
    opt->csi = NEW(1, CIDSysInfo);
    opt->csi->registry   = strdup(cmap_csi->registry);
    opt->csi->ordering   = strdup(cmap_csi->ordering);
    opt->csi->supplement = cmap_csi->supplement;
  }
  /*
   * Here, we do not compare font->ident and map_name because of
   * implicit CIDSystemInfo supplied by CMap for TrueType.
   */
  for (font_id = 0; font_id < __cache->num; font_id++) {
    font = __cache->fonts[font_id];
    if (!strcmp(font->name, name) && font->options->style == opt->style &&
	font->options->index == opt->index) {
      if (font->options->embed == opt->embed) {
	/*
	 * Case 1: CSI not available (Identity CMap)
	 *         Font is TrueType --> continue
	 *         Font is CIDFont  --> break
	 * Case 2: CSI matched      --> break
	 */
	if (!opt->csi) {
	  if (font->subtype == CIDFONT_TYPE2)
	    continue;
	  else
	    break;
	} else if (!strcmp(font->csi->registry, opt->csi->registry) &&
		   !strcmp(font->csi->ordering, opt->csi->ordering)) {
	  if (font->subtype == CIDFONT_TYPE2)
	    font->csi->supplement = MAX(opt->csi->supplement, font->csi->supplement);
	  break;
	}
      } else if (CIDFont_is_BaseFont(font)) {
	opt->embed = 0;
	break;
      }
    }
  }

  if (font_id < __cache->num && cmap_csi) {
    if (strcmp(font->csi->registry, cmap_csi->registry) ||
	strcmp(font->csi->ordering, cmap_csi->ordering))
      ERROR("%s: Incompatible CMap for CIDFont \"%s\"",
	    CIDFONT_DEBUG_STR, map_name);
  }

  if (font_id == __cache->num) {
    font = CIDFont_new();
    if (CIDFont_type0_open(font, name, cmap_csi, opt) < 0 &&
	CIDFont_type2_open(font, name, cmap_csi, opt) < 0 &&
	CIDFont_base_open (font, name, cmap_csi, opt) < 0 &&
	try_default(font, name, cmap_csi, opt) < 0) {
      CIDFont_release(font);
      release_opt(opt);
      RELEASE(name);
      return -1;
    } else {
      if (__cache->num >= __cache->max) {
	__cache->max  += CACHE_ALLOC_SIZE;
	__cache->fonts = RENEW(__cache->fonts, __cache->max, struct CIDFont *);
      }
      font->name    = name;
      font->ident   = strdup(map_name);
      font->options = opt;
      __cache->fonts[font_id] = font;
      (__cache->num)++;
    }
  }

  return font_id;
}

void
CIDFont_cache_close (void)
{
  if (__cache) {
    int font_id;
    for (font_id = 0; font_id < __cache->num; font_id++) {
      CIDFont *font = __cache->fonts[font_id];
      if (__verbose) MESG("(CID");
      CIDFont_dofont(font);
      CIDFont_flush(font);
      CIDFont_release(font);
      RELEASE(font);
      if (__verbose) MESG(")");
    }
    RELEASE(__cache->fonts);
    RELEASE(__cache);
    __cache = NULL;
  }
  if (__verbose > CIDFONT_DEBUG)
    MESG("%s: Close\n", CIDFONT_DEBUG_STR);
}

/******************************* OPTIONS *******************************/

/*
 * FORMAT:
 *
 *   (:int:)?!?string(/string)?(,string)?
 */

static void
release_opt (cid_opt *opt)
{
  if (opt->csi) {
    if (opt->csi->registry)
      RELEASE(opt->csi->registry);
    if (opt->csi->ordering)
      RELEASE(opt->csi->ordering);
    RELEASE(opt->csi);
  }
  RELEASE(opt);
}

static char *
substr (char **str, char stop)
{
  char *sstr, *endptr;

  endptr = strchr(*str, stop);
  if (!endptr || endptr == *str)
    return NULL;
  sstr = NEW(endptr-(*str)+1, char);
  memcpy(sstr, *str, endptr-(*str));
  sstr[endptr-(*str)] = '\0';

  *str = endptr+1;
  return sstr;
}

#include <ctype.h>
#define CID_MAPREC_CSI_DELIM '/'

static char *
strip_options (const char *map_name, cid_opt *opt)
{
  char *name;
  char *p, *next = NULL;
  int   pdf_ver = 0,  csi_idx = -1;
  int   have_csi = 0, have_style = 0;

  ASSERT(opt);
  p = (char *) map_name;
  name       = NULL;
  opt->csi   = NULL;
  opt->index = 0;
  opt->style = FONT_STYLE_NONE;
  opt->embed = 1;

  pdf_ver = pdf_get_version();
  if (pdf_ver < PDF_CID_SUPPORT_MIN)
    ERROR("%s: CID-keyed font not supported in version PDF-1.%d.",
	  CIDFONT_DEBUG_STR, pdf_ver);
  else if (pdf_ver > PDF_CID_SUPPORT_MAX)
    ERROR("%s: Unknown PDF version 1.%d.", CIDFONT_DEBUG_STR, pdf_ver);

  if (*p == ':' && isdigit(*(p+1))) {
    opt->index = (int) strtoul(p+1, &next, 10);
    if (*next == ':')
      p = next + 1;      
    else
      opt->index = 0;
  }
  if (*p == '!') { /* no-embedding */
    if (*(++p) == '\0')
      ERROR("%s: Invalid map record in %s (--> %s)",
	    CIDFONT_DEBUG_STR, map_name, p);
    opt->embed = 0;
  }

  if ((next = strchr(p, CID_MAPREC_CSI_DELIM)) != NULL) {
    if (next == p)
      ERROR("%s: Invalid map record in %s (--> %s)",
	    CIDFONT_DEBUG_STR, map_name, p);
    name = substr(&p, CID_MAPREC_CSI_DELIM);
    have_csi  = 1;
  } else if ((next = strchr(p, ',')) != NULL) {
    if (next == p)
      ERROR("%s: Invalid map record in %s (--> %s)",
	    CIDFONT_DEBUG_STR, map_name, p);
    name = substr(&p, ',');
    have_style = 1;
  } else
    name = strdup(p);

  if (have_csi) {
    int i;
    /* First try alias for standard one. */
    for (i = 0; CIDFont_stdcc_alias[i].name != NULL; i++) {
      if (!strncmp(p, CIDFont_stdcc_alias[i].name, strlen(CIDFont_stdcc_alias[i].name))) {
	csi_idx = CIDFont_stdcc_alias[i].index;
	opt->csi = NEW(1, CIDSysInfo);
	opt->csi->registry = strdup(CIDFont_stdcc_def[csi_idx].registry);
	opt->csi->ordering = strdup(CIDFont_stdcc_def[csi_idx].ordering);
	p += strlen(CIDFont_stdcc_alias[i].name);
	if (isdigit(*p)) { /* Optional supplement number specified. */
	  opt->csi->supplement = (int) strtoul(p, &next, 10);
	  p = next;
	} else /* Use heighest supported value for current output PDF version. */
	  opt->csi->supplement = CIDFont_stdcc_def[csi_idx].supplement[pdf_ver];
	break;
      }
    }
    if (opt->csi == NULL) {
      opt->csi = NEW(1, CIDSysInfo);
      /* Full REGISTRY-ORDERING-SUPPLEMENT */
      opt->csi->registry = substr(&p, '-');
      if (!opt->csi->registry)
	ERROR("%s: Invalid map record in %s (--> %s)",
	      CIDFONT_DEBUG_STR, map_name, p);
      opt->csi->ordering = substr(&p, '-');
      if (!opt->csi->ordering)
	ERROR("%s: Invalid map record in %s (--> %s)",
	      CIDFONT_DEBUG_STR, map_name, p);
      if (!isdigit(*p))
	ERROR("%s: Invalid map record in %s (--> %s)",
	      CIDFONT_DEBUG_STR, map_name, p);
      opt->csi->supplement = (int) strtoul(p, &next, 10);
      if (next == p)
	ERROR("%s: Invalid map record in %s (--> %s)",
	      CIDFONT_DEBUG_STR, map_name, p);
      /* Check for standart character collections. */
      for (i = 0; CIDFont_stdcc_def[i].ordering != NULL; i++) {
	if ((CIDFont_stdcc_def[i].registry &&
	     !strcmp(opt->csi->registry, CIDFont_stdcc_def[i].registry)) &&
	    !strcmp(opt->csi->ordering, CIDFont_stdcc_def[i].ordering)) {
	  csi_idx = i;
	  break;
	}
      }
      p = next;
    }
    if (*p == ',') {
      p++;
      have_style = 1;
    } else if (*p != '\0')
      ERROR("%s: Invalid map record in %s (--> %s)",
	    CIDFONT_DEBUG_STR, map_name, p);
  }

  if (have_style) {
    if (!strncmp(p, "BoldItalic", 10)) {
      if (*(p+10))
	ERROR("%s: Invalid map record in %s (--> %s)",
	      CIDFONT_DEBUG_STR, map_name, p);
      opt->style = FONT_STYLE_BOLDITALIC;
    } else if (!strncmp(p, "Bold", 4)) {
      if (*(p+4))
	ERROR("%s: Invalid map record in %s (--> %s)",
	      CIDFONT_DEBUG_STR, map_name, p);
      opt->style = FONT_STYLE_BOLD;
    } else if (!strncmp(p, "Italic", 6)) {
      if (*(p+6))
	ERROR("%s: Invalid map record in %s (--> %s)",
	      CIDFONT_DEBUG_STR, map_name, p);
      opt->style = FONT_STYLE_ITALIC;
    }
  }

  if (have_csi && csi_idx >= 0) {
    if (opt->csi->supplement > CIDFont_stdcc_def[csi_idx].supplement[pdf_ver]
	&& !opt->embed) {
      WARN("%s: Heighest supplement number supported in PDF-1.%d for %s-%s is %d.",
	   CIDFONT_DEBUG_STR, pdf_ver, opt->csi->registry, opt->csi->ordering,
	   CIDFont_stdcc_def[csi_idx].supplement[pdf_ver]);
      WARN("%s: Some character may not shown without embedded font (--> %s).",
	   CIDFONT_DEBUG_STR, map_name);
    }
  }

  return name;
}
