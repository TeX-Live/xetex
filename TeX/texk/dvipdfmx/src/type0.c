/*  $Header: /home/cvsroot/dvipdfmx/src/type0.c,v 1.9 2003/12/07 09:01:34 hirata Exp $
    
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
 * Type0 font support:
 * 
 * TODO:
 *
 *  Composite font (multiple descendants) - not supported in PDF
 */

#include <string.h>

#include "system.h"
#include "mem.h"
#include "error.h"

#include "pdfobj.h"

#include "cmap.h"
#include "cid.h"

#include "type0.h"

#define TYPE0FONT_DEBUG_STR "Type0"
#define TYPE0FONT_DEBUG     3

static int __verbose = 0;

void
Type0Font_set_verbose(void)
{
  __verbose++;
}

/*
 * used_chars:
 *
 *  Single bit is used for each CIDs since used_chars can be reused as a
 *  stream content of CIDSet by doing so. See, cid.h for add_to_used() and
 *  is_used().
 */

static char *
new_used_chars2(void)
{
  char *used_chars;

  used_chars = NEW(8192, char);
  memset(used_chars, 0, 8192);

  return used_chars;
}

#define FLAG_NONE              0
#define FLAG_USED_CHARS_SHARED (1 << 0)

struct Type0Font {
  char    *fontname;
  char    *encoding;   /* "Identity-H" or "Identity-V" (not ID) */
  char    *used_chars; /* Used chars (CIDs) */
  /*
   * Type0 only
   */
  CIDFont *descendant; /* Only single descendant is allowed. */
  int      wmode;
  int      flags;
  /*
   * PDF Font Resource
   */
  pdf_obj *indirect;
  pdf_obj *fontdict;
  pdf_obj *descriptor; /* MUST BE NULL */
};

Type0Font *
Type0Font_new (void)
{
  Type0Font *font;

  font = NEW(1, struct Type0Font);
  font->fontname   = NULL;
  font->fontdict   = NULL;
  font->indirect   = NULL;
  font->descriptor = NULL;
  font->encoding   = NULL;
  font->used_chars = NULL;
  font->descendant = NULL;
  font->wmode      = -1;
  font->flags      = FLAG_NONE;

  return font;
}

void
Type0Font_release (Type0Font *font)
{
  if (font) {
    if (font->fontdict)
      ERROR("%s: Object not flushed.", TYPE0FONT_DEBUG_STR);
    if (font->indirect)
      ERROR("%s: Object not flushed.", TYPE0FONT_DEBUG_STR);
    if (font->descriptor)
      ERROR("%s: FontDescriptor unexpected for Type0 font.", TYPE0FONT_DEBUG_STR);
    if (!(font->flags & FLAG_USED_CHARS_SHARED) && font->used_chars)
      RELEASE(font->used_chars);
    if (font->encoding)
      RELEASE(font->encoding);
    if (font->fontname)
      RELEASE(font->fontname);
    RELEASE(font);
  }
}

void
Type0Font_dofont (Type0Font *font)
{
  CIDFont *cidfont;

  if (!font || !font->indirect)
    return;
  /*
   * ToUnicode CMap:
   *
   *   ToUnicode CMaps are usually not required for standard character collections such as
   *   Adobe-Japan1. Identity-H is used for UCS ordering CID-keyed fonts. External resource
   *   must be loaded for others. (but not supported yet)
   */

  cidfont = font->descendant;
  if (!cidfont)
    ERROR("%s: No descendant CID-keyed font.", TYPE0FONT_DEBUG_STR);
#ifndef WITHOUT_COMPAT
  {
    pdf_obj *tucmap = NULL;

    if (CIDFont_is_UCSFont(cidfont)) {
      /*
       * Old version of dvipdfmx mistakenly used Adobe-Identity as Unicode.
       */
      tucmap = get_tounicode_cmap("Adobe-Identity-UCS2");
      if (!tucmap)
	tucmap = pdf_new_name("Identity-H");
    } else if (!CIDFont_is_ACCFont(cidfont)) { /* Not uses Adobe's character collections. */
      CIDSysInfo *csi;
      char       *tmp;
      csi = CIDFont_get_CIDSysInfo(cidfont);
      tmp = NEW(strlen(csi->registry)+strlen(csi->ordering)+7, char);
      sprintf(tmp, "%s-%s-UCS2", csi->registry, csi->registry);
      tucmap = get_tounicode_cmap(tmp);
      RELEASE(tmp);
    }

    if (tucmap)
      pdf_add_dict(font->fontdict, pdf_new_name("ToUnicode"), tucmap);
  }
#endif /* !WITHOUT_COMPAT */
}

