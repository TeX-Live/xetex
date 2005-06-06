/*  $Header: /home/cvsroot/dvipdfmx/src/cidtype0.c,v 1.15 2004/02/15 12:59:42 hirata Exp $
    
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
 * CID-Keyed Font support:
 *
 *  Only CFF/OpenType CID-Keyed Font with Type 2 charstrings is supported.
 *
 * TODO:
 *
 *   Widths from TFM ?, from Charstring ?, from OpenType table ?
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

/* ROUND() */
#include "numbers.h"
/* mangle_name() */
#include "dpxutil.h"

/* Font info. from OpenType tables */
#include "sfnt.h"
#include "tt_aux.h"
/* Metrics */
#include "tt_table.h"

#include "cff_types.h"
#include "cff_limits.h"
#include "cff.h"
#include "cff_dict.h"
#include "cs_type2.h"

/* typedef CID in cmap.h */
#include "cmap.h"
#include "type0.h"
#include "cid.h"
#include "cid_p.h"
#include "cidtype0.h"

#ifndef HAVE_KPSE_OPENTYPE_FORMAT
#define kpse_opentype_format kpse_program_binary_format
#endif

static int verbose = 0;

void
CIDFont_type0_set_verbose (void)
{
  verbose++;
}

/*
 * PDF Reference 3rd. ed., p.340, "Glyph Metrics in CID Fonts".
 */
#ifndef PDFUNIT
#define PDFUNIT(v) (ROUND((1000.0*(v))/(head->unitsPerEm),1))
#endif

static void
add_CIDHMetrics (sfnt *sfont, pdf_obj *fontdict,
		 unsigned char *CIDToGIDMap, unsigned short last_cid,
		 struct tt_maxp_table *maxp,
		 struct tt_head_table *head, struct tt_longMetrics *hmtx)
{
  pdf_obj *w_array, *an_array = NULL;
  long   cid, start = 0, prev = 0;
  double defaultAdvanceWidth;
  int    empty = 1;

  defaultAdvanceWidth = PDFUNIT(hmtx[0].advance);
  /*
   * We alway use format:
   *  c [w_1 w_2 ... w_n]
   */
  w_array = pdf_new_array();
  for (cid = 0; cid <= last_cid; cid++) {
    USHORT gid;
    double advanceWidth;
    gid = CIDToGIDMap ? ((CIDToGIDMap[2*cid] << 8)|CIDToGIDMap[2*cid+1]) : cid;
    if (gid >= maxp->numGlyphs || (cid != 0 && gid == 0))
      continue;
    advanceWidth = PDFUNIT(hmtx[gid].advance);
    if (advanceWidth == defaultAdvanceWidth) {
      if (an_array) {
	pdf_add_array(w_array, pdf_new_number(start));
	pdf_add_array(w_array, an_array);
	an_array = NULL;
	empty = 0;
      }
    } else {
      if (cid != prev + 1 &&  an_array) {
	  pdf_add_array(w_array, pdf_new_number(start));
	  pdf_add_array(w_array, an_array);
	  an_array = NULL;
	  empty = 0;
      }
      if (an_array == NULL) {
	an_array = pdf_new_array();
	start = cid;
      }
      pdf_add_array(an_array, pdf_new_number(advanceWidth));
      prev = cid;
    }
  }

  if (an_array) {
    pdf_add_array(w_array, pdf_new_number(start));
    pdf_add_array(w_array, an_array);
    empty = 0;
  }

  /*
   * We always write DW for older MacOS X's preview app.
   * PDF Reference 2nd. ed, wrongly described default value of DW as 0, and
   * MacOS X's (up to 10.2.8) preview app. implements this wrong description.
   */
  pdf_add_dict(fontdict,
	       pdf_new_name ("DW"),
	       pdf_new_number(defaultAdvanceWidth));
  if (!empty) {
    pdf_add_dict(fontdict,
		 pdf_new_name ("W"),
		 pdf_link_obj(pdf_ref_obj(w_array)));
  }
  pdf_release_obj(w_array);

  return;
}

