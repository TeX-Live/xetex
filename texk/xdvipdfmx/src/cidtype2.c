/*  $Header: /home/cvsroot/dvipdfmx/src/cidtype2.c,v 1.33 2005/07/17 09:53:38 hirata Exp $
    
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
 * TrueType glyf table is sorted by CID and no CIDToGIDMap is used here.
 * GhostScript can't handle CIDToGIDMap correctly.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "system.h"
#include "numbers.h"
#include "mem.h"
#include "error.h"
#include "dpxfile.h"

#include "pdfobj.h"
/* pseudo unique tag */
#include "pdffont.h"

#ifndef PDF_NAME_LEN_MAX
#  define PDF_NAME_LEN_MAX 255
#endif

/* TrueType */
#include "sfnt.h"
#include "tt_aux.h"
#include "tt_glyf.h"
#include "tt_cmap.h"
#include "tt_table.h"

#include "tt_gsub.h"

/* CID font */
#include "cmap.h"
#include "type0.h"
#include "cid.h"
#include "cid_p.h"
#include "cidtype2.h"

static int verbose   = 0;
static int opt_flags = 0;

void
CIDFont_type2_set_verbose (void)
{
  verbose++;
}

void
CIDFont_type2_set_flags (long flags)
{
  opt_flags = flags;
}

/*
 * PDF viewer applications use following tables (CIDFontType 2)
 *
 *  head, hhea, loca, maxp, glyf, hmtx, fpgm, cvt_, prep
 *
 *                                         - from PDF Ref. v.1.3, 2nd ed.
 *
 * The fpgm, cvt_, and prep tables appears only when TrueType instructions
 * requires them. Those tables must be preserved if they exist.
 * We use must_exist flag to indicate `preserve it if present'
 * and to make sure not to cause an error when it does not exist.
 *
 * post and name table must exist in ordinary TrueType font file,
 * but when a TrueType font is converted to CIDFontType 2 font, those tables
 * are no longer required.
 *
 * The OS/2 table (required for TrueType font for Windows and OS/2) contains
 * liscencing information, but PDF viewers seems not using them.
 *
 * The 'name' table added. See comments in ttf.c.
 */

static struct
{
  const char *name;
  int         must_exist;
} required_table[] = {
  {"OS/2", 0}, {"head", 1}, {"hhea", 1}, {"loca", 1}, {"maxp", 1},
  {"name", 1}, {"glyf", 1}, {"hmtx", 1}, {"fpgm", 0}, {"cvt ", 0},
  {"prep", 0}, {NULL, 0}
};

static void
validate_name (char *fontname, int len)
{
  int    i, count;
  char  *p;
  static const char *badstrlist[] = {
    "-WIN-RKSJ-H",
    "-WINP-RKSJ-H",
    "-WING-RKSJ-H",
    "-90pv-RKSJ-H",
    NULL
  };

  for (count = 0, i = 0; i < len; i++) {
    if (fontname[i] == 0) {
      memmove(fontname + i, fontname + i + 1, len - i);
      count++;
      len--;
    }
  }
  if (count > 0) {
    WARN("Removed %d null character(s) from fontname --> %s",
	 count, fontname);
  }
  fontname[len] = '\0';

  /* For some fonts that have bad PS name. ad hoc. remove me. */
  for (i = 0; badstrlist[i] != NULL; i++) {
    p = strstr(fontname, badstrlist[i]);
    if (p && p > fontname) {
      WARN("Removed string \"%s\" from fontname \"%s\".",
	   badstrlist[i], fontname);
      p[0] = '\0';
      len  = (int) (p - fontname);
      break;
    }
  }

  if (len < 1) {
    ERROR("No valid character found in fontname string.");
  }
}

/*
 * We will follow the convension for finding ToUnicode CMap described in PDF
 * Reference 4th ed., page 432. The name of "ToCode" (not limited to Unicode
 * here) CMap is obtained by concatenating the registry, ordering, and the
 * name of encoding.
 *
 * UCSms-UCS4, UCSms-UCS2, UCS4 added...
 */

