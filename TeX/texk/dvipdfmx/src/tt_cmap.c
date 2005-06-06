/*  $Header: /home/cvsroot/dvipdfmx/src/tt_cmap.c,v 1.5 2002/10/30 02:27:19 chofchof Exp $
    
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
  A large part of codes are brought from ttfdump-0.5.5.
*/

#include "system.h"
#include "mem.h"
#include "error.h"

#include "sfnt.h"
#include "tt_cmap.h"

/* format 0: byte encoding table */
struct cmap0
{
  BYTE glyphIndexArray[256];
};

static struct cmap0 *read_cmap0(sfnt *sfont, ULONG len)
{
  struct cmap0 *map;
  USHORT i;
  BYTE *array;

  if (len < 256)
    ERROR("invalid cmap subtable");

  map = NEW(1, struct cmap0);
  array = map->glyphIndexArray;

  for (i=0;i<256;i++)
    array[i] = sfnt_get_byte(sfont);

  return map;
}

static void release_cmap0(struct cmap0 *map)
{
  if (map)
    RELEASE(map);
}

static USHORT lookup_cmap0 (struct cmap0 *map, USHORT cc)
{
  if (cc > 255) {
    return 0;
  } else {
    return map->glyphIndexArray[cc & 0xff];
  }
}

/* format 2: high-byte mapping through table */
struct SubHeader
{
  USHORT firstCode;
  USHORT entryCount;
  SHORT  idDelta;
  USHORT idRangeOffset;
};

struct cmap2
{
  USHORT subHeaderKeys[256];
  struct SubHeader *subHeaders;
  USHORT *glyphIndexArray;
};

static struct cmap2 *read_cmap2(sfnt *sfont, ULONG len)
{
  struct cmap2 *map;
  USHORT *array, i, n = 0;
  USHORT numGlyphId;

  if (len < 512)
    ERROR("invalid cmap subtable");
    
  map = NEW(1, struct cmap2);
  array = map->subHeaderKeys;
    
  for (i=0;i<256;i++)
    array[i] = sfnt_get_ushort(sfont);

  for (i=0;i<256;i++) { /* find the max of subHeaderKeys */
    array[i] /= 8;
    if (n < array[i]) n = array[i];
  }
  n += 1; /* the number of subHeaders is one plus the max of subHeaderKeys */

  map->subHeaders = NEW(n, struct SubHeader); 
  for (i=0;i<n;i++) {
    (map->subHeaders)[i].firstCode = sfnt_get_ushort(sfont);
    (map->subHeaders)[i].entryCount = sfnt_get_ushort(sfont);
    (map->subHeaders)[i].idDelta = sfnt_get_short(sfont);
    (map->subHeaders)[i].idRangeOffset = sfnt_get_ushort(sfont);

    /* it makes things easier to let the offset starts from
     *  the beginning of glyphIndexArray */
    if ((map->subHeaders)[i].idRangeOffset != 0)
      (map->subHeaders)[i].idRangeOffset -= (2 + (n-i-1) * 8);
    /* (2 + (n-i-1) * sizeof(struct SubHeader)) */
  }

  /* caculate the length of glyphIndexArray, this is ugly, there should be
   * a better way to get this information. */
  numGlyphId = (USHORT)(len - 518 - n * 8);
  /* len - 518 - n * sizeof(struct SubHeader)) */
  numGlyphId /= 2;
  map->glyphIndexArray = NEW(numGlyphId, USHORT);
  for (i=0;i<numGlyphId;i++) {
    map->glyphIndexArray[i] = sfnt_get_ushort(sfont);
  }

  return map;
}

static void release_cmap2(struct cmap2 *map)
{
  if (map) {
    if (map->subHeaders) RELEASE(map->subHeaders);
    if (map->glyphIndexArray) RELEASE(map->glyphIndexArray);
    RELEASE(map);
  }
}

