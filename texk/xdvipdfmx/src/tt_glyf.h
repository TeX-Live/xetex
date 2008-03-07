/*  $Header: /home/cvsroot/dvipdfmx/src/tt_glyf.h,v 1.1 2004/09/11 14:50:29 hirata Exp $
    
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

#ifndef _TT_GLYF_H_
#define _TT_GLYF_H_

struct tt_glyph_desc
{
  USHORT gid;
  USHORT ogid; /* GID in original font */
  USHORT advw, advh;
  SHORT  lsb, tsb;
  SHORT  llx, lly, urx, ury;
  ULONG  length;
  BYTE  *data;
};

struct tt_glyphs
{
  USHORT num_glyphs;
  USHORT max_glyphs;
  USHORT last_gid;
  USHORT emsize;
  USHORT dw;           /* optimal value for DW */
  USHORT default_advh; /* default value */
  SHORT  default_tsb;  /* default value */
  struct tt_glyph_desc *gd;
  unsigned char *used_slot;
};

extern struct tt_glyphs *tt_build_init (void);
extern void   tt_build_finish (struct tt_glyphs *g);

extern USHORT tt_add_glyph  (struct tt_glyphs *g, USHORT gid, USHORT new_gid);
extern USHORT tt_get_index  (struct tt_glyphs *g, USHORT gid);
extern USHORT tt_find_glyph (struct tt_glyphs *g, USHORT gid);

extern int    tt_build_tables (sfnt *sfont, struct tt_glyphs *g);
extern int    tt_get_metrics  (sfnt *sfont, struct tt_glyphs *g);

#endif /* _TT_GLYF_H_ */
