/*  $Header: /home/cvsroot/dvipdfmx/src/tt_cmap.h,v 1.3 2002/10/30 02:27:19 chofchof Exp $
    
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

#ifndef _TT_CMAP_H_
#define _TT_CMAP_H_

#include "sfnt.h"

/* TrueType cmap table */
typedef struct
{
  USHORT format;
  USHORT platform;
  USHORT encoding;
  ULONG  language; /* or version, only for Mac */
  void  *map;
} tt_cmap;

/* Paltform ID */
#define TT_MAC 1u
#define TT_WIN 3u
/* Platform-specific encoding ID */
/* Windows */
#define TT_WIN_SYMBOL  0u
#define TT_WIN_UNICODE 1u
#define TT_WIN_SJIS    2u
#define TT_WIN_RPC     3u
#define TT_WIN_BIG5    4u
#define TT_WIN_WANSUNG 5u
#define TT_WIN_JOHAB   6u
/* not supported */
#define TT_WIN_UCS4    12u

/* Mac */
#define TT_MAC_ROMAN               0u
#define TT_MAC_JAPANESE            1u
#define TT_MAC_TRADITIONAL_CHINESE 2u
#define TT_MAC_KOREAN              3u
#define TT_MAC_SIMPLIFIED_CHINESE  25u

extern tt_cmap *tt_cmap_read (sfnt *sfont, USHORT platform, USHORT encoding);

extern USHORT   tt_cmap_lookup (tt_cmap *cmap, USHORT cc);
extern void     tt_cmap_release (tt_cmap *cmap);

#endif /* _TT_CMAP_H_ */
