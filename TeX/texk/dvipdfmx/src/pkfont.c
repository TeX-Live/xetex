/*  $Header: /home/cvsroot/dvipdfmx/src/pkfont.c,v 1.8 2004/01/24 16:27:23 hirata Exp $

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

#include <stdio.h>

#include "system.h"
#include "mem.h"
#include "error.h"
#include "mfileio.h"
#include "numbers.h"
#include "pkfont.h"
#include "pdfobj.h"
#include "pdflimits.h"
#include "tfm.h"
#include "ctype.h"

#ifndef PKFONT_DPI_DEFAULT
#define PKFONT_DPI_DEFAULT 600u
#endif

static int verbose = 0;
static unsigned font_dpi = PKFONT_DPI_DEFAULT;

void pk_set_verbose (void)
{
  verbose++;
}

void pk_set_dpi (int dpi)
{
  if (dpi < 0)
    ERROR("Invalid DPI: %d\n", dpi);
  font_dpi = dpi;
}

struct a_pk_font 
{
  pdf_obj *direct, *indirect;
  char  *tex_name, *filename;
  double ptsize;
  char *used_chars;
} *pk_fonts;

void init_pk_record (struct a_pk_font *p)
{
  p->tex_name = NULL;
  p->filename = NULL;
  p->direct   = NULL;
  p->indirect = NULL;
  p->ptsize   = 0;
  p->used_chars = NEW(256, char);
  memset(p->used_chars, 0, 256);
}

static int
pk_char2name (char *charname, unsigned char code)
{
  return sprintf(charname, "x%02X", code);
}

int num_pk_fonts = 0, max_pk_fonts = 0;

int
pk_font (const char *tex_name, double ptsize, int tfm_font_id, char *res_name)
{
  int   pk_id;
  char *filename;

  for (pk_id = 0; pk_id < num_pk_fonts; pk_id++) {
    if (!strcmp(tex_name, pk_fonts[pk_id].tex_name) &&
	(ptsize == pk_fonts[pk_id].ptsize))
      return pk_id;
  }

  {
    kpse_glyph_file_type kpse_file_info;
    filename = kpse_find_glyph((char *)tex_name,
			       (unsigned int)(font_dpi*ptsize/tfm_get_design_size(tfm_font_id)+0.5),
			       kpse_pk_format,
			       &kpse_file_info);
    if (filename == NULL)
      return -1;
  }

  if (num_pk_fonts >= max_pk_fonts) {
    max_pk_fonts += MAX_FONTS;
    pk_fonts = RENEW(pk_fonts, max_pk_fonts, struct a_pk_font);
  }
  init_pk_record(pk_fonts+pk_id);
  pk_fonts[pk_id].filename = NEW(strlen(filename)+1, char);
  strcpy(pk_fonts[pk_id].filename, filename);
  pk_fonts[pk_id].tex_name = NEW(strlen(tex_name)+1, char);
  strcpy(pk_fonts[pk_id].tex_name, tex_name);
  pk_fonts[pk_id].ptsize = ptsize;

  pk_fonts[pk_id].direct   = pdf_new_dict();
  pk_fonts[pk_id].indirect = NULL;
  pdf_add_dict(pk_fonts[pk_id].direct,
	       pdf_new_name("Type"), pdf_new_name("Font"));
  pdf_add_dict(pk_fonts[pk_id].direct,
	       pdf_new_name("Subtype"), pdf_new_name("Type3"));
#if 0
  /* OBSOLETE */
  pdf_add_dict(pk_fonts[pk_id].direct,
	       pdf_new_name("Name"), pdf_new_name(res_name));
#endif
  num_pk_fonts++;

  return pk_id;
}

pdf_obj *pk_font_resource (int pk_id)
{
  if (pk_id <0 || pk_id >= num_pk_fonts)
    ERROR ("Invalid pk_id: %d", pk_id);

  if (!pk_fonts[pk_id].indirect)
    pk_fonts[pk_id].indirect = pdf_ref_obj(pk_fonts[pk_id].direct);

  return pdf_link_obj(pk_fonts[pk_id].indirect);
}

char *pk_font_used (int pk_id)
{
  if (pk_id < 0 || pk_id >= num_pk_fonts)
    ERROR ("Invalid pk_id: %d", pk_id);

  return pk_fonts[pk_id].used_chars;
}

