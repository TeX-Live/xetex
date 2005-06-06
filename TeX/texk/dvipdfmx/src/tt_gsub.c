/*  $Header: /home/cvsroot/dvipdfmx/src/tt_gsub.c,v 1.5 2004/01/30 18:34:24 hirata Exp $
    
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
  TrueType GSUB support: (incomplete)
*/

#include "system.h"
#include "error.h"
#include "mem.h"
#include "mfileio.h"

#include "sfnt.h"
#include "tt_gsub.h"

typedef USHORT Offset;
typedef USHORT GlyphID;

/* OpenType Common Layout Table */
/* Records */
struct clt_rec
{
  char tag[5]; /* 4-byte identifier */
  Offset offset;
};

/* Ranges */
/* RangeRecord */
struct clt_range
{ 
  GlyphID Start; /* First GlyphID in the range */
  GlyphID End; /* Last GlyphID in the range */
  USHORT StartCoverageIndex; /* Converage Index of first GID */
};

static void clt_read_rec (sfnt *sfont, struct clt_rec *rec)
{
  int i;

  for (i=0;i<4;i++) {
    (rec->tag)[i] = sfnt_get_char(sfont);
  }
  (rec->tag)[4] = '\0';
  rec->offset = sfnt_get_ushort(sfont);
}

static void clt_read_range (sfnt *sfont, struct clt_range *rec)
{
  rec->Start = sfnt_get_ushort(sfont);
  rec->End = sfnt_get_ushort(sfont);
  rec->StartCoverageIndex = sfnt_get_ushort(sfont);
}

/*
  List structure:
   ScriptRecord (records), FeatureRecord (records), Lookup (offsets)
*/

struct clt_rec_list
{
  USHORT count;
  struct clt_rec *record;
};

/* offset and index list, Offset is USHORT */
struct clt_num_list
{
  USHORT count;
  USHORT *value;
};

/* get/release */
static struct clt_rec_list *get_rec_list (sfnt *sfont)
{
  struct clt_rec_list *list;
  USHORT i, count;
  
  count = sfnt_get_ushort(sfont);

  list = NEW(1, struct clt_rec_list);
  list->count = count;
  list->record = NEW(count, struct clt_rec);

  for (i=0;i<count;i++) {
    clt_read_rec(sfont, (list->record)+i);
  }

  return list;
}

static void clt_release_rec_list (struct clt_rec_list *list)
{
  if (list) {
    if (list->record) RELEASE(list->record);
    RELEASE(list);
  }
}

static struct clt_num_list *get_num_list (sfnt *sfont)
{
  struct clt_num_list *list;
  USHORT i, count;

  count = sfnt_get_ushort(sfont);

  list = NEW(1, struct clt_num_list);
  list->count = count;
  list->value = NEW(count, USHORT);

  for (i=0;i<count;i++) {
    (list->value)[i] = sfnt_get_ushort(sfont);
  }

  return list;
}

static void clt_release_num_list (struct clt_num_list *list)
{
  if (list) {
    if (list->value) RELEASE(list->value);
    RELEASE(list);
  }
}

/*
  Tables
*/

/* Feature Table */
struct clt_feature_table
{
  Offset FeatureParams;
  struct clt_num_list *LookupListIndex; /* LookupListIndex List */
};

/*
  Lookup Table
   Currently, only single substitution is supported. LookupFlag is ignored.
*/
struct clt_lookup_table
{
  USHORT LookupType; /* Different enumerations for GSUB and GPOS */
  USHORT LookupFlag; /* Lookup qualifiers */
  struct clt_num_list *SubTableList; /* offset */
  /* offset is from beginning of Lookup table */
};

/* Coverage Table: format 1 and format 2 */
struct clt_coverage
{
  USHORT format; /* Format identifier: 1 (list), 2 (range) */
  USHORT count; /* Glyphs/Range Count */
  union {
    GlyphID *list; /* Array of GlyphIDs - in numerical order */
    struct clt_range *range; /* Array of glyph ranges
				   - ordered by Start GlyphID */
  } data;
};