static void
add_CIDVMetrics (sfnt *sfont, pdf_obj *fontdict,
		 unsigned char *CIDToGIDMap, unsigned short last_cid,
		 struct tt_maxp_table *maxp,
		 struct tt_head_table *head, struct tt_longMetrics *hmtx)
{
  pdf_obj *w2_array, *an_array = NULL;
  long cid, prev, start;
  struct tt_VORG_table *vorg;
  struct tt_os2__table *os2;
  struct tt_vhea_table *vhea  = NULL;
  struct tt_longMetrics *vmtx = NULL;
  double defaultAdvanceHeight, defaultVertOriginY;
  int    empty = 1;

  /*
   * No accurate vertical metrics can be obtained by simple way if the
   * font does not have VORG table. Only CJK fonts may have VORG.
   */
  if (sfnt_find_table_pos(sfont, "VORG") <= 0)
    return;

  vorg = tt_read_VORG_table(sfont);
  defaultVertOriginY = PDFUNIT(vorg->defaultVertOriginY);
  if (sfnt_find_table_pos(sfont, "vhea") > 0)
    vhea = tt_read_vhea_table(sfont);
  if (vhea && sfnt_find_table_pos(sfont, "vmtx") > 0) {
    sfnt_locate_table(sfont, "vmtx");
    vmtx = tt_read_longMetrics(sfont, maxp->numGlyphs, vhea->numOfLongVerMetrics);
  }
  /*
   * OpenType font must have OS/2 table.
   */
  os2 = tt_read_os2__table(sfont);
  defaultVertOriginY   = PDFUNIT(os2->sTypoAscender);
  defaultAdvanceHeight = PDFUNIT(os2->sTypoAscender - os2->sTypoDescender);
  RELEASE(os2);

  w2_array = pdf_new_array();
  start = prev = 0;
  for (cid = 0; cid <= last_cid; cid++) {
    USHORT i, gid;
    double advanceHeight, vertOriginX, vertOriginY;
    gid = CIDToGIDMap ? ((CIDToGIDMap[2*cid] << 8)|CIDToGIDMap[2*cid+1]) : cid;
    if (gid >= maxp->numGlyphs || (cid != 0 && gid == 0))
      continue;
    advanceHeight = vmtx ? PDFUNIT(vmtx[gid].advance) : defaultAdvanceHeight;
    vertOriginX   = PDFUNIT(hmtx[gid].advance*0.5);
    vertOriginY   = defaultVertOriginY;
    for (i = 0;
	 i < vorg->numVertOriginYMetrics && gid > vorg->vertOriginYMetrics[i].glyphIndex;
	 i++) {
      if (gid == vorg->vertOriginYMetrics[i].glyphIndex)
	vertOriginY = PDFUNIT(vorg->vertOriginYMetrics[i].vertOriginY);
    }
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
     * This form may hit Acrobat's implementation limit of array element size, 8192.
     * AFPL GhostScript 8.11 stops with rangecheck error with this. Maybe GS's bug?
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
		 pdf_new_name ("W2"),
		 pdf_link_obj(pdf_ref_obj(w2_array)));
  }
  pdf_release_obj(w2_array);

  if (vorg->vertOriginYMetrics)
    RELEASE(vorg->vertOriginYMetrics);
  RELEASE(vorg);

  if (vmtx)
    RELEASE(vmtx);
  if (vhea)
    RELEASE(vhea);

  return;
}

void
add_CIDMetrics (sfnt *sfont, pdf_obj *fontdict,
		unsigned char *CIDToGIDMap, unsigned short last_cid, int need_vmetrics)
{
  struct tt_longMetrics *hmtx;
  struct tt_head_table  *head;
  struct tt_hhea_table  *hhea;
  struct tt_maxp_table  *maxp;

  /*
   * Read head, hhea, maxp:
   *
   *   unitsPerEm       --> head
   *   numHMetrics      --> hhea
   *   numGlyphs        --> maxp
   */
  head = tt_read_head_table(sfont);
  maxp = tt_read_maxp_table(sfont);
  hhea = tt_read_hhea_table(sfont);

  sfnt_locate_table(sfont, "hmtx");
  hmtx = tt_read_longMetrics(sfont, maxp->numGlyphs, hhea->numberOfHMetrics);

  add_CIDHMetrics(sfont, fontdict, CIDToGIDMap, last_cid, maxp, head, hmtx);
  if (need_vmetrics)
    add_CIDVMetrics(sfont, fontdict, CIDToGIDMap, last_cid, maxp, head, hmtx);

  RELEASE(hmtx);
  RELEASE(hhea);
  RELEASE(maxp);
  RELEASE(head);

  return;
}

