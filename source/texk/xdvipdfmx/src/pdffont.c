/*  

    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2008-2012 by Jin-Hwan Cho, Matthias Franz, and Shunsaku Hirata,
    the dvipdfmx project team.
    
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

#include <string.h>
#include <time.h>

#include "system.h"
#include "error.h"
#include "mem.h"

#include "dpxfile.h"
#include "dpxutil.h"

#include "pdfobj.h"

#include "pdfencoding.h"
#include "cmap.h"
#include "unicode.h"

#include "type1.h"
#include "type1c.h"
#include "truetype.h"

#include "pkfont.h"

#include "type0.h"
#include "tt_cmap.h"
#include "cidtype0.h"
#include "otl_conf.h"

#include "pdffont.h"

static int __verbose = 0;

#define MREC_HAS_TOUNICODE(m) ((m) && (m)->opt.tounicode)

void
pdf_font_set_verbose (void)
{
  __verbose++;
  CMap_set_verbose();
  Type0Font_set_verbose();
  CIDFont_set_verbose  ();
  pdf_encoding_set_verbose();
  UC_set_verbose ();
  agl_set_verbose();
  otl_conf_set_verbose();
  otf_cmap_set_verbose ();
}

int
pdf_font_get_verbose (void)
{
  return __verbose;
}

void
pdf_font_set_dpi (int font_dpi)
{
  PKFont_set_dpi(font_dpi);
}

void
pdf_font_make_uniqueTag (char *tag)
{
  int    i;
  char   ch;
  static char first = 1;

  if (first) {
    srand(time(NULL));
    first = 0;
  }

  for (i = 0; i < 6; i++) {
    ch = rand() % 26;
    tag[i] = ch + 'A';
  }
  tag[6] = '\0';
}


struct pdf_font
{
#ifdef XETEX
  FT_Face  ft_face;
  unsigned short *ft_to_gid;
#endif

  char    *ident;
  int      subtype;

  char    *map_name;

  int      encoding_id; /* encoding or CMap */

  /*
   * If subtype is Type0, it simply points font_id
   * of Type0 font. Type0 and simple font is not
   * unified yet.
   */
  int      font_id;

  /* For simple font */
  int      index;
  char    *fontname;
  char     uniqueID[7];

  /*
   * PDF font resource objects
   */
  pdf_obj *reference;
  pdf_obj *resource;
  pdf_obj *descriptor;

  /*
   * Font format specific data
   */
  char    *usedchars;
  int      flags;

  /* PK font */
  double   point_size;
  double   design_size;
};

static void
pdf_init_font_struct (pdf_font *font)
{
  ASSERT(font);

#ifdef XETEX
  font->ft_face  = NULL;
#endif

  font->ident    = NULL;
  font->map_name = NULL;
  font->subtype  = -1;
  font->font_id  = -1; /* Type0 ID */
  font->fontname = NULL;
  memset(font->uniqueID, 0, 7);
  font->index    = 0;

  font->encoding_id = -1;

  font->reference   = NULL;
  font->resource    = NULL;
  font->descriptor  = NULL;

  font->point_size  = 0;
  font->design_size = 0;

  font->usedchars   = NULL;
  font->flags       = 0;

  return;
}

