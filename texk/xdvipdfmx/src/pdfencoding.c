/*  $Header: /home/cvsroot/dvipdfmx/src/pdfencoding.c,v 1.11 2008/05/22 11:03:09 chofchof Exp $
    
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2008 by Jin-Hwan Cho, Matthias Franz, and Shunsaku Hirata,
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "mem.h"
#include "error.h"
#include "dpxutil.h"

#include "pdfparse.h"
#include "pdfobj.h"

#include "dpxfile.h"

#include "pdfencoding.h"

static int      is_similar_charset (char **encoding, const char **encoding2);
static pdf_obj *make_encoding_differences (char **encoding, char **baseenc,
					   const char *is_used);

static unsigned char verbose = 0;

static const char *MacRomanEncoding[256];
static const char *MacExpertEncoding[256];
static const char *WinAnsiEncoding[256];
#if 0
static const char *StandardEncoding[256];
static const char *ISOLatin1Encoding[256];
#endif

void
pdf_encoding_set_verbose (void)
{
  verbose++;
}

/*
 * ident:  File name, e.g., 8a.enc.
 * name:   Name of encoding, StandardEncoding, TeXBase1Encoding, ...
 * glyphs: List of encoded glyphs (name).
 * flags:
 *   IS_PREDEFINED:
 *     Encoding is one of the MacRomanEncoding, MacExpertEncoding, and
 *     WinAnsiEncoding.
 *   FLAG_USED_BY_TYPE3:
 *     Encoding is used by a Type 3 font.
 */
#define FLAG_IS_PREDEFINED  (1 << 0)
#define FLAG_USED_BY_TYPE3  (1 << 1)

typedef struct pdf_encoding
{
  char     *ident;

  char     *enc_name;
  int       flags;
  char     *glyphs[256];     /* ".notdef" must be represented as NULL */
  char      is_used[256];

  struct pdf_encoding *baseenc;
  pdf_obj  *tounicode;

  pdf_obj  *resource;
} pdf_encoding;

static int      pdf_encoding_new_encoding (const char *enc_name,
					   const char *ident,
					   const char **encoding_vec,
					   char *baseenc_name,
					   int flags);

static void
pdf_init_encoding_struct (pdf_encoding *encoding)
{
  ASSERT(encoding);

  encoding->ident    = NULL;

  encoding->enc_name = NULL;

  memset(encoding->glyphs,  0, 256*sizeof(char *));
  memset(encoding->is_used, 0, 256);

  encoding->tounicode = NULL;

  encoding->baseenc   = NULL;
  encoding->resource  = NULL;

  encoding->flags     = 0;

  return;
}

/* Creates the PDF Encoding entry for the encoding.
 * If baseenc is non-null, it is used as BaseEncoding entry.
 */
static pdf_obj *
create_encoding_resource (pdf_encoding *encoding, pdf_encoding *baseenc)
{
  pdf_obj *differences;
  ASSERT(encoding);
  ASSERT(!encoding->resource);

  differences = make_encoding_differences(encoding->glyphs,
					  baseenc ? baseenc->glyphs : NULL,
					  encoding->is_used);
  
  if (differences) {
    pdf_obj *resource = pdf_new_dict();
    if (baseenc)
      pdf_add_dict(resource, pdf_new_name("BaseEncoding"),
		   pdf_link_obj(baseenc->resource));
    pdf_add_dict(resource, pdf_new_name("Differences"),  differences);
    return resource; 
  } else {
    /* Fix a bug with the MinionPro package using MnSymbol fonts
     * in its virtual fonts:
     *
     * Some font may have font_id even if no character is used.
     * For example, suppose that a virtual file A.vf uses two
     * other fonts, B and C. Even if only characters of B are used
     * in a DVI document, C will have font_id too.
     * In this case, both baseenc and differences can be NULL.
     *
     * Actually these fonts will be ignored in pdffont.c.
     */
    return baseenc ? pdf_link_obj(baseenc->resource) : NULL;
  }
}

static void
pdf_flush_encoding (pdf_encoding *encoding)
{
  ASSERT(encoding);

  if (encoding->resource) {
    pdf_release_obj(encoding->resource);
    encoding->resource  = NULL;
  }
  if (encoding->tounicode) {
    pdf_release_obj(encoding->tounicode);
    encoding->tounicode = NULL;
  }

  return;
}

