/*  $Header: /home/cvsroot/dvipdfmx/src/tfm.c,v 1.11 2004/02/15 12:59:43 hirata Exp $
    
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "system.h"
#include "mem.h"
#include "mfileio.h"
#include "error.h"

#include "numbers.h"
#include "dpxutil.h"

#include "tfm.h"

#define FWBASE ((double) (1<<20))

#ifndef WITHOUT_ASCII_PTEX
/*
 * ID is 9 for vertical JFM file.
 */
#define JFM_ID  11
#define JFMV_ID  9
#define IS_JFM(i) ((i) == JFM_ID || (i) == JFMV_ID)
#endif /* !WITHOUT_ASCII_PTEX */

static unsigned char verbose = 0;

/*
 * TFM Record structure:
 * Multiple TFM's may be read in at once.
 */

struct a_tfm
{
  char *tex_name;

  SIGNED_QUAD max_width;
  SIGNED_QUAD max_height;
  SIGNED_QUAD max_depth;
  fixword *unpacked_widths;
  fixword *unpacked_heights;
  fixword *unpacked_depths;

  /* From header */
  fixword designsize;
  char   *codingscheme;

#ifndef WITHOUT_ASCII_PTEX
  /*
   * id: 11 (9) for JFM (vertical)
   * nt: number of charcter types in JFM.
   */
  UNSIGNED_BYTE id;
  UNSIGNED_BYTE nt;
#endif /* !WITHOUT_ASCII_PTEX */

  SIGNED_QUAD   level; /* Used only in OFMs */
  UNSIGNED_QUAD wlenfile, wlenheader;
  UNSIGNED_QUAD bc, ec;
  UNSIGNED_QUAD nwidths, nheights, ndepths;
  UNSIGNED_QUAD nitcor, nlig, nkern, nextens;
  UNSIGNED_QUAD nfonparm;
  UNSIGNED_QUAD font_dir; /* Used only in OFMs */
  UNSIGNED_QUAD nco, ncw, npc;
  SIGNED_QUAD   *header;
#ifndef WITHOUT_ASCII_PTEX
  UNSIGNED_PAIR *chartypes;
#endif /* !WITHOUT_ASCII_PTEX */
  UNSIGNED_QUAD *char_info;
  UNSIGNED_PAIR *width_index;
  UNSIGNED_BYTE *height_index;
  UNSIGNED_BYTE *depth_index;
  SIGNED_QUAD *width;
  SIGNED_QUAD *height;
  SIGNED_QUAD *depth;
};

static void
a_tfm_init (struct a_tfm *a_tfm) 
{
  a_tfm->tex_name = NULL;
  a_tfm->unpacked_widths = NULL;
  a_tfm->unpacked_heights = NULL;
  a_tfm->unpacked_depths = NULL;
  a_tfm->max_width  = 0;
  a_tfm->max_height = 0;
  a_tfm->max_depth  = 0;

  a_tfm->codingscheme = NULL;
  a_tfm->designsize   = 0;

#ifndef WITHOUT_ASCII_PTEX
  a_tfm->id = 0;
  a_tfm->nt = 0;
#endif /* !WITHOUT_ASCII_PTEX */
  a_tfm->header = NULL;
#ifndef WITHOUT_ASCII_PTEX
  a_tfm->chartypes = NULL;
#endif /* !WITHOUT_ASCII_PTEX */
  a_tfm->char_info = NULL;
  a_tfm->width_index  = NULL;
  a_tfm->height_index = NULL;
  a_tfm->depth_index  = NULL;
  a_tfm->width = NULL;
  a_tfm->height = NULL;
  a_tfm->depth = NULL;
}

static void
a_tfm_release (struct a_tfm *a_tfm)
{
  if (!a_tfm)
    return;

  if (a_tfm->tex_name)
    RELEASE (a_tfm->tex_name);
  if (a_tfm->unpacked_widths)
    RELEASE(a_tfm->unpacked_widths);
  if (a_tfm->unpacked_heights)
    RELEASE(a_tfm->unpacked_heights);
  if (a_tfm->unpacked_depths)
    RELEASE(a_tfm->unpacked_depths);
  if (a_tfm->codingscheme)
    RELEASE(a_tfm->codingscheme);

  if (a_tfm->header)
    RELEASE(a_tfm->header);
  if (a_tfm->char_info)
    RELEASE(a_tfm->char_info);
  if (a_tfm->width)
    RELEASE(a_tfm->width);
  if (a_tfm->height)
    RELEASE(a_tfm->height);
  if (a_tfm->depth)
    RELEASE(a_tfm->depth);
#ifndef WITHOUT_ASCII_PTEX
  if (a_tfm->chartypes)
    RELEASE(a_tfm->chartypes);
#endif /* !WITHOUT_ASCII_PTEX */
  if (a_tfm->width_index)
    RELEASE(a_tfm->width_index);
  if (a_tfm->height_index)
    RELEASE(a_tfm->height_index);
  if (a_tfm->depth_index)
    RELEASE(a_tfm->depth_index);
}