/* GSUB - The Glyph Substitution Table */
struct gsub_header
{
  Fixed version; /* 0x00010000 */
  Offset ScriptList; /* offset */
  Offset FeatureList; /* offset */
  Offset LookupList; /* offset */
};

/* Single Substitution Format 1 */
struct gsub_single1
{
  SHORT DeltaGlyphID; /* Add to original GlyphID to get substitute GlyphID */
  struct clt_coverage *coverage; /* Coverage table */
};

/* Single Substitution Format 2 */
struct gsub_single2
{
  USHORT GlyphCount; /* Number of GlyphIDs in the Substitute array */
  GlyphID *Substitute; /* Array of substitute GlyphIDs
			 - ordered by Coverage Index */
  struct clt_coverage *coverage; /* Coverage table */
};

/* GSUB subtable (single) */
struct gsub_subtable
{
  USHORT SubstFormat;
  union {
    struct gsub_single1 *single1;
    struct gsub_single2 *single2;
  } table;
};

/* Lookup Table */
static void clt_read_lookup_table (sfnt *sfont, struct clt_lookup_table *t)
{
  if (t == NULL) 
    ERROR("Passed NULL pointer");

  t->LookupType = sfnt_get_ushort(sfont);
  t->LookupFlag = sfnt_get_ushort(sfont);
  t->SubTableList = get_num_list(sfont);
}

static void clt_release_lookup_table (struct clt_lookup_table *t)
{
  if (t) {
    if (t->SubTableList)
      clt_release_num_list(t->SubTableList);
    RELEASE(t);
  }
}

static struct clt_coverage *clt_get_coverage (sfnt *sfont)
{
  struct clt_coverage *t;
  USHORT i;

  t = NEW(1, struct clt_coverage);

  t->format = sfnt_get_ushort(sfont);
  t->count = sfnt_get_ushort(sfont);
  switch(t->format) {
  case 1: /* list */
    {
      USHORT *gid;

      (t->data).list = gid = NEW(t->count, USHORT);
      for (i=0;i<(t->count);i++) {
	gid[i] = sfnt_get_ushort(sfont);
      }
    }
    break;
  case 2: /* range */
    {
      struct clt_range *RangeRecord;

      (t->data).range = RangeRecord = NEW(t->count, struct clt_range);
      for (i=0;i<(t->count);i++) {
	clt_read_range(sfont, RangeRecord+i);
      }
    }
    break;
  default:
    ERROR("Unknown coverage format");
  }

  return t;
}

static void clt_release_coverage (struct clt_coverage *t)
{
  if (t) {
    switch(t->format) {
    case 1: /* list */
      if ((t->data).list)
	RELEASE((t->data).list);
      break;
    case 2: /* range */
      if ((t->data).range)
	RELEASE((t->data).range);
      break;
    default:
      ERROR("Unknown coverage format");
    }
    RELEASE(t);
  }
}

/* returns cov->count if not found */
static USHORT clt_lookup_coverage (struct clt_coverage *cov, USHORT gid)
{
  USHORT i;

  switch(cov->format) {
  case 1: /* list */
    for (i=0;i<(cov->count);i++) {
      if ((cov->data).list[i] > gid) {
	break;
      } else if ((cov->data).list[i] == gid) {
	return i; /* found */
      }
    }
  case 2: /* range */
    {
      struct clt_range *ranges = (cov->data).range;
      for (i=0;i<(cov->count);i++) {
	if (gid > ranges[i].End) {
	  break;
	} else if (gid >= ranges[i].Start) { /* found */
	  return ranges[i].StartCoverageIndex + gid - ranges[i].Start;
	}
      }
    }
    break;
  default:
    ERROR("Unknown coverage format");
  }

  return cov->count; /* not found */
}

/* LookupType for GSUB */
#define GSUB_TYPE_SINGLE 1
#define GSUB_TYPE_MULTIPLE 2
#define GSUB_TYPE_ALTERNATE 3
#define GSUB_TYPE_LIGATURE 4
#define GSUB_TYPE_CONTEXT 5
#define GSUB_TYPE_CCONTEXT 6
#define GSUB_TYPE_ESUBST 7