#define WIN_UCS_INDEX_MAX   1
#define KNOWN_ENCODINGS_MAX 8
static struct
{
  unsigned short  platform;
  unsigned short  encoding;
  const char     *pdfnames[5];
} known_encodings[] = {
  {TT_WIN, TT_WIN_UCS4,     {"UCSms-UCS4", "UCSms-UCS2", "UCS4", "UCS2", NULL}},
  {TT_WIN, TT_WIN_UNICODE,  {"UCSms-UCS4", "UCSms-UCS2", "UCS4", "UCS2", NULL}},
  {TT_WIN, TT_WIN_SJIS,     {"90ms-RKSJ", NULL}},
  {TT_WIN, TT_WIN_RPC,      {"GBK-EUC",   NULL}},
  {TT_WIN, TT_WIN_BIG5,     {"ETen-B5",   NULL}},
  {TT_WIN, TT_WIN_WANSUNG,  {"KSCms-UHC", NULL}},
  {TT_MAC, TT_MAC_JAPANESE, {"90pv-RKSJ", NULL}},
  {TT_MAC, TT_MAC_TRADITIONAL_CHINESE, {"B5pc",     NULL}},
  {TT_MAC, TT_MAC_SIMPLIFIED_CHINESE,  {"GBpc-EUC", NULL}},
  {TT_MAC, TT_MAC_KOREAN,   {"KSCpc-EUC", NULL}},
  {0, 0, {NULL}}
};

static CMap *
find_tocode_cmap (const char *reg, const char *ord, int select)
{
  int   cmap_id = -1, i;
  char *cmap_name;
  char *append;

  if (!reg || !ord ||
      select < 0 || select > KNOWN_ENCODINGS_MAX)
    ERROR("Character set unknown.");

  if (!strcmp(ord, "UCS") &&
      select <= WIN_UCS_INDEX_MAX)
    return NULL;

  for (i = 0; cmap_id < 0 && i < 5; i++) {
    append = (char *) known_encodings[select].pdfnames[i];
    if (!append)
      break;
    cmap_name = NEW(strlen(reg) + strlen(ord) + strlen(append) + 3, char);
    sprintf(cmap_name, "%s-%s-%s", reg, ord, append);
    cmap_id = CMap_cache_find(cmap_name);
    RELEASE(cmap_name);
  }
  if (cmap_id < 0) {
    WARN("Could not find CID-to-Code mapping for \"%s-%s\".", reg, ord);
    WARN("I tried to load (one of) the following file(s):");
    for (i = 0; i < 5; i++) {
      append = (char *) known_encodings[select].pdfnames[i];
      if (!append)
	break;
      MESG(" %s-%s-%s", reg, ord, append);
    }
    WARN("Please check if this file exists.");
    ERROR("Cannot continue...");
  }

  return CMap_cache_get(cmap_id);
}


/*
 * CIDFont glyph metrics:
 * Mostly same as add_CID[HV]Metrics in cidtype0.c.
 */
#define PDFUNIT(v) ((double) (ROUND(1000.0*(v)/(g->emsize), 1)))

static void
add_TTCIDHMetrics (pdf_obj *fontdict, struct tt_glyphs *g,
		   char *used_chars, unsigned char *cidtogidmap, unsigned short last_cid)
{
  long cid, start = 0, prev = 0;
  pdf_obj *w_array, *an_array = NULL;
  double   dw;
  int      empty = 1;

  w_array = pdf_new_array();
  if (g->dw != 0 && g->dw <= g->emsize) {
    dw = PDFUNIT(g->dw);
  } else {
    dw = PDFUNIT(g->gd[0].advw);
  }
  for (cid = 0; cid <= last_cid; cid++) {
    USHORT idx, gid;
    double width;

    if (!is_used_char2(used_chars, cid))
      continue;
    gid = (cidtogidmap) ? ((cidtogidmap[2*cid] << 8)|cidtogidmap[2*cid+1]) : cid;
    idx = tt_get_index(g, gid);
    if (cid != 0 && idx == 0)
      continue;
    width = PDFUNIT((g->gd)[idx].advw);
    if (width == dw) {
      if (an_array) {
	pdf_add_array(w_array, pdf_new_number(start));
	pdf_add_array(w_array, an_array);
	an_array = NULL;
	empty = 0;
      }
    } else {
      if (cid != prev + 1) {
	if (an_array) {
	  pdf_add_array(w_array, pdf_new_number(start));
	  pdf_add_array(w_array, an_array);
	  an_array = NULL;
	  empty = 0;
	}
      }
      if (an_array == NULL) {
	an_array = pdf_new_array();
	start = cid;
      }
      pdf_add_array(an_array, pdf_new_number(width));
      prev = cid;
    }
  }

  if (an_array) {
    pdf_add_array(w_array, pdf_new_number(start));
    pdf_add_array(w_array, an_array);
    empty = 0;
  }

  pdf_add_dict(fontdict,
	       pdf_new_name("DW"),
	       pdf_new_number(dw));
  if (!empty) {
    pdf_add_dict(fontdict,
		 pdf_new_name("W"),
		 pdf_ref_obj(w_array));
  }
  pdf_release_obj(w_array);

  return;
}

