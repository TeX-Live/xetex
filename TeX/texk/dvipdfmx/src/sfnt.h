/*  $Header: /home/cvsroot/dvipdfmx/src/sfnt.h,v 1.5 2004/01/30 18:34:23 hirata Exp $
    
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

#ifndef _SFNT_H_
#define _SFNT_H_

#include <stdio.h>

#include "mfileio.h"
#include "numbers.h"

/* Data Types as described in Apple's TTRefMan */
typedef unsigned char  BYTE;
typedef signed char    CHAR;
typedef unsigned short USHORT;
typedef signed short   SHORT;
typedef unsigned long  ULONG;
typedef signed long    LONG;
typedef unsigned long  Fixed;   /* 16.16-bit signed fixed-point number */
typedef short          FWord;
typedef unsigned short uFWord;
typedef short          F2Dot14; /* 16-bit signed fixed number with the low
				   14 bits representing fraction. */

struct sfnt_table
{
  /* table header */
  char tag[5]; /* null terminated */
  ULONG check_sum;
  ULONG offset;
  ULONG length;
  /* table data */
  char *data;
};

#define SFNT_TABLE_REQUIRED (1 << 0)

struct sfnt_table_directory
{
  ULONG  version;         /* Fixed for Win */
  USHORT num_tables;
  USHORT search_range;
  USHORT entry_selector;
  USHORT range_shift;
  USHORT num_kept_tables; /* number of kept tables */
  char   *flag;           /* keep/omit */
  struct sfnt_table *tables;
};

/* sfnt resource */
#define SFNT_TYPE_TRUETYPE   (1 << 0)
#define SFNT_TYPE_OPENTYPE   (1 << 1)
#define SFNT_TYPE_POSTSCRIPT (1 << 2)
#define SFNT_TYPE_TTC        (1 << 4)

typedef struct
{
  FILE *stream;
  int   type;
  struct sfnt_table_directory *directory;
} sfnt;

/* Convert sfnt "fixed" type to double */
#define fixed(a) ((double)((a)%0x10000L)/(double)(0x10000L) + \
 (a)/0x10000L - (((a)/0x10000L > 0x7fffL) ? 0x10000L : 0))

/* get_***_*** from numbers.h */
#define sfnt_get_byte(s)   ((BYTE) get_unsigned_byte((s->stream)))
#define sfnt_get_char(s)   ((CHAR) get_signed_byte((s->stream)))
#define sfnt_get_ushort(s) ((USHORT) get_unsigned_pair((s->stream)))
#define sfnt_get_short(s)  ((SHORT) get_signed_pair((s->stream)))
#define sfnt_get_ulong(s)  ((ULONG) get_unsigned_quad((s->stream)))
#define sfnt_get_long(s)   ((LONG) get_signed_quad((s->stream)))

#define sfnt_seek_set(s, offset)   seek_absolute((s->stream), offset)
#define sfnt_read(dest, length, s) fread(dest, 1, length, (s->stream))

extern int put_big_endian (char *s, LONG q, int n);
#define sfnt_put_ushort(s, v) put_big_endian(s, v, 2);
#define sfnt_put_short(s, v)  put_big_endian(s, v, 2);
#define sfnt_put_ulong(s, v)  put_big_endian(s, v, 4);
#define sfnt_put_long(s, v)   put_big_endian(s, v, 4);

/* open/close */
extern sfnt *sfnt_open (const char *fullname);
extern void  sfnt_close (sfnt *sfont);

/* table directory */
extern int   sfnt_read_table_directory (sfnt *sfont, ULONG offset);
extern ULONG sfnt_find_table_len (sfnt *sfont, const char *tag);
extern ULONG sfnt_find_table_pos (sfnt *sfont, const char *tag);
extern ULONG sfnt_locate_table (sfnt *sfont, const char *tag);

extern void  sfnt_set_table (sfnt *sfont, const char *tag, char *data, ULONG length);
extern int   sfnt_require_table (sfnt *sfont, const char *tag, int must_exist);
extern long  sfnt_get_size (sfnt *sfont);
extern char *sfnt_build_font (sfnt *sfont, char *dest, long destlen);

#endif /* _SFNT_H_ */
