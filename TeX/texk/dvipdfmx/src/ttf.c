/*  $Header: /home/cvsroot/dvipdfmx/src/ttf.c,v 1.17 2004/02/15 12:59:43 hirata Exp $
    
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
 * OpenType GSUB support is not available yet.
 *
 *  Required features: onum (oldstyle digits), c2sc/smcp, and more.
 *
 */

#include "system.h"
#include <ctype.h>

#include "mem.h"
#include "error.h"
#include "mfileio.h"

#include "numbers.h"

#include "pdfobj.h"
#include "pdfresource.h"
#include "dpxutil.h"

#include "encodings.h"
#include "unicode.h"
#include "agl.h"

/* TrueType */
#include "sfnt.h"
#include "tt_cmap.h"
#include "tt_table.h"
#include "tt_build.h"
#include "tt_aux.h"

#include "ttf.h"

static int __verbose = 0;

#define TTFONT_DEBUG_STR "TTFont"
#define TTFONT_DEBUG     3

void
TTFont_set_verbose (void)
{
  __verbose++;
}

struct TTFont 
{
  char    *ident;
  char    *fontname;
  char    *filename;
  int      embed;
  int      encoding_id;
  char    *used_chars;
  /* PDF Objects */
  pdf_obj *indirect;
  pdf_obj *fontdict;
  pdf_obj *descriptor;
};

TTFont *
TTFont_new (void)
{
  TTFont *font;

  font = NEW(1, struct TTFont);
  font->ident    = NULL;
  font->fontname = NULL;
  font->filename = NULL;
  font->embed    = 1;
  font->encoding_id = -1;
  font->used_chars  = NULL;

  font->indirect   = NULL;
  font->fontdict   = NULL;
  font->descriptor = NULL;

  return font;
}

void
TTFont_flush (TTFont *font)
{
  if (!font)
    return;

  if (font->indirect)   pdf_release_obj(font->indirect);
  if (font->fontdict)   pdf_release_obj(font->fontdict);
  if (font->descriptor) pdf_release_obj(font->descriptor);

  font->indirect   = NULL;
  font->fontdict   = NULL;
  font->descriptor = NULL;

  return;
}

void
TTFont_release (TTFont *font)
{
  if (!font)
    return;

  if (font->ident)      RELEASE(font->ident);
  if (font->fontname)   RELEASE(font->fontname);
  if (font->filename)   RELEASE(font->filename);
  if (font->used_chars) RELEASE(font->used_chars);

  if (font->indirect)   ERROR("%s: Object not flushed.", TTFONT_DEBUG_STR);
  if (font->fontdict)   ERROR("%s: Object not flushed.", TTFONT_DEBUG_STR);
  if (font->descriptor) ERROR("%s: Object not flushed.", TTFONT_DEBUG_STR);

  return;
}

static char *
new_used_chars (void)
{
  char *used_chars;;

  used_chars = NEW(256, char);
  memset(used_chars, 0, 256*sizeof(char));

  return used_chars;
}

pdf_obj *
TTFont_get_resource (TTFont *font)
{
  ASSERT(font);

  if (!font->indirect)
    font->indirect = pdf_ref_obj(font->fontdict);

  return pdf_link_obj(font->indirect);
}

char *
TTFont_get_usedchars (TTFont *font)
{
  ASSERT(font);

  return font->used_chars;
}

/*
 * PDF_NAME_LEN_MAX: see, Appendix C of PDF Ref. v1.3, 2nd. ed.
 * This is Acrobat implementation limit.
 */
#define PDF_NAME_LEN_MAX 127