#ifndef MAX_FONTS
#define MAX_FONTS 16
#endif

struct a_tfm *tfm = NULL;
static unsigned numtfms = 0, max_tfms = 0; /* numtfms should equal
					      numfonts in dvi.c */
static void
tfms_need (unsigned n)
{
  if (n > max_tfms) {
    max_tfms = MAX(max_tfms + MAX_FONTS, n);
    tfm = RENEW(tfm, max_tfms, struct a_tfm);
  }
}

/* External Routine */

void
tfm_set_verbose (void)
{
  if (verbose < 255) verbose++;
}

static UNSIGNED_QUAD
sum_of_tfm_sizes (struct a_tfm *a_tfm)
{
  UNSIGNED_QUAD result = 6;

  result += (a_tfm->ec - a_tfm->bc + 1);
  result += a_tfm->wlenheader;
  result += a_tfm->nwidths;
  result += a_tfm->nheights;
  result += a_tfm->ndepths;
  result += a_tfm->nitcor;
  result += a_tfm->nlig;
  result += a_tfm->nkern;
  result += a_tfm->nextens;
  result += a_tfm->nfonparm;
#ifndef WITHOUT_ASCII_PTEX
  if (IS_JFM(a_tfm->id)) {
    result += a_tfm->nt + 1;
  }
#endif /* !WITHOUT_ASCII_PTEX */

  return result;
}

static UNSIGNED_QUAD
sum_of_ofm_sizes (struct a_tfm *a_tfm)
{
  UNSIGNED_QUAD result = 14;

  result += 2*(a_tfm->ec - a_tfm->bc + 1);
  result += a_tfm->wlenheader;
  result += a_tfm->nwidths;
  result += a_tfm->nheights;
  result += a_tfm->ndepths;
  result += a_tfm->nitcor;
  result += 2*(a_tfm->nlig);
  result += a_tfm->nkern;
  result += 2*(a_tfm->nextens);
  result += a_tfm->nfonparm;

  return result;
}


static void
get_sizes (FILE *tfm_file, SIGNED_QUAD tfm_file_size, struct a_tfm *a_tfm)
{
#ifndef WITHOUT_ASCII_PTEX
  {
    UNSIGNED_PAIR first_hword;

    /*
     * The first half word of TFM/JFM is TFM ID for JFM or size of
     * TFM file in word for TFM. TFM with 9*4 or 11*4 bytes is not
     * expected to be a valid TFM. So, we always assume that TFMs
     * starting with 00 09 or 00 0B is JFM.
     */
    first_hword = get_unsigned_pair(tfm_file);
    if (IS_JFM(first_hword)) {
      a_tfm->id = first_hword;
      a_tfm->nt = get_unsigned_pair(tfm_file);
      a_tfm->wlenfile = get_unsigned_pair(tfm_file);
    } else {
      a_tfm->wlenfile = first_hword;
    }
  }
#else /* WITHOUT_ASCII_PTEX */
  a_tfm->wlenfile = get_unsigned_pair(tfm_file);
#endif /* !WITHOUT_ASCII_PTEX */

  a_tfm->wlenheader = get_unsigned_pair(tfm_file);
  a_tfm->bc = get_unsigned_pair(tfm_file);
  a_tfm->ec = get_unsigned_pair(tfm_file);
  if (a_tfm->ec < a_tfm->bc) {
    ERROR("TFM file error: ec(%u) < bc(%u) ???", a_tfm->ec, a_tfm->bc);
  }
  a_tfm->nwidths  = get_unsigned_pair(tfm_file);
  a_tfm->nheights = get_unsigned_pair(tfm_file);
  a_tfm->ndepths  = get_unsigned_pair(tfm_file);
  a_tfm->nitcor   = get_unsigned_pair(tfm_file);
  a_tfm->nlig     = get_unsigned_pair(tfm_file);
  a_tfm->nkern    = get_unsigned_pair(tfm_file);
  a_tfm->nextens  = get_unsigned_pair(tfm_file);
  a_tfm->nfonparm = get_unsigned_pair(tfm_file);

  {
    UNSIGNED_QUAD expected_size;

    if (a_tfm->wlenfile != tfm_file_size / 4) {
      WARN("TFM file size is %ld bytes but it says it is %ld bytes!",
	   tfm_file_size, a_tfm->wlenfile * 4);
      if (tfm_file_size > a_tfm->wlenfile * 4) {
	WARN("Proceeding nervously...");
      } else {
	ERROR("Can't proceed...");
      }
    }

    expected_size = sum_of_tfm_sizes(a_tfm);
    if (expected_size != a_tfm->wlenfile) {
      WARN("TFM file size is expected to be %ld bytes but it says it is %ld bytes!",
	   expected_size * 4, a_tfm->wlenfile * 4);
      if (tfm_file_size > expected_size *4) {
	WARN("Proceeding nervously...");
      } else {
	ERROR("Can't proceed...");
      }
    }
  }

  return;
}