static void
pdf_clean_encoding_struct (pdf_encoding *encoding)
{
  int   code;

  ASSERT(encoding);

  if (encoding->resource)
    ERROR("Object not flushed.");

  if (encoding->tounicode)
    pdf_release_obj(encoding->tounicode);
  if (encoding->ident)
    RELEASE(encoding->ident);
  if (encoding->enc_name)
    RELEASE(encoding->enc_name);

  encoding->ident    = NULL;
  encoding->enc_name = NULL;

  for (code = 0; code < 256; code++) {
    if (encoding->glyphs[code])
      RELEASE(encoding->glyphs[code]);
    encoding->glyphs[code] = NULL;
  }
  encoding->ident    = NULL;
  encoding->enc_name = NULL;

  return;
}

#if 0
static int CDECL
glycmp (const void *pv1, const void *pv2)
{
  char *v1, *v2;

  v1 = (char *) pv1;
  v2 = *((char **) pv2);

  return strcmp(v1, v2);
}
#endif

static int
is_similar_charset (char **enc_vec, const char **enc_vec2)
{
  int   code, same = 0;

  for (code = 0; code < 256; code++)
    if (!(enc_vec[code] && strcmp(enc_vec[code], enc_vec2[code]))
	&& ++same >= 64)
      /* is 64 a good level? */
      return 1;

  return 0; 
}

/* Creates a PDF Differences array for the encoding, based on the
 * base encoding baseenc (if not NULL). Only character codes which
 * are actually used in the document are considered.
 */
static pdf_obj *
make_encoding_differences (char **enc_vec, char **baseenc, const char *is_used)
{
  pdf_obj *differences = NULL;
  int      code, count = 0;
  int      skipping = 1;

  ASSERT(enc_vec);

  /*
   *  Write all entries (except .notdef) if baseenc is unknown.
   *  If is_used is given, write only used entries.
   */
  differences = pdf_new_array();
  for (code = 0; code < 256; code++) {
    /* We skip NULL (= ".notdef"). Any character code mapped to ".notdef"
     * glyph should not be used in the document.
     */
    if ((is_used && !is_used[code]) || !enc_vec[code])
      skipping = 1;
    else if (!baseenc || !baseenc[code] ||
             strcmp(baseenc[code], enc_vec[code]) != 0) {
      /*
       * Difference found.
       */
      if (skipping)
        pdf_add_array(differences, pdf_new_number(code));
      pdf_add_array(differences,   pdf_new_name(enc_vec[code]));
      skipping = 0;
      count++;
    } else
      skipping = 1;
  }

  /*
   * No difference found. Some PDF viewers can't handle differences without
   * any differences. We return NULL.
   */
  if (count == 0) {
    pdf_release_obj(differences);
    differences = NULL;
  }

  return differences;
}

static int
load_encoding_file (const char *filename)
{
  FILE    *fp;
  pdf_obj *enc_name = NULL;
  pdf_obj *encoding_array = NULL;
  char    *wbuf, *p, *endptr;
  char    *enc_vec[256];
  int      code, fsize, enc_id;

  if (!filename)
    return -1;

  if (verbose) {
    MESG("(Encoding:%s", filename);
  }

  fp = DPXFOPEN(filename, DPX_RES_TYPE_ENC);
  if (!fp)
    return -1;
  /*
   * file_size do seek_end witout saving current position and
   * do rewind.
   */
  fsize = file_size(fp);

  wbuf = NEW(fsize + 1, char); 
  fread(wbuf, sizeof(char), fsize, fp);
  DPXFCLOSE(fp);

  p        = wbuf;
  endptr   = wbuf + fsize;
  p[fsize] = '\0';

  skip_white(&p, endptr);

  /*
   * Skip comment lines.
   */
  while (p < endptr && p[0] == '%') {
    skip_line (&p, endptr);
    skip_white(&p, endptr);
  }
  if (p[0] == '/')
    enc_name = parse_pdf_name(&p, endptr);

  skip_white(&p, endptr);
  encoding_array = parse_pdf_array(&p, endptr, NULL);
  RELEASE(wbuf);
  if (!encoding_array) {
    if (enc_name)
      pdf_release_obj(enc_name);
    return -1;
  }

  for (code = 0; code < 256; code++) {
    enc_vec[code] = pdf_name_value(pdf_get_array(encoding_array, code));
  }
  enc_id = pdf_encoding_new_encoding(enc_name ? pdf_name_value(enc_name) : NULL,
				     filename, (const char **) enc_vec, NULL, 0);

  if (enc_name) {
    if (verbose > 1)
      MESG("[%s]", pdf_name_value(enc_name));
    pdf_release_obj(enc_name);
  }
  pdf_release_obj(encoding_array);

  if (verbose) MESG(")");

  return enc_id;
}