static void
validate_name (char **name, int len)
{
  char *tmp;
  int i, pos;

  if (len > PDF_NAME_LEN_MAX)
    ERROR("Name string length too large");

  /* eg, 0x5b -> #5b */
  tmp = NEW(3*len + 1, char);
  pos = 0;
  for (i = 0; i < len; i++) {
    if (*(*name+i) == 0)
      continue;
    else if (*(*name+i) < '!' || *(*name+i) > '~' ||
	     /*         ^ `space' is here */
	     strchr("/()[]<>{}#", *(*name+i)) != NULL) {
      sprintf(tmp+pos, "#%02x", (unsigned char) *(*name+i));
      pos += 3;
    } else {
      *(tmp+pos) = *(*name+i);
      pos++;
    }
  }

  if (pos != len)
    *name = RENEW(*name, pos+1, char);
  memmove(*name, tmp, pos);
  *(*name+pos) = '\0';

  RELEASE(tmp);

  if (strlen(*name) == 0)
    ERROR("No valid character found in name string.");

  return;
}

int
TTFont_open (TTFont *font, const char *name, int encoding_id, int embed)
{
  sfnt   *sfont;
  char   *fullname, *fontname;
  int     namelen;

  ASSERT(font);

  fullname = kpse_find_file(name, kpse_truetype_format, 1);
  if (!fullname)
    return -1;
  if ((sfont = sfnt_open(fullname)) == NULL ||
      sfont->type != SFNT_TYPE_TRUETYPE     ||
      sfnt_read_table_directory(sfont, 0) < 0)
    return -1;

  font->ident       = strdup(name);
  font->filename    = strdup(fullname);
  font->encoding_id = encoding_id;
  font->embed       = embed;

  font->fontdict    = pdf_new_dict();
  font->descriptor  = tt_get_fontdesc(sfont, &(font->embed), 1);
  if (!font->descriptor)
    ERROR("Could not obtain neccesary font info.");

  if (!font->embed && font->encoding_id >= 0)
    ERROR("Custum encoding not allowed for non-embedded TrueType font.");

  fontname = NEW(PDF_NAME_LEN_MAX, char);
  namelen  = tt_get_ps_fontname(sfont, fontname, PDF_NAME_LEN_MAX);
  if (namelen == 0) {
    strncpy(fontname, name, PDF_NAME_LEN_MAX);
    fontname[PDF_NAME_LEN_MAX - 1] = 0;
    namelen = strlen(fontname);
  }
  validate_name(&fontname, namelen);

  if (font->embed) {
    font->fontname = NEW(strlen(fontname)+8, char);
    strcpy(font->fontname, fontname);
    mangle_name(font->fontname);
  } else
    font->fontname = strdup(fontname);
  RELEASE(fontname);

  sfnt_close(sfont);

  pdf_add_dict(font->fontdict,
	       pdf_new_name("Type"),
	       pdf_new_name("Font"));
  pdf_add_dict(font->fontdict,
	       pdf_new_name("Subtype"),
	       pdf_new_name("TrueType"));
  pdf_add_dict(font->fontdict,
	       pdf_new_name("BaseFont"),
	       pdf_new_name(font->fontname));
  /*
   * We use MacRoman as "default" encoding.
   */
  if (encoding_id < 0)
    pdf_add_dict(font->fontdict,
		 pdf_new_name("Encoding"),
		 pdf_new_name("MacRomanEncoding"));
  else {
    Encoding *encoding = Encoding_cache_get(encoding_id);
    if (Encoding_is_predefined(encoding))
      pdf_add_dict(font->fontdict,
		   pdf_new_name("Encoding"),
		   pdf_new_name(Encoding_get_name(encoding)));
  }
  pdf_add_dict(font->descriptor,
	       pdf_new_name("FontName"),
	       pdf_new_name(font->fontname));
  if (font->embed) {
    font->used_chars = new_used_chars();
  }

  return 0;
}

static struct
{
  const char *name;
  int   must_exist;
} required_table[] = {
  {"OS/2", 1}, {"head", 1}, {"hhea", 1}, {"loca", 1}, {"maxp", 1},
  {"glyf", 1}, {"hmtx", 1}, {"fpgm", 0}, {"cvt ", 0}, {"prep", 0},
  {"cmap", 1}, {NULL, 0}
};