static void
pdf_flush_font (pdf_font *font)
{
  char *fontname, *uniqueTag;

  if (!font) {
    return;
  }

  if (font->resource && font->reference) {
    if (font->subtype != PDF_FONT_FONTTYPE_TYPE3) {
      if (pdf_font_get_flag(font, PDF_FONT_FLAG_NOEMBED)) {
	pdf_add_dict(font->resource,
		     pdf_new_name("BaseFont"), pdf_new_name(font->fontname));
	if (font->descriptor) {
	  pdf_add_dict(font->descriptor,
		       pdf_new_name("FontName"), pdf_new_name(font->fontname));
	}
      } else {
	if (!font->fontname) {
	  ERROR("Undefined in fontname... (%s)", font->ident);
	}
	fontname  = NEW(7+strlen(font->fontname)+1, char);
	uniqueTag = pdf_font_get_uniqueTag(font);
	sprintf(fontname, "%6s+%s", uniqueTag, font->fontname);
	pdf_add_dict(font->resource,
		     pdf_new_name("BaseFont"), pdf_new_name(fontname));
	if (font->descriptor) {
	  pdf_add_dict(font->descriptor,
		       pdf_new_name("FontName"), pdf_new_name(fontname));
	}
	RELEASE(fontname);
      }
      if (font->descriptor) {
	pdf_add_dict(font->resource,
		     pdf_new_name("FontDescriptor"), pdf_ref_obj(font->descriptor));
      }
    }
  }

  if (font->resource)
    pdf_release_obj(font->resource);
  if (font->descriptor)
    pdf_release_obj(font->descriptor);
  if (font->reference)
    pdf_release_obj(font->reference);

  font->reference  = NULL;
  font->resource   = NULL;
  font->descriptor = NULL;

  return;
}

static void
pdf_clean_font_struct (pdf_font *font)
{
  if (font) {
    if (font->ident)
      RELEASE(font->ident);
    if (font->map_name)
      RELEASE(font->map_name);
    if (font->fontname)
      RELEASE(font->fontname);
    if (font->usedchars)
      RELEASE(font->usedchars);

    if (font->reference)
      ERROR("pdf_font>> Object not flushed.");
    if (font->resource)
      ERROR("pdf_font> Object not flushed.");
    if (font->descriptor)
      ERROR("pdf_font>> Object not flushed.");

    font->ident     = NULL;
    font->map_name  = NULL;
    font->fontname  = NULL;
    font->usedchars = NULL;
  }

  return;
}

#define CACHE_ALLOC_SIZE 16u

static struct {
  int       count;
  int       capacity;
  pdf_font *fonts;
} font_cache = {
  0, 0, NULL
};

void
pdf_init_fonts (void)
{
  ASSERT(font_cache.fonts == NULL);  

  agl_init_map();
  otl_init_conf();

  CMap_cache_init();
  pdf_init_encodings();

  Type0Font_cache_init();

  font_cache.count    = 0;
  font_cache.capacity = CACHE_ALLOC_SIZE;
  font_cache.fonts    = NEW(font_cache.capacity, pdf_font);
}

#define CHECK_ID(n) do {\
  if ((n) < 0 || (n) >= font_cache.count) {\
    ERROR("Invalid font ID: %d", (n));\
  }\
} while (0)
#define GET_FONT(n)  (&(font_cache.fonts[(n)]))


pdf_obj *
pdf_get_font_reference (int font_id)
{
  pdf_font  *font;

  CHECK_ID(font_id);

  font = GET_FONT(font_id);
  if (font->subtype == PDF_FONT_FONTTYPE_TYPE0) {
    Type0Font *t0font;

    t0font = Type0Font_cache_get(font->font_id);
    return Type0Font_get_resource(t0font);
  } else {
    if (!font->reference) {
      font->reference = pdf_ref_obj(pdf_font_get_resource(font));
    }
  }

  return pdf_link_obj(font->reference);
}

char *
pdf_get_font_usedchars (int font_id)
{
  pdf_font *font;

  CHECK_ID(font_id);

  font = GET_FONT(font_id);
  if (font->subtype == PDF_FONT_FONTTYPE_TYPE0) {
    Type0Font *t0font;

    t0font = Type0Font_cache_get(font->font_id);
    return Type0Font_get_usedchars(t0font);
  } else {
    if (!font->usedchars) {
      font->usedchars = NEW(256, char);
      memset(font->usedchars, 0, 256 * sizeof(char));
    }
    return font->usedchars;
  }
}

