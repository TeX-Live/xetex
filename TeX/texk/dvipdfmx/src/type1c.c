/*  $Header: /home/cvsroot/dvipdfmx/src/type1c.c,v 1.11 2004/02/15 12:59:44 hirata Exp $

    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2002 by Jin-Hwan Cho and Shunsaku Hirata,
    the dvipdfmx project team <dvipdfmx@project.ktug.or.kr>

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

/*
 * CFF/OpenType Font support:
 *
 *  Adobe Technical Note #5176, "The Compact Font Format Specfication"
 *
 * NOTE:
 *
 *  Many CFF/OpenType does not have meaningful/correct CFF encoding.
 *  Encoding should be expilicitly supplied in the fontmap.
 *
 */ 

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "system.h"
#include "mfileio.h"
#include "mem.h"
#include "error.h"
#include "pdfobj.h"

#include "encodings.h"
#include "unicode.h"

/* mangle_name() */
#include "dpxutil.h"

/* Font info. from OpenType tables */
#include "sfnt.h"
#include "tt_aux.h"

#include "cff_types.h"
#include "cff_limits.h"
#include "cff.h"
#include "cff_dict.h"
#include "cs_type2.h"

#include "type1c.h"

#ifndef HAVE_KPSE_OPENTYPE_FORMAT
#define kpse_opentype_format kpse_program_binary_format
#endif

static int __verbose = 0;

#define TYPE1CFONT_DEBUG_STR "Type1C"
#define TYPE1CFONT_DEBUG     3

void
Type1CFont_set_verbose (void)
{
  __verbose++;
}

struct Type1CFont 
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