#define CHECK_ID(n) do { \
  if ((n) < 0 || (n) >= enc_cache.count) { \
     ERROR("Invalid encoding id: %d", (n)); \
  } \
} while (0)

#define CACHE_ALLOC_SIZE 16u

struct {
  int           count;
  int           capacity;
  pdf_encoding *encodings;
} enc_cache = {
  0, 0, NULL
};

void
pdf_init_encodings (void)
{
  enc_cache.count     = 0;
  enc_cache.capacity  = 3;
  enc_cache.encodings = NEW(enc_cache.capacity, pdf_encoding);

  /*
   * PDF Predefined Encodings
   */
  pdf_encoding_new_encoding("WinAnsiEncoding", "WinAnsiEncoding",
			    WinAnsiEncoding, NULL, FLAG_IS_PREDEFINED);
  pdf_encoding_new_encoding("MacRomanEncoding", "MacRomanEncoding",
			    MacRomanEncoding, NULL, FLAG_IS_PREDEFINED);
  pdf_encoding_new_encoding("MacExpertEncoding", "MacExpertEncoding",
			    MacExpertEncoding, NULL, FLAG_IS_PREDEFINED);

  return;
}

/*
 * The original dvipdfm describes as:
 *
 *  Some software doesn't like BaseEncoding key (e.g., FastLane) 
 *  so this code is commented out for the moment.  It may reemerge in the
 *  future
 *
 * and the line for BaseEncoding is commented out.
 *
 * I'm not sure why this happens. But maybe BaseEncoding key causes problems
 * when the font is Symbol font or TrueType font.
 */

static int
pdf_encoding_new_encoding (const char *enc_name, const char *ident,
			   const char **encoding_vec,
			   char *baseenc_name, int flags)
{
  int      enc_id, code;

  pdf_encoding *encoding;

  enc_id   = enc_cache.count;
  if (enc_cache.count++ >= enc_cache.capacity) {
    enc_cache.capacity += 16;
    enc_cache.encodings = RENEW(enc_cache.encodings,
                                enc_cache.capacity,  pdf_encoding);
  }
  encoding = &enc_cache.encodings[enc_id];

  pdf_init_encoding_struct(encoding);

  encoding->ident = NEW(strlen(ident)+1, char);
  strcpy(encoding->ident, ident);
  encoding->enc_name  = NEW(strlen(enc_name)+1, char);
  strcpy(encoding->enc_name, enc_name);

  encoding->flags = flags;

  for (code = 0; code < 256; code++)
    if (encoding_vec[code] && strcmp(encoding_vec[code], ".notdef")) {
      encoding->glyphs[code] = NEW(strlen(encoding_vec[code])+1, char);
      strcpy(encoding->glyphs[code], encoding_vec[code]);
    }

  if (!baseenc_name && !(flags & FLAG_IS_PREDEFINED)
      && is_similar_charset(encoding->glyphs, WinAnsiEncoding)) {
    /* Dvipdfmx default setting. */
    baseenc_name = "WinAnsiEncoding";
  }

  /* TODO: make base encoding configurable */
  if (baseenc_name) {
    int baseenc_id = pdf_encoding_findresource(baseenc_name);
    if (baseenc_id < 0 || !pdf_encoding_is_predefined(baseenc_id))
      ERROR("Illegal base encoding %s for encoding %s\n",
	    baseenc_name, encoding->enc_name);
    encoding->baseenc = &enc_cache.encodings[baseenc_id];
  }

  if (flags & FLAG_IS_PREDEFINED)
    encoding->resource = pdf_new_name(encoding->enc_name);

  return enc_id;
}

/* Creates Encoding resource and ToUnicode CMap 
 * for all non-predefined encodings.
 */
void pdf_encoding_complete ()
{
  int  enc_id;

  for (enc_id = 0; enc_id < enc_cache.count; enc_id++) {
    if (!pdf_encoding_is_predefined(enc_id)) {
      pdf_encoding *encoding = &enc_cache.encodings[enc_id];
      /* Section 5.5.4 of the PDF 1.5 reference says that the encoding
       * of a Type 3 font must be completely described by a Differences
       * array, but implementation note 56 explains that this is rather
       * an incorrect implementation in Acrobat 4 and earlier. Hence,
       * we do use a base encodings for PDF versions >= 1.3.
       */
      int with_base = !(encoding->flags & FLAG_USED_BY_TYPE3)
	              || pdf_get_version() >= 4;
      ASSERT(!encoding->resource);
      encoding->resource = create_encoding_resource(encoding,
						    with_base ? encoding->baseenc : NULL);
      ASSERT(!encoding->tounicode);
      encoding->tounicode = pdf_create_ToUnicode_CMap(encoding->enc_name,
						      encoding->glyphs,
						      encoding->is_used);
    }
  }
}