static void do_skip(FILE *fp, unsigned long length) 
{
  while (length-- > 0)
    fgetc(fp);
}

static void add_raster_data (pdf_obj *glyph, long w, long h,
			     int dyn_f, int run_color,
			     unsigned char *pk_data,
			     unsigned char *eod)

     /* First define some macros to be used as "in-line" functions */
#define advance_nybble() \
{ \
  if (partial_byte) { \
    partial_byte=0; \
    pk_data++; \
  } else \
    partial_byte=1; \
}

#define current_nybble() (partial_byte? (*pk_data%16): (*pk_data/16))
#define set_bits(n) {\
  int i; \
  for (i=0; i<(n); i++) { \
    row[next_col] |= (128u>>next_bit++); \
    if (next_bit > 7) { \
      next_bit = 0; \
      next_col += 1; \
    } \
  } \
}

#define skip_bits(n) {\
  next_col += (n)/8; \
  next_bit += (n)%8; \
  if (next_bit > 7) { \
    next_bit -= 8; \
    next_col += 1; \
  } \
}

#define get_bit(p,n) (p[(n)/8] & (128u>>((n)%8)))

{
  long i, w_bytes, repeat_count, run_count = 0;
  int partial_byte = 0;
  unsigned char *row;

  w_bytes = (w+7)/8;
  row = NEW(w_bytes, unsigned char);
  /* Make sure we output "h" rows of data */
  if (dyn_f == 14) {
    for (i = 0; i < h; i++) {
      int j, next_col = 0, next_bit = 0;
      memset(row, 0, w_bytes);
      for (j=0; j<w; j++) {
	if (get_bit(pk_data,i*w+j)) {
	  skip_bits(1);
	} else {
	  set_bits(1);
	}
      }
      pdf_add_stream(glyph, (char *) row, w_bytes);
    }
  } else
    for (i = 0; i < h; i++) {
      int next_col = 0, next_bit = 0;
      int j, row_bits_left = w;
      /* Initialize row to all zeros */
      memset(row, 0, w_bytes);
      repeat_count = 0;
      /* Fill any run left over from previous rows */
      if (run_count != 0) {
	int nbits;
	nbits = MIN(w, run_count);
	run_count -= nbits;
	switch (run_color) {
	case 1:  /* This is actually white ! */
	  set_bits(nbits);
	  break;
	case 0:
	  skip_bits(nbits);
	  break;
	}
	row_bits_left -= nbits;
      }
      /* Read nybbles until we have a full row */
      while (pk_data < eod && row_bits_left>0) {
	int com_nyb;
	long packed = 0;
	com_nyb = current_nybble();
	if (com_nyb == 15) {
	  repeat_count = 1;
	  advance_nybble();
	  continue;
	} else if (com_nyb == 14) {
	  advance_nybble();
	}
	/* Get a packed number */
	{
	  int nyb;
	  nyb = current_nybble();
	  /* Test for single nybble case */
	  if (nyb > 0 && nyb <= dyn_f) {
	    packed = nyb;
	    advance_nybble();
	  }
	  if (nyb > dyn_f) {
	    advance_nybble();
	    packed = (nyb-dyn_f-1)*16+current_nybble()+dyn_f+1; 
	    advance_nybble();
	  }
	  if (nyb == 0) {
	    int nnybs = 1;
	    while (current_nybble() == 0) {
	      advance_nybble();
	      nnybs += 1;
	    }
	    packed = 0;
	    while (nnybs) {
	      packed = packed*16 + current_nybble();
	      advance_nybble();
	      nnybs -= 1;
	    }
	    packed += (13-dyn_f)*16-15+dyn_f;
	  }
	}
	if (com_nyb == 14) {
	  repeat_count = packed;
	  continue;
	}
	{
	  int nbits;
	  run_count = packed;    
	  run_color = !run_color;
	  nbits = MIN (row_bits_left, run_count);
	  run_count -= nbits;
	  row_bits_left -= nbits;
	  switch (run_color) {
	  case 1: 
	    set_bits(nbits);
	    break;
	  case 0:
	    skip_bits(nbits);
	    break;
	  }
	}
	continue;
      }
      pdf_add_stream(glyph, (char *) row, w_bytes);
      /* Duplicate the row "repeat_count" times */
      for (j = 0; j < repeat_count; j++) {
	pdf_add_stream (glyph, (char *) row, w_bytes);
      }
      /* Skip repeat_count interations */
      i += repeat_count;
    }
  RELEASE (row);
}

