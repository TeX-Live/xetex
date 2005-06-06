/*  $Header: /home/cvsroot/dvipdfmx/src/tt_table.c,v 1.6 2004/01/30 18:34:24 hirata Exp $
    
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

#include <stdio.h>

#include "system.h"
#include "error.h"
#include "mem.h"
#include "mfileio.h"

#include "sfnt.h"
#include "tt_table.h"

/*
  tables contains information refered by other tables
  maxp->numGlyphs, etc --> loca, etc
  hhea->numberOfHMetrics --> hmtx
  head->indexToLocFormat --> loca
  head->glyphDataFormat --> glyf
*/

char *tt_pack_head_table (struct tt_head_table *table)
{
  int i;
  char *p, *data;

  if (table == NULL)
    ERROR("passed NULL pointer\n");

  p = data = NEW(TT_HEAD_TABLE_SIZE, char);
  p += sfnt_put_ulong(p, table->version);
  p += sfnt_put_ulong(p, table->fontRevision);
  p += sfnt_put_ulong(p, table->checkSumAdjustment);
  p += sfnt_put_ulong(p, table->magicNumber);
  p += sfnt_put_ushort(p, table->flags);
  p += sfnt_put_ushort(p, table->unitsPerEm);
  for (i=0; i<8; i++) {
    *(p++) = (table->created)[i];
  }
  for (i=0; i<8; i++) {
    *(p++) = (table->modified)[i];
  }
  p += sfnt_put_short(p, table->xMin);
  p += sfnt_put_short(p, table->yMin);
  p += sfnt_put_short(p, table->xMax);
  p += sfnt_put_short(p, table->yMax);
  p += sfnt_put_ushort(p, table->macStyle);
  p += sfnt_put_ushort(p, table->lowestRecPPEM);
  p += sfnt_put_short(p, table->fontDirectionHint);
  p += sfnt_put_short(p, table->indexToLocFormat);
  p += sfnt_put_short(p, table->glyphDataFormat);

  return data;
}

struct tt_head_table *tt_read_head_table (sfnt *sfont)
{
  int i;
  struct tt_head_table *table = NULL;

  sfnt_locate_table(sfont, "head");

  table = NEW(1, struct tt_head_table);
  table->version = sfnt_get_ulong(sfont);
  table->fontRevision = sfnt_get_ulong(sfont);
  table->checkSumAdjustment = sfnt_get_ulong(sfont);
  table->magicNumber = sfnt_get_ulong(sfont);
  table->flags = sfnt_get_ushort(sfont);
  table->unitsPerEm = sfnt_get_ushort(sfont);
  for (i=0; i<8; i++) {
    (table->created)[i] = sfnt_get_byte (sfont);
  }
  for (i=0; i<8; i++) {
    (table->modified)[i] = sfnt_get_byte (sfont);
  }
  table->xMin = sfnt_get_short(sfont);
  table->yMin = sfnt_get_short(sfont);
  table->xMax = sfnt_get_short(sfont);
  table->yMax = sfnt_get_short(sfont);
  table->macStyle = sfnt_get_short(sfont);
  table->lowestRecPPEM = sfnt_get_short(sfont);
  table->fontDirectionHint = sfnt_get_short(sfont);
  table->indexToLocFormat = sfnt_get_short(sfont);
  table->glyphDataFormat = sfnt_get_short(sfont);

  return table;
}

char *tt_pack_maxp_table (struct tt_maxp_table *table)
{
  char *p, *data;

  p = data = NEW(TT_MAXP_TABLE_SIZE, char);
  p += sfnt_put_ulong(p, table->version);
  p += sfnt_put_ushort(p, table->numGlyphs);
  p += sfnt_put_ushort(p, table->maxPoints);
  p += sfnt_put_ushort(p, table->maxContours);
  p += sfnt_put_ushort(p, table->maxComponentPoints);
  p += sfnt_put_ushort(p, table->maxComponentContours);
  p += sfnt_put_ushort(p, table->maxZones);
  p += sfnt_put_ushort(p, table->maxTwilightPoints);
  p += sfnt_put_ushort(p, table->maxStorage);
  p += sfnt_put_ushort(p, table->maxFunctionDefs);
  p += sfnt_put_ushort(p, table->maxInstructionDefs);
  p += sfnt_put_ushort(p, table->maxStackElements);
  p += sfnt_put_ushort(p, table->maxSizeOfInstructions);
  p += sfnt_put_ushort(p, table->maxComponentElements);
  p += sfnt_put_ushort(p, table->maxComponentDepth);

  return data;
}