void
pdf_close_encodings (void)
{
  int  enc_id;

  if (enc_cache.encodings) {
    for (enc_id = 0; enc_id < enc_cache.count; enc_id++) {
      pdf_encoding *encoding;

      encoding = &enc_cache.encodings[enc_id];
      if (encoding) {
        pdf_flush_encoding(encoding);
        pdf_clean_encoding_struct(encoding);
      }
    }
    RELEASE(enc_cache.encodings);
  }
  enc_cache.encodings = NULL;
  enc_cache.count     = 0;
  enc_cache.capacity  = 0;
}

int
pdf_encoding_findresource (char *enc_name)
{
  int           enc_id;
  pdf_encoding *encoding;

  ASSERT(enc_name);
  for (enc_id = 0; enc_id < enc_cache.count; enc_id++) {
    encoding = &enc_cache.encodings[enc_id];
    if (encoding->ident &&
        !strcmp(enc_name, encoding->ident))
      return enc_id;
    else if (encoding->enc_name &&
             !strcmp(enc_name, encoding->enc_name))
      return enc_id;
  }

  return load_encoding_file(enc_name);
}


/*
 * Pointer will change if other encoding is loaded...
 */

char **
pdf_encoding_get_encoding (int enc_id)
{
  pdf_encoding *encoding;

  CHECK_ID(enc_id);

  encoding = &enc_cache.encodings[enc_id];

  return encoding->glyphs;
}

pdf_obj *
pdf_get_encoding_obj (int enc_id)
{
  pdf_encoding *encoding;

  CHECK_ID(enc_id);

  encoding = &enc_cache.encodings[enc_id];

  return encoding->resource;
}

int
pdf_encoding_is_predefined (int enc_id)
{
  pdf_encoding *encoding;

  CHECK_ID(enc_id);

  encoding = &enc_cache.encodings[enc_id];

  return (encoding->flags & FLAG_IS_PREDEFINED) ? 1 : 0;
}

void
pdf_encoding_used_by_type3 (int enc_id)
{
  pdf_encoding *encoding;

  CHECK_ID(enc_id);

  encoding = &enc_cache.encodings[enc_id];

  encoding->flags |= FLAG_USED_BY_TYPE3;
}


char *
pdf_encoding_get_name (int enc_id)
{
  pdf_encoding *encoding;

  CHECK_ID(enc_id);

  encoding = &enc_cache.encodings[enc_id];

  return encoding->enc_name;
}

/* CSI_UNICODE */
#include "cid.h"

#include "cmap.h"
#include "cmap_read.h"
#include "cmap_write.h"

#include "agl.h"

#define WBUF_SIZE 1024
static unsigned char wbuf[WBUF_SIZE];
static unsigned char range_min[1] = {0x00u};
static unsigned char range_max[1] = {0xFFu};

void
pdf_encoding_add_usedchars (int encoding_id, const char *is_used)
{
  pdf_encoding *encoding;
  int code;

  CHECK_ID(encoding_id);

  if (!is_used || pdf_encoding_is_predefined(encoding_id))
    return;

  encoding = &enc_cache.encodings[encoding_id];

  for (code = 0; code <= 0xff; code++)
    encoding->is_used[code] |= is_used[code];
}

pdf_obj *
pdf_encoding_get_tounicode (int encoding_id)
{
  CHECK_ID(encoding_id);

  return enc_cache.encodings[encoding_id].tounicode;
}


/* Creates a ToUnicode CMap. An empty CMap is replaced by NULL.
 *
 * For PDF <= 1.4 a complete CMap is created unless all character codes
 * are predefined in PDF. For PDF >= 1.5 only those character codes which
 * are not predefined appear in the CMap.
 *
 * Note: The PDF 1.4 reference is not consistent: Section 5.9 describes
 * the Unicode mapping of PDF 1.3 and Section 9.7.2 (in the context of
 * Tagged PDF) the one of PDF 1.5.
 */