int
pdf_get_font_wmode (int font_id)
{
  pdf_font *font;

  CHECK_ID(font_id);

  font = GET_FONT(font_id);
  if (font->subtype == PDF_FONT_FONTTYPE_TYPE0) {
    Type0Font *t0font;

    t0font = Type0Font_cache_get(font->font_id);
    return Type0Font_get_wmode(t0font);
  } else {
    return 0;
  }
}

int
pdf_get_font_subtype (int font_id)
{
  pdf_font *font;

  CHECK_ID(font_id);

  font = GET_FONT(font_id);

  return font->subtype;
}

#if 0
char *
pdf_get_font_fontname (int font_id)
{
  pdf_font *font;

  CHECK_ID(font_id);

  font = GET_FONT(font_id);

  return font->fontname;
}
#endif /* 0 */

int
pdf_get_font_encoding (int font_id)
{
  pdf_font *font;

  CHECK_ID(font_id);

  font = GET_FONT(font_id);

  return font->encoding_id;
}

/* The rule for ToUnicode creation is:
 *
 *  If "tounicode" option is specified in fontmap, use that.
 *  If there is ToUnicode CMap with same name as TFM, use that.
 *  If no "tounicode" option is used and no ToUnicode CMap with
 *  same name as TFM is found, create ToUnicode CMap from glyph
 *  names and AGL file.
 */
static int
try_load_ToUnicode_CMap (pdf_font *font)
{
  pdf_obj     *fontdict;
  pdf_obj     *tounicode;
  const char  *cmap_name = NULL;
  fontmap_rec *mrec; /* Be sure fontmap is still alive here */

  ASSERT(font);

  /* We are using different encoding for Type0 font.
   * This feature is unavailable for them.
   */
  if (font->subtype == PDF_FONT_FONTTYPE_TYPE0)
    return  0;

  ASSERT(font->map_name);

  mrec = pdf_lookup_fontmap_record(font->map_name);
  if (MREC_HAS_TOUNICODE(mrec))
    cmap_name = mrec->opt.tounicode;
  else {
    cmap_name = font->map_name;
  }

  fontdict  = pdf_font_get_resource(font);
  tounicode = pdf_load_ToUnicode_stream(cmap_name);
  if (!tounicode && MREC_HAS_TOUNICODE(mrec))
    WARN("Failed to read ToUnicode mapping \"%s\"...", mrec->opt.tounicode);
  else if (tounicode) {
    if (pdf_obj_typeof(tounicode) != PDF_STREAM)
      ERROR("Object returned by pdf_load_ToUnicode_stream() not stream object! (This must be bug)");
    else if (pdf_stream_length(tounicode) > 0) {
      pdf_add_dict(fontdict,
                   pdf_new_name("ToUnicode"),
                   pdf_ref_obj (tounicode)); /* _FIXME_ */
      if (__verbose)
        MESG("pdf_font>> ToUnicode CMap \"%s\" attached to font id=\"%s\".\n",
             cmap_name, font->map_name);
    }
    pdf_release_obj(tounicode);
  }

  return  0;
}