static void
ofm_get_sizes (FILE *ofm_file, UNSIGNED_QUAD ofm_file_size, struct a_tfm *a_tfm)
{
  a_tfm->level = get_signed_quad(ofm_file);

  a_tfm->wlenfile   = get_signed_quad(ofm_file);
  a_tfm->wlenheader = get_signed_quad(ofm_file);
  a_tfm->bc = get_signed_quad(ofm_file);
  a_tfm->ec = get_signed_quad(ofm_file);
  if (a_tfm->ec < a_tfm->bc) {
    ERROR("OFM file error: ec(%u) < bc(%u) ???", a_tfm->ec, a_tfm->bc);
  }
  a_tfm->nwidths  = get_signed_quad(ofm_file);
  a_tfm->nheights = get_signed_quad(ofm_file);
  a_tfm->ndepths  = get_signed_quad(ofm_file);
  a_tfm->nitcor   = get_signed_quad(ofm_file);
  a_tfm->nlig     = get_signed_quad(ofm_file);
  a_tfm->nkern    = get_signed_quad(ofm_file);
  a_tfm->nextens  = get_signed_quad(ofm_file);
  a_tfm->nfonparm = get_signed_quad(ofm_file);
  a_tfm->font_dir = get_signed_quad(ofm_file);
  if (a_tfm->font_dir) {
    WARN("I may be interpreting a font direction incorrectly.");
  }
  if (a_tfm->level == 0) {
    if (a_tfm->wlenfile != ofm_file_size/4 ||
	sum_of_ofm_sizes(a_tfm) != a_tfm->wlenfile) {
      ERROR("OFM file problem.  Table sizes don't agree.");
    }
  } else if (a_tfm->level == 1) {
    a_tfm->nco = get_signed_quad(ofm_file);
    a_tfm->ncw = get_signed_quad(ofm_file);
    a_tfm->npc = get_signed_quad(ofm_file);
    seek_absolute(ofm_file, 4*(a_tfm->nco - a_tfm->wlenheader));
  } else {
    ERROR("Can't handle OFM files with level > 1");
  }

  return;
}

static void
get_fix_word_array (FILE *tfm_file, SIGNED_QUAD *a_word, SIGNED_QUAD length)
{
  long i;

  for (i = 0; i < length; i++) {
    a_word[i] = get_signed_quad(tfm_file);
  }

  return;
}

static void
get_unsigned_quad_array (FILE *tfm_file, UNSIGNED_QUAD *a_word, SIGNED_QUAD length)
{
  long i;

  for (i = 0; i < length; i++) {
    a_word[i] = get_unsigned_quad(tfm_file);
  }

  return;
}

static void
do_fix_word_array (FILE *tfm_file, SIGNED_QUAD **a, SIGNED_QUAD len)
{
  if (len != 0) {
    *a = NEW(len, SIGNED_QUAD);
    get_fix_word_array(tfm_file, *a, len);
  } else
    *a = NULL;

  return;
}

static void
do_unsigned_quad_array (FILE *tfm_file, UNSIGNED_QUAD **a, UNSIGNED_PAIR len)
{
  if (len != 0) {
    *a = NEW(len, UNSIGNED_QUAD);
    get_unsigned_quad_array(tfm_file, *a, len);
  } else
    *a = NULL;

  return;
}