struct tt_maxp_table *tt_read_maxp_table (sfnt *sfont)
{
  struct tt_maxp_table *table = NULL;

  sfnt_locate_table(sfont, "maxp");

  table = NEW(1, struct tt_maxp_table);
  table->version = sfnt_get_ulong(sfont);
  table->numGlyphs = sfnt_get_ushort(sfont);
  table->maxPoints = sfnt_get_ushort(sfont);
  table->maxContours = sfnt_get_ushort(sfont);
  table->maxComponentPoints = sfnt_get_ushort(sfont);
  table->maxComponentContours = sfnt_get_ushort(sfont);
  table->maxZones = sfnt_get_ushort(sfont);
  table->maxTwilightPoints = sfnt_get_ushort(sfont);
  table->maxStorage = sfnt_get_ushort(sfont);
  table->maxFunctionDefs = sfnt_get_ushort(sfont);
  table->maxInstructionDefs = sfnt_get_ushort(sfont);
  table->maxStackElements = sfnt_get_ushort(sfont);
  table->maxSizeOfInstructions = sfnt_get_ushort(sfont);
  table->maxComponentElements = sfnt_get_ushort(sfont);
  table->maxComponentDepth = sfnt_get_ushort(sfont);

  return table;
}

char *tt_pack_hhea_table (struct tt_hhea_table *table)
{
  int i;
  char *p, *data;

  p = data = NEW(TT_HHEA_TABLE_SIZE, char);
  p += sfnt_put_ulong(p, table->version);
  p += sfnt_put_short(p, table->Ascender);
  p += sfnt_put_short(p, table->Descender);
  p += sfnt_put_short(p, table->LineGap);
  p += sfnt_put_ushort(p, table->advanceWidthMax);
  p += sfnt_put_short(p, table->minLeftSideBearing);
  p += sfnt_put_short(p, table->minRightSideBearing);
  p += sfnt_put_short(p, table->xMaxExtent);
  p += sfnt_put_short(p, table->caretSlopeRise);
  p += sfnt_put_short(p, table->caretSlopeRun);
  for(i=0;i<5;i++) {
    p += sfnt_put_short(p, (table->reserved)[i]);
  }
  p += sfnt_put_short(p, table->metricDataFormat);
  p += sfnt_put_ushort(p, table->numberOfHMetrics);

  return data;
}

struct tt_hhea_table *tt_read_hhea_table (sfnt *sfont)
{
  int i;
  struct tt_hhea_table *table = NULL;

  sfnt_locate_table(sfont, "hhea");

  table = NEW(1, struct tt_hhea_table);
  table->version = sfnt_get_ulong(sfont);
  table->Ascender = sfnt_get_short (sfont);
  table->Descender = sfnt_get_short(sfont);
  table->LineGap = sfnt_get_short(sfont);
  table->advanceWidthMax = sfnt_get_ushort(sfont);
  table->minLeftSideBearing = sfnt_get_short(sfont);
  table->minRightSideBearing = sfnt_get_short(sfont);
  table->xMaxExtent = sfnt_get_short(sfont);
  table->caretSlopeRise = sfnt_get_short(sfont);
  table->caretSlopeRun = sfnt_get_short(sfont);
  for(i=0;i<5;i++) {
    (table->reserved)[i] = sfnt_get_short(sfont);
  }
  table->metricDataFormat = sfnt_get_short(sfont);
  if (table->metricDataFormat != 0)
    ERROR("unknown metricDaraFormat");
  table->numberOfHMetrics = sfnt_get_ushort(sfont);

  return table;
}

/* vhea */
char *tt_pack_vhea_table (struct tt_vhea_table *table)
{
  int i;
  char *p, *data;

  p = data = NEW(TT_VHEA_TABLE_SIZE, char);
  p += sfnt_put_ulong(p, table->version);
  p += sfnt_put_short(p, table->vertTypoAscender);
  p += sfnt_put_short(p, table->vertTypoDescender);
  p += sfnt_put_short(p, table->vertTypoLineGap);
  p += sfnt_put_short(p, table->advanceHeightMax);  /* ushort ? */
  p += sfnt_put_short(p, table->minTopSideBearing);
  p += sfnt_put_short(p, table->minBottomSideBearing);
  p += sfnt_put_short(p, table->yMaxExtent);
  p += sfnt_put_short(p, table->caretSlopeRise);
  p += sfnt_put_short(p, table->caretSlopeRun);
  p += sfnt_put_short(p, table->caretOffset);
  for(i=0;i<5;i++) {
    p += sfnt_put_short(p, (table->reserved)[i]);
  }
  p += sfnt_put_ushort(p, table->numOfLongVerMetrics);

  return data;
}