void
pdf_close_fonts (void)
{
  int  font_id;

  for (font_id = 0;
       font_id < font_cache.count; font_id++) {
    pdf_font  *font;

    font = GET_FONT(font_id);

    if (__verbose) {
      if (font->subtype != PDF_FONT_FONTTYPE_TYPE0) {
	MESG("(%s", pdf_font_get_ident(font));
	if (__verbose > 2 &&
	    !pdf_font_get_flag(font, PDF_FONT_FLAG_NOEMBED)) {
	  MESG("[%s+%s]",
	       pdf_font_get_uniqueTag(font),
	       pdf_font_get_fontname(font));
	} else if (__verbose > 1) {
	  MESG("[%s]",
	       pdf_font_get_fontname(font));
	}
	if (__verbose > 1) {
	  if (pdf_font_get_encoding(font) >= 0) {
	    MESG("[%s]",
		 pdf_encoding_get_name(pdf_font_get_encoding(font)));
	  } else {
	    MESG("[built-in]");
	  }
	}

      }
    }

    /* Must come before load_xxx */
    try_load_ToUnicode_CMap(font);

    /* Type 0 is handled separately... */
    switch (font->subtype) {
    case PDF_FONT_FONTTYPE_TYPE1:
      if (__verbose)
	MESG("[Type1]");
      if (!pdf_font_get_flag(font, PDF_FONT_FLAG_BASEFONT))
	pdf_font_load_type1(font);
      break;
    case PDF_FONT_FONTTYPE_TYPE1C:
      if (__verbose)
	MESG("[Type1C]");
      pdf_font_load_type1c(font);
      break;
    case PDF_FONT_FONTTYPE_TRUETYPE:
      if (__verbose)
	MESG("[TrueType]");
      pdf_font_load_truetype(font);
      break;
    case PDF_FONT_FONTTYPE_TYPE3:
      if (__verbose)
	MESG("[Type3/PK]");
      pdf_font_load_pkfont (font);
      break;
    case PDF_FONT_FONTTYPE_TYPE0:
      break;
    default:
      ERROR("Unknown font type: %d", font->subtype);
      break;
    }

    if (font->encoding_id >= 0 && font->subtype != PDF_FONT_FONTTYPE_TYPE0)
      pdf_encoding_add_usedchars(font->encoding_id, font->usedchars);

    if (__verbose) {
      if (font->subtype != PDF_FONT_FONTTYPE_TYPE0)
	MESG(")");
    }
  }

  pdf_encoding_complete();

  for (font_id = 0; font_id < font_cache.count; font_id++) {
    pdf_font *font = GET_FONT(font_id);

    if (font->encoding_id >= 0 && font->subtype != PDF_FONT_FONTTYPE_TYPE0) {
      pdf_obj *enc_obj = pdf_get_encoding_obj(font->encoding_id);
      pdf_obj *tounicode;

      /* Predefined encodings (and those simplified to them) are embedded
	 as direct objects, but this is purely a matter of taste. */
      if (enc_obj)
        pdf_add_dict(font->resource,
		     pdf_new_name("Encoding"),
		     PDF_OBJ_NAMETYPE(enc_obj) ? pdf_link_obj(enc_obj) : pdf_ref_obj(enc_obj));

      if (!pdf_lookup_dict(font->resource, "ToUnicode")
	  && (tounicode = pdf_encoding_get_tounicode(font->encoding_id)))
	pdf_add_dict(font->resource,
		     pdf_new_name("ToUnicode"), pdf_ref_obj(tounicode));
    } else if (font->subtype == PDF_FONT_FONTTYPE_TRUETYPE) {
      /* encoding_id < 0 means MacRoman here (but not really)
       * We use MacRoman as "default" encoding. */
      pdf_add_dict(font->resource,
                   pdf_new_name("Encoding"),
		   pdf_new_name("MacRomanEncoding"));
    }

    pdf_flush_font(font);
    pdf_clean_font_struct(font);
  }
  RELEASE(font_cache.fonts);
  font_cache.fonts    = NULL;
  font_cache.count    = 0;
  font_cache.capacity = 0;

  Type0Font_cache_close();

  CMap_cache_close();
  pdf_close_encodings();

  otl_close_conf();
  agl_close_map (); /* After encoding */

  return;
}