static USHORT lookup_cmap2 (struct cmap2 *map, USHORT cc) {
  USHORT idx = 0;
  SHORT idDelta;
  USHORT firstCode, entryCount, idRangeOffset;
  unsigned char hi, lo;
  USHORT i = 0;
   
  hi = cc >> 8;
  lo = cc & 0xff;

  /* select which subHeader to use */
  i = (map->subHeaderKeys)[hi];

  firstCode = (map->subHeaders)[i].firstCode;
  entryCount = (map->subHeaders)[i].entryCount;
  idDelta = (map->subHeaders)[i].idDelta;
  idRangeOffset = ((map->subHeaders)[i].idRangeOffset) / 2;

  if (lo >= firstCode && lo < firstCode+entryCount) {
    idRangeOffset += (lo - firstCode);
    idx = (map->glyphIndexArray)[idRangeOffset];
    if (idx != 0)
      idx = (idx + idDelta) & 0xffff;
  } else {
    idx = 0;
  }

  return idx;
}

/*
  format 4: segment mapping to delta values
  - Microsoft standard character to glyph index mapping table
*/
struct cmap4
{
  USHORT segCountX2;
  USHORT searchRange;
  USHORT entrySelector;
  USHORT rangeShift;
  USHORT *endCount;
  USHORT reservedPad;
  USHORT *startCount;
  USHORT *idDelta;
  USHORT *idRangeOffset;
  USHORT *glyphIndexArray;
};

static struct cmap4 *read_cmap4(sfnt *sfont, ULONG len)
{
  struct cmap4 *map;
  USHORT i, segCount;

  if (len < 8)
    ERROR("invalid cmap subtable");

  map = NEW(1, struct cmap4);

  map->segCountX2 = segCount = sfnt_get_ushort(sfont);
  map->searchRange = sfnt_get_ushort(sfont);
  map->entrySelector = sfnt_get_ushort(sfont);
  map->rangeShift = sfnt_get_ushort(sfont);
  
  segCount /= 2;

  map->endCount = NEW(segCount, USHORT);
  for (i=0; i<segCount; i++)
    (map->endCount)[i] = sfnt_get_ushort(sfont);

  map->reservedPad = sfnt_get_ushort(sfont);

  map->startCount = NEW(segCount, USHORT);
  for (i=0; i<segCount; i++)
    (map->startCount)[i] = sfnt_get_ushort(sfont);

  map->idDelta = NEW(segCount, USHORT);
  for (i=0; i<segCount; i++)
    (map->idDelta)[i] = sfnt_get_ushort(sfont);

  map->idRangeOffset = NEW(segCount, USHORT);
  for (i=0; i<segCount; i++)
    (map->idRangeOffset)[i] = sfnt_get_ushort(sfont);

  /* caculate the length of glyphIndexArray, this is ugly, there should be
   * a better way to get this information. */
  len -= 16 + 4 * segCount * 2;
  len /= 2;
  map->glyphIndexArray = NEW(len, USHORT);
  for (i=0; i<len; i++)
    (map->glyphIndexArray)[i] = sfnt_get_ushort(sfont);

  return map;
}

static void release_cmap4(struct cmap4 *map)
{
  if (map) {
    if (map->endCount) RELEASE(map->endCount);
    if (map->startCount) RELEASE(map->startCount);
    if (map->idDelta) RELEASE(map->idDelta);
    if (map->idRangeOffset) RELEASE(map->idRangeOffset);
    if (map->glyphIndexArray) RELEASE(map->glyphIndexArray);
    RELEASE(map);
  }
}