struct tt_vhea_table *tt_read_vhea_table (sfnt *sfont)
{
  int i;
  struct tt_vhea_table *table = NULL;

  sfnt_locate_table(sfont, "vhea");

  table = NEW(1, struct tt_vhea_table);
  table->version = sfnt_get_ulong(sfont);
  table->vertTypoAscender = sfnt_get_short (sfont);
  table->vertTypoDescender = sfnt_get_short(sfont);
  table->vertTypoLineGap = sfnt_get_short(sfont);
  table->advanceHeightMax = sfnt_get_short(sfont); /* ushort ? */
  table->minTopSideBearing = sfnt_get_short(sfont);
  table->minBottomSideBearing = sfnt_get_short(sfont);
  table->yMaxExtent = sfnt_get_short(sfont);
  table->caretSlopeRise = sfnt_get_short(sfont);
  table->caretSlopeRun = sfnt_get_short(sfont);
  table->caretOffset = sfnt_get_short(sfont);
  for(i=0;i<5;i++) {
    (table->reserved)[i] = sfnt_get_short(sfont);
  }
  table->numOfLongVerMetrics = sfnt_get_ushort(sfont);

  return table;
}

/* post table */
struct tt_post_table *tt_read_post_table (sfnt *sfont)
{ /* this table may not exist */
  struct tt_post_table *table = NULL;

  sfnt_locate_table(sfont, "post");

  table = NEW(1, struct tt_post_table);
  table->Format = sfnt_get_ulong(sfont);      /* Fixed */
  table->italicAngle = sfnt_get_ulong(sfont); /* Fixed */
  table->underlinePosition = sfnt_get_short(sfont); /* FWord */
  table->underlineThickness = sfnt_get_short(sfont); /* FWord */
  table->isFixedPitch = sfnt_get_ulong(sfont);
  table->minMemType42 = sfnt_get_ulong(sfont);
  table->maxMemType42 = sfnt_get_ulong(sfont);
  table->minMemType1 = sfnt_get_ulong(sfont);
  table->maxMemType1 = sfnt_get_ulong(sfont);
  /* remaining table data ignored */

  return table;
}

struct tt_VORG_table *
tt_read_VORG_table (sfnt *sfont)
{
  struct tt_VORG_table *vorg;

  if (sfnt_find_table_pos(sfont, "VORG") > 0) {
    USHORT i;
    vorg = NEW(1, struct tt_VORG_table);
    sfnt_locate_table(sfont, "VORG");
    if (sfnt_get_ushort(sfont) != 1 || sfnt_get_ushort(sfont) != 0)
      ERROR("Unsupported VORG version.");
    vorg->defaultVertOriginY = sfnt_get_short(sfont);
    vorg->numVertOriginYMetrics = sfnt_get_ushort(sfont);
    vorg->vertOriginYMetrics = NEW(vorg->numVertOriginYMetrics,
				   struct tt_vertOriginYMetrics);
    /*
     * The vertOriginYMetrics array must be sorted in increasing
     * glyphIndex order.
     */
    for (i = 0; i < vorg->numVertOriginYMetrics; i++) {
      vorg->vertOriginYMetrics[i].glyphIndex  = sfnt_get_ushort(sfont);
      vorg->vertOriginYMetrics[i].vertOriginY = sfnt_get_short(sfont);
    }
  } else
    vorg = NULL;

  return vorg;
}

/*
 * hmtx and vmtx
 *
 *  Reading/writing hmtx and vmtx depend on other tables, maxp and hhea/vhea.
 */

struct tt_longMetrics *
tt_read_longMetrics (sfnt *sfont, USHORT numGlyphs, USHORT numLongMetrics)
{
  struct tt_longMetrics *m;
  USHORT gid, last_adv = 0;

  m = NEW(numGlyphs, struct tt_longMetrics);
  for (gid = 0; gid < numGlyphs; gid++) {
    if (gid < numLongMetrics)
      last_adv = sfnt_get_ushort(sfont);
    m[gid].sideBearing = sfnt_get_short(sfont);
    m[gid].advance     = last_adv;
  }

  return m;
}