static void
do_widths (TTFont *font, double *widths)
{
  int code, firstchar, lastchar;

  firstchar = 255; lastchar = 0;
  for (code = 0; code < 256; code++) {
    if (font->used_chars[code]) {
      if (code < firstchar) firstchar = code;
      if (code > lastchar)  lastchar  = code;
    }
  }
  {
    pdf_obj *tmp;

    tmp = pdf_new_array();
    for (code = firstchar; code <= lastchar; code++) {
      if (font->used_chars[code])
	pdf_add_array(tmp, pdf_new_number(ROUND(widths[code], 1)));
      else
	pdf_add_array(tmp, pdf_new_number(0.0));
    }
    pdf_add_dict(font->fontdict, pdf_new_name("Widths"), pdf_link_obj(pdf_ref_obj(tmp)));
    pdf_release_obj(tmp);
  }
  pdf_add_dict(font->fontdict,
	       pdf_new_name("FirstChar"), pdf_new_number(firstchar));
  pdf_add_dict(font->fontdict,
	       pdf_new_name("LastChar"),  pdf_new_number(lastchar));
}


#define PDFUNIT(v) ((double) (ROUND(1000.0*(v)/(glyphs->emsize), 1)))

/*
 * Default encoding "Mac-Roman" is used.
 *
 * BUG:
 *  This uses cmap format 0 (byte encoding table).
 *  It does not work with encodings that uses full 256 range since
 *  GID = 0 is reserved for .notdef. GID = 256 is not accessible.
 */
static void
do_builtin_encoding (TTFont *font, sfnt *sfont)
{
  struct tt_glyphs *glyphs;
  unsigned char  *cmap_table;
  tt_cmap *tt_cmap;
  int      code, count;

  tt_cmap = tt_cmap_read(sfont, TT_MAC, TT_MAC_ROMAN);
  if (!tt_cmap)
    ERROR("Cannot read Mac-Roman TrueType cmap table");

  cmap_table = NEW(274, unsigned char);
  memset(cmap_table, 0, 274);
  sfnt_put_ushort(cmap_table,    0);            /* Version  */
  sfnt_put_ushort(cmap_table+2,  1);            /* Number of subtables */
  sfnt_put_ushort(cmap_table+4,  TT_MAC);       /* Platform ID */
  sfnt_put_ushort(cmap_table+6,  TT_MAC_ROMAN); /* Encoding ID */
  sfnt_put_ulong (cmap_table+8,  12);           /* Offset   */
  sfnt_put_ushort(cmap_table+12, 0);            /* Format   */
  sfnt_put_ushort(cmap_table+14, 262);          /* Length   */
  sfnt_put_ushort(cmap_table+16, 0);            /* Language */

  glyphs = tt_build_init();

  if (__verbose > 2)
    MESG("[glyphs:/.notdef");

  count = 1; /* .notdef */
  for (code = 0; code < 256; code++) {
    unsigned short new_gid, gid = 0;
    if (!font->used_chars[code])
      continue;
    if (__verbose > 2) MESG("/.c0x%02x", code);
    gid = tt_cmap_lookup(tt_cmap, (unsigned short) code);
    if (gid == 0) {
      WARN("Character 0x%02x missing in font \"%s\".", code, font->ident);
      new_gid = 0;
    } else {
      if ((new_gid = tt_find_glyph(glyphs, gid)) == 0) {
	new_gid = tt_add_glyph(glyphs, gid, count); /* count returned. */
      }
    }
    cmap_table[18+code] = new_gid;
    count++;
  }
  tt_cmap_release(tt_cmap);

  if (__verbose > 2)
    MESG("]");

  if (tt_build_tables(sfont, glyphs) < 0)
    ERROR("Could not created FontFile stream.");

  {
    double widths[256];

    for (code = 0; code < 256; code++) {
      if (font->used_chars[code]) {
	USHORT idx;
	idx = tt_get_index(glyphs, (USHORT) cmap_table[18+code]);
	widths[code] = PDFUNIT(glyphs->gd[idx].advw);
      } else {
	widths[code] = 0.0;
      }
    }
    do_widths(font, widths);
  }

  if (__verbose > 1) 
    MESG("[%d glyphs]", glyphs->num_glyphs);

  tt_build_finish(glyphs);

  sfnt_set_table(sfont, "cmap", cmap_table, 274);

}