static void
add_TTCIDVMetrics (pdf_obj *fontdict, struct tt_glyphs *g,
		   char *used_chars, unsigned char *cidtogidmap, unsigned short last_cid)
{
  pdf_obj *w2_array, *an_array = NULL;
  long cid, prev, start;
  double defaultVertOriginY, defaultAdvanceHeight;
  int    empty = 1;

  defaultVertOriginY   = PDFUNIT(g->default_advh - g->default_tsb);
  defaultAdvanceHeight = PDFUNIT(g->default_advh);

  w2_array = pdf_new_array();
  start = prev = 0;
  for (cid = 0; cid <= last_cid; cid++) {
    USHORT idx, gid;
    double vertOriginX, vertOriginY, advanceHeight;

    if (!is_used_char2(used_chars, cid))
      continue;
    gid = (cidtogidmap) ? ((cidtogidmap[2*cid] << 8)|cidtogidmap[2*cid+1]) : cid;
    idx = tt_get_index(g, cid);
    if (cid != 0 && idx == 0)
      continue;
    advanceHeight = PDFUNIT(g->gd[idx].advh);
    vertOriginX   = PDFUNIT(0.5*(g->gd[idx].advw));
    vertOriginY   = PDFUNIT(g->gd[idx].tsb + g->gd[idx].ury);
#if 0
    /*
     * c [w1_1y v_1x v_1y w1_2y v_2x v_2y ...]
     * Not working... Why?
     * Acrobat Reader:
     *  Wrong rendering, interpretation of position vector is wrong.
     * Xpdf and gs: ignores W2?
     */
    if (vertOriginY == defaultVertOriginY &&
	advanceHeight == defaultAdvanceHeight) {
      if (an_array) {
	pdf_add_array(w2_array, pdf_new_number(start));
	pdf_add_array(w2_array, an_array);
	an_array = NULL;
	empty = 0;
      }
    } else {
      if (cid != prev + 1 && an_array) {
	pdf_add_array(w2_array, pdf_new_number(start));
	pdf_add_array(w2_array, an_array);
	an_array = NULL;
	empty = 0;
      }
      if (an_array == NULL) {
	an_array = pdf_new_array();
	start = cid;
      }
      pdf_add_array(an_array, pdf_new_number(-advanceHeight));
      pdf_add_array(an_array, pdf_new_number(vertOriginX));
      pdf_add_array(an_array, pdf_new_number(vertOriginY));
      prev = cid;
    }
#else
    /*
     * c_first c_last w1_y v_x v_y
     * This form may hit Acrobat's implementation limit of array element size,
     * 8192. AFPL GhostScript 8.11 stops with rangecheck error with this.
     * Maybe GS's bug?
     */
    if (vertOriginY != defaultVertOriginY ||
	advanceHeight != defaultAdvanceHeight) {
      pdf_add_array(w2_array, pdf_new_number(cid));
      pdf_add_array(w2_array, pdf_new_number(cid));
      pdf_add_array(w2_array, pdf_new_number(-advanceHeight));
      pdf_add_array(w2_array, pdf_new_number(vertOriginX));
      pdf_add_array(w2_array, pdf_new_number(vertOriginY));
      empty = 0;
    }
#endif
  }

#if 0
  if (an_array) {
    pdf_add_array(w2_array, pdf_new_number(start));
    pdf_add_array(w2_array, an_array);
    empty = 0;
  }
#endif

  if (defaultVertOriginY != 880 || defaultAdvanceHeight != 1000) {
    an_array = pdf_new_array();
    pdf_add_array(an_array, pdf_new_number(defaultVertOriginY));
    pdf_add_array(an_array, pdf_new_number(-defaultAdvanceHeight));
    pdf_add_dict(fontdict, pdf_new_name ("DW2"), an_array);
  }
  if (!empty) {
    pdf_add_dict(fontdict,
		 pdf_new_name("W2"),
		 pdf_ref_obj(w2_array));
  }
  pdf_release_obj(w2_array);

  return;
}

/*
 * The following routine fixes few problems caused by vendor specific
 * Unicode mappings.
 */

#define FIX_CJK_UNIOCDE_SYMBOLS 1