#include "macglyphs.h"

/* offset from begenning of the post table */
#define NAME_STR_OFFSET 32

struct tt_glyph_names *tt_get_glyph_names (sfnt *sfont)
{
  struct tt_glyph_names *names;
  ULONG offset;

  offset = sfnt_locate_table(sfont, "post");

  names = NEW(1, struct tt_glyph_names);
  names->version = sfnt_get_ulong(sfont);
  if (names->version == 0x00010000UL) {
    names->count = 258;
    names->glyphs = (char **) macglyphorder;
    names->names = NULL;
  } else if (names->version == 0x00020000UL) {
    USHORT idx, *indices, i;
    sfnt_seek_set(sfont, offset + NAME_STR_OFFSET);
    names->count = sfnt_get_ushort(sfont); /* number of glyphs */
    indices = NEW(names->count, USHORT); /* temporary */
    names->count2 = 0; /* number of non-standard glyph names */
    for (i=0;i<(names->count);i++) {
      if ((indices[i] = sfnt_get_ushort(sfont)) > 257) {
	names->count2 += 1;
      }
    }
    if (names->count2 > 0) {
      names->names = NEW(names->count2, char *);
      for (i=0;i<(names->count2);i++) { /* read Pascal strings */
	unsigned char len;
	len = sfnt_get_byte(sfont);
	if (len) {
	  (names->names)[i] = NEW(len + 1, char);
	  sfnt_read((names->names)[i], len, sfont);
	  (names->names)[i][len] = 0;
	} else {
	  (names->names)[i] = NULL;
	}
      }
    } else {
      names->names = NULL;
    }
    names->glyphs = NEW(names->count, char *);
    for (i=0;i<(names->count);i++) {
      idx = indices[i];
      if (idx < 258) {
	(names->glyphs)[i] = (char *) macglyphorder[idx];
      } else if (idx - 258 < names->count2) {
	(names->glyphs)[i] = (names->names)[idx-258];
      } else {
	ERROR("Invalid glyph name index number");
      }
    }
    RELEASE(indices);
  } else {
    RELEASE(names);
    return NULL;
  }

  return names;
}

USHORT tt_glyph_lookup (struct tt_glyph_names *names, char *glyph)
{
  USHORT i;

  if (names) {
    for (i=0;i<(names->count);i++) {
      if (glyph && (names->glyphs)[i] &&
	  (strcmp(glyph, (names->glyphs)[i]) == 0)) {
	return i;
      }
    }
  }

  return 0;
}

void tt_release_glyph_names (struct tt_glyph_names *names)
{
  if (names) {
    if (names->glyphs)
      RELEASE(names->glyphs);
    if (names->names) {
      USHORT i;
      for (i=0;i<(names->count2);i++) {
	if ((names->names)[i])
	  RELEASE((names->names)[i]);
      }
      RELEASE(names->names);
    }
    RELEASE(names);
  }
}

/* OS/2 table */
/* this table may not exist */
struct tt_os2__table *tt_read_os2__table (sfnt *sfont)
{
  struct tt_os2__table *table = NULL;
  int i;

  sfnt_locate_table(sfont, "OS/2");