static void
do_custum_encoding (TTFont *font, sfnt *sfont)
{
  struct tt_glyphs *glyphs;
  unsigned char   *cmap_table;
  struct tt_glyph_names *ttgn;
  Encoding *encoding;
  int       code, aglm_id, count;
  tt_cmap  *ttcm;
  AGLmap   *aglm;
  char    **enc_vec;

  encoding = Encoding_cache_get(font->encoding_id);
  enc_vec  = Encoding_get_encoding(encoding);
  ttgn     = tt_get_glyph_names(sfont);
  ttcm     = tt_cmap_read(sfont, TT_WIN, TT_WIN_UNICODE);
  aglm_id  = AGLmap_cache_find(AGLMAP_DEFAULT_GLYPHLIST);
  aglm     = (aglm_id >= 0) ? AGLmap_cache_get(aglm_id) : NULL;

  ASSERT(enc_vec);
  if (!ttgn && (!aglm || !ttcm))
    WARN("PostScript glyph name list nor Unicode cmap table not available.");

  cmap_table = NEW(274, unsigned char);
  memset(cmap_table, 0, 274);
  sfnt_put_ushort(cmap_table,    0);            /* Version  */
  sfnt_put_ushort(cmap_table+2,  1);            /* Number of subtables */
  sfnt_put_ushort(cmap_table+4,  TT_MAC);       /* Platform ID */
  sfnt_put_ushort(cmap_table+6,  TT_MAC_ROMAN); /* Encoding ID */
  sfnt_put_ulong (cmap_table+8,  12);           /* Offset   */
  sfnt_put_ushort(cmap_table+12, 0);            /* Format   */
  sfnt_put_ushort(cmap_table+14, 262);          /* Length   */
  sfnt_put_ushort(cmap_table+16, 0);            /* Language */

  glyphs = tt_build_init();

  if (__verbose > 2)
    MESG("[glyphs:/.notdef");

  count = 1; /* +1 for .notdef */
  for (code = 0; code < 256; code++) {
    USHORT new_gid, gid = 0;
    if (!font->used_chars[code])
      continue;
    if (!enc_vec[code] || !strcmp(enc_vec[code], ".notdef")) {
      WARN("%s: Character mapped to .notdef used. (char: 0x%02X, font: %s, encoding: %s)",
	   TTFONT_DEBUG_STR, code, font->ident, Encoding_get_name(encoding));
      WARN("%s: Maybe incorrect encoding specified.", TTFONT_DEBUG_STR);
      new_gid = 0;
    } else {
      if (__verbose > 2)
	MESG("/%s", enc_vec[code]);
      /*
       * First we try glyph name to GID mapping using post table if post table
       * is available. If post table is not available or glyph is not listed 
       * in the post table, then we try Unicode if Windows-Unicode TrueType
       * cmap is available.
       */
      if (ttgn)
	gid = tt_glyph_lookup(ttgn, enc_vec[code]);
      /*
       * UCS-4 not supported yet.
       */
      if (gid == 0 && ttcm) {
	if (AGName_is_unicode(enc_vec[code]))
	  gid = tt_cmap_lookup(ttcm, (USHORT) AGName_convert_uni(enc_vec[code]));
	else if (aglm) {
	  AGList *agl;
	  agl = AGLmap_lookup(aglm, enc_vec[code], NULL);
	  /*
	   * Try all alternatives.
	   */
	  while (agl) {
	    /*
	     * We can't support decomposed form.
	     */
	    if (!AGList_is_composite(agl)) {
	      gid = tt_cmap_lookup(ttcm, (USHORT) AGList_get_code(agl)); /* TODO: UCS4 */
	      if (gid > 0) {
		if (__verbose > 3)
		  MESG("(U+%04X)", AGList_get_code(agl));
		break;
	      }
	    }
	    agl = AGList_next_alternative(agl);
	  }
	}
      }
      /*
       * Older versions of gs had problem with glyphs (other than .notdef)
       * mapped to gid = 0.
       */
      if (gid == 0 && enc_vec[code])
	WARN("Glyph \"%s\" missing in font \"%s\".", enc_vec[code], font->ident);
      if ((new_gid = tt_find_glyph(glyphs, gid)) == 0) {
	new_gid = tt_add_glyph(glyphs, gid, count); /* count returned. */
	count++;
      }
    }
    cmap_table[18+code] = new_gid;
  }
  tt_cmap_release(ttcm);
  tt_release_glyph_names(ttgn);

  if (__verbose > 2)
    MESG("]");

  if (tt_build_tables(sfont, glyphs) < 0)
    ERROR("Could not created FontFile stream.");

  {
    double widths[256];

    for (code = 0; code < 256; code++) {
      if (font->used_chars[code]) {
	USHORT idx;
	idx = tt_get_index(glyphs, (USHORT) cmap_table[18+code]);
	widths[code] = PDFUNIT(glyphs->gd[idx].advw);
      } else {
	widths[code] = 0.0;
      }
    }
    do_widths(font, widths);
  }

  if (__verbose > 1) 
    MESG("[%d glyphs]", glyphs->num_glyphs);

  tt_build_finish(glyphs);

  sfnt_set_table(sfont, "cmap", cmap_table, 274);

}