pdf_obj *
pdf_create_ToUnicode_CMap (const char *enc_name,
                           char **enc_vec, const char *is_used)
{
  pdf_obj  *stream;
  CMap     *cmap;
  int       code, all_predef;
  char     *cmap_name;
  unsigned char *p, *endptr;

  ASSERT(enc_name && enc_vec);

  cmap_name = NEW(strlen(enc_name)+strlen("-UTF16")+1, char);
  sprintf(cmap_name, "%s-UTF16", enc_name);

  cmap = CMap_new();
  CMap_set_name (cmap, cmap_name);
  CMap_set_type (cmap, CMAP_TYPE_TO_UNICODE);
  CMap_set_wmode(cmap, 0);

  CMap_set_CIDSysInfo(cmap, &CSI_UNICODE);

  CMap_add_codespacerange(cmap, range_min, range_max, 1);

  all_predef = 1;
  for (code = 0; code <= 0xff; code++) {
    if (is_used && !is_used[code])
      continue;

    if (enc_vec[code]) {
      long   len;
      int    fail_count = 0;
      agl_name *agln = agl_lookup_list(enc_vec[code]);
      /* Adobe glyph naming conventions are not used by viewers,
       * hence even ligatures (e.g, "f_i") must be explicitly defined
       */
      if (pdf_get_version() < 5 || !agln || !agln->is_predef) {
        wbuf[0] = (code & 0xff);
        p      = wbuf + 1;
        endptr = wbuf + WBUF_SIZE;
        len = agl_sput_UTF16BE(enc_vec[code], &p, endptr, &fail_count);
        if (len >= 1 && !fail_count) {
          CMap_add_bfchar(cmap, wbuf, 1, wbuf + 1, len);
	  all_predef &= agln && agln->is_predef;
        }
      }
    }
  }

  stream = all_predef ? NULL : CMap_create_stream(cmap, 0);

  CMap_release(cmap);
  RELEASE(cmap_name);

  return stream;
}


pdf_obj *
pdf_load_ToUnicode_stream (const char *ident)
{
  pdf_obj *stream = NULL;
  CMap    *cmap;
  FILE    *fp;

  if (!ident)
    return NULL;

  fp = DPXFOPEN(ident, DPX_RES_TYPE_CMAP);
  if (!fp)
    return NULL;
  else if (CMap_parse_check_sig(fp) < 0) {
    DPXFCLOSE(fp);
    return NULL;
  }

  cmap = CMap_new();
  if (CMap_parse(cmap, fp) < 0) {
    WARN("Reading CMap file \"%s\" failed.", ident);
  } else {
    if (verbose) {
      MESG("(CMap:%s)", ident);
    }
    stream = CMap_create_stream(cmap, 0);
    if (!stream) {
      WARN("Failed to creat ToUnicode CMap stream for \"%s\".", ident);
    }
  }
  CMap_release(cmap);
  DPXFCLOSE(fp);

  return  stream;
}


static const char *
MacRomanEncoding[256] = {
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  "space", "exclam",  "quotedbl", "numbersign",
  "dollar", "percent", "ampersand", "quotesingle",
  "parenleft", "parenright", "asterisk", "plus",
  "comma", "hyphen", "period", "slash",
  "zero", "one", "two", "three",
  "four", "five", "six", "seven",
  "eight", "nine", "colon", "semicolon",
  "less", "equal", "greater", "question",
  "at", "A", "B", "C",
  "D", "E", "F", "G", "H",
  "I", "J", "K", "L",
  "M", "N", "O", "P",
  "Q", "R", "S", "T",
  "U", "V", "W", "X",
  "Y", "Z", "bracketleft", "backslash",
  "bracketright", "asciicircum", "underscore",
  "grave", "a", "b", "c",
  "d", "e", "f", "g",
  "h", "i", "j", "k",
  "l", "m", "n", "o",
  "p", "q", "r", "s",
  "t", "u", "v", "w",
  "x", "y", "z", "braceleft",
  "bar", "braceright", "asciitilde", ".notdef",
  "Adieresis", "Aring", "Ccedilla", "Eacute",
  "Ntilde", "Odieresis", "Udieresis", "aacute",
  "agrave", "acircumflex", "adieresis", "atilde",
  "aring", "ccedilla", "eacute", "egrave",
  "ecircumflex", "edieresis", "iacute", "igrave",
  "icircumflex", "idieresis", "ntilde", "oacute",
  "ograve", "ocircumflex", "odieresis", "otilde",
  "uacute", "ugrave", "ucircumflex", "udieresis",
  "dagger", "degree", "cent", "sterling",
  "section", "bullet", "paragraph", "germandbls",
  "registered", "copyright", "trademark", "acute",
  "dieresis", "notequal", "AE", "Oslash",
  "infinity", "plusminus", "lessequal", "greaterequal",
  "yen", "mu", "partialdiff", "summation",
  "product", "pi", "integral", "ordfeminine",
  "ordmasculine", "Omega", "ae", "oslash",
  "questiondown", "exclamdown", "logicalnot", "radical",
  "florin", "approxequal", "Delta", "guillemotleft",
  "guillemotright", "ellipsis", "space", "Agrave",
  "Atilde", "Otilde", "OE", "oe",
  "endash", "emdash", "quotedblleft", "quotedblright",
  "quoteleft", "quoteright", "divide", "lozenge",
  "ydieresis", "Ydieresis", "fraction", "currency",
  "guilsinglleft", "guilsinglright", "fi", "fl",
  "daggerdbl", "periodcentered", "quotesinglbase", "quotedblbase",
  "perthousand", "Acircumflex", "Ecircumflex", "Aacute",
  "Edieresis", "Egrave", "Iacute", "Icircumflex",
  "Idieresis", "Igrave", "Oacute", "Ocircumflex",
  "apple", "Ograve", "Uacute", "Ucircumflex",
  "Ugrave", "dotlessi", "circumflex", "tilde",
  "macron", "breve", "dotaccent", "ring",
  "cedilla", "hungarumlaut", "ogonek", "caron"
};