  table = NEW(1, struct tt_os2__table);
  table->version = sfnt_get_ushort(sfont);
  table->xAvgCharWidth = sfnt_get_short(sfont);
  table->usWeightClass = sfnt_get_ushort(sfont);
  table->usWidthClass = sfnt_get_ushort(sfont);
  table->fsType = sfnt_get_short(sfont);
  table->ySubscriptXSize = sfnt_get_short(sfont);
  table->ySubscriptYSize = sfnt_get_short(sfont);
  table->ySubscriptXOffset = sfnt_get_short(sfont);
  table->ySubscriptYOffset = sfnt_get_short(sfont);
  table->ySuperscriptXSize = sfnt_get_short(sfont);
  table->ySuperscriptYSize = sfnt_get_short(sfont);
  table->ySuperscriptXOffset = sfnt_get_short(sfont);
  table->ySuperscriptYOffset = sfnt_get_short(sfont);
  table->yStrikeoutSize = sfnt_get_short(sfont);
  table->yStrikeoutPosition = sfnt_get_short(sfont);
  table->sFamilyClass = sfnt_get_short(sfont);
  for (i=0;i<10;i++) {
    (table->panose)[i] = sfnt_get_byte(sfont);
  }
  table->ulUnicodeRange1 = sfnt_get_ulong(sfont);
  table->ulUnicodeRange2 = sfnt_get_ulong(sfont);
  table->ulUnicodeRange3 = sfnt_get_ulong(sfont);
  table->ulUnicodeRange4 = sfnt_get_ulong(sfont);
  for (i=0;i<4;i++) {
    (table->achVendID)[i] = sfnt_get_char(sfont);
  }
  table->fsSelection = sfnt_get_ushort(sfont);
  table->usFirstCharIndex = sfnt_get_ushort(sfont);
  table->usLastCharIndex = sfnt_get_ushort(sfont);
  table->sTypoAscender = sfnt_get_short(sfont);
  table->sTypoDescender = sfnt_get_short(sfont);
  table->sTypoLineGap = sfnt_get_short(sfont);
  table->usWinAscent = sfnt_get_ushort(sfont);
  table->usWinDescent = sfnt_get_ushort(sfont);
  table->ulCodePageRange1 = sfnt_get_ulong(sfont);
  table->ulCodePageRange2 = sfnt_get_ulong(sfont);
  if (table->version == 0x0002) {
    table->sxHeight = sfnt_get_short(sfont);
    table->sCapHeight = sfnt_get_short(sfont);
    table->usDefaultChar = sfnt_get_ushort(sfont);
    table->usBreakChar = sfnt_get_ushort(sfont);
    table->usMaxContext = sfnt_get_ushort(sfont);
  }

  return table;
}

USHORT tt_get_name (sfnt *sfont, char *dest, USHORT destlen,
		    USHORT plat_id, USHORT enco_id,
		    USHORT lang_id, USHORT name_id)
{
  USHORT length = 0;
  USHORT num_names, string_offset;
  ULONG name_offset;
  int i;

  name_offset = sfnt_locate_table (sfont, "name");

  if (sfnt_get_ushort(sfont)) 
    ERROR ("Expecting zero");

  num_names = sfnt_get_ushort(sfont);
  string_offset = sfnt_get_ushort(sfont);
  for (i=0;i<num_names;i++) {
    USHORT p_id, e_id, n_id, l_id;
    USHORT offset;

    p_id = sfnt_get_ushort(sfont);
    e_id = sfnt_get_ushort(sfont);
    l_id = sfnt_get_ushort(sfont);
    n_id = sfnt_get_ushort(sfont);
    length = sfnt_get_ushort(sfont);
    offset = sfnt_get_ushort(sfont);
    /* language ID value 0xffffu for `accept any language ID' */
    if ((p_id == plat_id) && (e_id == enco_id) &&
	(lang_id == 0xffffu || l_id == lang_id) && (n_id == name_id)) {
      if (length > destlen - 1) {
	fprintf(stderr, "\n** Notice: Name string too long. Truncating **\n");
	length = destlen - 1;
      }
      sfnt_seek_set (sfont, name_offset+string_offset+offset);
      sfnt_read(dest, length, sfont);
      dest[length] = '\0';
      break;
    }
  }
  if (i == num_names) {
    length = 0;
  }

  return length;
}

USHORT tt_get_ps_fontname (sfnt *sfont, char *dest, USHORT destlen)
{
  USHORT namelen = 0;

  /* First try Mac-Roman PS name and then Win-Unicode PS name */
  if ((namelen = tt_get_name(sfont, dest, destlen, 1, 0, 0, 6)) != 0 ||
      (namelen = tt_get_name(sfont, dest, destlen, 3, 1, 0x409u, 6)) != 0 ||
      (namelen = tt_get_name(sfont, dest, destlen, 3, 5, 0x412u, 6)) != 0)
    return namelen;

  fprintf(stderr, "\n** Warning: No valid PostScript name available **\n");
  /*
    Wrokaround for some bad TTfonts:
    Language ID value 0xffffu for `accept any language ID'
  */
  if ((namelen = tt_get_name(sfont, dest, destlen, 1, 0, 0xffffu, 6)) == 0) {
    /*
      Finally falling back to Mac Roman name field.
      Warning: Some bad Japanese TTfonts using SJIS encoded string in the
      Mac Roman name field. 
    */
    namelen = tt_get_name(sfont, dest, destlen, 1, 0, 0, 1);
  }

  return namelen;
}