/*
 * Create an instance of embeddable font.
 */
void
CIDFont_type0_dofont (CIDFont *font)
{
  pdf_obj  *stream_dict;
  sfnt     *sfont;
  cff_font *cff;
  cff_index *charstrings, *topdict, *fdarray, *private, *idx;
  cff_charsets *charset = NULL;
  cff_fdselect *fdselect = NULL;
  long topdict_offset, fdarray_offset;
  long charstring_len, max_len;
  long destlen = 0;
  long size, offset = 0;
  card8 *dest, *data;
  unsigned short num_glyphs, max_glyphs;
  CID    cid, last_cid;
  card16 cid_count, cs_count;
  int    fd, prev_fd, parent_id;
  char  *used_chars;
  unsigned char *CIDToGIDMap = NULL;
  int    i;

  ASSERT(font);

  pdf_add_dict(font->fontdict, 
	       pdf_new_name("FontDescriptor"),
	       pdf_link_obj(pdf_ref_obj(font->descriptor)));

  if (CIDFont_is_BaseFont(font) || !CIDFont_get_embedding(font))
    return;

  if ((parent_id = CIDFont_get_parent_id(font, 0)) < 0 &&
      (parent_id = CIDFont_get_parent_id(font, 1)) < 0)
    ERROR("No parent Type 0 font !");
  if (!(used_chars = Type0Font_get_usedchars(Type0Font_cache_get(parent_id))))
    ERROR("Cannot create embedable font without used_chars.");

  if ((sfont = sfnt_open(font->filename)) == NULL)
    ERROR("Could not open OpenType font file: %s", font->filename);
  if (sfnt_read_table_directory(sfont, 0) < 0)
    ERROR("Could not read OpenType table directory.");
  if (sfont->type != SFNT_TYPE_POSTSCRIPT ||
      (offset = sfnt_find_table_pos(sfont, "CFF ")) == 0)
    ERROR("Not a CFF/OpenType font ?");

  if ((cff = cff_open(sfont->stream, offset, font->options->index)) == NULL)
    ERROR("Could not open CFF font.");
  if (!(cff->flag & FONTTYPE_CIDFONT))
    ERROR("Not a CIDFont.");

  /* Offsets from DICTs */
  cff_read_charsets(cff);
  cff_read_fdselect(cff);
  cff_read_fdarray(cff);
  cff_read_private(cff);

  cff_read_subrs(cff);

  offset = (long) cff_dict_get(cff->topdict, "CharStrings", 0);
  cff_seek_set(cff, offset);
  idx = cff_get_index_header(cff);
  /* offset is now absolute offset ... bad */
  offset = ftell(cff->stream);
  
  if ((cs_count = idx->count) < 2) {
    ERROR("No valid charstring data found.");
  }

#define MAX_GLYPHS 1024
  max_glyphs = MAX_GLYPHS;

  /* New Charsets data */
  charset = NEW(1, cff_charsets);
  charset->format = 0;
  charset->num_entries = 0;
  charset->data.glyphs = NEW(max_glyphs, s_SID);

  /* New FDSelect data */
  fdselect = NEW(1, cff_fdselect);
  fdselect->format = 3;
  fdselect->num_entries = 0;
  fdselect->data.ranges = NEW(max_glyphs, cff_range3);

  /* New CharStrings INDEX */
  charstrings = cff_new_index(max_glyphs);
  max_len = 2 * CS_STR_LEN_MAX;
  charstrings->data = NEW(max_len, card8);
  charstring_len = 0;

  if (cff_dict_known(cff->topdict, "CIDCount")) {
    cid_count = (card16) cff_dict_get(cff->topdict, "CIDCount", 0);
  } else {
    cid_count = CFF_CIDCOUNT_DEFAULT;
  }

  /*
   * FD number is preserved.
   */
  add_to_used_chars2(used_chars, 0); /* .notdef */
  data = NEW(CS_STR_LEN_MAX, card8);
  cid = 0; last_cid = 0; prev_fd = -1; num_glyphs = 0;
  CIDToGIDMap = NEW(2*cid_count, unsigned char);
  memset(CIDToGIDMap, 0, 2*cid_count);
  for (cid = 0; cid < cid_count; cid++) {
    if (is_used_char2(used_chars, cid)) {
      unsigned short gid;
      if (cid > cid_count - 1)
	ERROR("Invalid CID %u (CIDCount %u).", cid, cid_count);
      gid = cff_charsets_lookup(cff, cid);
      if (cid != 0 && gid == 0) {
	WARN("Missing character: %u", cid);
	continue;
      }
      if ((size = (idx->offset)[gid+1] - (idx->offset)[gid])
	  > CS_STR_LEN_MAX)
	ERROR("Charstring too long: gid=%u", gid);
      if (charstring_len + CS_STR_LEN_MAX >= max_len) {
	max_len = charstring_len + 2 * CS_STR_LEN_MAX;
	charstrings->data = RENEW(charstrings->data, max_len, card8);
      }
      if (num_glyphs >= max_glyphs) {
	max_glyphs += MAX_GLYPHS;
	charstrings->offset   = RENEW(charstrings->offset,
				      max_glyphs+1, l_offset);
	charset->data.glyphs  = RENEW(charset->data.glyphs, max_glyphs, s_SID);
	fdselect->data.ranges = RENEW(fdselect->data.ranges,
				      max_glyphs, cff_range3);
      }

      (charstrings->offset)[num_glyphs] = charstring_len + 1;
      seek_absolute(cff->stream, offset + (idx->offset)[gid] - 1);
      fread(data, 1, size, cff->stream);
      fd = cff_fdselect_lookup(cff, gid);
      charstring_len += cs_copy_charstring(charstrings->data + charstring_len,
					   max_len - charstring_len,
					   data, size,
					   cff->gsubr, (cff->subrs)[fd], 0, 0, NULL);
      if (cid > 0 && gid > 0) {
	charset->data.glyphs[charset->num_entries] = cid;
	charset->num_entries += 1;
      }
      if (fd != prev_fd) {
	fdselect->data.ranges[fdselect->num_entries].first = num_glyphs;
	fdselect->data.ranges[fdselect->num_entries].fd = fd;
	fdselect->num_entries += 1;
	prev_fd = fd;
      }
      CIDToGIDMap[2*cid] = (gid >> 8) & 0xff; CIDToGIDMap[2*cid+1] = gid & 0xff;
      last_cid = cid;
      num_glyphs++;
    }
  }
  RELEASE(data);
  cff_release_index(idx);
  
  (charstrings->offset)[num_glyphs] = charstring_len + 1;
  charstrings->count = num_glyphs;
  charstring_len     = cff_index_size(charstrings);
  cff->num_glyphs    = num_glyphs;

  /* discard old one, set new data */
  cff_release_charsets(cff->charsets);
  cff->charsets = charset;
  cff_release_fdselect(cff->fdselect);
  cff->fdselect = fdselect;

  /* no Global subr */
  if (cff->gsubr)
    cff_release_index(cff->gsubr);
  cff->gsubr = cff_new_index(0);

  /*  DICT sizes (offset set to long int) */
  topdict = cff_new_index(1);
  fdarray = cff_new_index(cff->num_fds);
  private = cff_new_index(cff->num_fds);
  cff_dict_remove(cff->topdict, "Private");  /* some bad font may have */
  cff_dict_remove(cff->topdict, "Encoding"); /* some bad font may have */
  (topdict->offset)[1] = cff_dict_pack(cff->topdict,
				       (card8 *) work_buffer,
				       WORK_BUFFER_SIZE) + 1;
  for (i=0;i<cff->num_fds;i++) {
    if (cff->subrs[i]) {
      cff_release_index(cff->subrs[i]);
      cff->subrs[i] = NULL;
    }
    size = 0;
    if ((cff->private)[i]) {
      cff_dict_remove((cff->private)[i], "Subrs"); /* no Subrs */
      size = cff_dict_pack((cff->private)[i],
			   (card8 *) work_buffer, WORK_BUFFER_SIZE);
      if (size < 1) { /* Private had contained only Subr */
	cff_dict_remove((cff->fdarray)[i], "Private");
      }
    }
    (private->offset)[i+1] = (private->offset)[i] + size;
    (fdarray->offset)[i+1] = (fdarray->offset)[i] +
      cff_dict_pack((cff->fdarray)[i],
		    (card8 *) work_buffer, WORK_BUFFER_SIZE);
  }

  destlen = 4; /* header size */
  destlen += cff_set_name(cff, font->fontname);
  destlen += cff_index_size(topdict);
  destlen += cff_index_size(cff->string);
  destlen += cff_index_size(cff->gsubr);
  destlen += (charset->num_entries) * 2 + 1; /* charset format 0 */
  destlen += (fdselect->num_entries) * 3 + 5; /* fdselect format 3 */
  destlen += charstring_len;
  destlen += cff_index_size(fdarray);
  destlen += (private->offset)[private->count] - 1; /* Private is not INDEX */

  dest = NEW(destlen, card8);

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

  /* charset */
  cff_dict_set(cff->topdict, "charset", 0, offset);
  offset += cff_pack_charsets(cff, dest + offset, destlen - offset);

  /* FDSelect */
  cff_dict_set(cff->topdict, "FDSelect", 0, offset);
  offset += cff_pack_fdselect(cff, dest + offset, destlen - offset);

  /* CharStrings */
  cff_dict_set(cff->topdict, "CharStrings", 0, offset);
  offset += cff_pack_index(charstrings, dest + offset, charstring_len);
  cff_release_index(charstrings);

  /* FDArray and Private */
  cff_dict_set(cff->topdict, "FDArray", 0, offset);
  fdarray_offset = offset;
  offset += cff_index_size(fdarray);

  fdarray->data = NEW(fdarray->offset[fdarray->count] - 1, card8);
  for (i=0;i<cff->num_fds;i++) {
    size = (private->offset)[i+1] - (private->offset)[i];
    if ((cff->private)[i] && size > 0) {
      cff_dict_pack((cff->private)[i], dest + offset, size);
      cff_dict_set((cff->fdarray)[i], "Private", 0, size);
      cff_dict_set((cff->fdarray)[i], "Private", 1, offset);
    }
    cff_dict_pack((cff->fdarray)[i],
		  fdarray->data + (fdarray->offset)[i] - 1,
		  fdarray->offset[fdarray->count] - 1);
    offset += size;
  }
  cff_pack_index(fdarray, dest + fdarray_offset, cff_index_size(fdarray));
  cff_release_index(fdarray);
  cff_release_index(private);

  /* Finally Top DICT */
  topdict->data = NEW(topdict->offset[topdict->count] - 1, card8);
  cff_dict_pack(cff->topdict,
		topdict->data, topdict->offset[topdict->count] - 1);
  cff_pack_index(topdict, dest + topdict_offset, cff_index_size(topdict));
  cff_release_index(topdict);

  /* Copyright and Trademark Notice ommited... */

  /*
   * DW, W, DW2 and W2:
   * Those values are obtained from OpenType table (not TFM).
   */
  {
    add_CIDMetrics(sfont, font->fontdict, CIDToGIDMap, last_cid,
		   ((CIDFont_get_parent_id(font, 1) < 0) ? 0 : 1));
    if (CIDToGIDMap)
      RELEASE(CIDToGIDMap);
  }

  cff_close(cff);
  sfnt_close(sfont);

  if (verbose > 1)
    MESG("[%u/%u glyphs][%ld bytes]", num_glyphs, cs_count, offset);

  /*
   * CIDSet:
   * Length of CIDSet stream is not clear. Must be 8192 bytes long?
   */
  {
    pdf_obj *cidset;

    cidset = pdf_new_stream(STREAM_COMPRESS);
    pdf_add_stream(cidset, used_chars, (last_cid/8)+1);
    pdf_add_dict(font->descriptor,
		 pdf_new_name("CIDSet"), pdf_link_obj(pdf_ref_obj(cidset)));
    pdf_release_obj(cidset);
  }

  /*
   * FontFile
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
		 pdf_new_name("CIDFontType0C"));
    pdf_add_stream(fontfile, (char *) dest, offset);
    pdf_release_obj(fontfile);
    RELEASE(dest);
  }

  return;
}

int
CIDFont_type0_open (CIDFont *font, char *name, CIDSysInfo *cmap_csi, cid_opt *opt)
{
  CIDSysInfo *csi;
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
    ERROR("Not a CFF/OpenType font?");

  if ((cff = cff_open(sfont->stream, offset, opt->index)) == NULL)
    ERROR("Cannot read CFF font data");

  if (!(cff->flag & FONTTYPE_CIDFONT)) {
    cff_close(cff);
    sfnt_close(sfont);
    return -1;
  }

  csi = NEW(1, CIDSysInfo);
  csi->registry =
    cff_get_string(cff, (s_SID)cff_dict_get(cff->topdict, "ROS", 0));
  csi->ordering =
    cff_get_string(cff, (s_SID)cff_dict_get(cff->topdict, "ROS", 1));
  csi->supplement = (int)cff_dict_get(cff->topdict, "ROS", 2);

  if (cmap_csi) {
    if (strcmp(csi->registry, cmap_csi->registry) != 0 ||
	strcmp(csi->ordering, cmap_csi->ordering) != 0) {
      MESG("\nCharacter collection mismatched:\n");
      MESG("\tFont: %s-%s-%d\n", csi->registry, csi->ordering, csi->supplement);
      MESG("\tCMap: %s-%s-%d\n", cmap_csi->registry, cmap_csi->ordering, cmap_csi->supplement);
      ERROR("Inconsistent CMap specified for this font.");
    }
    if (csi->supplement < cmap_csi->supplement) {
      WARN("CMap have higher supplmement number.");
      WARN("Some characters may not be displayed or printed.");
    }
  }

  {
    char *short_fontname = cff_get_name(cff);
    if (!short_fontname)
      ERROR("No valid FontName found.");
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

  font->fontname = fontname;
  font->filename = strdup(fullname);
  font->subtype  = CIDFONT_TYPE0;
  font->csi      = csi;

  font->fontdict = pdf_new_dict();
  pdf_add_dict(font->fontdict,
	       pdf_new_name("Type"),
	       pdf_new_name("Font"));
  pdf_add_dict(font->fontdict,
	       pdf_new_name("Subtype"),
	       pdf_new_name("CIDFontType0"));

  /* getting font info. from TrueType tables */
  if ((font->descriptor
       = tt_get_fontdesc(sfont, &(opt->embed), 0)) == NULL)
    ERROR("Could not obtain neccesary font info.");

  if (opt->embed)
    mangle_name(fontname);

  pdf_add_dict(font->descriptor,
	       pdf_new_name("FontName"),
	       pdf_new_name(fontname));
  pdf_add_dict(font->fontdict, 
	       pdf_new_name("BaseFont"),
	       pdf_new_name(fontname));
  {
    pdf_obj *csi_dict = pdf_new_dict();
    pdf_add_dict(csi_dict,
		 pdf_new_name("Registry"),
		 pdf_new_string(csi->registry, strlen(csi->registry)));
    pdf_add_dict(csi_dict,
		 pdf_new_name("Ordering"),
		 pdf_new_string(csi->ordering, strlen(csi->ordering)));
    pdf_add_dict(csi_dict,
		 pdf_new_name("Supplement"),
		 pdf_new_number(csi->supplement));
    pdf_add_dict(font->fontdict, pdf_new_name("CIDSystemInfo"), csi_dict);
  }
  pdf_add_dict(font->fontdict, 
	       pdf_new_name("DW"),
	       pdf_new_number(1000)); /* not sure */

  sfnt_close(sfont);

  return 0;
}

void
CIDFont_type0_release(CIDFont *font)
{
  return;
}