static const char *
MacExpertEncoding[256] = {
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  "space", "exclamsmall", "Hungarumlautsmall", "centoldstyle",
  "dollaroldstyle", "dollarsuperior", "ampersandsmall", "Acutesmall",
  "parenleftsuperior", "parenrightsuperior", "twodotenleader", "onedotenleader",
  "comma", "hyphen", "period", "fraction",
  "zerooldstyle", "oneoldstyle", "twooldstyle", "threeoldstyle",
  "fouroldstyle", "fiveoldstyle", "sixoldstyle", "sevenoldstyle",
  "eightoldstyle", "nineoldstyle", "colon", "semicolon",
  ".notdef", "threequartersemdash", ".notdef", "questionsmall",
  ".notdef", ".notdef", ".notdef", ".notdef",
  "Ethsmall", ".notdef", ".notdef", "onequarter",
  "onehalf", "threequarters", "oneeighth", "threeeighths",
  "fiveeighths", "seveneighths", "onethird", "twothirds",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", "ff", "fi",
  "fl", "ffi", "ffl", "parenleftinferior",
  ".notdef", "parenrightinferior", "Circumflexsmall", "hypheninferior",
  "Gravesmall", "Asmall", "Bsmall", "Csmall",
  "Dsmall", "Esmall", "Fsmall", "Gsmall",
  "Hsmall", "Ismall", "Jsmall", "Ksmall",
  "Lsmall", "Msmall", "Nsmall", "Osmall",
  "Psmall", "Qsmall", "Rsmall", "Ssmall",
  "Tsmall", "Usmall", "Vsmall", "Wsmall",
  "Xsmall", "Ysmall", "Zsmall", "colonmonetary",
  "onefitted", "rupiah", "Tildesmall", ".notdef",
  ".notdef", "asuperior", "centsuperior", ".notdef",
  ".notdef", ".notdef", ".notdef", "Aacutesmall",
  "Agravesmall", "Acircumflexsmall", "Adieresissmall", "Atildesmall",
  "Aringsmall", "Ccedillasmall", "Eacutesmall", "Egravesmall",
  "Ecircumflexsmall", "Edieresissmall", "Iacutesmall", "Igravesmall",
  "Icircumflexsmall", "Idieresissmall", "Ntildesmall", "Oacutesmall",
  "Ogravesmall", "Ocircumflexsmall", "Odieresissmall", "Otildesmall",
  "Uacutesmall", "Ugravesmall", "Ucircumflexsmall", "Udieresissmall",
  ".notdef", "eightsuperior", "fourinferior", "threeinferior",
  "sixinferior", "eightinferior", "seveninferior", "Scaronsmall",
  ".notdef", "centinferior", "twoinferior", ".notdef",
  "Dieresissmall", ".notdef", "Caronsmall", "osuperior",
  "fiveinferior", ".notdef", "commainferior", "periodinferior",
  "Yacutesmall", ".notdef", "dollarinferior", ".notdef",
  ".notdef", "Thornsmall", ".notdef", "nineinferior",
  "zeroinferior", "Zcaronsmall", "AEsmall", "Oslashsmall",
  "questiondownsmall", "oneinferior", "Lslashsmall", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", "Cedillasmall", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", "OEsmall",
  "figuredash", "hyphensuperior", ".notdef", ".notdef",
  ".notdef", ".notdef", "exclamdownsmall", ".notdef",
  "Ydieresissmall", ".notdef", "onesuperior", "twosuperior",
  "threesuperior", "foursuperior", "fivesuperior", "sixsuperior",
  "sevensuperior", "ninesuperior", "zerosuperior", ".notdef",
  "esuperior", "rsuperior", "tsuperior", ".notdef",
  ".notdef", "isuperior", "ssuperior", "dsuperior",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", "lsuperior", "Ogoneksmall", "Brevesmall",
  "Macronsmall", "bsuperior", "nsuperior", "msuperior",
  "commasuperior", "periodsuperior", "Dotaccentsmall", "Ringsmall",
  ".notdef", ".notdef", ".notdef", ".notdef"
};

