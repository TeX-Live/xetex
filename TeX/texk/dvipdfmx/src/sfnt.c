/*  $Header: /home/cvsroot/dvipdfmx/src/sfnt.c,v 1.7 2004/01/30 18:34:23 hirata Exp $
    
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
  A large part of codes are copied from dvipdfm-0.13.2c.
*/

#include <string.h>

#include "system.h"
#include "mem.h"
#include "error.h"
#include "mfileio.h"
#include "sfnt.h"

sfnt *sfnt_open (const char *fullname)
{
  sfnt *sfont = NULL;
  ULONG type;

  sfont = NEW(1, sfnt);

  if (fullname == NULL ||
      (sfont->stream = MFOPEN (fullname, FOPEN_RBIN_MODE)) == NULL) {
    fprintf (stderr, "sfnt_open: Unable to find or open font file\n");
    RELEASE(sfont);
    return NULL;
  }

/*
  type:
   `true' (0x74727565): TrueType (Mac)
   `typ1' (0x74797031) (Mac): PostScript font housed in a sfnt wrapper
   0x00010000: TrueType (Win)/OpenType
   `OTTO': PostScript CFF font with OpenType wrapper
   `ttcf': TrueType Collection
*/
#define SFNT_TRUETYPE   0x00010000ul
#define SFNT_OPENTYPE   0x00010000ul
#define SFNT_POSTSCRIPT 0x4f54544ful
#define SFNT_TTC        0x74746366ul

  type = sfnt_get_ulong(sfont);

  if (type == SFNT_TRUETYPE) {
    sfont->type = SFNT_TYPE_TRUETYPE;
  } else if (type == SFNT_OPENTYPE) {
    sfont->type = SFNT_TYPE_OPENTYPE;
  } else if (type == SFNT_POSTSCRIPT) { 
    sfont->type = SFNT_TYPE_POSTSCRIPT;
  } else if (type == SFNT_TTC) {
    sfont->type = SFNT_TYPE_TTC;
  }

  rewind(sfont->stream);

  sfont->directory = NULL;

  return sfont;
}

static void release_directory (struct sfnt_table_directory *td)
{
  USHORT i;

  if (td) {
    if (td->tables) {
      for (i=0;i<(td->num_tables);i++) {
	if ((td->tables)[i].data)
	  RELEASE((td->tables)[i].data);
      }
      RELEASE (td->tables);
    }
    if (td->flag)
      RELEASE (td->flag);
    RELEASE (td);
  }

  return;
}

void sfnt_close (sfnt *sfont)
{

  if (sfont) {
    if (sfont->directory)
      release_directory(sfont->directory);
    if (sfont->stream)
      MFCLOSE(sfont->stream);
    RELEASE(sfont);
  }

  return;
}

int put_big_endian (char *s, LONG q, int n)
{
  int i;

  for (i=n-1;i>=0;i--) {
    s[i] = (char) (q & 0xff);
    q >>= 8;
  }

  return n;
}

/* Convert four-byte number to big endianess in a machine independent way */
static void convert_tag (char *tag, unsigned long u_tag)
{
  int i;

  for (i=3; i>= 0; i--) {
    tag[i] = (char)(u_tag % 256);
    u_tag /= 256;
  }
  tag[4] = 0;

  return;
}

static unsigned max2floor(unsigned n)
/* Computes the max power of 2 <= n */
{
  int i = 1;

  while (n > 1) {
    n /= 2;
    i *= 2;
  }

  return i;
}

static unsigned log2floor(unsigned  n)
/* Computes the log2 of the max power of 2 <= n */
{
  unsigned i = 0;

  while (n > 1) {
    n /= 2;
    i += 1;
  }

  return i;
}

static ULONG sfnt_calc_checksum(BYTE *data, ULONG length)
{
  ULONG sum = 0;
  BYTE *end = data+length;
  int count = 0;

  while (data < end) {
    sum += ((*data++) << (8*(3-count)));
    count = ((count+1) & 3);
  }

  return sum;
}

static int find_table_index (struct sfnt_table_directory *td, const char *tag)
{
  int idx;

  if (td == NULL)
    ERROR("Invalid data");

  for (idx=0; idx<td->num_tables; idx++) {
    if (tag && strncmp((td->tables)[idx].tag, tag, 4) == 0)
      break;
  }

  if (idx >= td->num_tables) {
    idx = -1;
  }

  return idx;
}