static void gsub_read_single (sfnt *sfont, struct gsub_subtable *st)
{
  ULONG offset; /* not Offset which USHORT */
  Offset cov_offset; /* subtable offset, offset to Coverage table */

  offset = tell_position(sfont->stream);

  st->SubstFormat = sfnt_get_ushort(sfont);

  if (st->SubstFormat == 1) {
    struct gsub_single1 *data;

    (st->table).single1 = data = NEW(1, struct gsub_single1);
    cov_offset = sfnt_get_ushort(sfont);
    data->DeltaGlyphID = sfnt_get_short(sfont);
    sfnt_seek_set(sfont, offset+cov_offset);
    data->coverage = clt_get_coverage(sfont);

  } else if (st->SubstFormat == 2) {
    struct gsub_single2 *data;
    USHORT count;

    (st->table).single2 = data = NEW(1, struct gsub_single2);
    cov_offset = sfnt_get_ushort(sfont);
    data->GlyphCount = sfnt_get_ushort(sfont);
    data->Substitute = NEW(data->GlyphCount, GlyphID);
    for (count=0;count<data->GlyphCount;count++) {
      (data->Substitute)[count] = sfnt_get_ushort(sfont);
    }
    sfnt_seek_set(sfont, offset+cov_offset);
    data->coverage = clt_get_coverage(sfont);

  } else {
    ERROR("unexpected SubstFormat");
  }
  /* not implemented yet */
}

static void gsub_release_single (struct gsub_subtable *st)
{
  if (st) {
    switch(st->SubstFormat) {
    case 1:
      {
	struct gsub_single1 *data = (st->table).single1;
	if (data) {
	  if (data->coverage)
	    clt_release_coverage(data->coverage);
	  RELEASE(data);
	}
      }
    break;
    case 2:
      {
	struct gsub_single2 *data = (st->table).single2;
	if (data) {
	  if (data->Substitute)
	    RELEASE(data->Substitute);
	  if (data->coverage)
	    clt_release_coverage(data->coverage);
	}
      }
    break;
    default:
      ERROR("Unknown format for single substitution");
    }
    /* RELEASE(st); */
  }
}

static struct gsub_header *get_gsub_header (sfnt *sfont)
{
  struct gsub_header *h;

  h = NEW(1, struct gsub_header);
  h->version = sfnt_get_ulong(sfont);
  h->ScriptList = sfnt_get_ushort(sfont);
  h->FeatureList = sfnt_get_ushort(sfont);
  h->LookupList = sfnt_get_ushort(sfont);

  return h;
}

static void gsub_release_header (struct gsub_header *h)
{
  if (h) RELEASE(h);
}

/*
  script -- langsys --> feature indices
         |
          - langsys --> feature indices

  feature --> lookup indices
*/

struct tt_gsub_s
{
  USHORT num_subtable;
  struct gsub_subtable *subtable; /* only single subst. */
};