void
Type0Font_flush (Type0Font *font)
{
  if (font) {
    if (font->fontdict)
      pdf_release_obj(font->fontdict);
    font->fontdict = NULL;
    if (font->indirect)
      pdf_release_obj(font->indirect);
    font->indirect = NULL;
    if (font->descriptor)
      ERROR("%s: FontDescriptor unexpected for Type0 font.", TYPE0FONT_DEBUG_STR);
    font->descriptor = NULL;
  }
}

int
Type0Font_get_wmode (Type0Font *font)
{
  ASSERT(font);

  return font->wmode;
}

char *
Type0Font_get_encoding (Type0Font *font)
{
  ASSERT(font);

  return font->encoding;
}

char *
Type0Font_get_usedchars (Type0Font *font)
{
  ASSERT(font);

  return font->used_chars;
}

pdf_obj *
Type0Font_get_resource (Type0Font *font)
{
  ASSERT(font);

  /*
   * This looks somewhat strange.
   */
  if (!font->indirect) {
    pdf_obj *array;

    array = pdf_new_array();
    pdf_add_array(array, CIDFont_get_resource(font->descendant));
    pdf_add_dict(font->fontdict, pdf_new_name("DescendantFonts"), array);
    font->indirect = pdf_ref_obj(font->fontdict);
  }

  return pdf_link_obj(font->indirect);
}

/******************************** CACHE ********************************/

#define CHECK_ID(n) do {\
                        if (! __cache)\
                           ERROR("%s: Not initialized.", TYPE0FONT_DEBUG_STR);\
                        if ((n) < 0 || (n) >= __cache->num)\
                           ERROR("%s: Invalid ID %d", TYPE0FONT_DEBUG_STR, (n));\
                    } while (0)


#define CACHE_ALLOC_SIZE 16u

struct FontCache {
  int         num;
  int         max;
  Type0Font **fonts;
};

static struct FontCache *__cache = NULL;

void
Type0Font_cache_init (void)
{
  if (__verbose > TYPE0FONT_DEBUG)
    MESG("%s: Initialize\n", TYPE0FONT_DEBUG_STR);

  if (__cache)
    ERROR("%s: Already initialized.", TYPE0FONT_DEBUG_STR);
  __cache = NEW(1, struct FontCache);
  __cache->num   = 0;
  __cache->max   = CACHE_ALLOC_SIZE;
  __cache->fonts = NEW(__cache->max, struct Type0Font *);
}

Type0Font *
Type0Font_cache_get (int id)
{
  CHECK_ID(id);
  return __cache->fonts[id];
}