void sfnt_set_table (sfnt *sfont, const char *tag, char *data, ULONG length)
{
  struct sfnt_table_directory *td;
  int idx;

  if (sfont == NULL)
    ERROR("Invalid data");

  td = sfont->directory;
  if ((idx = find_table_index(td, tag)) < 0) {
#ifdef DEBUG
    fprintf(stderr, "table `%s' not found. creating ...", tag);
#endif
    idx = td->num_tables;
    td->num_tables++;
    td->tables = RENEW(td->tables, td->num_tables, struct sfnt_table);
    strncpy((td->tables)[idx].tag, tag, 4);
    (td->tables)[idx].tag[4] = 0;
  }

  (td->tables)[idx].check_sum = sfnt_calc_checksum((BYTE *) data, length);
  (td->tables)[idx].offset = 0;
  (td->tables)[idx].length = length;
  (td->tables)[idx].data = data;

  return;
}

ULONG sfnt_find_table_len (sfnt *sfont, const char *tag)
{
  struct sfnt_table_directory *td;
  int idx;
  ULONG result = 0;

  if (sfont == NULL || tag == NULL)
    ERROR("Invalid data");

  td = sfont->directory;
  if ((idx = find_table_index(td, tag)) >= 0) {
    result = (td->tables)[idx].length;
  }

  return result;
}

ULONG sfnt_find_table_pos (sfnt *sfont, const char *tag) 
{
  struct sfnt_table_directory *td;
  int idx;
  ULONG result = 0;

  if (sfont == NULL || tag == NULL)
    ERROR("Invalid data");

  td = sfont->directory;
  if ((idx = find_table_index(td, tag)) >= 0) {
    result = (td->tables)[idx].offset;
  }

  return result;
}

ULONG sfnt_locate_table(sfnt *sfont, const char *tag)
{
  ULONG offset;

  if (sfont == NULL || tag == NULL)
    ERROR("Invalid data");

  offset = sfnt_find_table_pos(sfont, tag);

  if (offset == 0)
    ERROR("in sfnt_locate_table(): sfnt table not found");

  sfnt_seek_set(sfont, offset);

  return offset;
}

int sfnt_read_table_directory(sfnt *sfont, ULONG offset)
{
  struct sfnt_table_directory *td;
  unsigned long i;

  if (sfont == NULL || sfont->stream == NULL)
    ERROR("file not opend");

  sfnt_seek_set(sfont, offset);

#ifdef DEBUG
  fprintf(stderr, "table directory: offset %lu \n", offset);
#endif

  if (sfont->directory) {
#ifdef DEBUG
    fprintf(stderr, "discarding old table directory.\n");
#endif 
    release_directory(sfont->directory);    
  }

  sfont->directory = td = NEW (1, struct sfnt_table_directory);
  td->version = sfnt_get_ulong(sfont);
  td->num_tables = sfnt_get_ushort(sfont);
  td->search_range = sfnt_get_ushort(sfont);
  td->entry_selector = sfnt_get_ushort(sfont);
  td->range_shift = sfnt_get_ushort(sfont);
  td->flag = NEW(td->num_tables, char);
  td->tables = NEW (td->num_tables, struct sfnt_table);
#ifdef DEBUG
  fprintf(stderr, " number of tables: %d\n", td->num_tables);
#endif
  for (i=0; i < td->num_tables; i++) {
    unsigned long u_tag;

    u_tag = sfnt_get_ulong(sfont);
    convert_tag ((td->tables)[i].tag, u_tag);
    (td->tables)[i].check_sum = sfnt_get_ulong(sfont);
    (td->tables)[i].offset = sfnt_get_ulong(sfont);
    (td->tables)[i].length = sfnt_get_ulong(sfont);
    (td->tables)[i].data = NULL;
    (td->flag)[i] = 0;
#ifdef DEBUG
    fprintf(stderr, " table `%s' (%ld):\n", (td->tables)[i].tag, i);
    fprintf(stderr, "  checksum 0x%08lx, offset, %lu, length %lu\n", (td->tables)[i].check_sum, (td->tables)[i].offset, (td->tables)[i].length);
#endif
  }

  td->num_kept_tables = 0;

  return 0;
}

/* 
   o All tables begin on four byte boundries, and pad any remaining space
     between tables with zeros

   o Entries in the Table Directory must be sorted in ascending order by tag

   o The head table contains checksum of the whole font file.
      To compute:  first set it to 0, sum the entire font as ULONG,
      then store 0xB1B0AFBA - sum.
*/