static void do_preamble (FILE *fp)
{
  /* Check for id byte */
  if (fgetc(fp) == 89) {
    /* Skip comment */
    do_skip(fp, get_unsigned_byte(fp));
    /* Skip other header info.  It's normally used for verifying this
       is the file wethink it is */
    do_skip(fp, 16);
  } else {
    ERROR("embed_pk_font: PK ID byte is incorrect.  Are you sure this is a PK file?");
  }
  return;
}

#define SHORT_FORM 1
#define MED_FORM   2
#define LONG_FORM  3
static void do_character (FILE *fp, unsigned char flag,
			  char *used_chars, double pix2charu,
			  pdf_obj *char_procs, double *widths, long *bbox)
{
  int format;
  unsigned long packet_length = 0, code = 0;
  /* Last three bits of flag determine packet size in a complex way */
  if ((flag & 4) == 0) {
    format = SHORT_FORM;
  } else if ((flag & 7) == 7) {
    format = LONG_FORM;
  } else {
    format = MED_FORM;
  }

  switch (format) {
  case SHORT_FORM:
    packet_length = (flag & 3) * 256u + get_unsigned_byte(fp);
    code = get_unsigned_byte(fp);
    break;
  case MED_FORM:
    packet_length = (flag & 3) * 65536ul + get_unsigned_pair(fp);
    code = get_unsigned_byte(fp);
    break;
  case LONG_FORM:
    packet_length = get_unsigned_quad(fp);
    code = get_unsigned_quad(fp);
    if (code > 255)
      ERROR ("Unable to handle long characters in PK files");
    break;
  }

  if (used_chars[code%256]) {
    int dyn_f;
    unsigned long tfm_width = 0;
    long dm=0, dx=0, dy=0, w=0, h=0, hoff=0, voff=0;
    dyn_f = flag/16;
    switch (format) {
    case SHORT_FORM:
      tfm_width = get_unsigned_triple(fp);
      dm = get_unsigned_byte(fp);
      w  = get_unsigned_byte(fp);
      h  = get_unsigned_byte(fp);
      hoff = get_signed_byte(fp);
      voff = get_signed_byte(fp);
      packet_length -= 8;
      break;
    case MED_FORM:
      tfm_width = get_unsigned_triple(fp);
      dm = get_unsigned_pair(fp);
      w  = get_unsigned_pair(fp);
      h  = get_unsigned_pair(fp);
      hoff = get_signed_pair(fp);
      voff = get_signed_pair(fp);
      packet_length -= 13;
      break;
    case LONG_FORM:
      tfm_width = get_signed_quad(fp);
      dx = get_signed_quad(fp);
      dy = get_signed_quad(fp);
      w  = get_signed_quad(fp);
      h  = get_signed_quad(fp);
      hoff = get_signed_quad(fp);
      voff = get_signed_quad(fp);
      packet_length -= 28;
      break;
    }
    {
      pdf_obj *glyph;
      double char_width;
      long llx, lly, urx, ury;
      int  len;

      char_width = ROUND(1000.0*tfm_width/(((double) (1<<20))*pix2charu), 0.1);
      widths[code%256] = char_width;
      llx = -hoff; lly = voff - h; urx = w - hoff; ury = voff;
      if (llx < bbox[0]) bbox[0] = llx;
      if (lly < bbox[1]) bbox[1] = lly;
      if (urx > bbox[2]) bbox[2] = urx;
      if (ury > bbox[3]) bbox[3] = ury;
      glyph = pdf_new_stream(STREAM_COMPRESS);
      /*
       * The following line is a "metric" for the PDF reader:
       *
       * PDF Reference Reference, 4th ed., p.385.
       *
       * The wx (first operand of d1) must be consistent with the corresponding
       * width in the font's Widths array. The format string of sprint() must be
       * consistent with write_number() in pdfobj.c.
       */
      len = sprintf(work_buffer, "%.10g 0 %ld %ld %ld %ld d1\n", char_width, llx, lly, urx, ury);
      pdf_add_stream(glyph, work_buffer, len);
      /*
       * Acrobat dislike transformation [0 0 0 0 dx dy].
       * PDF Reference, 4th ed., p.147, says,
       *
       *   Use of a noninvertible matrix when painting graphics objects can result in
       *   unpredictable behavior.
       *
       * but it does not forbid use of such transformation.
       */
      if (w != 0 && h != 0 && packet_length != 0) {
	unsigned char *pk_data;
	long read_len;
	/* Scale and translate origin to lower left corner for raster data */
	len = sprintf(work_buffer, "q\n%ld 0 0 %ld %ld %ld cm\n", w, h, llx, lly);
	pdf_add_stream(glyph, work_buffer, len);
	len = sprintf(work_buffer, "BI\n/W %ld\n/H %ld\n/IM true\n/BPC 1\nID ", w, h);
	pdf_add_stream(glyph, work_buffer, len);
	pk_data = NEW(packet_length, unsigned char);
	if ((read_len = fread(pk_data, 1, packet_length, fp))!= packet_length) {
	  ERROR("Only %ld bytes PK packet read. (expected %ld bytes)", read_len, packet_length);
	}
	add_raster_data(glyph, w, h, dyn_f, (flag&8)>>3, pk_data, pk_data+packet_length);
	RELEASE(pk_data);
	len = sprintf(work_buffer, "\nEI\nQ");
	pdf_add_stream(glyph, work_buffer, len);
      } /* Otherwise we embed an empty stream :-( */
      pk_char2name(work_buffer, (unsigned char) (code%256));
      pdf_add_dict(char_procs, pdf_new_name(work_buffer), pdf_ref_obj(glyph));
      pdf_release_obj(glyph);
    }
  } else {
    do_skip(fp, packet_length);
  }
}