static unsigned short
fix_CJK_symbols (unsigned short code)
{
  unsigned short alt_code;
  static struct
  {
    unsigned short alt1;
    unsigned short alt2;
  } CJK_Uni_symbols[] = {
    /*
     * Microsoft/Apple Unicode mapping difference:
     * They are taken from SJIS-Unicode mapping difference but nearly
     * same thing might be applied to Chinese (e.g., Big5) too.
     */
    {0x2014, 0x2015}, /* EM DASH <-> HORIZONTAL BAR */
    {0x2016, 0x2225}, /* DOUBLE VERTICAL LINE <-> PARALLEL TO */
    {0x203E, 0xFFE3}, /* OVERLINE <-> FULLWIDTH MACRON */
    {0x2026, 0x22EF}, /* HORIZONTAL ELLIPSIS <-> MIDLINE HORIZONTAL ELLIPSIS */
    {0x2212, 0xFF0D}, /* MINUS SIGN <-> FULLWIDTH HYPHEN-MINUS */
    {0x301C, 0xFF5E}, /* WAVE DASH <-> FULLWIDTH TILDE */
    {0xFFE0, 0x00A2}, /* FULLWIDTH CENT SIGN <-> CENT SIGN */
    {0xFFE1, 0x00A3}, /* FULLWIDTH POUND SIGN <-> POUND SIGN */
    {0xFFE2, 0x00AC}, /* FULLWIDTH NOT SIGN <-> NOT SIGN */
    {0xFFFF, 0xFFFF}, /* EOD */
  };
#define NUM_CJK_SYMBOLS (sizeof(CJK_Uni_symbols)/sizeof(CJK_Uni_symbols[0]))
  int i;

  alt_code = code;
  for (i = 0; i < NUM_CJK_SYMBOLS; i++) {
    if (CJK_Uni_symbols[i].alt1 == code) {
      alt_code = CJK_Uni_symbols[i].alt2;
      break;
    } else if (CJK_Uni_symbols[i].alt2 == code) {
      alt_code = CJK_Uni_symbols[i].alt1;
      break;
    }
  }

  return alt_code;
}

static long
cid_to_code (CMap *cmap, CID cid)
{
  unsigned char  inbuf[2], outbuf[32];
  long           inbytesleft = 2, outbytesleft = 32;
  unsigned char *p, *q;

  if (!cmap)
    return cid;

  inbuf[0] = (cid >> 8) & 0xff;
  inbuf[1] = cid & 0xff;
  p = inbuf; q = outbuf;

  CMap_decode_char(cmap, (const unsigned char **) &p, &inbytesleft, &q, &outbytesleft);

  if (inbytesleft != 0)
    return 0;
  else if (outbytesleft == 31)
    return (long) outbuf[0];
  else if (outbytesleft == 30)
    return (long) (outbuf[0] << 8|outbuf[1]);
  else if (outbytesleft == 28)
    return (long) (outbuf[0] << 24|outbuf[1] << 16|outbuf[2] << 8|outbuf[3]);

  return 0;
}

/* #define NO_GHOSTSCRIPT_BUG 1 */