Type1CFont *
Type1CFont_new (void)
{
  Type1CFont *font;

  font = NEW(1, Type1CFont);
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
Type1CFont_flush (Type1CFont *font)
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
Type1CFont_release (Type1CFont *font)
{
  if (!font)
    return;

  if (font->ident)      RELEASE(font->ident);
  if (font->fontname)   RELEASE(font->fontname);
  if (font->filename)   RELEASE(font->filename);
  if (font->used_chars) RELEASE(font->used_chars);

  if (font->indirect)   ERROR("%s: Object not flushed.", TYPE1CFONT_DEBUG_STR);
  if (font->fontdict)   ERROR("%s: Object not flushed.", TYPE1CFONT_DEBUG_STR);
  if (font->descriptor) ERROR("%s: Object not flushed.", TYPE1CFONT_DEBUG_STR);

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
Type1CFont_get_resource (Type1CFont *font)
{
  ASSERT(font);

  if (!font->indirect)
    font->indirect = pdf_ref_obj(font->fontdict);

  return pdf_link_obj(font->indirect);
}

char *
Type1CFont_get_usedchars (Type1CFont *font)
{
  ASSERT(font);

  return font->used_chars;
}

int
Type1CFont_open (Type1CFont *font, const char *name, int encoding_id, int embed)
{
  char       *fontname;
  sfnt       *sfont;
  cff_font   *cff;
  char       *fullname = NULL;
  unsigned long offset = 0;

  ASSERT(font);

  {
    char *shortname;

    shortname = NEW(strlen(name)+5, char);
    memset(shortname, 0, strlen(name)+5);
    strcpy(shortname, name);
    if (strlen(shortname) < 5 || strcmp(shortname+strlen(shortname)-4, ".otf"))
      strcat(shortname, ".otf");
#ifdef MIKTEX
    if (!miktex_find_app_input_file("dvipdfm", shortname, fullname = work_buffer)) {
      RELEASE(shortname);
      return -1;
    }
#else
    if ((fullname = kpse_find_file(shortname, kpse_opentype_format, 1))
	== NULL) {
      RELEASE(shortname);
      return -1;
    }
#endif
    RELEASE(shortname);
  }

  if ((sfont = sfnt_open(fullname)) == NULL   ||
      sfont->type != SFNT_TYPE_POSTSCRIPT     ||
      sfnt_read_table_directory(sfont, 0) < 0 ||
      (offset = sfnt_find_table_pos(sfont, "CFF ")) == 0)
    ERROR("%s: Not a CFF/OpenType font?", TYPE1CFONT_DEBUG_STR);

  if ((cff = cff_open(sfont->stream, offset, 0)) == NULL)
    ERROR("%s: Cannot read CFF font data", TYPE1CFONT_DEBUG_STR);

  if (cff->flag & FONTTYPE_CIDFONT) {
    cff_close(cff);
    sfnt_close(sfont);
    return -1;
  }

  {
    char *short_fontname = cff_get_name(cff);
    if (!short_fontname)
      ERROR("%s: No valid FontName found.", TYPE1CFONT_DEBUG_STR);
    /*
     * Mangled name requires more 7 bytes.
     * Style requires more 11 bytes.
     */
    fontname = NEW(strlen(short_fontname)+19, char);
    memset(fontname, 0, strlen(short_fontname)+19);
    strcpy(fontname, short_fontname);
    RELEASE(short_fontname);
  }
  cff_close(cff);

  /*
   * Font like AdobePiStd does not have meaningful built-in encoding.
   * Some software generate CFF/OpenType font with incorrect encoding.
   */
  if (encoding_id < 0) {
    WARN("Built-in encoding used for CFF/OpenType font.");
    WARN("CFF font in OpenType font sometimes have strange built-in encoding.");
    WARN("If you find text is not encoded properly in the generated PDF file,");
    WARN("please specify appropriate \".enc\" file in your fontmap.");
  }

  font->ident       = strdup(name);
  font->fontname    = fontname;
  font->filename    = strdup(fullname);
  font->encoding_id = encoding_id;
  font->embed       = embed;
  font->fontdict    = pdf_new_dict();

  pdf_add_dict(font->fontdict,
	       pdf_new_name("Type"),
	       pdf_new_name("Font"));
  pdf_add_dict(font->fontdict,
	       pdf_new_name("Subtype"),
	       pdf_new_name("Type1"));

  /*
   * Create font descriptor from OpenType tables.
   * We can also use CFF TOP DICT/Private DICT for this.
   */
  if ((font->descriptor
       = tt_get_fontdesc(sfont, &(font->embed), 1)) == NULL)
    ERROR("%s: Could not obtain neccesary font info from OpenType table.", TYPE1CFONT_DEBUG_STR);

  if (font->embed)
    mangle_name(font->fontname);

  pdf_add_dict(font->fontdict,
	       pdf_new_name("BaseFont"),
	       pdf_new_name(font->fontname));
  pdf_add_dict(font->descriptor,
	       pdf_new_name("FontName"),
	       pdf_new_name(font->fontname));
  if (font->embed)
    font->used_chars = new_used_chars();

  sfnt_close(sfont);

  return 0;
}

void
add_SimpleMetrics (Type1CFont *font, double *widths, card16 num_glyphs)
{
  int code, firstchar, lastchar;
  pdf_obj *tmp;

  tmp = pdf_new_array();
  if (num_glyphs <= 1) {
    /* This should be error. */
    firstchar = lastchar = 0;
    pdf_add_array(tmp, pdf_new_number(0.0));
  } else {
    firstchar = 255; lastchar = 0;
    for (code = 0; code < 256; code++) {
      if (font->used_chars[code]) {
	if (code < firstchar) firstchar = code;
	if (code > lastchar)  lastchar  = code;
      }
    }
    tmp = pdf_new_array();
    for (code = firstchar; code <= lastchar; code++) {
      if (font->used_chars[code]) {
	pdf_add_array(tmp, pdf_new_number(ROUND(widths[code], 1.0)));
      } else {
	pdf_add_array(tmp, pdf_new_number(0.0));
      }
    }
  }
  pdf_add_dict(font->fontdict,
	       pdf_new_name("FirstChar"), pdf_new_number(firstchar));
  pdf_add_dict(font->fontdict,
	       pdf_new_name("LastChar"),  pdf_new_number(lastchar));
  pdf_add_dict(font->fontdict, pdf_new_name("Widths"), pdf_ref_obj(tmp));
  pdf_release_obj(tmp);

}

void
Type1CFont_dofont (Type1CFont *font)
{
  pdf_obj   *stream_dict;
  sfnt      *sfont;
  char     **enc_vec;
  cff_font  *cff;
  cff_index *charstrings, *topdict, *cs_idx;
  cff_charsets *charset  = NULL;
  cff_encoding *encoding = NULL;
  long   topdict_offset, private_size;
  long   charstring_len, max_len;
  long   destlen = 0;
  long   size, offset = 0;
  card8 *dest, *data;
  card16 num_glyphs;
  card16 cs_count, code;
  cs_ginfo ginfo;
  double   nominal_width, default_width, notdef_width;
  double   widths[256];

  ASSERT(font);

  if (!font->indirect)
    return;

  if (!font->embed)
    ERROR("%s: Only embedded font supported for CFF/OpenType font.", TYPE1CFONT_DEBUG_STR);

  if (!Type1CFont_get_usedchars(font))
    ERROR("%s: Unexpected error: No used_chars?", TYPE1CFONT_DEBUG_STR);

  pdf_add_dict(font->fontdict, 
	       pdf_new_name("FontDescriptor"),
	       pdf_link_obj(pdf_ref_obj(font->descriptor)));

  /*
   * Open CFF font.
   */
  {
    if ((sfont = sfnt_open(font->filename)) == NULL)
      ERROR("%s: Could not open OpenType font: %s", TYPE1CFONT_DEBUG_STR, font->filename);
    if (sfnt_read_table_directory(sfont, 0) < 0)
      ERROR("%s: Could not read OpenType table directory.", TYPE1CFONT_DEBUG_STR);
    if (sfont->type != SFNT_TYPE_POSTSCRIPT ||
	(offset = sfnt_find_table_pos(sfont, "CFF ")) == 0)
      ERROR("%s: Not a CFF/OpenType font ?", TYPE1CFONT_DEBUG_STR);

    if ((cff = cff_open(sfont->stream, offset, 0)) == NULL)
      ERROR("%s: Could not open CFF font.", TYPE1CFONT_DEBUG_STR);
    if (cff->flag & FONTTYPE_CIDFONT)
      ERROR("%s: This is CIDFont...", TYPE1CFONT_DEBUG_STR);
  }

  /* Offsets from DICTs */
  cff_read_charsets(cff);
  if (font->encoding_id < 0)
    cff_read_encoding(cff);
  cff_read_private(cff);
  cff_read_subrs(cff);

  /* FIXME */
  cff->_string = cff_new_index(0);

  /* New Charsets data */
  charset = NEW(1, cff_charsets);
  charset->format      = 0;
  charset->num_entries = 0;
  charset->data.glyphs = NEW(256, s_SID);

  /*
   * Encoding related things.
   */
  enc_vec = NULL;
  if (font->encoding_id >= 0) {
    Encoding *enc = Encoding_cache_get(font->encoding_id);
    if (Encoding_is_predefined(enc)) {
      pdf_add_dict(font->fontdict,
		   pdf_new_name("Encoding"),
		   pdf_new_name(Encoding_get_name(enc)));
    } else {
#if 0
      /*
       * Gs not working with this.
       */
      pdf_add_dict(font->fontdict,
		   pdf_new_name("Encoding"),
		   Encoding_get_resource(enc));
#endif
      UC_add_ToUnicode_CMap(font->fontdict, enc);
    }
    enc_vec = Encoding_get_encoding(enc);
  } else {
    pdf_obj *tounicode;
    char    *tmpname;
    /*
     * Create enc_vec and ToUnicode CMap for built-in encoding.
     */
    enc_vec = NEW(256, char *);
    for (code = 0; code < 256; code++) {
      if (font->used_chars[code]) {
	card16 gid = cff_encoding_lookup(cff, code);
	enc_vec[code] = cff_get_string(cff, cff_charsets_lookup_inverse(cff, gid));
      } else
	enc_vec[code] = NULL;
    }
    /*
     * The naming convention for embedded ToUnicode is not clear.
     * We use PseudoUniqTag+FontName-UTF16 here. Not -UCS2, it is to-UTF16BE CMap.
     */
    tmpname = NEW(strlen(font->fontname)+7, char);
    sprintf(tmpname, "%s-UTF16", font->fontname);
    tounicode = UC_make_ToUnicode_CMap(tmpname, enc_vec, NULL);
    if (tounicode) {
      pdf_add_dict(font->fontdict,
		   pdf_new_name("ToUnicode"), pdf_link_obj(pdf_ref_obj(tounicode)));
      pdf_release_obj(tounicode);
    }
    RELEASE(tmpname);
  }
  /*
   * New Encoding data:
   *
   *  We should not use format 0 here.
   *  The number of encoded glyphs (num_entries) is limited to 255 in format 0,
   *  and hence it causes problem for encodings that uses full 256 code-points.
   *  As we always sort glyphs by encoding, we can avoid this problem simply
   *  by using format 1; Using full range result in a single range, 0 255.
   *
   *  Creating actual encoding date is delayed to eliminate character codes to
   *  be mapped to .notdef and to handle multiply-encoded glyphs.
   */
  encoding = NEW(1, cff_encoding);
  encoding->format      = 1;
  encoding->num_entries = 0;
  encoding->data.range1 = NEW(255, cff_range1);
  encoding->num_supps   = 0;
  encoding->supp        = NEW(255, cff_map);

  /*
   * Charastrings.
   */
  offset = (long) cff_dict_get(cff->topdict, "CharStrings", 0);
  cff_seek_set(cff, offset);
  cs_idx = cff_get_index_header(cff);

  /* Offset is now absolute offset ... fixme */
  offset   = ftell(cff->stream);
  cs_count = cs_idx->count;
  if (cs_count < 2)
    ERROR("%s: No valid charstring data found.", TYPE1CFONT_DEBUG_STR);

  /* New CharStrings INDEX */
  charstrings       = cff_new_index(256);
  max_len           = 2 * CS_STR_LEN_MAX;
  charstrings->data = NEW(max_len, card8);
  charstring_len    = 0;

  /*
   * Information from OpenType table is rough estimate. Replace with accurate value.
   */
  if (cff->private[0] && cff_dict_known(cff->private[0], "StdVW")) {
    double stemv;
    stemv = cff_dict_get(cff->private[0], "StdVW", 0);
    pdf_add_dict(font->descriptor,
		 pdf_new_name("StemV"), pdf_new_number(stemv));
  }
  
  /*
   * Widths
   */
  {
    if (cff->private[0] && cff_dict_known(cff->private[0], "defaultWidthX")) {
      default_width = (double) cff_dict_get(cff->private[0], "defaultWidthX", 0);
    } else {
      default_width = CFF_DEFAULTWIDTHX_DEFAULT;
    }
    if (cff->private[0] && cff_dict_known(cff->private[0], "nominalWidthX")) {
      nominal_width = (double) cff_dict_get(cff->private[0], "nominalWidthX", 0);
    } else {
      nominal_width = CFF_NOMINALWIDTHX_DEFAULT;
    }
  }

  data = NEW(CS_STR_LEN_MAX, card8);
  /*
   * First we add .notdef glyph.
   */
  {
    if (__verbose > 2)
      MESG("[glyphs:/.notdef");

    if ((size = (cs_idx->offset)[1] - (cs_idx->offset)[0]) > CS_STR_LEN_MAX)
      ERROR("%s: Charstring too long: gid=%u", 0, TYPE1CFONT_DEBUG_STR);
    (charstrings->offset)[0] = charstring_len + 1;
    seek_absolute(cff->stream, offset + (cs_idx->offset)[0] - 1);
    fread(data, 1, size, cff->stream);
    charstring_len += cs_copy_charstring(charstrings->data + charstring_len,
					 max_len - charstring_len,
					 data, size,
					 cff->gsubr, (cff->subrs)[0],
					 default_width, nominal_width, &ginfo);
    notdef_width = ginfo.wx;
  }
  /*
   * Subset font
   */
  num_glyphs = 1;
  for (code = 0; code < 256; code++) {
    card16 gid;
    s_SID  sid_orig, sid;

    widths[code] = notdef_width;

    if (!font->used_chars[code] || !enc_vec[code] ||
	!strcmp(enc_vec[code], ".notdef"))
      continue;

    /*
     * FIXME:
     *  cff_get_sid() obtain SID from original String INDEX.
     *  It should be cff_string_get_sid(string, ...).
     *  cff_add_string(cff, ...) -> cff_string_add(string, ...).
     */
    sid_orig = cff_get_sid(cff, enc_vec[code]);
    sid = cff_add_string(cff, enc_vec[code]);
    /*
     * Check if multiply-encoded glyph.
     */
    {
      card16 j;
      for (j = 0; j < charset->num_entries; j++) {
	if (sid == charset->data.glyphs[j]) {
	  /* Already have this glyph. */
	  encoding->supp[encoding->num_supps].code  = code;
	  encoding->supp[encoding->num_supps].glyph = sid;
	  font->used_chars[code] = 0; /* Used but multiply-encoded. */
	  encoding->num_supps += 1;
	  break;
	}
      }
      if (j < charset->num_entries)
	continue; /* Prevent duplication. */
    }
    gid = cff_charsets_lookup(cff, sid_orig); /* FIXME */
    if (gid == 0) {
      WARN("Glyph \"%s\" missing in font \"%s\".", enc_vec[code], font->ident);
      WARN("Maybe incorrect encoding specified.");
      font->used_chars[code] = 0; /* Set unused for writing correct encoding */
      continue;
    }
    if (__verbose > 2)
      MESG("/%s", enc_vec[code]);

    if ((size = (cs_idx->offset)[gid+1] - (cs_idx->offset)[gid]) > CS_STR_LEN_MAX) {
      ERROR("%s: Charstring too long: gid=%u", gid, TYPE1CFONT_DEBUG_STR);
    }

    if (charstring_len + CS_STR_LEN_MAX >= max_len) {
      max_len = charstring_len + 2 * CS_STR_LEN_MAX;
      charstrings->data = RENEW(charstrings->data, max_len, card8);
    }
    (charstrings->offset)[num_glyphs] = charstring_len + 1;
    seek_absolute(cff->stream, offset + (cs_idx->offset)[gid] - 1);
    fread(data, sizeof(char), size, cff->stream);
    charstring_len += cs_copy_charstring(charstrings->data + charstring_len,
					 max_len - charstring_len,
					 data, size,
					 cff->gsubr, (cff->subrs)[0],
					 default_width, nominal_width, &ginfo);
    widths[code] = ginfo.wx;
    charset->data.glyphs[charset->num_entries] = sid;
    charset->num_entries  += 1;
    num_glyphs++;
  }
  if (__verbose > 2)
    MESG("]");

  RELEASE(data);

  /*
   * Now we create encoding data.
   */
  if (encoding->num_supps > 0)
    encoding->format |= 0x80; /* Have supplemantary data. */
  for (code = 0; code < 256; code++) {
    if (!font->used_chars[code] ||
	!enc_vec[code] || !strcmp(enc_vec[code], ".notdef"))
      continue;
    encoding->data.range1[encoding->num_entries].first  = code;
    encoding->data.range1[encoding->num_entries].n_left = 0;
    code++;
    while (code < 256 && font->used_chars[code] &&
	   enc_vec[code] && strcmp(enc_vec[code], ".notdef")) {
      encoding->data.range1[encoding->num_entries].n_left += 1;
      code++;
    }
    encoding->num_entries += 1;
    /* The above while() loop stopped at unused char or code == 256. */
  }
  
  if (font->encoding_id < 0 && enc_vec) {
    for (code = 0; code < 256; code++)
      if (enc_vec[code])
	RELEASE(enc_vec[code]);
    RELEASE(enc_vec);
  }

  cff_release_index(cs_idx);

  (charstrings->offset)[num_glyphs] = charstring_len + 1;
  charstrings->count = num_glyphs;
  charstring_len     = cff_index_size(charstrings);
  cff->num_glyphs    = num_glyphs;

  /*
   * Discard old one, set new data.
   */
  {
    if (cff->charsets)
      cff_release_charsets(cff->charsets);
    cff->charsets = charset;
    if (cff->encoding)
      cff_release_encoding(cff->encoding);
    cff->encoding = encoding;
    /*
     * No Subrs.
     */
    if (cff->gsubr)
      cff_release_index(cff->gsubr);
    cff->gsubr = cff_new_index(0);
    if (cff->subrs[0])
      cff_release_index(cff->subrs[0]);
    cff->subrs[0] = NULL;
  }

  /*
   * Flag must be reset since cff_pack_encoding(charset) does not write
   * encoding(charset) if HAVE_STANDARD_ENCODING(CHARSET) is set. We are
   * re-encoding font.
   */
  cff->flag = FONTTYPE_FONT;

  /*
   * FIXME:
   *  Update String INDEX to delete unused strings.
   */
  cff_dict_update(cff->topdict, cff);
  if (cff->private[0])
    cff_dict_update(cff->private[0], cff);
  cff_update_string(cff);

  /*
   * Calculate sizes of Top DICT and Private DICT.
   * All offset values in DICT are set to long (32-bit) integer
   * in cff_dict_pack(), those values are updated later.
   */
  {
    topdict = cff_new_index(1);
    /*
     * Force existence of Encoding.
     */
    if (!cff_dict_known(cff->topdict, "Encoding"))
      cff_dict_add(cff->topdict, "Encoding", 1);
    (topdict->offset)[1] = cff_dict_pack(cff->topdict,
					 (card8 *) work_buffer,
					 WORK_BUFFER_SIZE) + 1;
    private_size = 0;
    if ((cff->private)[0]) {
      cff_dict_remove((cff->private)[0], "Subrs"); /* no Subrs */
      private_size = cff_dict_pack((cff->private)[0],
				   (card8 *) work_buffer, WORK_BUFFER_SIZE);
    }
  }

  /*
   * Estimate total size of fontfile.
   */
  {
    destlen = 4; /* header size */
    destlen += cff_set_name(cff, font->fontname);
    destlen += cff_index_size(topdict);
    destlen += cff_index_size(cff->string);
    destlen += cff_index_size(cff->gsubr);
    /*
     * We are using format 1 for Encoding and format 0 for charset.
     * TODO: Should implement cff_xxx_size().
     */
    destlen += 2 + (encoding->num_entries)*2 + 1 + (encoding->num_supps)*3;
    destlen += 1 + (charset->num_entries)*2;
    destlen += charstring_len;
    destlen += private_size;
  }

  /*
   * Now we create FontFile data.
   */
  {
    dest = NEW(destlen, card8);
    /*
     * Data Layout order as described in CFF spec., sec 2 "Data Layout".
     */
    offset = 0;
    /* Header */
    offset += cff_put_header(cff, dest + offset, destlen - offset);
    /* Name */
    offset += cff_pack_index(cff->name, dest + offset, destlen - offset);
    /* Top DICT */
    topdict_offset = offset;
    offset += cff_index_size(topdict);
    /* Strings */
    offset += cff_pack_index(cff->string, dest + offset, destlen - offset);
    /* Global Subrs */
    offset += cff_pack_index(cff->gsubr, dest + offset, destlen - offset);
    /* Encoding */
    cff_dict_set(cff->topdict, "Encoding", 0, offset);
    offset += cff_pack_encoding(cff, dest + offset, destlen - offset);
    /* charset */
    cff_dict_set(cff->topdict, "charset", 0, offset);
    offset += cff_pack_charsets(cff, dest + offset, destlen - offset);
    /* CharStrings */
    cff_dict_set(cff->topdict, "CharStrings", 0, offset);
    offset += cff_pack_index(charstrings, dest + offset, charstring_len);
    cff_release_index(charstrings);
    /* Private */
    cff_dict_set(cff->topdict, "Private", 1, offset);
    if ((cff->private)[0] && private_size > 0)
      private_size = cff_dict_pack((cff->private)[0], dest + offset, private_size);
    cff_dict_set(cff->topdict, "Private", 0, private_size);
    offset += private_size;

    /* Finally Top DICT */
    topdict->data = NEW(topdict->offset[1] - 1, card8);
    cff_dict_pack(cff->topdict, topdict->data, topdict->offset[1] - 1);
    cff_pack_index(topdict, dest + topdict_offset, cff_index_size(topdict));
    cff_release_index(topdict);

    /* Copyright and Trademark Notice ommited. */
  }

  /* Close font */
  cff_close(cff);
  sfnt_close(sfont);

  if (__verbose > 1)
    MESG("[%u/%u glyphs][%ld bytes]", num_glyphs, cs_count, offset);

  /*
   * Widths
   */
  add_SimpleMetrics(font, widths, num_glyphs);

  /*
   * CharSet might be recommended for subsetted font, but it is meaningful
   * only for Type 1 font...
   */

  /*
   * Write PDF FontFile data.
   */
  {
    pdf_obj *fontfile;

    fontfile    = pdf_new_stream(STREAM_COMPRESS);
    stream_dict = pdf_stream_dict(fontfile);
    pdf_add_dict(font->descriptor,
		 pdf_new_name("FontFile3"),
		 pdf_link_obj(pdf_ref_obj(fontfile)));
    pdf_add_dict(stream_dict,
		 pdf_new_name("Subtype"),
		 pdf_new_name("Type1C"));
    pdf_add_stream(fontfile, (char *) dest, offset);
    pdf_release_obj(fontfile);
  }

  RELEASE(dest);

  return;
}


/******************************** CACHE ********************************/

#define CACHE_ALLOC_SIZE 16u

struct FontCache {
  int num;
  int max;
  Type1CFont **fonts;
};

static struct FontCache *__cache = NULL;

void
Type1CFont_cache_init (void)
{
  if (__cache)
    ERROR("%s: Already initialized.", TYPE1CFONT_DEBUG_STR);

  if (__verbose > TYPE1CFONT_DEBUG)
    MESG("%s: Initialize\n", TYPE1CFONT_DEBUG_STR);

  __cache = NEW(1, struct FontCache);
  __cache->num   = 0;
  __cache->max   = CACHE_ALLOC_SIZE;
  __cache->fonts = NEW(__cache->max, Type1CFont *);
}

#define CHECK_ID(n) do {\
                        if (! __cache)\
                           ERROR("%s: Font cache not initialized.", TYPE1CFONT_DEBUG_STR);\
                        if ((n) < 0 || (n) >= __cache->num)\
                           ERROR("%s: Invalid ID %d", TYPE1CFONT_DEBUG_STR, (n));\
                    } while (0)

Type1CFont *
Type1CFont_cache_get (int font_id)
{
  CHECK_ID(font_id);

  return __cache->fonts[font_id];
}

/*
 * tfm_id, remap ... 
 */
int
Type1CFont_cache_find (const char *map_name, char *res_name, int encoding_id, int tfm_id, int remap)
{
  Type1CFont *font;
  int      font_id;

  if (!__cache)
    Type1CFont_cache_init();
  ASSERT(__cache);

  for (font_id = 0; font_id < __cache->num; font_id++) {
    font = __cache->fonts[font_id];
    if (map_name && font->ident && !strcmp(font->ident, map_name)
	&& (encoding_id == font->encoding_id))
      return font_id;
  }

  font_id = __cache->num;
  font    = Type1CFont_new();
  if (Type1CFont_open(font, map_name, encoding_id, 1) < 0) {
    Type1CFont_release(font);
    return -1;
  }

  if (remap)
    WARN("%s: Obsolete option remap used.", TYPE1CFONT_DEBUG_STR);

  if (__cache->num >= __cache->max) {
    __cache->max  += CACHE_ALLOC_SIZE;
    __cache->fonts = RENEW(__cache->fonts, __cache->max, Type1CFont *);
  }
  __cache->fonts[font_id] = font;
  (__cache->num)++;

  return font_id;
}

void
Type1CFont_cache_close (void)
{
  if (__cache) {
    int i;
    for (i = 0; i < __cache->num; i++) {
      Type1CFont *font = __cache->fonts[i];
      if (__verbose) {
	MESG("(Type1C:%s", font->ident);
	if (__verbose > 1)
	  MESG("[%s][%s]", font->filename, font->fontname);
      }
      Type1CFont_dofont (font);
      Type1CFont_flush  (font);
      Type1CFont_release(font);
      if (__verbose)
	MESG(")");
    }
    RELEASE(__cache);
  }
  if (__verbose > TYPE1CFONT_DEBUG)
    MESG("%s: Close\n", TYPE1CFONT_DEBUG_STR);
}


/******************************** COMPAT ********************************/

void
type1c_disable_partial (void)
{
  WARN("Only subsetted embedding supported for CFF/OpenType font.");
}

void
type1c_set_verbose (void)
{
  Type1CFont_set_verbose();
}

pdf_obj *
type1c_font_resource (int font_id)
{
  return Type1CFont_get_resource(Type1CFont_cache_get(font_id));
}

char *
type1c_font_used (int font_id)
{
  return Type1CFont_get_usedchars(Type1CFont_cache_get(font_id));
}

int
type1c_font (const char *tex_name, int tfm_id, char *resource_name, int encoding_id, int remap)
{
  return Type1CFont_cache_find(tex_name, resource_name, encoding_id, tfm_id, remap);
}

void
type1c_set_mapfile (const char *name)
{
  return;
}

void
type1c_close_all (void)
{
  Type1CFont_cache_close();
}