#define PK_XXX1  240
#define PK_XXX2  241
#define PK_XXX3  242
#define PK_XXX4  243
#define PK_YYY   244
#define PK_POST  245
#define PK_NO_OP 246
#define PK_PRE   247

static void do_pk_font (int pk_id)
{
  FILE *fp;
  int opcode, code, firstchar, lastchar;
  pdf_obj *charprocs, *tmp;
  double pix2charu, widths[256];
  long   bbox[4];

  if (!pk_fonts[pk_id].indirect)
    return;

  if (verbose)
    MESG("(Type3[PK]:%s", pk_fonts[pk_id].tex_name);
  if (verbose > 1)
    MESG("[%s]", pk_fonts[pk_id].filename);

  fp = MFOPEN(pk_fonts[pk_id].filename, FOPEN_RBIN_MODE);
  if (!fp)
    ERROR("Could not open PK file.");

  charprocs = pdf_new_dict();
  pix2charu = 72000.0/((double) font_dpi)/pk_fonts[pk_id].ptsize;
  bbox[0] = 0; bbox[1] = 0; bbox[2] = 0; bbox[3] = 0;
  while ((opcode = fgetc(fp)) >= 0 && opcode != PK_POST) {
    if (opcode < 240) {
      do_character(fp, (unsigned char)opcode,
		   pk_fonts[pk_id].used_chars, pix2charu, charprocs, widths, bbox);
    } else { /* A command byte */
      switch (opcode) {
      case PK_NO_OP: break;
      case PK_XXX1: do_skip(fp, get_unsigned_byte(fp));   break;
      case PK_XXX2: do_skip(fp, get_unsigned_pair(fp));   break;
      case PK_XXX3: do_skip(fp, get_unsigned_triple(fp)); break;
      case PK_XXX4: do_skip(fp, get_unsigned_quad(fp));   break;
      case PK_YYY:  do_skip(fp, 4);  break;
      case PK_PRE:  do_preamble(fp); break;
      }
    }
  }
  MFCLOSE(fp);
  pdf_add_dict(pk_fonts[pk_id].direct,
	       pdf_new_name("CharProcs"), pdf_ref_obj(charprocs));
  pdf_release_obj(charprocs);

  /*
   * Resources:
   *  PDF Reference 4th ed. describes it as "Optional but strongly recommended".
   *  There are no reason to put it in our case, but we will put this.
   *  We do not care about compatibility with Acrobat 2.x. (See implementation
   *  note 47, Appendix H of PDF Ref., 4th ed.).
   */
  {
    pdf_obj *procset;

    procset = pdf_new_dict();
    tmp     = pdf_new_array();
    pdf_add_array(tmp, pdf_new_name("PDF"));
    pdf_add_array(tmp, pdf_new_name("ImageB"));
    pdf_add_dict(procset, pdf_new_name("ProcSet"), tmp);
    pdf_add_dict(pk_fonts[pk_id].direct, pdf_new_name("Resources"), procset);
  }
  /*
   * Encoding
   */
  {
    pdf_obj *encoding;
    int prev = -2;

    encoding = pdf_new_dict();
    tmp      = pdf_new_array();
    firstchar = 255; lastchar = 0;
    for (code = 0; code < 256; code++) {
      if (pk_fonts[pk_id].used_chars[code]) {
	if (code < firstchar) firstchar = code;
	if (code > lastchar)  lastchar  = code;
	if (code != prev + 1)
	  pdf_add_array(tmp, pdf_new_number(code));
	pk_char2name(work_buffer, (unsigned char) code);
	pdf_add_array(tmp, pdf_new_name(work_buffer));
	prev = code;
      }
    }
    pdf_add_dict(encoding, pdf_new_name("Type"), pdf_new_name("Encoding"));
    pdf_add_dict(encoding, pdf_new_name("Differences"), tmp);
    pdf_add_dict(pk_fonts[pk_id].direct,
		 pdf_new_name("Encoding"), pdf_ref_obj(encoding));
    pdf_release_obj(encoding);
  }
  /*
   * FontBBox: Accurate value is rather important.
   */
  {
    tmp = pdf_new_array();
    pdf_add_array(tmp, pdf_new_number(bbox[0]));
    pdf_add_array(tmp, pdf_new_number(bbox[1]));
    pdf_add_array(tmp, pdf_new_number(bbox[2]));
    pdf_add_array(tmp, pdf_new_number(bbox[3]));
    pdf_add_dict(pk_fonts[pk_id].direct, pdf_new_name("FontBBox"), tmp);
  }
  /*
   * Widths:
   *  Indirect reference preffered. (See PDF Reference)
   */
  {
    tmp = pdf_new_array ();
    for (code = firstchar; code <= lastchar; code++) {
      if (pk_fonts[pk_id].used_chars[code])
	pdf_add_array(tmp, pdf_new_number(widths[code]));
      else
	pdf_add_array(tmp, pdf_new_number(0));
    }
    pdf_add_dict(pk_fonts[pk_id].direct, pdf_new_name("Widths"), pdf_ref_obj(tmp));
    pdf_release_obj(tmp);
  }
  /*
   * FontMatrix
   */
  {
    pdf_obj *tmp = pdf_new_array();
    pdf_add_array(tmp, pdf_new_number(0.001*pix2charu));
    pdf_add_array(tmp, pdf_new_number(0.0));
    pdf_add_array(tmp, pdf_new_number(0.0));
    pdf_add_array(tmp, pdf_new_number(0.001*pix2charu));
    pdf_add_array(tmp, pdf_new_number(0.0));
    pdf_add_array(tmp, pdf_new_number(0.0));
    pdf_add_dict (pk_fonts[pk_id].direct, pdf_new_name("FontMatrix"), tmp);
  }
  pdf_add_dict(pk_fonts[pk_id].direct,
	       pdf_new_name("FirstChar"), pdf_new_number(firstchar));
  pdf_add_dict(pk_fonts[pk_id].direct,
	       pdf_new_name("LastChar"), pdf_new_number(lastchar));

  if (verbose) 
    MESG(")");
}

void pk_close_all (void)
{
  int i;
  for (i=0; i<num_pk_fonts; i++) {
    do_pk_font (i);
    pdf_release_obj (pk_fonts[i].direct);
    pdf_release_obj (pk_fonts[i].indirect);
    RELEASE (pk_fonts[i].tex_name);
    RELEASE (pk_fonts[i].filename);
    RELEASE (pk_fonts[i].used_chars);
  }
  if (pk_fonts)
    RELEASE (pk_fonts);
}