void
CIDFont_type2_dofont (CIDFont *font)
{
  pdf_obj *fontfile;
  sfnt    *sfont;
  char    *h_used_chars, *v_used_chars, *used_chars;
  struct tt_glyphs *glyphs;
  CMap    *cmap = NULL;
  tt_cmap *ttcmap = NULL;
  unsigned long offset = 0;
  CID      cid, last_cid;
  unsigned char *cidtogidmap;
  USHORT   num_glyphs;
  int      i, glyph_ordering = 0, unicode_cmap = 0;
  FILE    *fp = NULL;

  if (!font->indirect)
    return;

  pdf_add_dict(font->fontdict, 
	       pdf_new_name("FontDescriptor"), pdf_ref_obj(font->descriptor));

  if (CIDFont_is_BaseFont(font))
    return;

  /*
   * CIDSystemInfo comes here since Supplement can be increased.
   */
  {
    pdf_obj *tmp;

    tmp = pdf_new_dict ();
    pdf_add_dict(tmp,
		 pdf_new_name("Registry"),
		 pdf_new_string(font->csi->registry, strlen(font->csi->registry)));
    pdf_add_dict(tmp,
		 pdf_new_name("Ordering"),
		 pdf_new_string(font->csi->ordering, strlen(font->csi->ordering)));
    pdf_add_dict(tmp,
		 pdf_new_name("Supplement"),
		 pdf_new_number(font->csi->supplement));
    pdf_add_dict(font->fontdict, pdf_new_name("CIDSystemInfo"), tmp);
  }

  /* Quick exit for non-embedded & fixed-pitch font. */
  if (!CIDFont_get_embedding(font) &&
      (opt_flags & CIDFONT_FORCE_FIXEDPITCH)) {
    pdf_add_dict(font->fontdict,
		 pdf_new_name("DW"), pdf_new_number(1000.0));
    return;
  }

#ifdef XETEX
  sfont = sfnt_open(font->ft_face, SFNT_TYPE_TTC | SFNT_TYPE_TRUETYPE);
#else
  fp = DPXFOPEN(font->ident, DPX_RES_TYPE_TTFONT);
  if (!fp)
    ERROR("Could not open TTF file: %s", font->ident);

  sfont = sfnt_open(fp);
#endif
  if (!sfont) {
    ERROR("Could not open TTF file: %s", font->ident);
  }

  switch (sfont->type) {
  case SFNT_TYPE_TTC:
    offset = ttc_read_offset(sfont, font->options->index);
    if (offset == 0)
      ERROR("Invalid TTC index in %s.", font->ident);
    break;
  case SFNT_TYPE_TRUETYPE:
    if (font->options->index > 0)
      ERROR("Found TrueType font file while expecting TTC file (%s).", font->ident);
    offset = 0;
    break;
  default:
    ERROR("Not a TrueType/TTC font (%s)?", font->ident);
    break;
  }

  if (sfnt_read_table_directory(sfont, offset) < 0)
    ERROR("Could not read TrueType table directory (%s).", font->ident);

  /*
   * Adobe-Identity means font's internal glyph ordering here.
   */
  if (!strcmp(font->csi->registry, "Adobe") &&
      !strcmp(font->csi->ordering, "Identity")) {
    glyph_ordering = 1;
  } else {
    glyph_ordering = 0;
  }

  /*
   * Select TrueType cmap table, find ToCode CMap for each TrueType encodings.
   */
  if (glyph_ordering) {
    ttcmap = NULL;
    cmap   = NULL;
  } else {
    /*
     * This part contains a bug. It may choose SJIS encoding TrueType cmap
     * table for Adobe-GB1.
     */
    for (i = 0; i <= KNOWN_ENCODINGS_MAX; i++) {
      ttcmap = tt_cmap_read(sfont,
			    known_encodings[i].platform,
			    known_encodings[i].encoding);
      if (ttcmap)
	break;
    }
    if (!ttcmap) {
      WARN("No usable TrueType cmap table found for font \"%s\".", font->ident);
      WARN("CID character collection for this font is set to \"%s-%s\"",
	   font->csi->registry, font->csi->ordering);
      ERROR("Cannot continue without this...");
    } else if (i <= WIN_UCS_INDEX_MAX) {
      unicode_cmap = 1;
    } else {
      unicode_cmap = 0;
    }

    /*
     * NULL is returned if CMap is Identity CMap.
     */
    cmap = find_tocode_cmap(font->csi->registry,
			    font->csi->ordering, i);
  }

  glyphs = tt_build_init();

  last_cid   = 0;
  num_glyphs = 1; /* .notdef */
  used_chars = h_used_chars = v_used_chars = NULL;
  {
    Type0Font *parent;
    int        parent_id, c;

    if ((parent_id = CIDFont_get_parent_id(font, 0)) >= 0) {
      parent = Type0Font_cache_get(parent_id);
      h_used_chars = Type0Font_get_usedchars(parent);
    }
    if ((parent_id = CIDFont_get_parent_id(font, 1)) >= 0) {
      parent = Type0Font_cache_get(parent_id);
      v_used_chars = Type0Font_get_usedchars(parent);
    }
    if (!h_used_chars && !v_used_chars)
      ERROR("Unexpected error.");

    /*
     * Quick check of max CID.
     */
    c = 0;
    for (i = 8191; i >= 0; i--) {
      if (h_used_chars && h_used_chars[i] != 0) {
	last_cid = i * 8 + 7;
	c = h_used_chars[i];
	break;
      }
      if (v_used_chars && v_used_chars[i] != 0) {
	last_cid = i * 8 + 7;
	c = v_used_chars[i];
	break;
      }
    }
    if (last_cid > 0) {
      for (i = 0; i < 8; i++) {
	if ((c >> i) & 1)
	  break;
	last_cid--;
      }
    }
    if (last_cid >= 0xFFFFu) {
      ERROR("CID count > 65535");
    }
  }

#ifndef NO_GHOSTSCRIPT_BUG
  cidtogidmap = NULL;
#else
  cidtogidmap = NEW((last_cid + 1) * 2, unsigned char);
  memset(cidtogidmap, 0, (last_cid + 1) * 2);
#endif /* !NO_GHOSTSCRIPT_BUG */

  /*
   * Map CIDs to GIDs.
   * Horizontal and vertical used_chars are merged.
   */

  /*
   * Horizontal
   */
  if (h_used_chars) {
    used_chars = h_used_chars;
    for (cid = 1; cid <= last_cid; cid++) {
      long           code;
      unsigned short gid;

      if (!is_used_char2(h_used_chars, cid))
	continue;

      if (glyph_ordering) {
	gid  = cid;
	code = cid;
      } else {
	code = cid_to_code(cmap, cid);
	gid  = tt_cmap_lookup(ttcmap, code);
#ifdef FIX_CJK_UNIOCDE_SYMBOLS
	if (gid == 0 && unicode_cmap) {
	  long alt_code;

	  alt_code = fix_CJK_symbols(code);
	  if (alt_code != code) {
	    gid = tt_cmap_lookup(ttcmap, alt_code);
	    if (gid != 0) {
	      WARN("Unicode char U+%04x replaced with U+%04x.",
		   code, alt_code);
	    }
	  }
	}
#endif /* FIX_CJK_UNIOCDE_SYMBOLS */
      }

      if (gid == 0) {
	WARN("Glyph missing in font. (CID=%u, code=0x%04x)", cid, code);
      }

      /* TODO: duplicated glyph */
#ifndef NO_GHOSTSCRIPT_BUG
      gid = tt_add_glyph(glyphs, gid, cid);
#else
      gid = tt_add_glyph(glyphs, gid, num_glyphs);
      cidtogidmap[2*cid  ] = gid >> 8;
      cidtogidmap[2*cid+1] = gid & 0xff;
#endif /* !NO_GHOSTSCRIPT_BUG */

      num_glyphs++;
    }
  }

  /*
   * Vertical
   */
  if (v_used_chars) {
    otl_gsub *gsub_list = NULL;

    /*
     * Require `vrt2' or `vert'.
     */
    if (glyph_ordering) {
      gsub_list = NULL;
    } else {
      gsub_list = otl_gsub_new();
      if (otl_gsub_add_feat(gsub_list,
			    "*", "*", "vrt2", sfont) < 0) {
	if (otl_gsub_add_feat(gsub_list,
			      "*", "*", "vert", sfont) < 0) {
	  WARN("GSUB feature vrt2/vert not found.");
	  otl_gsub_release(gsub_list);
	  gsub_list = NULL;
	} else {
	  otl_gsub_select(gsub_list, "*", "*", "vert");
	}
      } else {
	otl_gsub_select(gsub_list, "*", "*", "vrt2");
      }
    }

    for (cid = 1; cid <= last_cid; cid++) {
      long           code;
      unsigned short gid;

      if (!is_used_char2(v_used_chars, cid))
	continue;

      /* There may be conflict of horizontal and vertical glyphs
       * when font is used with /UCS. However, we simply ignore
       * that...
       */
      if (h_used_chars && is_used_char2(h_used_chars, cid)) {
	continue;
      }

      if (glyph_ordering) {
	gid  = cid;
	code = cid;
      } else {
	code = cid_to_code(cmap, cid);
	gid  = tt_cmap_lookup(ttcmap, code);
#ifdef FIX_CJK_UNIOCDE_SYMBOLS
	if (gid == 0 && unicode_cmap) {
	  long alt_code;

	  alt_code = fix_CJK_symbols(code);
	  if (alt_code != code) {
	    gid = tt_cmap_lookup(ttcmap, alt_code);
	    if (gid != 0) {
	      WARN("Unicode char U+%04x replaced with U+%04x.",
		   code, alt_code);
	    }
	  }
	}
#endif /* FIX_CJK_UNIOCDE_SYMBOLS */
      }
      if (gid == 0) {
	WARN("Glyph missing in font. (CID=%u, code=0x%04x)", cid, code);
      } else if (gsub_list) {
	otl_gsub_apply(gsub_list, &gid);
      }

#ifndef NO_GHOSTSCRIPT_BUG
      gid = tt_add_glyph(glyphs, gid, cid);
#else
      gid = tt_add_glyph(glyphs, gid, num_glyphs);
      cidtogidmap[2*cid  ] = gid >> 8;
      cidtogidmap[2*cid+1] = gid & 0xff;
#endif /* !NO_GHOSTSCRIPT_BUG */

      if (used_chars) /* merge vertical used_chars to horizontal */
	add_to_used_chars2(used_chars, cid);

      num_glyphs++;
    }

    if (gsub_list)
      otl_gsub_release(gsub_list);

    if (!used_chars) /* We have no horizontal. */
      used_chars = v_used_chars;
  }

  if (!used_chars)
    ERROR("Unexpected error.");

  tt_cmap_release(ttcmap);

  if (CIDFont_get_embedding(font)) {
    if (tt_build_tables(sfont, glyphs) < 0)
      ERROR("Could not created FontFile stream.");
    if (verbose > 1)
      MESG("[%u glyphs (Max CID: %u)]", glyphs->num_glyphs, last_cid);
  } else {
    if (tt_get_metrics(sfont, glyphs) < 0)
      ERROR("Reading glyph metrics failed...");
  }

  /*
   * DW, W, DW2, and W2
   */
  if (opt_flags & CIDFONT_FORCE_FIXEDPITCH) {
    pdf_add_dict(font->fontdict,
		 pdf_new_name("DW"), pdf_new_number(1000.0));
  } else {
    add_TTCIDHMetrics(font->fontdict, glyphs, used_chars, cidtogidmap, last_cid);
    if (v_used_chars)
      add_TTCIDVMetrics(font->fontdict, glyphs, used_chars, cidtogidmap, last_cid);
  }

  tt_build_finish(glyphs);

  /* Finish here if not embedded. */
  if (!CIDFont_get_embedding(font)) {
    if (cidtogidmap)
      RELEASE(cidtogidmap);
    sfnt_close(sfont);
    if (fp)
      DPXFCLOSE(fp);

    return;
  }

  /* Create font file */
  for (i = 0; required_table[i].name; i++) {
    if (sfnt_require_table(sfont,
			   required_table[i].name,
			   required_table[i].must_exist) < 0) {
      ERROR("Some required TrueType table does not exist.");
    }
  }

  /*
   * FontFile2
   */
  fontfile = sfnt_create_FontFile_stream(sfont);

  sfnt_close(sfont);
  if (fp)
    DPXFCLOSE(fp);

  if (!fontfile)
    ERROR("Could not created FontFile stream for \"%s\".", font->ident);

  if (verbose > 1) {
    MESG("[%ld bytes]", pdf_stream_length(fontfile));
  }

  pdf_add_dict(font->descriptor,
	       pdf_new_name("FontFile2"),
	       pdf_ref_obj (fontfile));
  pdf_release_obj(fontfile);

  /*
   * CIDSet
   */
  {
    pdf_obj *cidset;

    cidset = pdf_new_stream(STREAM_COMPRESS);
    pdf_add_stream(cidset, used_chars, last_cid/8 + 1);
    pdf_add_dict(font->descriptor,
		 pdf_new_name("CIDSet"),
		 pdf_ref_obj(cidset));
    pdf_release_obj(cidset);
  }

  /*
   * CIDToGIDMap
   */
  if (cidtogidmap) {
    pdf_obj *c2gmstream;

    c2gmstream = pdf_new_stream(STREAM_COMPRESS);
    pdf_add_stream(c2gmstream, cidtogidmap, (last_cid + 1) * 2);
    pdf_add_dict(font->fontdict,
                 pdf_new_name("CIDToGIDMap"),
                 pdf_ref_obj (c2gmstream));
    pdf_release_obj(c2gmstream);
    RELEASE(cidtogidmap);
  }
  
  return;
}