long sfnt_get_size (sfnt *sfont)
{
  long size;
  struct sfnt_table_directory *td;
  int i, count;

  if (sfont == NULL || sfont->directory == NULL)
    ERROR("in snft_get_size(): no table directory read.");

  td = sfont->directory;
  /*
    all tables should begin at four-byte boundaries.
    first table always start at four-byte boundary
  */
  size = 0;
  count = 0;
  for (i=0; i<(td->num_tables); i++) {
    /* this table must exist in FontFile */
    if ((td->flag)[i] & SFNT_TABLE_REQUIRED) {
      size += (td->tables)[i].length;
      size += ((size % 4) != 0) ? 4 - (size % 4) : 0; /* alignment */
      count += 1;
    }
  }

  if (count != td->num_kept_tables) {
    ERROR("Some TrueType table lost ?");
  }

  size += count*16;
  size += 12;

  return size;
}

int sfnt_require_table (sfnt *sfont, const char *tag, int must_exist)
{
  int status = 0;
  struct sfnt_table_directory *td;
  int idx;

  if (sfont == NULL || sfont->directory == NULL)
    ERROR("invalid data type");

  td = sfont->directory;
  idx = find_table_index(td, tag);
  if (idx >= 0) {
    (td->flag)[idx] |= SFNT_TABLE_REQUIRED;
    td->num_kept_tables++;
    status = 0;
  } else {
    if (must_exist)
      status = -1;
  }

  return status;
}

char *sfnt_build_font (sfnt *sfont, char *dest, long destlen)
{
  struct sfnt_table_directory *td;
  ULONG offset = 0;
  int i, new_search_range;
  char *p;

#ifdef DEBUG
  fprintf(stderr, "entered sfnt_build_font()\n");
  fprintf(stderr, " %lu bytes of memory space available for destination buffer\n", destlen);
#endif 

  if (sfont == NULL || sfont->directory == NULL)
    ERROR("invalid data type");

  p = dest;
  td = sfont->directory;
  /* Header */
  p += sfnt_put_ulong (p, td->version);
  p += sfnt_put_ushort (p, td->num_kept_tables);
  new_search_range = max2floor(td->num_kept_tables) * 16;
  p += sfnt_put_ushort (p, new_search_range);
  p += sfnt_put_ushort (p, log2floor(td->num_kept_tables));
  p += sfnt_put_ushort (p, (td->num_kept_tables)*16-new_search_range);

  /* Computer start of actual tables (after headers) */
  offset = 12 + 16 * td->num_kept_tables;
  for (i=0; i<(td->num_tables); i++) {
    /* this table must exist in FontFile */
    if ((td->flag)[i] & SFNT_TABLE_REQUIRED) {
      if ((offset % 4) != 0) {
	unsigned padlen;
	padlen =  4 - (offset % 4);
	memset(dest+offset, 0, padlen);
	offset += padlen;
#ifdef DEBUG
	fprintf(stderr, " ** aligning table on four-byte boundaries **\n");
#endif
      }

      p += sprintf (p, "%4s", (td->tables)[i].tag);
      p += sfnt_put_ulong (p, (td->tables)[i].check_sum);
      p += sfnt_put_ulong (p, offset);
      p += sfnt_put_ulong (p, (td->tables)[i].length);

#ifdef DEBUG
      fprintf(stderr, " table `%s' (%d):\n", (td->tables)[i].tag, i);
      fprintf(stderr, "  checksum 0x%08lx, offset %lu, length %lu\n", (td->tables)[i].check_sum, offset, (td->tables)[i].length);
#endif 

      if ((td->tables)[i].data == NULL) {
#ifdef DEBUG
	fprintf(stderr, "  copying table data from file\n");
#endif
	if (sfont->stream == NULL)
	  ERROR("in sfnt_build_font(): file not opened");
	sfnt_seek_set(sfont, (td->tables)[i].offset); 
	sfnt_read(dest+offset, (td->tables)[i].length, sfont);
      } else {
#ifdef DEBUG
	fprintf(stderr, "  copying table data from memory\n");
#endif
	memcpy(dest+offset, (td->tables)[i].data, (td->tables)[i].length);
	RELEASE((td->tables)[i].data);
	(td->tables)[i].data = NULL;
      }
      /* Set offset for next table */
      offset += (td->tables)[i].length;

      if (offset > destlen) ERROR ("Uh oh");
    }
  }

#ifdef DEBUG
  fprintf(stderr, " %lu bytes written\n", offset + 1);
  fprintf(stderr, "sfnt_build_font() completed\n");
#endif

  return dest;
}