static const char *
WinAnsiEncoding[256] = {
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  "space", "exclam", "quotedbl", "numbersign",
  "dollar", "percent", "ampersand", "quotesingle",
  "parenleft", "parenright", "asterisk", "plus",
  "comma", "hyphen", "period", "slash",
  "zero", "one", "two", "three",
  "four", "five", "six", "seven",
  "eight", "nine", "colon", "semicolon",
  "less", "equal", "greater", "question",
  "at", "A", "B", "C",
  "D", "E", "F", "G",
  "H", "I", "J", "K",
  "L", "M", "N", "O",
  "P", "Q", "R", "S",
  "T", "U", "V", "W",
  "X", "Y", "Z", "bracketleft",
  "backslash", "bracketright", "asciicircum", "underscore",
  "grave", "a", "b", "c",
  "d", "e", "f", "g",
  "h", "i", "j", "k",
  "l", "m", "n", "o",
  "p", "q", "r", "s",
  "t", "u", "v", "w",
  "x", "y", "z", "braceleft",
  "bar", "braceright", "asciitilde", "bullet",
  "Euro", "bullet", "quotesinglbase", "florin",
  "quotedblbase", "ellipsis", "dagger", "daggerdbl",
  "circumflex", "perthousand", "Scaron", "guilsinglleft",
  "OE", "bullet", "Zcaron", "bullet",
  "bullet", "quoteleft", "quoteright", "quotedblleft",
  "quotedblright", "bullet", "endash", "emdash",
  "tilde", "trademark", "scaron", "guilsinglright",
  "oe", "bullet", "zcaron", "Ydieresis",
  "space", "exclamdown", "cent", "sterling",
  "currency", "yen", "brokenbar", "section",
  "dieresis", "copyright", "ordfeminine", "guillemotleft",
  "logicalnot", "hyphen", "registered", "macron",
  "degree", "plusminus", "twosuperior", "threesuperior",
  "acute", "mu", "paragraph", "periodcentered",
  "cedilla", "onesuperior", "ordmasculine", "guillemotright",
  "onequarter", "onehalf", "threequarters", "questiondown",
  "Agrave", "Aacute", "Acircumflex", "Atilde",
  "Adieresis", "Aring", "AE", "Ccedilla",
  "Egrave", "Eacute", "Ecircumflex", "Edieresis",
  "Igrave", "Iacute", "Icircumflex", "Idieresis",
  "Eth", "Ntilde", "Ograve", "Oacute",
  "Ocircumflex", "Otilde", "Odieresis", "multiply",
  "Oslash", "Ugrave", "Uacute", "Ucircumflex",
  "Udieresis", "Yacute", "Thorn", "germandbls",
  "agrave", "aacute", "acircumflex", "atilde",
  "adieresis", "aring", "ae", "ccedilla",
  "egrave", "eacute", "ecircumflex", "edieresis",
  "igrave", "iacute", "icircumflex", "idieresis",
  "eth", "ntilde", "ograve", "oacute",
  "ocircumflex", "otilde", "odieresis", "divide",
  "oslash", "ugrave", "uacute", "ucircumflex",
  "udieresis", "yacute", "thorn", "ydieresis"
};