int
CIDFont_type2_open (CIDFont *font, const char *name,
		    CIDSysInfo *cmap_csi, cid_opt *opt)
{
  char    *fontname;
  sfnt    *sfont;
  unsigned long offset = 0;
  FILE    *fp = NULL;

  ASSERT(font && opt);

#ifdef XETEX
  sfont = sfnt_open(font->ft_face, SFNT_TYPE_TTC | SFNT_TYPE_TRUETYPE);
  if (!sfont)
    return -1;
#else
  fp = DPXFOPEN(name, DPX_RES_TYPE_TTFONT);
  if (!fp)
    return -1;

  sfont = sfnt_open(fp);
  if (!sfont) {
    DPXFCLOSE(fp);
    return -1;
  }
#endif

  switch (sfont->type) {
  case SFNT_TYPE_TTC:
    offset = ttc_read_offset(sfont, opt->index);
    break;
  case SFNT_TYPE_TRUETYPE:
    if (opt->index > 0) {
      ERROR("Invalid TTC index (not TTC font): %s", name);
    } else {
      offset = 0;
    }
    break;
  default:
    sfnt_close(sfont);
    if (fp)
      DPXFCLOSE(fp);
    return -1;
    break;
  }

  if (sfnt_read_table_directory(sfont, offset) < 0) {
    ERROR("Reading TrueType table directory failed.");
  }

  {
    char *shortname;
    long  namelen;

    /* MAC-ROMAN-EN-POSTSCRIPT or WIN-UNICODE-EN(US)-POSTSCRIPT */
    shortname = NEW(PDF_NAME_LEN_MAX, char);
    namelen   = tt_get_ps_fontname(sfont, shortname, PDF_NAME_LEN_MAX);
    if (namelen == 0) {
      memset(shortname, 0, PDF_NAME_LEN_MAX);
      strncpy(shortname, name, PDF_NAME_LEN_MAX);
      namelen = strlen(shortname);
    }
    validate_name(shortname, namelen); /* for SJIS, UTF-16, ... string */
    /*
     * Strlen works, after validate_named string.
     * Mangled name requires more 7 bytes.
     * Style requires more 11 bytes.
     */
    fontname = NEW(strlen(shortname)+19, char);
    strcpy(fontname, shortname);
    RELEASE(shortname);
  }

  if (opt->embed && opt->style != FONT_STYLE_NONE) {
    WARN("Embedding disabled due to style option for %s.", name);
    opt->embed = 0;
  }
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
  /*
   * CIDSystemInfo is determined from CMap or from map record option.
   */
  font->fontname = fontname;
  font->subtype  = CIDFONT_TYPE2;
  font->csi      = NEW(1, CIDSysInfo);
  if (opt->csi) {
    if (cmap_csi) {
      if (strcmp(opt->csi->registry, cmap_csi->registry) ||
	  strcmp(opt->csi->ordering, cmap_csi->ordering)) {
	WARN("CID character collection mismatched:\n");
	MESG("\tFont: %s-%s-%d\n",
	     opt->csi->registry, opt->csi->ordering, opt->csi->supplement);
	MESG("\tCMap: %s-%s-%d\n",
	     cmap_csi->registry, cmap_csi->ordering, cmap_csi->supplement);
	ERROR("Incompatible CMap specified for this font.");
      }
      if (opt->csi->supplement < cmap_csi->supplement) {
	WARN("Supplmement value in CIDSystemInfo increased.");
	WARN("Some characters may not shown.");
	opt->csi->supplement = cmap_csi->supplement;
      }
    }
    font->csi->registry   = NEW(strlen(opt->csi->registry)+1, char);
    strcpy(font->csi->registry, opt->csi->registry);
    font->csi->ordering   = NEW(strlen(opt->csi->ordering)+1, char);
    strcpy(font->csi->ordering, opt->csi->ordering);
    font->csi->supplement = opt->csi->supplement;
  } else if (cmap_csi) {
    font->csi->registry   = NEW(strlen(cmap_csi->registry)+1, char);
    strcpy(font->csi->registry, cmap_csi->registry);
    font->csi->ordering   = NEW(strlen(cmap_csi->ordering)+1, char);
    strcpy(font->csi->ordering, cmap_csi->ordering);
    font->csi->supplement = cmap_csi->supplement;
  } else { /* This means font's internal glyph ordering. */
    font->csi->registry   = NEW(strlen("Adobe")+1, char);
    strcpy(font->csi->registry, "Adobe");
    font->csi->ordering   = NEW(strlen("Identity")+1, char);
    strcpy(font->csi->ordering, "Identity");
    font->csi->supplement = 0;
  }

  font->fontdict = pdf_new_dict();
  pdf_add_dict(font->fontdict,
	       pdf_new_name("Type"),
	       pdf_new_name("Font"));
  pdf_add_dict(font->fontdict,
	       pdf_new_name("Subtype"),
	       pdf_new_name("CIDFontType2"));

  font->descriptor = tt_get_fontdesc(sfont, &(opt->embed), 0, name);
  if (!font->descriptor) {
    ERROR("Could not obtain necessary font info.");
  }

  if (opt->embed) {
    memmove(fontname + 7, fontname, strlen(fontname) + 1);
    pdf_font_make_uniqueTag(fontname);
    fontname[6] = '+';
  }

  pdf_add_dict(font->descriptor,
	       pdf_new_name("FontName"),
	       pdf_new_name(fontname));
  pdf_add_dict(font->fontdict, 
	       pdf_new_name("BaseFont"),
	       pdf_new_name(fontname));

  sfnt_close(sfont);
  if (fp)
    DPXFCLOSE(fp);

  /*
   * Don't write fontdict here.
   * /Supplement in /CIDSystemInfo may change.
   */

  return 0;
}

void
CIDFont_type2_release (CIDFont *font)
{
  return;
}