static USHORT lookup_cmap4 (struct cmap4 *map, USHORT cc)
{
  USHORT idx = 0;
  USHORT i, j, segCount;

  /*
    o segments are sorted in order of increasing endCode values
    o last segment maps 0xffff to gid 0 (?)
  */
  i = segCount = (map->segCountX2)/2;
  while (i-- > 0 &&  cc <= map->endCount[i]) {
    if (cc >= map->startCount[i]) {
      if (map->idRangeOffset[i] == 0) {
	idx = (cc + map->idDelta[i]) & 0xffff;
      } else {
	j = map->idRangeOffset[i] - (segCount - i) * 2;
	j = (cc - map->startCount[i]) + (j/2);
	idx = map->glyphIndexArray[j];
	/* The idDelta arithmetic is modulo 65536 */
	if (idx != 0)
	  idx = (idx + map->idDelta[i]) & 0xffff;
      }
      break;
    }
  } /* while */

  return idx;
}

/* format 6: trimmed table mapping */
struct cmap6
{
  USHORT firstCode;
  USHORT entryCount;
  USHORT *glyphIndexArray;
};

static struct cmap6 *read_cmap6(sfnt *sfont, ULONG len)
{
  struct cmap6 *map;
  USHORT i;
  
  if (len < 4)
    ERROR("invalid cmap subtable");

  map =  NEW(1, struct cmap6);
  map->firstCode = sfnt_get_ushort(sfont);
  len = map->entryCount = sfnt_get_ushort(sfont);
  map->glyphIndexArray = NEW(map->entryCount, USHORT);
  
  for (i=0; i<len; i++)
    (map->glyphIndexArray)[i] = sfnt_get_ushort(sfont);

  return map;
}

static void release_cmap6(struct cmap6 *map)
{
  if (map) {
    if (map->glyphIndexArray) RELEASE(map->glyphIndexArray);
    RELEASE(map);
  }
}

static USHORT lookup_cmap6 (struct cmap6 *map, USHORT cc)
{
  USHORT idx;

  idx = cc - map->firstCode; 
  if (idx < map->entryCount) {
    return  map->glyphIndexArray[idx];
  } else { /* out of range -- missing glyph */
    return 0;
  }
}

/* format 8:  mixed 16-bit and 32-bit coverage - not supported */
/* format 10: trimmed array - not supported */

/*
  format 12: segmented coverage

   startGlyphID is 32-bit long, however, GlyphID is still 16-bit long !
*/
struct charGroup {
  ULONG startCharCode;
  ULONG endCharCode;
  ULONG startGlyphID;
};

struct cmap12 {
  ULONG  nGroups;
  struct charGroup *groups;
};

/* ULONG length ! */
static struct cmap12 *read_cmap12 (sfnt *sfont, ULONG len)
{
  struct cmap12 *map;
  ULONG i;
  
  if (len < 4)
    ERROR("invalid cmap subtable");

  map =  NEW(1, struct cmap12);
  map->nGroups = sfnt_get_ulong(sfont);
  
  map->groups = NEW(map->nGroups, struct charGroup);

  for (i=0;i<map->nGroups;i++) {
    (map->groups)[i].startCharCode = sfnt_get_ulong(sfont);
    (map->groups)[i].endCharCode = sfnt_get_ulong(sfont);
    (map->groups)[i].startGlyphID = sfnt_get_ulong(sfont);
  }

  return map;
}

static void release_cmap12 (struct cmap12 *map)
{
  if (map) {
    if (map->groups) RELEASE(map->groups);
    RELEASE(map);
  }
}

static USHORT lookup_cmap12 (struct cmap12 *map, ULONG cccc)
{
  USHORT idx = 0;
  ULONG i;

  i = map->nGroups;
  while (i-- >= 0 && cccc <= map->groups[i].endCharCode) {
    if (cccc >= (map->groups[i]).startCharCode) {
      idx = (USHORT) ((map->groups[i]).startGlyphID & 0xffff);
      break;
    }
  }

  return idx;
}