int
pdf_font_findresource (const char *tex_name,
		       double font_scale, fontmap_rec *mrec)
{
  int          font_id = -1;
  pdf_font    *font;
  int          encoding_id = -1, cmap_id = -1;
  const char  *fontname;

  /*
   * Get appropriate info from map file. (PK fonts at two different
   * point sizes would be looked up twice unecessarily.)
   */
  fontname = mrec ? mrec->font_name : tex_name;
  if (mrec && mrec->enc_name) {
#define MAYBE_CMAP(s) (!strstr((s), ".enc") || strstr((s), ".cmap"))
    if (MAYBE_CMAP(mrec->enc_name)) {
      cmap_id = CMap_cache_find(mrec->enc_name);
      if (cmap_id >= 0) {
	CMap  *cmap;
	int    cmap_type, minbytes;

	cmap      = CMap_cache_get(cmap_id);
	cmap_type = CMap_get_type (cmap);
	minbytes  = CMap_get_profile(cmap, CMAP_PROF_TYPE_INBYTES_MIN);
	/*
	 * Check for output encoding.
	 */
	if (cmap_type != CMAP_TYPE_IDENTITY    &&
	    cmap_type != CMAP_TYPE_CODE_TO_CID &&
	    cmap_type != CMAP_TYPE_TO_UNICODE) {
	  WARN("Only 16-bit encoding supported for output encoding.");
	}
	/*
	 * Turn on map option.
	 */
	if (minbytes == 2 && mrec->opt.mapc < 0) {
	  if (__verbose) {
	    MESG("\n");
	    MESG("pdf_font>> Input encoding \"%s\" requires at least 2 bytes.\n",
		 CMap_get_name(cmap));
	    MESG("pdf_font>> The -m <00> option will be assumed for \"%s\".\n", mrec->font_name);
	  }
	  mrec->opt.mapc = 0; /* _FIXME_ */
	}
      } else if (!strcmp(mrec->enc_name, "unicode")) {
	cmap_id = otf_load_Unicode_CMap(mrec->font_name,
					mrec->opt.index, mrec->opt.otl_tags,
					((mrec->opt.flags & FONTMAP_OPT_VERT) ? 1 : 0));
	if (cmap_id < 0) {
	  cmap_id = t1_load_UnicodeCMap(mrec->font_name, mrec->opt.otl_tags,
					((mrec->opt.flags & FONTMAP_OPT_VERT) ? 1 : 0));
	}
	if (cmap_id < 0)
	  ERROR("Failed to read UCS2/UCS4 TrueType cmap...");
      }
    }
    if (cmap_id < 0) {
      encoding_id = pdf_encoding_findresource(mrec->enc_name);
      if (encoding_id < 0)
	ERROR("Could not find encoding file \"%s\".", mrec->enc_name);
    }
  }

  if (mrec && cmap_id >= 0) {
    /*
     * Composite Font
     */
    int  type0_id, found = 0;

    type0_id = pdf_font_findfont0(mrec->font_name, cmap_id, &mrec->opt);
    if (type0_id < 0) {
      return -1;
    }

    for (font_id = 0;
	 font_id < font_cache.count; font_id++) {
      font = GET_FONT(font_id);
      if (font->subtype == PDF_FONT_FONTTYPE_TYPE0 &&
	  font->font_id == type0_id &&
	  font->encoding_id == cmap_id) {
	found = 1;
	if (__verbose) {
	  MESG("\npdf_font>> Type0 font \"%s\" (cmap_id=%d) found at font_id=%d.\n",
	       mrec->font_name, cmap_id, font_id);
	}
	break;
      }
    }

    if (!found) {
      font_id = font_cache.count;
      if (font_cache.count >= font_cache.capacity) {
	font_cache.capacity += CACHE_ALLOC_SIZE;
	font_cache.fonts     = RENEW(font_cache.fonts, font_cache.capacity, pdf_font);
      }
      font    = GET_FONT(font_id);
      pdf_init_font_struct(font);

#ifdef XETEX
      font->ft_to_gid = Type0Font_get_ft_to_gid(type0_id);
      font->ft_face = mrec->opt.ft_face;
#endif

      font->font_id     = type0_id;
      font->subtype     = PDF_FONT_FONTTYPE_TYPE0;
      font->encoding_id = cmap_id;

      font_cache.count++;

      if (__verbose) {
	MESG("\npdf_font>> Type0 font \"%s\"", fontname);
        MESG(" cmap_id=<%s,%d>", mrec->enc_name, font->encoding_id);
        MESG(" opened at font_id=<%s,%d>.\n", tex_name, font_id);
      }

    }
  } else {
    /*
     * Simple Font - always embed.
     */
    int  found = 0;

    for (font_id = 0;
	 font_id < font_cache.count; font_id++) {
      font = GET_FONT(font_id);
      switch (font->subtype) {
      case PDF_FONT_FONTTYPE_TYPE1:
      case PDF_FONT_FONTTYPE_TYPE1C:
      case PDF_FONT_FONTTYPE_TRUETYPE:
	/* fontname here is font file name.
	 * We must compare both font file name and encoding
	 *
	 * TODO: Embed a font only once if it is used
	 *       with two different encodings
	 */
	if (!strcmp(fontname, font->ident)   &&
	    encoding_id == font->encoding_id) {
          if (mrec && mrec->opt.index == font->index)
            found = 1;
	}
	break;
      case PDF_FONT_FONTTYPE_TYPE3:
	/* There shouldn't be any encoding specified for PK font.
	 * It must be always font's build-in encoding.
	 *
	 * TODO: a PK font with two encodings makes no sense. Change?
         */
	if (!strcmp(fontname, font->ident) &&
	    font_scale == font->point_size) {
	  found = 1;
	}
	break;
      case PDF_FONT_FONTTYPE_TYPE0:
	break;
      default:
	ERROR("Unknown font type: %d", font->subtype);
	break;
      }

      if (found) {
	if (__verbose) {
	  MESG("\npdf_font>> Simple font \"%s\" (enc_id=%d) found at id=%d.\n",
	       fontname, encoding_id, font_id);
	}
	break;
      }
    }


    if (!found) {
      font_id = font_cache.count;
      if (font_cache.count >= font_cache.capacity) {
	font_cache.capacity += CACHE_ALLOC_SIZE;
	font_cache.fonts     = RENEW(font_cache.fonts, font_cache.capacity, pdf_font);
      }

      font = GET_FONT(font_id);

      pdf_init_font_struct(font);

#ifdef XETEX
      font->ft_face = mrec ? mrec->opt.ft_face : NULL;
#endif

      font->point_size  = font_scale;
      font->encoding_id = encoding_id;
      font->ident       = NEW(strlen(fontname) + 1, char);
      strcpy(font->ident, fontname);
      font->map_name    = NEW(strlen(tex_name) + 1, char);
      strcpy(font->map_name, tex_name);
      font->index       = (mrec && mrec->opt.index) ? mrec->opt.index : 0;

      if (pdf_font_open_type1(font) >= 0) {
	font->subtype = PDF_FONT_FONTTYPE_TYPE1;
      } else if (pdf_font_open_type1c(font) >= 0) {
	font->subtype = PDF_FONT_FONTTYPE_TYPE1C;
      } else if (pdf_font_open_truetype(font) >= 0) {
	font->subtype = PDF_FONT_FONTTYPE_TRUETYPE;
      } else if (pdf_font_open_pkfont(font) >= 0) {
	font->subtype = PDF_FONT_FONTTYPE_TYPE3;
      } else {
	pdf_clean_font_struct(font);
	return -1;
      }

      font_cache.count++;

      if (__verbose) {
	MESG("\npdf_font>> Simple font \"%s\"", fontname);
        MESG(" enc_id=<%s,%d>",
             (mrec && mrec->enc_name) ? mrec->enc_name : "builtin", font->encoding_id);
        MESG(" opened at font_id=<%s,%d>.\n", tex_name, font_id);
      }
    }
  }

  return  font_id;
}