tt_gsub_t tt_gsub_require_feature (sfnt *sfont, const char *tag)
{
  struct tt_gsub_s *gsub;
  USHORT idx;
  ULONG gsub_offset, offset;
  struct gsub_header *h;
  struct clt_rec_list *feature_list;
  struct clt_num_list *lookup_list, *required_lookups;
  struct gsub_subtable *subtable = NULL;
  USHORT num_subtables = 0;

  if (sfont == NULL || sfont->stream == NULL)
    ERROR("file not opened");

  if (tag == NULL)
    return NULL;

  if ((gsub_offset = sfnt_find_table_pos(sfont, "GSUB")) == 0)
    return NULL; /* not found */

  /* GSUB header */
  sfnt_seek_set(sfont, gsub_offset);
  h = get_gsub_header(sfont);

  /* Script and LangSys table currently ignored */

  /* Feature List */
  sfnt_seek_set(sfont, gsub_offset+h->FeatureList);
  feature_list = get_rec_list(sfont);

  /* Lookup List */
  sfnt_seek_set(sfont, gsub_offset+h->LookupList);
  lookup_list = get_num_list(sfont);

  for (idx=0;idx<feature_list->count;idx++) {
    if (strncmp((feature_list->record)[idx].tag, tag, 4) == 0) {
      USHORT i, ii, iii;

      /* Feature Table */
      offset = gsub_offset + h->FeatureList +
	(feature_list->record)[idx].offset;

      sfnt_seek_set(sfont, offset);
      if (sfnt_get_ushort(sfont) != 0) {
	ERROR("unrecognized FeatureParams");
      }
      required_lookups = get_num_list(sfont); /* LookupListIndices */

      /* Lookup table */
      for (i=0;i<(required_lookups->count);i++) {
	struct clt_lookup_table *lookup;
	struct clt_num_list *list;

	ii = (required_lookups->value)[i];

	if (ii >= lookup_list->count)
	  ERROR("invalid Lookup index\n");

	lookup = NEW(1, struct clt_lookup_table);

	offset = gsub_offset + h->LookupList + (lookup_list->value)[ii];
	sfnt_seek_set(sfont, offset);
	clt_read_lookup_table(sfont, lookup);

	list = lookup->SubTableList; /* subtable list */

	switch (lookup->LookupType) {
	case GSUB_TYPE_SINGLE:
	  subtable = RENEW(subtable, num_subtables + list->count,
			   struct gsub_subtable);
	  for (iii=0;iii<(list->count);iii++) {
	    offset = gsub_offset + h->LookupList
	      + (lookup_list->value)[ii] + (list->value)[iii];
	    sfnt_seek_set(sfont, offset);
	    gsub_read_single(sfont, subtable + num_subtables + iii);
	  }
	  num_subtables += list->count;
	  break;
	case GSUB_TYPE_MULTIPLE: /* not supported */
	case GSUB_TYPE_ALTERNATE:
	case GSUB_TYPE_LIGATURE:
	case GSUB_TYPE_CONTEXT:
	case GSUB_TYPE_CCONTEXT:
	case GSUB_TYPE_ESUBST:
	  break;
	default:
	  break;
	} /* switch */
	clt_release_lookup_table(lookup);
      } /* for required_lookups */
      clt_release_num_list(required_lookups);
    }  /* if tab match */
  } /* for feature_list */

  clt_release_num_list(lookup_list);
  clt_release_rec_list(feature_list);
  gsub_release_header(h); /* release header */

  if (subtable != NULL) {
    gsub = NEW(1, struct tt_gsub_s);
    gsub->num_subtable = num_subtables;
    gsub->subtable = subtable;
  } else {
    return NULL;
  }

  return (tt_gsub_t) gsub;
}

void tt_gsub_release (tt_gsub_t gsubs)
{
  struct tt_gsub_s *gsub;
  USHORT i;

  if (gsubs == NULL)
    return;

  gsub = (struct tt_gsub_s *) gsubs;
  for (i=0;i<gsub->num_subtable;i++) {
    gsub_release_single(gsub->subtable+i);
  }
  RELEASE(gsub);

  gsub = NULL;
}

/* Lookup order ? */
void tt_gsub_substitute (tt_gsub_t gsubs, USHORT *gid)
{
  struct tt_gsub_s *gsub;
  struct gsub_subtable *st;
  USHORT i, idx;

  if (gsubs == NULL)
    return;

  gsub = (struct tt_gsub_s *) gsubs;

  for (i=0;i<gsub->num_subtable;i++) {
    st = gsub->subtable + i;
    if (st->SubstFormat == 1) {
      struct gsub_single1 *data;

      data = (st->table).single1;
      idx = clt_lookup_coverage(data->coverage, *gid);
      if (idx < data->coverage->count) {
	*gid += data->DeltaGlyphID;
	break; /* found */
      }
    } else if (st->SubstFormat == 2) {
      struct gsub_single2 *data;

      data = (st->table).single2;
      idx = clt_lookup_coverage(data->coverage, *gid);
      if (idx < data->coverage->count) {
	*gid = (data->Substitute)[idx];
	break; /* found */
      }
    } else {
      ERROR("Unknown format for single substitution");
    }
  }
}