#ifndef WITHOUT_ASCII_PTEX
static void
do_char_type_array (FILE *tfm_file, struct a_tfm *a_tfm)
{
  long i;
  UNSIGNED_PAIR charcode;
  UNSIGNED_PAIR chartype;

  a_tfm->chartypes = NEW(65536, UNSIGNED_PAIR);
  for (i = 0; i < 65536; i++) {
    (a_tfm->chartypes)[i] = 0;
  }
  for (i = 0; i < a_tfm->nt; i++) {
    charcode = get_unsigned_pair(tfm_file);
    chartype = get_unsigned_pair(tfm_file);
    (a_tfm->chartypes)[charcode] = chartype;
  }
}
#endif /* !WITHOUT_ASCII_PTEX */

static void
unpack_widths (struct a_tfm *a_tfm)
{
  int i;
  UNSIGNED_QUAD charinfo;
  UNSIGNED_PAIR width_index;

  a_tfm->unpacked_widths = NEW(256, fixword);

  for (i = 0; i < 256; i++) {
    (a_tfm->unpacked_widths)[i] = 0;
  }

  for (i = a_tfm->bc; i <= a_tfm->ec; i++ ) {
    charinfo = (a_tfm->char_info)[i-(a_tfm->bc)];
    width_index = (charinfo / 16777216ul);
    (a_tfm->unpacked_widths)[i] = (a_tfm->width)[width_index];
    if (a_tfm->width[width_index] > a_tfm->max_width)
      a_tfm->max_width = (a_tfm->width)[width_index];
  }

  return;
}

static void
unpack_heights (struct a_tfm *a_tfm)
{
  int i;
  UNSIGNED_QUAD charinfo;
  UNSIGNED_PAIR height_index;

  a_tfm->unpacked_heights = NEW(256, fixword);

  for (i = 0; i < 256; i++) {
    (a_tfm->unpacked_heights)[i] = 0;
  }

  for (i = a_tfm->bc; i<= a_tfm->ec; i++) {
    charinfo = (a_tfm->char_info)[i-(a_tfm->bc)];
    height_index = (charinfo / 0x100000ul) & 0xf;
    (a_tfm->unpacked_heights)[i] = (a_tfm->height)[height_index];
    if (a_tfm->height[height_index] > a_tfm->max_height)
      a_tfm->max_height = (a_tfm->height)[height_index];
  }

  return;
}

static void
unpack_depths (struct a_tfm *a_tfm)
{
  int i;
  UNSIGNED_QUAD charinfo;
  UNSIGNED_PAIR depth_index;

  a_tfm->unpacked_depths = NEW(256, fixword);

  for (i = 0; i < 256; i++) {
    (a_tfm->unpacked_depths)[i] = 0;
  }

  for (i = a_tfm->bc; i <= a_tfm->ec; i++ ) {
    charinfo = (a_tfm->char_info)[i-(a_tfm->bc)];
    depth_index = (charinfo / 0x10000ul) & 0xf;
    (a_tfm->unpacked_depths)[i] = (a_tfm->depth)[depth_index];
    if (a_tfm->depth[depth_index] > a_tfm->max_depth)
      a_tfm->max_depth = (a_tfm->depth)[depth_index];
  }

  return;
}

static int
sput_bigendian (char *s, SIGNED_QUAD v, int n)
{
  int i;

  for (i = n-1; i >= 0; i--) {
    s[i] = (char) (v & 0xff);
    v >>= 8;
  }

  return n;
}

static void
unpack_header (struct a_tfm *a_tfm)
{
  if (a_tfm->wlenheader < 12) {
    a_tfm->codingscheme = NULL;
  } else {
    int   i, len;
    char *p;
    len = ((a_tfm->header)[2] >> 24);
    if (len < 0 || len > 39)
      ERROR("Invalid TFM header.");
    if (len > 0) {
      a_tfm->codingscheme = p = NEW(len+1, char);
      for (i = 0; i <= len/4; i++) {
	p += sput_bigendian(p, (a_tfm->header)[2+i], (i == 0) ? 3 : 4);
      }
      (a_tfm->codingscheme)[len] = '\0';
    } else {
      a_tfm->codingscheme = NULL;
    }
  }

  a_tfm->designsize = (a_tfm->header)[1];
}