int 
pdf_font_is_in_use (pdf_font *font)
{
  ASSERT(font);

  return ((font->reference) ? 1 : 0);
}

#ifdef XETEX
FT_Face
pdf_font_get_ft_face (pdf_font *font)
{
  ASSERT(font);

  return font->ft_face;
}

unsigned short *
pdf_get_font_ft_to_gid (int font_id)
{
  pdf_font *font;

  CHECK_ID(font_id);

  font = GET_FONT(font_id);

  return font->ft_to_gid;
}
#endif

int
pdf_font_get_index (pdf_font *font)
{
  ASSERT(font);

  return font->index;
}

char *
pdf_font_get_ident (pdf_font *font)
{
  ASSERT(font);

  return font->ident;
}

char *
pdf_font_get_mapname (pdf_font *font)
{
  ASSERT(font);

  return font->map_name;
}

char *
pdf_font_get_fontname (pdf_font *font)
{
  ASSERT(font);

  return font->fontname;
}

pdf_obj *
pdf_font_get_resource (pdf_font *font)
{
  ASSERT(font);

  if (!font->resource) {
    font->resource = pdf_new_dict();
    pdf_add_dict(font->resource,
		 pdf_new_name("Type"),      pdf_new_name("Font"));
    switch (font->subtype) {
    case PDF_FONT_FONTTYPE_TYPE1:
    case PDF_FONT_FONTTYPE_TYPE1C:
      pdf_add_dict(font->resource,
		   pdf_new_name("Subtype"), pdf_new_name("Type1"));
      break;
    case PDF_FONT_FONTTYPE_TYPE3:
      pdf_add_dict(font->resource,
		   pdf_new_name("Subtype"), pdf_new_name("Type3"));
      break;
    case PDF_FONT_FONTTYPE_TRUETYPE:
      pdf_add_dict(font->resource,
		   pdf_new_name("Subtype"), pdf_new_name("TrueType"));
      break;
    default:
      break;
    }
  }

  return font->resource;
}