#if 0
static const char *
StandardEncoding[256] = {
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  "space", "exclam", "quotedbl", "numbersign",
  "dollar", "percent", "ampersand", "quoteright",
  "parenleft", "parenright", "asterisk", "plus",
  "comma", "hyphen", "period", "slash",
  "zero", "one", "two", "three",
  "four", "five", "six", "seven",
  "eight", "nine", "colon", "semicolon",
  "less", "equal", "greater", "question",
  "at", "A", "B", "C",
  "D", "E", "F", "G",
  "H", "I", "J", "K",
  "L", "M", "N", "O",
  "P", "Q", "R", "S",
  "T", "U", "V", "W",
  "X", "Y", "Z", "bracketleft",
  "backslash", "bracketright", "asciicircum", "underscore",
  "quoteleft", "a", "b", "c",
  "d", "e", "f", "g",
  "h", "i", "j", "k",
  "l", "m", "n", "o",
  "p", "q", "r", "s",
  "t", "u", "v", "w",
  "x", "y", "z", "braceleft",
  "bar", "braceright", "asciitilde", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", "exclamdown", "cent", "sterling",
  "fraction", "yen", "florin", "section",
  "currency", "quotesingle", "quotedblleft", "guillemotleft",
  "guilsinglleft", "guilsinglright", "fi", "fl",
  ".notdef", "endash", "dagger", "daggerdbl",
  "periodcentered", ".notdef", "paragraph", "bullet",
  "quotesinglbase", "quotedblbase", "quotedblright", "guillemotright",
  "ellipsis", "perthousand", ".notdef", "questiondown",
  ".notdef", "grave", "acute", "circumflex",
  "tilde", "macron", "breve", "dotaccent",
  "dieresis", ".notdef", "ring", "cedilla",
  ".notdef", "hungarumlaut", "ogonek", "caron",
  "emdash", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", "AE", ".notdef", "ordfeminine",
  ".notdef", ".notdef", ".notdef", ".notdef",
  "Lslash", "Oslash", "OE", "ordmasculine",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", "ae", ".notdef", ".notdef",
  ".notdef", "dotlessi", ".notdef", ".notdef",
  "lslash", "oslash", "oe", "germandbls",
  ".notdef", ".notdef", ".notdef", ".notdef"
};

static const char *
ISOLatin1Encoding[256] = {
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  "space", "exclam", "quotedbl", "numbersign",
  "dollar", "percent", "ampersand", "quotesingle",
  "parenleft", "parenright", "asterisk", "plus",
  "comma", "hyphen", "period", "slash",
  "zero", "one", "two", "three",
  "four", "five", "six", "seven",
  "eight", "nine", "colon", "semicolon",
  "less", "equal", "greater", "question",
  "at", "A", "B", "C",
  "D", "E", "F", "G",
  "H", "I", "J", "K",
  "L", "M", "N", "O",
  "P", "Q", "R", "S",
  "T", "U", "V", "W",
  "X", "Y", "Z", "bracketleft",
  "backslash", "bracketright", "asciicircum", "underscore",
  "grave", "a", "b", "c",
  "d", "e", "f", "g",
  "h", "i", "j", "k",
  "l", "m", "n", "o",
  "p", "q", "r", "s",
  "t", "u", "v", "w",
  "x", "y", "z", "braceleft",
  "bar", "braceright", "asciitilde", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  ".notdef", ".notdef", ".notdef", ".notdef",
  "dotlessi", "quoteleft", "quoteright", "circumflex",
  "tilde", "macron", "breve", "dotaccent",
  "dieresis", ".notdef", "ring", "cedilla",
  ".notdef", "hungarumlaut", "ogonek", "caron",
  "space", "exclamdown", "cent", "sterling",
  "currency", "yen", "brokenbar", "section",
  "dieresis", "copyright", "ordfeminine", "guillemotleft",
  "logicalnot", "hyphen", "registered", "macron",
  "degree", "plusminus", "twosuperior", "threesuperior",
  "acute", "mu", "paragraph", "periodcentered",
  "cedilla", "onesuperior", "ordmasculine", "guillemotright",
  "onequarter", "onehalf", "threequarters", "questiondown",
  "Agrave", "Aacute", "Acircumflex", "Atilde",
  "Adieresis", "Aring", "AE", "Ccedilla",
  "Egrave", "Eacute", "Ecircumflex", "Edieresis",
  "Igrave", "Iacute", "Icircumflex", "Idieresis",
  "Eth", "Ntilde", "Ograve", "Oacute",
  "Ocircumflex", "Otilde", "Odieresis", "multiply",
  "Oslash", "Ugrave", "Uacute", "Ucircumflex",
  "Udieresis", "Yacute", "Thorn", "germandbls",
  "agrave", "aacute", "acircumflex", "atilde",
  "adieresis", "aring", "ae", "ccedilla",
  "egrave", "eacute", "ecircumflex", "edieresis",
  "igrave", "iacute", "icircumflex", "idieresis",
  "eth", "ntilde", "ograve", "oacute",
  "ocircumflex", "otilde", "odieresis", "divide",
  "oslash", "ugrave", "uacute", "ucircumflex",
  "udieresis", "yacute", "thorn", "ydieresis"
};
#endif