static void
do_ofm_zero_char_info (FILE *tfm_file, struct a_tfm *a_tfm)
{
  UNSIGNED_QUAD i;
  UNSIGNED_QUAD num_chars;

  num_chars = a_tfm->ec - a_tfm->bc + 1;
  if (num_chars != 0) {
    a_tfm->width_index  = NEW(num_chars, UNSIGNED_PAIR);
    a_tfm->height_index = NEW(num_chars, UNSIGNED_BYTE);
    a_tfm->depth_index  = NEW(num_chars, UNSIGNED_BYTE);
    a_tfm->unpacked_widths  = NEW(a_tfm->bc + num_chars, fixword);
    a_tfm->unpacked_heights = NEW(a_tfm->bc + num_chars, fixword);
    a_tfm->unpacked_depths  = NEW(a_tfm->bc + num_chars, fixword);
    for (i = 0; i < num_chars; i++) {
      (a_tfm->width_index)[i]  = get_unsigned_pair(tfm_file);
      (a_tfm->height_index)[i] = get_unsigned_byte(tfm_file);
      (a_tfm->depth_index)[i]  = get_unsigned_byte(tfm_file);
      /* Ignore remaining quad */
      get_unsigned_quad(tfm_file);
    }
  }
}

static void
do_ofm_one_char_info (FILE *tfm_file, struct a_tfm *a_tfm)
{
  UNSIGNED_QUAD i;
  UNSIGNED_QUAD num_char_infos, char_infos_read;
  UNSIGNED_QUAD num_chars;
  UNSIGNED_QUAD char_info_size;

  char_info_size = 3 + (a_tfm->npc/2);
  num_char_infos = (a_tfm->ncw) / char_info_size;
  num_chars = (a_tfm->ec - a_tfm ->bc) + 1;

  if (num_chars != 0) {
    a_tfm->width_index  = NEW(num_chars, UNSIGNED_PAIR);
    a_tfm->height_index = NEW(num_chars, UNSIGNED_BYTE);
    a_tfm->depth_index  = NEW(num_chars, UNSIGNED_BYTE);
    a_tfm->unpacked_widths  = NEW(a_tfm->bc + num_chars, fixword);
    a_tfm->unpacked_heights = NEW(a_tfm->bc + num_chars, fixword);
    a_tfm->unpacked_depths  = NEW(a_tfm->bc + num_chars, fixword);

    char_infos_read = 0;
    for (i = 0; i < num_chars && char_infos_read < num_char_infos; i++) {
      int repeats, j;

      (a_tfm->width_index)[i]  = get_unsigned_pair(tfm_file);
      (a_tfm->height_index)[i] = get_unsigned_byte(tfm_file);
      (a_tfm->depth_index)[i]  = get_unsigned_byte(tfm_file);
      /* Ignore next quad */
      get_unsigned_quad(tfm_file);
      repeats = get_unsigned_pair(tfm_file);
      /* Skip params */
      for (j = 0; j < a_tfm->npc; j++) {
	get_unsigned_pair(tfm_file);
      }
      /* Remove word padding if necessary */
      if (ISEVEN(a_tfm->npc)){
	get_unsigned_pair(tfm_file);
      }
      char_infos_read++;
      if (i + repeats > num_chars) {
	ERROR("Repeats causes number of characters to be exceeded.");
      }
      for (j = 0; j < repeats; j++) {
	a_tfm->width_index[i+j+1]  = a_tfm->width_index[i];
	a_tfm->height_index[i+j+1] = a_tfm->height_index[i];
	a_tfm->depth_index[i+j+1]  = a_tfm->depth_index[i];
      }
      /* Skip ahead because we have already handled repeats */
      i += repeats;
    }
  }
}

static void
ofm_unpack_arrays (struct a_tfm *a_tfm, UNSIGNED_QUAD num_chars)
{
  long i;

  for (i = 0; i < num_chars; i++) {
    (a_tfm->unpacked_widths)[a_tfm->bc+i]  = (a_tfm->width)[(a_tfm->width_index)[i]];
    (a_tfm->unpacked_heights)[a_tfm->bc+i] = (a_tfm->height)[(a_tfm->height_index)[i]];
    (a_tfm->unpacked_depths)[a_tfm->bc+i]  = (a_tfm->depth)[(a_tfm->depth_index)[i]];
    if ((a_tfm->width)[(a_tfm->width_index)[i]] > a_tfm->max_width)
      a_tfm->max_width  = (a_tfm->width)[(a_tfm->width_index)[i]];
    if ((a_tfm->height)[(a_tfm->height_index)[i]] > a_tfm->max_height)
      a_tfm->max_height = (a_tfm->height)[(a_tfm->height_index)[i]];
    if ((a_tfm->depth)[(a_tfm->depth_index)[i]] > a_tfm->max_depth)
      a_tfm->max_depth  = (a_tfm->depth)[(a_tfm->depth_index)[i]];
  }
}