pdf_obj *
pdf_font_get_descriptor (pdf_font *font)
{
  ASSERT(font);

  if (!font->descriptor) {
    font->descriptor = pdf_new_dict();
    pdf_add_dict(font->descriptor,
		 pdf_new_name("Type"), pdf_new_name("FontDescriptor"));
  }

  return font->descriptor;
}

char *
pdf_font_get_usedchars (pdf_font *font)
{
  ASSERT(font);

  return font->usedchars;
}

int
pdf_font_get_encoding (pdf_font *font)
{
  ASSERT(font);

  return font->encoding_id;
}

int
pdf_font_get_flag (pdf_font *font, int mask)
{
  ASSERT(font);

  return ((font->flags & mask) ? 1 : 0);
}

#if 0
int
pdf_font_get_flags (pdf_font *font)
{
  ASSERT(font);

  return font->flags;
}
#endif /* 0 */

double
pdf_font_get_param (pdf_font *font, int param_type)
{
  double param = 0.0;

  ASSERT(font);

  switch (param_type) {
  case PDF_FONT_PARAM_DESIGN_SIZE:
    param = font->design_size;
    break;
  case PDF_FONT_PARAM_POINT_SIZE:
    param = font->point_size;
    break;
  default:
    break;
  }

  return param;
}

char *
pdf_font_get_uniqueTag (pdf_font *font)
{
  ASSERT(font);

  if (font->uniqueID[0] == '\0') {
    pdf_font_make_uniqueTag(font->uniqueID);
  }

  return font->uniqueID;
}

int
pdf_font_set_fontname (pdf_font *font, const char *fontname)
{
  ASSERT(font && fontname);

  if (strlen(fontname) > PDF_NAME_LEN_MAX) {
    ERROR("Unexpected error...");
    return -1;
  }
  if (font->fontname) {
    RELEASE(font->fontname);
  }
  font->fontname = NEW(strlen(fontname)+1, char);
  strcpy(font->fontname, fontname);

  return 0;
}

int
pdf_font_set_subtype (pdf_font *font, int subtype)
{
  ASSERT(font);

  font->subtype = subtype;

  return 0;
}

int
pdf_font_set_flags (pdf_font *font, int flags)
{
  ASSERT(font);

  font->flags |= flags;

  return 0;
}