int
Type0Font_cache_find (const char *map_name, const char *res_name, int cmap_id)
{
  int         font_id = -1;
  Type0Font  *font;
  CIDFont    *cidfont;
  CMap       *cmap;
  CIDSysInfo *csi;
  char       *fontname = NULL;
  int         cid_id = -1, parent_id = -1, wmode = 0;
  int         pdf_ver;

  pdf_ver = pdf_get_version();
  if (!map_name || cmap_id < 0 || pdf_ver < 2)
    return -1;

  if (!__cache)
    Type0Font_cache_init();
  ASSERT(__cache);

  /*
   * Encoding is Identity-H or Identity-V according as thier WMode value.
   * 
   * We do not use match against the map_name since fonts (TrueType) covers characters
   * across multiple character collection (eg, Adobe-Japan1 and Adobe-Japan2) must be
   * splited into multiple CID-keyed fonts.
   */

  cmap = CMap_cache_get(cmap_id);
  csi  = (CMap_is_Identity(cmap)) ? NULL : CMap_get_CIDSysInfo(cmap) ;

  cid_id = CIDFont_cache_find(map_name, csi);

  if (cid_id < 0) 
    return -1;

  /*
   * The descendant CID-keyed font has already been registerd.
   * If CID-keyed font with ID = cid_id is new font, then create new parent
   * Type 0 font. Otherwise, there already exists parent Type 0 font and
   * then find him and return his ID. We must check against their WMode.
   */

  cidfont = CIDFont_cache_get(cid_id);
  wmode   = CMap_get_wmode(cmap);

  /* Does CID-keyed font already have parent ? */
  parent_id = CIDFont_get_parent_id(cidfont, wmode);
  if (parent_id >= 0)
    return parent_id; /* We don't need new one. */

  /*
   * CIDFont does not have parent or his parent's WMode does not matched with
   * wmode. Create new Type0 font.
   */

  font    = Type0Font_new();
  font_id = __cache->num;

  /*
   * All CJK double-byte characters are mapped so that resulting
   * character codes coincide with CIDs of given character collection.
   * So, the Encoding is always Identity-H for horizontal fonts or
   * Identity-V for vertical fonts.
   */
  font->encoding = strdup((wmode ? "Identity-V" : "Identity-H"));
  font->wmode    = wmode;

  /*
   * Now we start Font Dict.
   */
  font->fontdict = pdf_new_dict();
  pdf_add_dict(font->fontdict, pdf_new_name ("Type"),    pdf_new_name ("Font"));
  pdf_add_dict(font->fontdict, pdf_new_name ("Subtype"), pdf_new_name ("Type0"));

#if 0
  /* OBSOLETE */
  /*
   * Name:
   *
   *  Name used in font resource dictionary - required only in PDF-1.0.
   *  Note: This entry is obsolescent and its use is no longer recommended.
   *  (See implementation note 39 in Appendix H., PDF Reference v1.3, 2nd ed.)
   *
   *  I'll put this, but pdf_ver must be >=2 here.
   */
  if (pdf_ver < 1)
    pdf_add_dict(font->fontdict, pdf_new_name("Name"), pdf_new_name(res_name));
#endif

  /*
   * Type0 font does not have FontDescriptor because it is not a simple font.
   * Instead, DescendantFonts appears here.
   *
   * Up to PDF version 1.5, Type0 font must have single descendant font which
   * is a CID-keyed font. Future PDF spec. will allow multiple desecendant
   * fonts.
   */
  font->descendant = cidfont;
  CIDFont_attach_parent(cidfont, font_id, wmode);

  /*
   * PostScript Font name:
   *
   *  Type0 font's fontname is usually descendant CID-keyed font's fontname 
   *  appended by -ENCODING.
   */
  fontname = CIDFont_get_fontname(cidfont);

  if (__verbose) {
    if (CIDFont_get_embedding(cidfont) && strlen(fontname) > 7)
      MESG("(CID:%s)", fontname+7); /* skip XXXXXX+ */
    else
      MESG("(CID:%s)", fontname);
  }

  /*
   * The difference between CID-keyed font and TrueType font appears here.
   *
   * Glyph substitution for vertical writing is done in CMap mapping process
   * for CID-keyed fonts. But we must rely on OpenType layout table in the
   * case of TrueType fonts. So, we must use different used_chars for each
   * horizontal and vertical fonts in that case.
   *
   * In most PDF file, encoding name is not appended to fontname for Type0
   * fonts having CIDFontType 2 font as their descendant.
   */

  font->used_chars = NULL;
  font->flags      = FLAG_NONE;

  switch (CIDFont_get_subtype(cidfont)) {
  case CIDFONT_TYPE0:
    font->fontname = NEW(strlen(fontname)+strlen(font->encoding)+2, char);
    sprintf(font->fontname, "%s-%s", fontname, font->encoding);
    pdf_add_dict(font->fontdict,
		 pdf_new_name("BaseFont"), pdf_new_name(font->fontname));
    /* TODO: Need used_chars to write W, W2. */
    if (CIDFont_get_embedding(cidfont)) {
      if ((parent_id = CIDFont_get_parent_id(cidfont, wmode ? 0 : 1)) < 0)
	font->used_chars = new_used_chars2();
      else {
	/* Don't allocate new one. */
	font->used_chars = Type0Font_get_usedchars(Type0Font_cache_get(parent_id));
	font->flags     |= FLAG_USED_CHARS_SHARED;
      }
    }
    break;
  case CIDFONT_TYPE2:
    /*
     * TrueType:
     *
     *  Use different used_chars for H and V.
     */
    pdf_add_dict(font->fontdict,
		 pdf_new_name("BaseFont"), pdf_new_name(fontname));
    if (CIDFont_get_embedding(cidfont))
      font->used_chars = new_used_chars2();
    break;
  default:
    ERROR("Unrecognized CIDFont Type");
    break;
  }

  pdf_add_dict(font->fontdict, pdf_new_name("Encoding"), pdf_new_name(font->encoding));

  if (__cache->num >= __cache->max) {
    __cache->max  += CACHE_ALLOC_SIZE;
    __cache->fonts = RENEW(__cache->fonts, __cache->max, struct Type0Font *);
  }

  __cache->fonts[font_id] = font;
  (__cache->num)++;

  return font_id;
}