static void
get_ofm (FILE *ofm_file, UNSIGNED_QUAD ofm_file_size, struct a_tfm *a_tfm)
{
  ofm_get_sizes(ofm_file, ofm_file_size, a_tfm);

  if (a_tfm->level == 0) {
    do_fix_word_array(ofm_file, &(a_tfm->header), a_tfm->wlenheader);
    do_ofm_zero_char_info(ofm_file, a_tfm);
  } else if (a_tfm->level == 1) {
    do_fix_word_array(ofm_file, &(a_tfm -> header), a_tfm->wlenheader);
    do_ofm_one_char_info(ofm_file, a_tfm);
  } else {
    ERROR ("OFM level %d not supported.", a_tfm->level);
  }
  do_fix_word_array(ofm_file, &(a_tfm->width),  a_tfm->nwidths);
  do_fix_word_array(ofm_file, &(a_tfm->height), a_tfm->nheights);
  do_fix_word_array(ofm_file, &(a_tfm->depth),  a_tfm->ndepths);

  ofm_unpack_arrays(a_tfm, a_tfm->ec - a_tfm->bc + 1);
  unpack_header(a_tfm);

  return;
}

static void
get_tfm (FILE *tfm_file, UNSIGNED_QUAD tfm_file_size, struct a_tfm *a_tfm)
{
  get_sizes(tfm_file, tfm_file_size, a_tfm);

  do_fix_word_array(tfm_file, &(a_tfm->header), a_tfm->wlenheader);
#ifndef WITHOUT_ASCII_PTEX
  if (IS_JFM(a_tfm->id)) {
    do_char_type_array(tfm_file, a_tfm);
  }
#endif /* !WITHOUT_ASCII_PTEX */
  do_unsigned_quad_array(tfm_file, &(a_tfm->char_info), a_tfm->ec - a_tfm->bc + 1);
  do_fix_word_array(tfm_file, &(a_tfm->width),  a_tfm->nwidths);
  do_fix_word_array(tfm_file, &(a_tfm->height), a_tfm->nheights);
  do_fix_word_array(tfm_file, &(a_tfm->depth),  a_tfm->ndepths);

  unpack_widths(a_tfm);
  unpack_heights(a_tfm);
  unpack_depths(a_tfm);
  unpack_header(a_tfm);

  return;
}

/* External Routine */
#define TFM_FORMAT 1
#define OFM_FORMAT 2

