/*  $Header: /home/cvsroot/dvipdfmx/src/unicode.c,v 1.6 2004/02/15 12:59:44 hirata Exp $

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
 * Unicode related:
 *  Conversion between UTF-* and UCS-*.
 *  ToUnicode CMap
 *
 * Normalization?
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "system.h"
#include "mem.h"
#include "error.h"
#include "dpxutil.h"

#include "pdfobj.h"
#include "pdfresource.h"

#include "encodings.h"
#include "agl.h"
#include "unicode.h"

static int __verbose = 0;

#define UC_DEBUG     3
#define UC_DEBUG_STR "UC"

void
UC_set_verbose (void)
{
  __verbose++;
}

#define UC_REPLACEMENT_CHAR 0x0000FFFDL

/*
 * Should move to other place.
 */
static int
sputx (unsigned char c, unsigned char **s, unsigned char *end)
{
  char hi = (c >> 4), lo = c & 0x0f;

  if (*s + 2 > end)
    ERROR("Buffer overflow.");
  **s = (hi < 10) ? hi + '0' : hi + '7';
  *(*s+1) = (lo < 10) ? lo + '0' : lo + '7';
  *s += 2;

  return 2;
}

static struct
{
  struct {
    long lower; /* lower bound */
    long upper; /* upper bound */
  } range;
  char tag;
} utf8def[6] = {
  { {0x00000000L, 0x0000007FL}, 0x00 },
  { {0x00000080L, 0x000007FFL}, 0xc0 },
  { {0x00000800L, 0x0000FFFFL}, 0xe0 },
  { {0x00010000L, 0x001FFFFFL}, 0xf0 },
  { {0x00200000L, 0x03FFFFFFL}, 0xfc },
  { {0x04000000L, 0x7FFFFFFFL}, 0xfe }
};

int
UC_sputc_UTF8 (long ucv, unsigned char **s, unsigned char *end)
{
  int  idx = 0;
  char tag;

  while (idx < 6 && ucv > utf8def[idx].range.upper) {
    idx++;
  } 

  if (idx == 6)
    return 0;

  if (*s + idx >= end)
    ERROR("Buffer overflow.");

  tag = utf8def[idx].tag;
  do {
    if (idx == 0)
      *((*s)+idx) = ((char) ucv) | tag;
    else
      *((*s)+idx) = (((char) ucv) & 0x3f) | 0x80;
    ucv >>= 6;
  } while (idx-- > 0);

  *s += idx+1;
  return idx+1;
}

#define UC_SUR_SHIFT      10
#define UC_SUR_MASK       0x3FFUL
#define UC_SUR_LOW_START  0xDC00UL
#define UC_SUR_HIGH_START 0xD800UL

int
UC_sputc_UTF16BE (long ucv, unsigned char **s, unsigned char *end)
{
  int count = 0;

  if (ucv >= 0 && ucv <= 0xFFFF) {
    if (*s + 2 > end)
      ERROR("Buffer overflow.");
    *(*s++) = ucv >> 8;
    *(*s++) = ucv & 0xff;
    count = 2;
  } else if (ucv >= 0x010000L && ucv <= 0x10FFFFL) {
    unsigned short high, low;
    if (*s + 4 > end)
      ERROR("Buffer overflow.");
    ucv  -= 0x00010000L;
    high = (ucv >> UC_SUR_SHIFT) + UC_SUR_HIGH_START;
    low  = (ucv &  UC_SUR_MASK)  + UC_SUR_LOW_START;
    *(*s++) = high >> 8; *(*s++) = high & 0xff;
    *(*s++) = low  >> 8; *(*s++) = low  & 0xff;
    count = 4;
  } else {
    /*
     * Strict ?
     */
    if (*s + 2 > end)
      ERROR("Buffer overflow.");
    *(*s++) = UC_REPLACEMENT_CHAR >> 8;
    *(*s++) = UC_REPLACEMENT_CHAR & 0xff;
    count = 2;
  }

  return count;
}

int
UC_sputx_UTF16BE (long ucv, unsigned char **s, unsigned char *end)
{
  int count = 0;

  if (ucv >= 0 && ucv <= 0xFFFF) {
    count += sputx((unsigned char) (ucv >> 8),   s, end);
    count += sputx((unsigned char) (ucv & 0xff), s, end);
  } else if (ucv >= 0x010000L && ucv <= 0x10FFFFL) {
    unsigned short high, low;
    ucv  -= 0x00010000L;
    high = (ucv >> UC_SUR_SHIFT) + UC_SUR_HIGH_START;
    low  = (ucv &  UC_SUR_MASK)  + UC_SUR_LOW_START;
    count += sputx((unsigned char) (high >> 8),   s, end);
    count += sputx((unsigned char) (high & 0xff), s, end);
    count += sputx((unsigned char) (low >> 8),    s, end);
    count += sputx((unsigned char) (low & 0xff),  s, end);
  } else {
    /*
     * Strict ?
     */
    count += sputx((unsigned char) (UC_REPLACEMENT_CHAR >> 8),   s, end);
    count += sputx((unsigned char) (UC_REPLACEMENT_CHAR & 0xff), s, end);
  }

  return count;
}

int
UC_is_valid (long ucv)
{
  if (ucv < 0 || (ucv >= 0x0000D800L && ucv <= 0x0000DFFFL))
    return 0;
  return 1;
}

/*
 * ToUnicode CMap
 */