void
TTFont_dofont (TTFont *font)
{
  char    *stream_data;
  pdf_obj *stream_dict;
  long     stream_len;
  sfnt    *sfont;

  ASSERT(font);

  if (!font->indirect)
    return;

  pdf_add_dict(font->fontdict, 
	       pdf_new_name("FontDescriptor"),
	       pdf_link_obj(pdf_ref_obj(font->descriptor)));
  if (font->encoding_id >= 0)
    UC_add_ToUnicode_CMap(font->fontdict, Encoding_cache_get(font->encoding_id));

  if (!font->embed)
    return;

  sfont = sfnt_open(font->filename);
  if (!sfont)
    ERROR("%s: Unable to open file (%s)", TTFONT_DEBUG_STR, font->filename);
  if (sfont->type != SFNT_TYPE_TRUETYPE || 
      sfnt_read_table_directory(sfont, 0) < 0)
    ERROR("%s: Not TrueType font ?", TTFONT_DEBUG_STR);

  if (!font->used_chars)
    ERROR("%s: Unexpected error.", TTFONT_DEBUG_STR);

  /*
   * Create new TrueType cmap table with MacRoman encoding.
   */
  if (font->encoding_id < 0)
    do_builtin_encoding(font, sfont);
  else
    do_custum_encoding(font, sfont);

  /*
   * TODO: post table?
   */

  {
    int i;
    for (i = 0; required_table[i].name != NULL; i++) {
      if (sfnt_require_table(sfont,
			     required_table[i].name,
			     required_table[i].must_exist) < 0)
	ERROR("%s: TrueType table \"%s\" does not exist.",
	      TTFONT_DEBUG_STR, required_table[i].name);
    }
  }

  stream_len  = sfnt_get_size(sfont);
  stream_data = NEW(stream_len, char);
  if (sfnt_build_font(sfont, stream_data, stream_len) == NULL)
    ERROR("%s: Could not created FontFile stream.", TTFONT_DEBUG_STR);
  sfnt_close(sfont);

  /*
   * FontFile2
   */
  {
    pdf_obj *fontfile;

    fontfile = pdf_new_stream(STREAM_COMPRESS);
    pdf_add_dict(font->descriptor,
		 pdf_new_name("FontFile2"), pdf_link_obj(pdf_ref_obj(fontfile)));
    stream_dict = pdf_stream_dict(fontfile);
    pdf_add_dict(stream_dict, pdf_new_name("Length1"), pdf_new_number(stream_len));
    pdf_add_stream(fontfile, stream_data, stream_len);
    pdf_release_obj(fontfile);
    RELEASE(stream_data);
  }

  if (__verbose > 1)
    MESG("[%ld bytes]", stream_len);

  return;
}