int
tfm_open (const char *tfm_name, int must_exist)
{
  FILE *tfm_file;
  int i, format = TFM_FORMAT;
  UNSIGNED_QUAD tfm_file_size;
  char *file_name = NULL;

  for (i = 0; i < numtfms; i++) {
    if (!strcmp(tfm_name, tfm[i].tex_name))
      return i;
  }

  /* The procedure to search tfm or ofm files:
     1. Search tfm file with the given name with the must_exist flag unset.
     2. Search ofm file with the given name with the must_exist flag unset.
     3. If not found and must_exist flag is set, try again to search
        tfm file with the must_exist flag set.
     4. If not found and must_exist flag is not set, return -1. */

#ifndef OFM_PREFERED
 {
   char *ofm_name, *suffix;

   suffix = strrchr(tfm_name, '.');
   if (!suffix || (strcmp(suffix, ".tfm") != 0 && strcmp(suffix, ".ofm") != 0)) {
     ofm_name = NEW(strlen(tfm_name) + strlen(".ofm") + 1, char);
     strcpy(ofm_name, tfm_name);
     strcat(ofm_name, ".ofm");
   } else {
     ofm_name = NULL;
   }
   if (ofm_name &&
       (file_name = kpse_find_file(ofm_name, kpse_ofm_format, 0)) != NULL) {
     format = OFM_FORMAT;
   } else if ((file_name = kpse_find_file(tfm_name, kpse_tfm_format, 0)) != NULL) {
     format = TFM_FORMAT;
   } else if ((file_name = kpse_find_file(tfm_name, kpse_ofm_format, 0)) != NULL) {
     format = OFM_FORMAT;
   }
   if (ofm_name)
     RELEASE(ofm_name);
 }
#else /* !OFM_PREFERED */
  if ((file_name = kpse_find_file(tfm_name, kpse_tfm_format, 0))) {
    format = TFM_FORMAT;
  } else if ((file_name = kpse_find_file(tfm_name, kpse_ofm_format, 0))) {
    format = OFM_FORMAT;
  }
#endif /* OFM_PREFERED */

  /*
   * In case that must_exist is set, MiKTeX returns always non-NULL value
   * even if the tfm file is not found.
   */
  if (file_name == NULL) {
    if (must_exist) {
      if ((file_name = kpse_find_file(tfm_name, kpse_tfm_format, 1)) != NULL)
	format = TFM_FORMAT;
      else {
	ERROR("Unable to find TFM file \"%s\".", tfm_name);
      }
    } else {
      return -1;
    }
  }

  tfms_need(numtfms + 1);
  a_tfm_init(tfm + numtfms);

  if ((tfm_file = MFOPEN(file_name, FOPEN_RBIN_MODE)) == NULL) {
    ERROR("Could not open specified TFM/OFM file \"%s\".", tfm_name);
  }

  if (verbose) {
    if (format == TFM_FORMAT)
      MESG("(TFM:%s", tfm_name);
    else
      MESG("(OFM:%s", tfm_name);
    if (verbose > 1)
      MESG("[%s]", file_name);
  }

  if ((tfm_file_size = file_size(tfm_file)) < 24) {
    ERROR("TFM/OFM file too small to be a valid file.");
  }

  if (format == OFM_FORMAT)
    get_ofm(tfm_file, tfm_file_size, &tfm[numtfms]);
  else
    get_tfm(tfm_file, tfm_file_size, &tfm[numtfms]);

  MFCLOSE (tfm_file);

  tfm[numtfms].tex_name = NEW (strlen(tfm_name)+1, char);
  strcpy(tfm[numtfms].tex_name, tfm_name);

  if (verbose) 
    MESG(")");

  return numtfms++;
}

void
tfm_close_all (void)
{
  int i;

  for (i = 0; i < numtfms; i++) {
    a_tfm_release(&(tfm[i]));
  }
  if (tfm)
    RELEASE (tfm);
}

#define CHECK_ID(n) do {\
                        if ((n) < 0 || (n) >= numtfms)\
                           ERROR("TFM: Invalid TFM ID: %d", (n));\
                    } while (0)


/*
 * tfm_get_width returns the width of the font
 * as a (double) fraction of the design size.
 */
double
tfm_get_width (int font_id, UNSIGNED_QUAD ch)
{
  CHECK_ID(font_id);

#ifndef WITHOUT_ASCII_PTEX
  if (IS_JFM(tfm[font_id].id))
    ch = (tfm[font_id].chartypes)[ch];
#endif /* !WITHOUT_ASCII_PTEX */

  if (tfm[font_id].unpacked_widths &&
      ch >= tfm[font_id].bc && ch <= tfm[font_id].ec)
    return (double) (tfm[font_id].unpacked_widths)[ch] / FWBASE;

  return 0.0;
}

double
tfm_get_height (int font_id, UNSIGNED_QUAD ch)
{
  CHECK_ID(font_id);

#ifndef WITHOUT_ASCII_PTEX
  if (IS_JFM(tfm[font_id].id))
    ch = (tfm[font_id].chartypes)[ch];
#endif /* !WITHOUT_ASCII_PTEX */

  if (tfm[font_id].unpacked_heights &&
      ch >= tfm[font_id].bc && ch <= tfm[font_id].ec)
    return (double) (tfm[font_id].unpacked_heights)[ch] / FWBASE;

  return 0.0;
}

double
tfm_get_depth (int font_id, UNSIGNED_QUAD ch)
{
  CHECK_ID(font_id);

#ifndef WITHOUT_ASCII_PTEX
  if (IS_JFM(tfm[font_id].id))
    ch = (tfm[font_id].chartypes)[ch];
#endif /* !WITHOUT_ASCII_PTEX */

  if (tfm[font_id].unpacked_depths &&
      ch >= tfm[font_id].bc && ch <= tfm[font_id].ec)
    return (double) (tfm[font_id].unpacked_depths)[ch] / FWBASE;

  return 0.0;
}