void
Type0Font_cache_close (void)
{
  int font_id;

  if (!__cache)
    return;
  /*
   * This need to be fixed.
   * CIDFont_cache_close() comes before Type0Font_release because of used_chars.
   */
  if (__cache) {
    for (font_id = 0; font_id < __cache->num; font_id++)
      Type0Font_dofont (__cache->fonts[font_id]);
  }
  CIDFont_cache_close();
  if (__cache) {
    for (font_id = 0; font_id < __cache->num; font_id++) {
      Type0Font_flush  (__cache->fonts[font_id]);
      Type0Font_release(__cache->fonts[font_id]);
    }
    RELEASE(__cache);
  }
  if (__verbose > TYPE0FONT_DEBUG)
    MESG("%s: Close\n", TYPE0FONT_DEBUG_STR);
}

/******************************** COMPAT ********************************/
void
type0_set_verbose (void)
{
  Type0Font_set_verbose();
}

pdf_obj *
type0_font_resource (int id)
{
  return Type0Font_get_resource(Type0Font_cache_get(id));
}

int
type0_font_wmode (int id)
{
  return Type0Font_get_wmode(Type0Font_cache_get(id));
}

char *
type0_font_encoding (int id)
{
  return Type0Font_get_encoding(Type0Font_cache_get(id));
}

char *
type0_font_used (int id)
{
  return Type0Font_get_usedchars(Type0Font_cache_get(id));
}

int
type0_font (const char *tex_name, int tfm_id, char *res_name, int enc_id, int remap)
{
  return Type0Font_cache_find(tex_name, res_name, enc_id);
}

void
type0_close_all (void)
{
  Type0Font_cache_close();
}

#include "mfileio.h"
#include "pdfresource.h"
#define WBUF_SIZE 4096
pdf_obj *
get_tounicode_cmap (const char *cmap_name)
{
  pdf_obj *cmap_ref, *cmap;
  char    *fullname;
  FILE    *fp;
  char     wbuf[WBUF_SIZE];
  long     len, length;

  if (!cmap_name)
    return NULL;

  cmap_ref = PDF_findresource(cmap_name, PDF_RES_TYPE_CMAP);
  if (cmap_ref)
    return cmap_ref;

#ifdef MIKTEX
  fullname = work_buffer;
  if (!miktex_find_app_input_file("dvipdfm", cmap_name, fullname)) {
    if (miktex_get_acrobat_font_dir(fullname)) {
      strcat(fullname, "\\..\\CMap\\");
      strcat(fullname, CMAP_ADOBE_IDENTITY_UCS2);
    } else
      fullname = NULL;
  }
#else
  fullname = kpse_find_file(cmap_name, kpse_program_text_format, 0);
#endif

  if (!fullname || !(fp = MFOPEN(fullname, FOPEN_RBIN_MODE)) ||
      (length = file_size(fp)) < 1)
    return NULL;

  /*
   * BUG:
   *
   *  This does not check usecmap.
   */
  cmap = pdf_new_stream(STREAM_COMPRESS);
  while (length > 0) {
    if ((len = fread(wbuf, sizeof(char), MIN(WBUF_SIZE, length), fp))
	!= MIN(WBUF_SIZE, length)) {
      pdf_release_obj(cmap);
      return NULL;
    }
    pdf_add_stream(cmap, wbuf, len);
    length -= len;
  }
  MFCLOSE(fp);

  if (cmap)
    PDF_defineresource(cmap_name, cmap, PDF_RES_TYPE_CMAP);

  return PDF_findresource(cmap_name, PDF_RES_TYPE_CMAP);
}