/******************************** CACHE ********************************/

#define CACHE_ALLOC_SIZE 16u

struct FontCache {
  int num;
  int max;
  struct TTFont **fonts;
};

static struct FontCache *__cache = NULL;

void
TTFont_cache_init (void)
{
  if (__cache)
    ERROR("%s: Already initialized.", TTFONT_DEBUG_STR);

  __cache = NEW(1, struct FontCache);
  __cache->num   = 0;
  __cache->max   = CACHE_ALLOC_SIZE;
  __cache->fonts = NEW(__cache->max, struct TTFont *);
}

#define CHECK_ID(n) do {\
                        if (! __cache)\
                           ERROR("%s: Font cache not initialized.", TTFONT_DEBUG_STR);\
                        if ((n) < 0 || (n) >= __cache->num)\
                           ERROR("%s: Invalid ID %d", TTFONT_DEBUG_STR, (n));\
                    } while (0)

TTFont *
TTFont_cache_get (int font_id)
{
  CHECK_ID(font_id);

  return __cache->fonts[font_id];
}

/*
 * remap ... 
 */
int
TTFont_cache_find (const char *map_name, char *res_name, int encoding_id, int tfm_id, int remap)
{
  TTFont *font;
  int     font_id;

  if (!__cache)
    TTFont_cache_init();
  ASSERT(__cache);

  for (font_id = 0; font_id < __cache->num; font_id++) {
    font = __cache->fonts[font_id];
    if (map_name && font->ident && !strcmp(font->ident, map_name)
	&& (encoding_id == font->encoding_id))
      return font_id;
  }

  font_id = __cache->num;
  font    = TTFont_new();
  if (TTFont_open(font, map_name, encoding_id, 1) < 0) {
    TTFont_release(font);
    return -1;
  }

  if (remap)
    WARN("%s: \"remap\" used.", TTFONT_DEBUG_STR);

  if (__cache->num >= __cache->max) {
    __cache->max  += CACHE_ALLOC_SIZE;
    __cache->fonts = RENEW(__cache->fonts, __cache->max, struct TTFont *);
  }
  __cache->fonts[font_id] = font;
  (__cache->num)++;

  return font_id;
}

void
TTFont_cache_close (void)
{
  if (__cache) {
    int i;
    for (i = 0; i < __cache->num; i++) {
      TTFont *font = __cache->fonts[i];
      if (__verbose) {
	MESG("(TrueType:%s", font->ident);
	if (__verbose > 1)
	  MESG("[%s][%s]", font->filename, font->fontname);
      }
      TTFont_dofont (font);
      TTFont_flush  (font);
      TTFont_release(font);
      if (__verbose)
	MESG(")");
    }
    RELEASE(__cache);
  }
}


/******************************** COMPAT ********************************/

void
ttf_disable_partial (void)
{
  WARN("Only subsetted embedding supported for TrueType font.");
}

void
ttf_set_verbose (void)
{
  TTFont_set_verbose();
}

pdf_obj *
ttf_font_resource (int font_id)
{
  return TTFont_get_resource(TTFont_cache_get(font_id));
}

char *
ttf_font_used (int font_id)
{
  return TTFont_get_usedchars(TTFont_cache_get(font_id));
}

int
ttf_font (const char *tex_name, int tfm_id, char *resource_name, int encoding_id, int remap)
{
  return TTFont_cache_find(tex_name, resource_name, encoding_id, tfm_id, remap);
}

void
ttf_set_mapfile (const char *name)
{
  return;
}

void
ttf_close_all (void)
{
  TTFont_cache_close();
}