fixword
tfm_get_fw_width (int font_id, UNSIGNED_QUAD ch)
{
  CHECK_ID(font_id);

#ifndef WITHOUT_ASCII_PTEX
  if (IS_JFM(tfm[font_id].id))
    ch = (tfm[font_id].chartypes)[ch];
#endif /* !WITHOUT_ASCII_PTEX */

  if (tfm[font_id].unpacked_widths &&
      ch >= tfm[font_id].bc && ch <= tfm[font_id].ec)
    return (tfm[font_id].unpacked_widths)[ch];

  return 0;
}

fixword
tfm_get_fw_height (int font_id, UNSIGNED_QUAD ch)
{
  CHECK_ID(font_id);

#ifndef WITHOUT_ASCII_PTEX
  if (IS_JFM(tfm[font_id].id))
    ch = (tfm[font_id].chartypes)[ch];
#endif /* !WITHOUT_ASCII_PTEX */

  if (tfm[font_id].unpacked_heights &&
      ch >= tfm[font_id].bc && ch <= tfm[font_id].ec)
    return (tfm[font_id].unpacked_heights)[ch];

  return 0;
}

fixword
tfm_get_fw_depth (int font_id, UNSIGNED_QUAD ch)
{
  CHECK_ID(font_id);

#ifndef WITHOUT_ASCII_PTEX
  if (IS_JFM(tfm[font_id].id))
    ch = (tfm[font_id].chartypes)[ch];
#endif /* !WITHOUT_ASCII_PTEX */

  if (tfm[font_id].unpacked_depths &&
      ch >= tfm[font_id].bc && ch <= tfm[font_id].ec)
    return (tfm[font_id].unpacked_depths)[ch];

  return 0;
}

fixword
tfm_string_width (int font_id, const unsigned char *s, unsigned len)
{
  fixword result = 0;
  unsigned i;

  CHECK_ID(font_id);

  for (i = 0; i < len; i++) {
    result += tfm_get_fw_width(font_id, s[i]);
  }

  return result;
}

fixword
tfm_string_depth (int font_id, const unsigned char *s, unsigned len)
{
  fixword result = 0;
  unsigned i;

  CHECK_ID(font_id);

  for (i = 0; i < len; i++) {
    result = MAX(result, tfm_get_fw_depth(font_id, s[i]));
  }

  return result;
}

fixword
tfm_string_height (int font_id, const unsigned char *s, unsigned len)
{
  fixword result = 0;
  unsigned i;

  CHECK_ID(font_id);

  for (i = 0; i < len; i++) {
    result = MAX(result, tfm_get_fw_height(font_id, s[i]));
  }

  return result;
}

double
tfm_get_max_width (int font_id)
{
  CHECK_ID(font_id);

  return ((double) tfm[font_id].max_width/FWBASE);
}

double
tfm_get_max_height (int font_id)
{
  CHECK_ID(font_id);

  return ((double) tfm[font_id].max_height/FWBASE);
}

double
tfm_get_max_depth (int font_id)
{
  CHECK_ID(font_id);

  return ((double) tfm[font_id].max_depth/FWBASE);
}

UNSIGNED_PAIR
tfm_get_firstchar (int font_id)
{
  CHECK_ID(font_id);

  return tfm[font_id].bc;
}

UNSIGNED_PAIR
tfm_get_lastchar (int font_id)
{
  CHECK_ID(font_id);

  return tfm[font_id].ec;
}

double
tfm_get_design_size (int font_id)
{
  CHECK_ID(font_id);

  return (double) (tfm[font_id].designsize)/FWBASE*(72.0/72.27);
}

char *
tfm_get_codingscheme (int font_id)
{
  CHECK_ID(font_id);

  return tfm[font_id].codingscheme;
}

#ifndef WITHOUT_ASCII_PTEX
int
tfm_is_jfm (int font_id)
{
  CHECK_ID(font_id);

  return (IS_JFM(tfm[font_id].id) ? 1 : 0);
}

/* Vertical version of JFM */
int
tfm_is_vert (int font_id)
{
  CHECK_ID(font_id);

  return (tfm[font_id].id == JFMV_ID) ? 1 : 0;
}
#else /* WITHOUT_ASCII_PTEX */
int
tfm_is_jfm (int font_id)
{
  return 0;
}

int
tfm_is_vert (int font_id)
{
  return 0;
}
#endif /* !WITHOUT_ASCII_PTEX */