#define TOUNICODE_PART1 "\
/CIDInit /ProcSet findresource begin\n\
12 dict begin\n\
begincmap\n\
/CIDSystemInfo <<\n\
/Registry (Adobe)\n/Ordering (UCS)\n/Supplement 0\n\
>> def\n\
/CMapType 2 def\n\
"
/*
 * PAET1.99: CMapName.
 */
#define TOUNICODE_PART2 "\
1 begincodespacerange\n\
<00> <FF>\n\
endcodespacerange\n\
"
/*
 * PART3: CMap mapping.
 */
#define TOUNICODE_PART4 "\
endcmap\n\
CMapName currentdict /CMap defineresource pop\n\
end\n\
end\n\
"

/*
 * The wbuf requires at least 1217 bytes.
 *  100 beginbfchar
 *  <XX> <XXXX>
 *  ... 100 bfchar entries ...
 *  endbfchar
 */
#define WBUF_SIZE 4096

pdf_obj *
UC_make_ToUnicode_CMap (char *cmap_name, char **enc_vec, AGLmap *aglm)
{
  pdf_obj  *cmap;
  int       code, count;
  unsigned char wbuf[WBUF_SIZE], *p, *end;

  if (!cmap_name || !enc_vec)
    ERROR("%s: CMap name/Encoding vector not defined.", UC_DEBUG_STR);

  if (!aglm) {
    int aglm_id;
    aglm_id = AGLmap_cache_find(AGLMAP_DEFAULT_GLYPHLIST);
    if (aglm_id >= 0)
      aglm = AGLmap_cache_get(aglm_id);
    if (!aglm) {
      if (__verbose > UC_DEBUG)
	WARN("%s: Glyph name to Unicode mapping not available.", UC_DEBUG_STR);
      return NULL;
    }
  }

  cmap = pdf_new_stream(STREAM_COMPRESS);

  pdf_add_stream(cmap, TOUNICODE_PART1, strlen(TOUNICODE_PART1));
  sprintf(wbuf, "/CMapName /%s def\n", cmap_name);
  pdf_add_stream(cmap, wbuf, strlen(wbuf));
  pdf_add_stream(cmap, TOUNICODE_PART2, strlen(TOUNICODE_PART2));

  count = 0;
  p     = wbuf;
  end   = wbuf + WBUF_SIZE;
  memset(wbuf, 0, WBUF_SIZE);
  for (code = 0; code < 256; code++) {
    if (enc_vec[code] && strcmp(enc_vec[code], ".notdef")) {
      unsigned char *save;
      long len;
      int  fail_count;

      save = p;
      *p++ = '<'; sputx(code & 0xff, &p, end); *p++ = '>';
      *p++ = ' ';
      *p++ = '<';
      len = AGLmap_decode_UTF16BE(aglm, enc_vec[code], &p, end, &fail_count);
      if (len > 0 && fail_count == 0) {
	*p++ = '>';
	*p++ = '\n';
	count++;
      } else {
	/* Conversion of one or more glyph failed, mapping is not valid, backup. */
	p = save;
	p[0] = '\0';
      }
    }
    /*
     * Flush buffer.
     */
    if (count == 100 || p > end - 1024) {
      pdf_add_stream(cmap, "100 beginbfchar\n", strlen("100 beginbfchar\n"));
      pdf_add_stream(cmap, wbuf, strlen(wbuf));
      pdf_add_stream(cmap, "endbfchar\n", strlen("endbfchar\n"));
      count = 0;
      p     = wbuf;
      memset(wbuf, 0, WBUF_SIZE);
    }
  }

  /*
   * Flush buffer.
   */
  if (count != 0) {
    char tmp[17];
    sprintf(tmp, "%3d beginbfchar\n", count);
    pdf_add_stream(cmap, tmp,  strlen(tmp));
    pdf_add_stream(cmap, wbuf, strlen(wbuf));
    pdf_add_stream(cmap, "endbfchar\n", strlen("endbfchar\n"));
  }

  pdf_add_stream(cmap, TOUNICODE_PART4, strlen(TOUNICODE_PART4));

  return cmap;
}

void
UC_add_ToUnicode_CMap (pdf_obj *fontdict, Encoding *encoding)
{
  char    *encoding_name, **encoding_vec;
  char    *cmap_name;
  pdf_obj *cmap_ref;

  ASSERT(fontdict);
  ASSERT(encoding);

  if (Encoding_is_predefined(encoding))
    return;

  encoding_name = Encoding_get_name(encoding);
  encoding_vec  = Encoding_get_encoding(encoding);
  ASSERT(encoding_name && encoding_vec);

  cmap_name = NEW(strlen(encoding_name)+strlen("-UCS2")+1, char);
  strcpy(cmap_name, encoding_name);
  strcat(cmap_name, "-UCS2");

  if (!PDF_findresource(cmap_name, PDF_RES_TYPE_CMAP)) {
    pdf_obj *cmap;
    AGLmap  *aglm = NULL;
    int      aglm_id;

    aglm_id = AGLmap_cache_find(AGLMAP_DEFAULT_GLYPHLIST);
    if (aglm_id >= 0) {
      aglm = AGLmap_cache_get(aglm_id);
      cmap = UC_make_ToUnicode_CMap(cmap_name, encoding_vec, aglm);
      if (cmap)
	PDF_defineresource(cmap_name, cmap, PDF_RES_TYPE_CMAP);
    }
  }

  cmap_ref = PDF_findresource(cmap_name, PDF_RES_TYPE_CMAP);
  if (cmap_ref)
    pdf_add_dict(fontdict, pdf_new_name("ToUnicode"), cmap_ref);

  RELEASE(cmap_name);

  return;
}