/* read cmap */
tt_cmap *tt_cmap_read (sfnt *sfont, USHORT platform, USHORT encoding)
{
  tt_cmap *cmap = NULL;
  ULONG offset, length = 0;
  USHORT version, num_subtables, p_id, e_id;
  USHORT i;

  if (sfont == NULL)
    ERROR("tt_cmap_read(): invalid font type");

  offset = sfnt_locate_table(sfont, "cmap");

  version = sfnt_get_ushort(sfont);
  num_subtables = sfnt_get_ushort(sfont);

  for (i=0;i<num_subtables;i++) {
    ULONG st_offset;
    p_id = sfnt_get_ushort(sfont);
    e_id = sfnt_get_ushort(sfont);
    st_offset = sfnt_get_ulong(sfont);
    if ((p_id == platform) && (e_id == encoding)) {
      offset += st_offset;
      break;
    }
  }

  if (i == num_subtables)
    return NULL;

  cmap = NEW(1, tt_cmap);
  cmap->map = NULL;
  cmap->platform = platform;
  cmap->encoding = encoding;

  sfnt_seek_set(sfont, offset);
  cmap->format = sfnt_get_ushort(sfont);

  /* length and version (language) is ULONG for format 8, 10, 12 ! */
  if (cmap->format <= 6) {
    length = sfnt_get_ushort(sfont);
    cmap->language = sfnt_get_ushort(sfont); /* language (only for Mac) */
  } else {
    if (sfnt_get_ushort(sfont) != 0) { /* reverved - 0 */
      fprintf(stderr, "unrecognized cmap subtable format\n");
      tt_cmap_release(cmap);
      return NULL;
    } else {
      length = sfnt_get_ulong(sfont);
      cmap->language = sfnt_get_ulong(sfont);
    }
  }
  
  switch(cmap->format) {
  case 0:
    cmap->map = read_cmap0(sfont, length);
    break;
  case 2:
    cmap->map = read_cmap2(sfont, length);
    break;
  case 4:
    cmap->map = read_cmap4(sfont, length);
    break;
  case 6:
    cmap->map = read_cmap6(sfont, length);
    break;
  case 12:
    fprintf(stderr, "** Warning: trying to use UCS-4 mapping table **\n");
    cmap->map = read_cmap12(sfont, length);
    break;
  default:
    fprintf(stderr, "unrecognized cmap format\n");
    tt_cmap_release(cmap);
    return NULL;
  }

  if (cmap->map == NULL) {
    tt_cmap_release(cmap);
    return NULL;
  }

  return cmap;
}

void tt_cmap_release (tt_cmap *cmap)
{

  if (cmap) {
    if (cmap->map) {
      switch(cmap->format) {
      case 0:
	release_cmap0(cmap->map);
	break;
      case 2:
	release_cmap2(cmap->map);
	break;
      case 4:
	release_cmap4(cmap->map);
	break;
      case 6:
	release_cmap6(cmap->map);
	break;
      case 12:
	fprintf(stderr, "** Warning: trying to use UCS-4 mapping table **\n");
	release_cmap12(cmap->map);
	break;
      default:
	ERROR("Unrecognized CMAP format");
      }
    }
    RELEASE(cmap);
  }

  return;
}

/* === lookup === */

USHORT tt_cmap_lookup (tt_cmap *cmap, USHORT cc)
{
  USHORT idx = 0;

  if (cmap == NULL)
    ERROR("not cmap table found");

  switch (cmap->format) {
  case 0:
    idx = lookup_cmap0(cmap->map, cc);
    break;
  case 2:
    idx = lookup_cmap2(cmap->map, cc);
    break;
  case 4:
    idx = lookup_cmap4(cmap->map, cc);
    break;
  case 6:
    idx = lookup_cmap6(cmap->map, cc);
    break;
  case 12:
    fprintf(stderr, "** Warning: trying to use UCS-4 mapping table **\n");
    idx = lookup_cmap12(cmap->map, cc); /* wrong */
    break;
  default:
    ERROR("Unrecognized TrueType cmap subtable format");
    break;
  }

  return idx;
}
