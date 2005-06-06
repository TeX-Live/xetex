/*  $Header: /home/cvsroot/dvipdfmx/src/cmap.c,v 1.19 2004/02/12 12:20:20 hirata Exp $

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
 * References:
 *
 *  PostScript Language Reference Manual, 3rd. ed. (Adobe Systems Inc.)
 *    5.11.4 CMap Dictionaries
 *    5.11.5 FMapType 9 Composite Fonts
 *  Building CMap Files for CID-Keyed Fonts, Adobe Technical Note #5099
 *  CID-Keyed Font Technology Overview, Adobe Technical Note #5092
 *  Adobe CMap and CIDFont Files Specification, Adobe Technical Specification #5014
 *
 *  Undefined Character Handling:
 *    PLRM 3rd. ed., sec. 5.11.5., "Handling Undefined Characters"
 *
 * TODO:
 *   Only cid(range|char) allowed for CODE_TO_CID and bf(range|char) for CID_TO_CODE ?
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "system.h"
#include "mem.h"
#include "error.h"
#include "dpxutil.h"
#include "cmap.h"

/* Mapping types, MAP_IS_NAME is not supported. */
#define MAP_IS_CID      (1 << 0)
#define MAP_IS_NAME     (1 << 1)
#define MAP_IS_CODE     (1 << 2)
#define MAP_IS_NOTDEF   (1 << 3)

#define MAP_IS_UNDEF    0
#define MAP_TYPE_MASK   0x00f

#define MAP_DEFINED(e)  (((e) & MAP_TYPE_MASK) != MAP_IS_UNDEF ? 1 : 0)
#define MAP_TYPE(e)     ((e) & MAP_TYPE_MASK)

/* Lookup flags */
#define MAP_LOOKUP_END      0
#define MAP_LOOKUP_CONTINUE (1 << 4)
#define LOOKUP_CONTINUE(f) ((f) & MAP_LOOKUP_CONTINUE)
#define LOOKUP_END(f)      (!LOOKUP_CONTINUE((f)))

/* DEBUG */
#define CMAP_DEBUG_STR "CMap"
#define CMAP_DEBUG     3

static int __verbose = 0;

void
CMap_set_verbose (void)
{
  __verbose++;
}

/* Codespacerange */
typedef struct rangeDef {
  int dim;               /* Dimension of this codespacerange */
  unsigned char *codeLo; /* Lower bounds of valid input code */
  unsigned char *codeHi; /* Upper bounds of valid input code */
} rangeDef;

typedef struct mapDef {
  int            flag;
  int            len;  /* 2 for CID, variable for Code..  */
  unsigned char *code; /* CID (as 16-bit BE), Code ...    */
  struct mapDef *next; /* Next Subtbl for LOOKUP_CONTINUE */
} mapDef;

#define MEM_ALLOC_SIZE  4096
typedef struct mapData {
  long            pos;  /* Position of next free data segment */
  unsigned char  *data; /* CID, Code... MEM_ALLOC_SIZE bytes  */
  struct mapData *prev; /* Previous mapData data segment      */
} mapData;

struct CMap {
  char  *name;
  int    type;     /* CMapType: 1 for usual CMaps,
		    *           2 for ToUnicode CMaps,
		    *           0 for IDENTITY is also defined for convenience.
		    */
  int    wmode;    /* WMode: 0 for Horizontal, 1 for Vertical. */
  double version;  /* Unused */
                   /* XUID, UIDOffet unused. */
  CIDSysInfo *CSI; /* CIDSystemInfo */

  struct CMap *useCMap;

  struct {
    int        num;
    int        max;
    rangeDef  *ranges;
  } codespace;

  mapDef  *mapTbl;  /* First 256 segment of mapping table */
  mapData *mapData; /* Storage for actual CMap data       */

  /* Additional data used by cmap.c, etc. */

  int flags; /* Decoder flags Not used yet. */

  struct {
    int minBytesIn;
    int maxBytesIn;
    int minBytesOut;
    int maxBytesOut;
  } profile;

};

/* Private funcs. */
static int  bytes_consumed   (CMap *cmap, const unsigned char *instr, long inbytes);
static void CMap_decode_char (CMap *cmap,
			      const unsigned char **inbuf, long *inbytesleft,
			      unsigned char **outbuf, long *outbytesleft);
static void handle_undefined (CMap *cmap,
			      const unsigned char **inbuf, long *inbytesleft,
			      unsigned char **outbuf, long *outbytesleft);

static int  check_range      (CMap *cmap,
			      const unsigned char *srclo, const unsigned char *srchi, int srcdim,
			      const unsigned char *dst, int dstdim);

static unsigned char *get_mem (CMap *cmap, int size);
static mapDef *mapDef_new     (void);
static void    mapDef_release (mapDef *t);
static int     locate_tbl     (mapDef **cur, const unsigned char *code, int dim);

CMap *
CMap_new (void)
{
  CMap *cmap;

  cmap = NEW(1, struct CMap);
  cmap->name     = NULL;
  cmap->type     = CMAP_TYPE_CODE_TO_CID;
  cmap->wmode    = 0;
  cmap->useCMap  = NULL;
  cmap->CSI      = NULL;

  cmap->profile.minBytesIn  = 2;
  cmap->profile.maxBytesIn  = 2;
  cmap->profile.minBytesOut = 2;
  cmap->profile.maxBytesOut = 2;

  cmap->flags = 0;

  cmap->codespace.num    = 0;
  cmap->codespace.max    = 10;
  cmap->codespace.ranges = NEW(10, struct rangeDef);

  cmap->mapTbl  = NULL;

  cmap->mapData = NEW(1, struct mapData);
  cmap->mapData->prev = NULL;
  cmap->mapData->pos  = 0;
  cmap->mapData->data = NEW(MEM_ALLOC_SIZE, unsigned char);

  return cmap;
}

void
CMap_release (CMap *cmap)
{
  if (!cmap)
    return;

  if (cmap->name)
    RELEASE(cmap->name);
  if (cmap->CSI) {
    if (cmap->CSI->registry) RELEASE(cmap->CSI->registry);
    if (cmap->CSI->ordering) RELEASE(cmap->CSI->ordering);
    RELEASE(cmap->CSI);
  }
  if (cmap->codespace.ranges)
    RELEASE(cmap->codespace.ranges);
  if (cmap->mapTbl)
    mapDef_release(cmap->mapTbl);
  {
    mapData *map = cmap->mapData;
    while (map != NULL) {
      mapData *prev = map->prev;
      if (map->data != NULL)
	RELEASE(map->data);
      RELEASE(map);
      map = prev;
    }
  }

  RELEASE(cmap);
}

int
CMap_is_Identity (CMap *cmap)
{
  ASSERT(cmap);
  if (!strcmp(cmap->name, "Identity-H") || !strcmp(cmap->name, "Identity-V"))
    return 1;
  else
    return 0;
}

#if 0
/* NOT IMPLEMENTED YET */
int
CMap_is_realy_Identity(CMap *cmap)
{
  return 0;
}
#endif

int
CMap_is_valid (CMap *cmap)
{
  /* Quick check */
  if (!cmap || !(cmap->name) || cmap->type < CMAP_TYPE_IDENTITY ||
      cmap->type > CMAP_TYPE_CID_TO_CODE || cmap->codespace.num < 1 ||
      (cmap->type != CMAP_TYPE_IDENTITY && !cmap->mapTbl))
    return 0;

  if (cmap->useCMap) {
    CIDSysInfo *csi1, *csi2;
    csi1 = CMap_get_CIDSysInfo(cmap);
    csi2 = CMap_get_CIDSysInfo(cmap->useCMap);
    if (strcmp(csi1->registry, csi2->registry) && strcmp(csi1->ordering, csi2->ordering))
      WARN("Mismatched CIDSystemInfo in included CMap resource %s <-- %s",
	   CMap_get_name(cmap), CMap_get_name(cmap->useCMap));
    return 0;
  }

  return 1;
}

int
CMap_get_profile (CMap *cmap, int type)
{
  int value = 0;

  ASSERT(cmap);
  switch (type) {
  case CMAP_PROF_TYPE_INBYTES_MIN:
    value = cmap->profile.minBytesIn;
    break;
  case CMAP_PROF_TYPE_INBYTES_MAX:
    value = cmap->profile.maxBytesIn;
    break;
  case CMAP_PROF_TYPE_OUTBYTES_MIN:
    value = cmap->profile.maxBytesOut;
    break;
  case CMAP_PROF_TYPE_OUTBYTES_MAX:
    value = cmap->profile.maxBytesOut;
    break;
  default:
    ERROR("%s: Unrecognized profile type %d.", CMAP_DEBUG_STR, type);
  }

  return value;
}

/*
 * Put notdef chars for codes not declared in notdef(range|char)
 */
static void
handle_undefined (CMap *cmap,
		  const unsigned char **inbuf,  long *inbytesleft,
		  unsigned char **outbuf, long *outbytesleft)
{
  long len = 0;

  if (*outbytesleft < 2)
    ERROR("%s: Buffer overflow.", CMAP_DEBUG_STR);

  switch (cmap->type) {
  case CMAP_TYPE_CODE_TO_CID:
    memcpy(*outbuf, CID_NOTDEF_CHAR, 2);
    break;
  case CMAP_TYPE_TO_UNICODE:
    memcpy(*outbuf, UCS_NOTDEF_CHAR, 2);
    break;
  default:
    WARN("Cannot handle undefined mapping for this type of CMap mapping: %d", cmap->type);
    WARN("<0000> is used for .notdef char.");
    memset(*outbuf, 0, 2);
  }
  *outbuf += 2;
  *outbytesleft -= 2;

  len = bytes_consumed(cmap, *inbuf, *inbytesleft);

  *inbuf  += len;
  *inbytesleft  -= len;
}

static void
CMap_decode_char (CMap *cmap,
		  const unsigned char **inbuf, long *inbytesleft,
		  unsigned char **outbuf, long *outbytesleft)
{
  mapDef *t;
  unsigned char *p, c = 0;
  long    count = 0;

  p = (unsigned char *) *inbuf;
  /*
   * First handle some special cases:
   */
  if (cmap->type == CMAP_TYPE_IDENTITY) {
    if ((*inbytesleft) % 2)
      ERROR("%s: Invalid/truncated input string.", CMAP_DEBUG_STR);
    if (*outbytesleft < 2)
      ERROR("%s: Buffer overflow.", CMAP_DEBUG_STR);
    memcpy(*outbuf, *inbuf, 2);
    *inbuf  += 2;
    *outbuf += 2;
    *outbytesleft -= 2;
    *inbytesleft  -= 2;
    return;
  } else if (!cmap->mapTbl) {
    if (cmap->useCMap) {
      CMap_decode_char(cmap->useCMap, inbuf, inbytesleft, outbuf, outbytesleft);
      return;
    } else {
      /* no mapping available in this CMap */
      WARN("No mapping available for this character.");
      handle_undefined(cmap, inbuf, inbytesleft, outbuf, outbytesleft);
      return;
    }
  }

  ASSERT(cmap->mapTbl);
  t = cmap->mapTbl;
  while (count < *inbytesleft) {
    c = *p++;
    count++;
    if (LOOKUP_END(t[c].flag))
      break;
    t = t[c].next;
  }
  if (LOOKUP_CONTINUE(t[c].flag)) /* need more bytes */
    ERROR("%s: Premature end of input string.", CMAP_DEBUG_STR);
  else if (!MAP_DEFINED(t[c].flag)) {
    if (cmap->useCMap) {
      CMap_decode_char(cmap->useCMap, inbuf, inbytesleft, outbuf, outbytesleft);
      return;
    } else {
      /* no mapping available in this CMap */
      WARN("No mapping available for this character.");
      /*
       * We know partial match found up to `count' bytes,
       * but we will not use this information for the sake of simplicity.
       */ 
      handle_undefined(cmap, inbuf, inbytesleft, outbuf, outbytesleft);
      return;
    }
  } else {
    switch (MAP_TYPE(t[c].flag)) {
    case MAP_IS_NOTDEF:
      WARN("Character mapped to .notdef found.");
      /* continue */
    case MAP_IS_CID: case MAP_IS_CODE:
      if (*outbytesleft >= t[c].len)
	memcpy(*outbuf, t[c].code, t[c].len);
      else
	ERROR("%s: Buffer overflow.", CMAP_DEBUG_STR);
      *outbuf       += t[c].len;
      *outbytesleft -= t[c].len;
      break;
    case MAP_IS_NAME:
      ERROR("%s: CharName mapping not supported.", CMAP_DEBUG_STR);
      break;
    default:
      ERROR("%s: Unknown mapping type.", CMAP_DEBUG_STR);
    }
    if (inbytesleft)
      *inbytesleft -= count;
    *inbuf = p;
  }
}

/*
 * For convenience.
 */
unsigned short
CMap_lookup_char2 (CMap *cmap, unsigned short code)
{
  unsigned char  inbuf[2], outbuf[32];
  long    inbytesleft = 2, outbytesleft = 32;
  unsigned char *p = inbuf, *q = outbuf;

  ASSERT(cmap);
  inbuf[0] = (code >> 8) & 0xff;
  inbuf[1] = code & 0xff;
  CMap_decode_char(cmap, (const unsigned char **) &p, &inbytesleft, &q, &outbytesleft);
  if (inbytesleft != 0 || outbytesleft < 30)
    return 0;
  else if (outbytesleft == 31)
    return (unsigned short) outbuf[0];
  else
    return (unsigned short) (outbuf[0] << 8|outbuf[1]);
}

long
CMap_decode (CMap *cmap,
	     const unsigned char **inbuf,  long *inbytesleft,
	     unsigned char **outbuf, long *outbytesleft)
{
  int count;

  ASSERT(cmap && inbuf && outbuf);
  ASSERT(inbytesleft && outbytesleft);
  for (count = 0;*inbytesleft > 0 && *outbytesleft > 0; count++)
    CMap_decode_char(cmap, inbuf, inbytesleft, outbuf, outbytesleft);

  return count;
}

char *
CMap_get_name (CMap *cmap)
{
  ASSERT(cmap);
  return cmap->name;
}

int
CMap_get_type (CMap *cmap)
{
  ASSERT(cmap);
  return cmap->type;
}

int
CMap_get_wmode (CMap *cmap)
{
  ASSERT(cmap);
  return cmap->wmode;
}

CIDSysInfo *
CMap_get_CIDSysInfo (CMap *cmap)
{
  ASSERT(cmap);
  return cmap->CSI;
}

void
CMap_set_name (CMap *cmap, const char *name)
{
  ASSERT(cmap);
  if (cmap->name)
    RELEASE(cmap->name);
  cmap->name = strdup(name);
}

void
CMap_set_type (CMap *cmap, int type)
{
  ASSERT(cmap);
  cmap->type = type;
}

void
CMap_set_wmode (CMap *cmap, int wmode)
{
  ASSERT(cmap);
  cmap->wmode = wmode;
}

void
CMap_set_CIDSysInfo (CMap *cmap, CIDSysInfo *csi)
{
  ASSERT(cmap);

  if (cmap->CSI) {
    if (cmap->CSI->registry) RELEASE(cmap->CSI->registry);
    if (cmap->CSI->ordering) RELEASE(cmap->CSI->ordering);
    RELEASE(cmap->CSI);
  }

  if (csi) {
    cmap->CSI = NEW(1, CIDSysInfo);
    cmap->CSI->registry = strdup(csi->registry);
    cmap->CSI->ordering = strdup(csi->ordering);
    cmap->CSI->supplement = csi->supplement;
  } else
    cmap->CSI = NULL;
}

/*
 * Can have muliple entry ?
 */
void
CMap_set_usecmap (CMap *cmap, CMap *ucmap)
{
  int i;

  ASSERT(cmap);
  ASSERT(ucmap); /* Maybe if (!ucmap) ERROR() is better for this. */

  if (cmap == ucmap)
    ERROR("%s: Identical CMap object cannot be used for usecmap CMap: 0x%p=0x%p",
	  CMAP_DEBUG_STR, cmap, ucmap);

  /* Check if ucmap have neccesary information. */
  if (!CMap_is_valid(ucmap))
    ERROR("%s: Invalid CMap.", CMAP_DEBUG_STR);

  /*
   *  CMapName of cmap can be undefined when usecmap is executed in CMap parsing.
   *  And it is also possible CSI is not defined at that time.
   */
  if (cmap->name && strcmp(cmap->name, ucmap->name) == 0)
    ERROR("%s: CMap refering itself not allowed: CMap %s --> %s",
	  CMAP_DEBUG_STR, cmap->name, ucmap->name);

  if (cmap->CSI && cmap->CSI->registry && cmap->CSI->ordering) {
    if (strcmp(cmap->CSI->registry, ucmap->CSI->registry) ||
	strcmp(cmap->CSI->ordering, ucmap->CSI->ordering))
      ERROR("%s: CMap %s required by %s have different CSI.",
	    CMAP_DEBUG_STR, CMap_get_name(cmap), CMap_get_name(ucmap));
  }

  /* We must copy codespaceranges. */
  for (i = 0; i < ucmap->codespace.num; i++) {
    rangeDef *csr = ucmap->codespace.ranges + i;
    CMap_add_codespacerange(cmap, csr->codeLo, csr->codeHi, csr->dim);
  }

  cmap->useCMap = ucmap;
}

/* Test the validity of character c. */
int
CMap_match_codespace (CMap *cmap, const unsigned char *c, int dim)
{
  int i, pos;

  ASSERT(cmap);
  for (i = 0; i < cmap->codespace.num; i++) {
    rangeDef *csr = cmap->codespace.ranges + i;
    if (csr->dim != dim)
      continue;
    for (pos = 0; pos < dim; pos++) {
      if (c[pos] > csr->codeHi[pos] || c[pos] < csr->codeLo[pos])
	break;
    }
    if (pos == dim)
      return 0; /* Valid */
  }

  return -1; /* Invalid */
}

/*
 * No overlapping codespace ranges are allowed, otherwise mapping is ambiguous.
 */
int
CMap_add_codespacerange (CMap *cmap,
			 const unsigned char *codelo, const unsigned char *codehi, int dim)
{
  rangeDef *csr = NULL;
  int       i;

  ASSERT(cmap && dim > 0);

  for (i = 0; i < cmap->codespace.num; i++) {
    int j, overlap = 1;
    csr = cmap->codespace.ranges + i;
    for (j = 0; j < MIN(csr->dim, dim) && overlap; j++) {
      if ((codelo[j] >= csr->codeLo[j] && codelo[j] <= csr->codeHi[j]) ||
	  (codehi[j] >= csr->codeLo[j] && codehi[j] <= csr->codeHi[j]))
	overlap = 1;
      else
	overlap = 0;
    }
    if (overlap) {
      WARN("Overlapping codespace found. (ingored)");
      return -1;
    }
  }

  if (dim < cmap->profile.minBytesIn)
    cmap->profile.minBytesIn = dim;
  if (dim > cmap->profile.maxBytesIn)
    cmap->profile.maxBytesIn = dim;

  if (cmap->codespace.num + 1 > cmap->codespace.max) {
    cmap->codespace.max += 10;
    cmap->codespace.ranges = RENEW(cmap->codespace.ranges, cmap->codespace.max, struct rangeDef);
  }

  csr = cmap->codespace.ranges + cmap->codespace.num;
  csr->dim    = dim;
  csr->codeHi = get_mem(cmap, dim);
  csr->codeLo = get_mem(cmap, dim);
  memcpy(csr->codeHi, codehi, dim);
  memcpy(csr->codeLo, codelo, dim);

  (cmap->codespace.num)++;

  return 0;
}

int
CMap_add_notdefchar (CMap *cmap, const unsigned char *src, int srcdim, CID dst)
{
  return CMap_add_notdefrange(cmap, src, src, srcdim, dst);
}

int
CMap_add_notdefrange (CMap *cmap,
		      const unsigned char *srclo, const unsigned char *srchi, int srcdim, CID dst)
{
  int     c;
  mapDef *cur;

  ASSERT(cmap);
  /* dst not used here */
  /* FIXME */
  if (check_range(cmap, srclo, srchi, srcdim, (const unsigned char *)&dst, 2) < 0)
    return -1;
    
  if (cmap->mapTbl == NULL )
    cmap->mapTbl = mapDef_new();

  cur = cmap->mapTbl;
  if (locate_tbl(&cur, srclo, srcdim) < 0)
    return -1;

  for (c = srclo[srcdim-1]; c <= srchi[srcdim-1]; c++) {
    if (MAP_DEFINED(cur[c].flag)) {
      WARN("Trying to redefine already defined code mapping. (ignored)");
    } else {
      cur[c].flag = (MAP_LOOKUP_END|MAP_IS_NOTDEF);
      cur[c].code = get_mem(cmap, 2);
      cur[c].len  = 2;
      cur[c].code[0] = dst >> 8;
      cur[c].code[1] = dst & 0xff;
    }
    /* Do not do dst++ for notdefrange  */
  }

  return 0;
}

int
CMap_add_bfchar (CMap *cmap,
		 const unsigned char *src, int srcdim,
		 const unsigned char *dst, int dstdim)
{
  return CMap_add_bfrange(cmap, src, src, srcdim, dst, dstdim);
}

int
CMap_add_bfrange (CMap *cmap,
		  const unsigned char *srclo, const unsigned char *srchi, int srcdim,
		  const unsigned char *base, int dstdim)
{
  int     c;
  mapDef *cur;

  ASSERT(cmap);
  if (check_range(cmap, srclo, srchi, srcdim, base, dstdim) < 0)
    return -1;

  if (cmap->mapTbl == NULL )
    cmap->mapTbl = mapDef_new();

  cur = cmap->mapTbl;
  if (locate_tbl(&cur, srclo, srcdim) < 0)
    return -1;

  for (c = srclo[srcdim-1]; c <= srchi[srcdim-1]; c++) {
    if (MAP_DEFINED(cur[c].flag)) {
      WARN("Trying to redefine already defined code mapping. (ignored)");
    } else {
      /*
       * We assume restriction to code ranges also applied here.
       * Addition <00FF> + 1 is undefined.
       */
      cur[c].flag = (MAP_LOOKUP_END|MAP_IS_CODE);
      cur[c].code = get_mem(cmap, dstdim);
      cur[c].len  = dstdim;
      memcpy(cur[c].code, base, dstdim);
      cur[c].code[dstdim-1] += c - srclo[srcdim-1];
    }
  }

  return -1;
}

int
CMap_add_cidchar (CMap *cmap, const unsigned char *src, int srcdim, CID dst)
{
  return CMap_add_cidrange(cmap, src, src, srcdim, dst);
}

int
CMap_add_cidrange (CMap *cmap,
		   const unsigned char *srclo, const unsigned char *srchi, int srcdim, CID base)
{
  int     c;
  mapDef *cur;

  ASSERT(cmap);
  /* base not used here */
  if (check_range(cmap, srclo, srchi, srcdim, (const unsigned char *)&base, 2) < 0) /* FIXME */
    return -1;

  if (cmap->mapTbl == NULL )
    cmap->mapTbl = mapDef_new();

  cur = cmap->mapTbl;
  if (locate_tbl(&cur, srclo, srcdim) < 0)
    return -1;

  for (c = srclo[srcdim-1]; c <= srchi[srcdim-1]; c++) {
    if (cur[c].flag != 0) {
      WARN("Trying to redefine already defined CID mapping. (ignored)");
    } else {
      cur[c].flag = (MAP_LOOKUP_END|MAP_IS_CID);
      cur[c].len  = 2;
      cur[c].code = get_mem(cmap, 2);
      cur[c].code[0] = base >> 8;
      cur[c].code[1] = base & 0xff;
    }
    base++;
    if (base > CID_MAX)
      WARN("CID number too large.");
  }

  return 0;
}

static void
mapDef_release (mapDef *t)
{
  int c;

  ASSERT(t);
  for (c = 0; c < 256; c++) {
    if (LOOKUP_CONTINUE(t[c].flag))
      mapDef_release(t[c].next);
  }
  RELEASE(t);
}

static mapDef *
mapDef_new (void)
{
  mapDef *t;
  int     c;

  t = NEW(256, mapDef);
  for (c=0; c<256; c++) {
    t[c].flag = (MAP_LOOKUP_END|MAP_IS_UNDEF);
    t[c].code = NULL;
    t[c].next = NULL;
  }

  return t;
}

static unsigned char *
get_mem (CMap *cmap, int size)
{
  mapData *map;
  unsigned char  *p;

  ASSERT(cmap && cmap->mapData && size >= 0);
  map = cmap->mapData;
  if (map->pos + size >= MEM_ALLOC_SIZE) {
    mapData *prev = map->prev;
    map = NEW(1, struct mapData); 
    map->data = NEW(MEM_ALLOC_SIZE, unsigned char);
    map->prev = prev;
    map->pos  = 0;
    cmap->mapData = map;
  }
  p = map->data + map->pos;
  map->pos += size;

  return p;
}

static int
locate_tbl (mapDef **cur, const unsigned char *code, int dim)
{
  int i, c;

  ASSERT(cur && *cur);
  for (i = 0; i < dim-1; i++) {
    c = code[i];
    if (MAP_DEFINED((*cur)[c].flag)) {
      WARN("Ambiguous CMap entry.");
      return -1;
    }
    if ((*cur)[c].next == NULL)  /* create new node */
      (*cur)[c].next = mapDef_new();
    (*cur)[c].flag  |= MAP_LOOKUP_CONTINUE;
    *cur = (*cur)[c].next;
  }

  return 0;
}

/*
 * Guess how many bytes consumed as a `single' character:
 * Substring of length bytesconsumed bytes of input string is interpreted as
 * a `single' character by CMap_decode().
 */
static int
bytes_consumed (CMap *cmap, const unsigned char *instr, long inbytes)
{
  int i, pos, longest = 0, bytesconsumed;

  ASSERT(cmap);
  for (i = 0; i < cmap->codespace.num; i++) {
    rangeDef *csr = cmap->codespace.ranges + i;
    for (pos = 0; pos < MIN(csr->dim, inbytes); pos++) {
      if (instr[pos] > csr->codeHi[pos] || instr[pos] < csr->codeLo[pos])
	break;
    }
    if (pos == csr->dim) /* part of instr is totally valid in this codespace. */
      return csr->dim;
    if (pos > longest)
      longest = pos;
  }

  if (i == cmap->codespace.num) /* No matching at all */
    bytesconsumed = cmap->profile.minBytesIn;
  else {
    bytesconsumed = cmap->profile.maxBytesIn;
    for (i = 0; i< cmap->codespace.num; i++) {
      rangeDef *csr = cmap->codespace.ranges + i;
      if (csr->dim > longest && csr->dim < bytesconsumed)
	bytesconsumed = csr->dim;
    }
  }

  return bytesconsumed;
}

static int
check_range (CMap *cmap,
	     const unsigned char *srclo, const unsigned char *srchi, int srcdim,
	     const unsigned char *dst, int dstdim)
{
  if ((srcdim < 1 || dstdim < 1) ||
      (!srclo || !srchi || !dst) ||
      strncmp(srclo, srchi, srcdim - 1) ||
      srclo[srcdim-1] > srchi[srcdim-1]) {
    WARN("Invalid CMap mapping entry. (ignored)");
    return -1;
  }

  if (CMap_match_codespace(cmap, srclo, srcdim) < 0 ||
      CMap_match_codespace(cmap, srchi, srcdim) < 0) {
    WARN("Invalid CMap mapping entry. (ignored)");
    return -1;
  }

  if (srcdim < cmap->profile.minBytesIn)
    cmap->profile.minBytesIn  = srcdim;
  if (srcdim > cmap->profile.maxBytesIn)
    cmap->profile.maxBytesIn  = srcdim;
  if (dstdim < cmap->profile.minBytesOut)
    cmap->profile.minBytesOut = dstdim;
  if (dstdim > cmap->profile.maxBytesOut)
    cmap->profile.maxBytesOut = dstdim;

  return 0;
}

/************************** CMAP_CACHE **************************/
#include "cmap_parse.h"

#define CMAP_CACHE_ALLOC_SIZE 16u

struct CMap_cache {
  int    num;
  int    max;
  CMap **cmaps;
};

static struct CMap_cache *__cache = NULL;

#define CHECK_ID(n) do {\
                        if (! __cache)\
                           ERROR("%s: CMap cache not initialized.", CMAP_DEBUG_STR);\
                        if ((n) < 0 || (n) >= __cache->num)\
                           ERROR("Invalid CMap ID %d", (n));\
                    } while (0)

#ifdef HAVE_DPXFILE
#include "dpxfile.h"
#else /* !HAVE_DPXFILE */
#include "mfileio.h"
#ifndef HAVE_KPSE_ENC_FORMATS
#define kpse_cmap_format kpse_program_text_format
#endif
static FILE *dpx_open_cmap_file (const char *cmap_name);
#define DPX_RES_TYPE_CMAP 0
#define DPXFOPEN(n,t)     dpx_open_cmap_file((const char *)(n))
#define DPXFCLOSE(f)      MFCLOSE((f))

static FILE *
dpx_open_cmap_file (const char *filename)
{
  char *fullname;

#ifdef MIKTEX
  if (!miktex_find_app_input_file("dvipdfm", filename, work_buffer)) {
    if (miktex_get_acrobat_font_dir(work_buffer)) {
      strcat(work_buffer, "\\..\\CMap\\");
      strcat(work_buffer, filename);
      fullname = work_buffer;
    } else {
      fullname = NULL;
    }
  } else {
    fullname = work_buffer;
  }
#else
  fullname = kpse_find_file(filename, kpse_cmap_format, 0);
#ifndef HAVE_KPSE_ENC_FORMATS
  {
    if (!fullname) {
      char *altname;
      char *suffix;

      suffix = strrchr(filename, '.');
      if (suffix == NULL || strcmp(suffix, ".cmap") != 0) {
	altname = NEW(strlen(filename)+strlen(".cmap")+1, char);
	strcpy(altname, filename);
	strcat(altname, ".cmap");
      }  else {
	altname = NULL;
      }
      if (altname)
	fullname = kpse_find_file(altname, kpse_cmap_format, 0);
      if (!fullname) {
	kpse_reset_program_name("cmap");
	fullname = kpse_find_file(filename, kpse_cmap_format, 0);
	if (!fullname && altname)
	  fullname = kpse_find_file(altname, kpse_cmap_format, 0);
	kpse_reset_program_name("dvipdfm");
      }
      if (altname)
	RELEASE(altname);
    }
  }
#endif /* !KPSE_ENC_FORMATS */
#endif /* MIKTEX */

  if (fullname == NULL)
    return NULL;

  return MFOPEN(fullname, FOPEN_RBIN_MODE);
}
#endif /* HAVE_DPXFILE */

void
CMap_cache_init (void)
{
  if (__cache)
    ERROR("%s: Already initialized.", CMAP_DEBUG_STR);

  __cache = NEW(1, struct CMap_cache);

  __cache->max   = CMAP_CACHE_ALLOC_SIZE;
  __cache->cmaps = NEW(__cache->max, CMap *);
  __cache->num   = 0;

  /* Create Identity mapping */
#define IDENTITY_RANGE_MIN "0000"
#define IDENTITY_RANGE_MAX "\255\255\255\255"
  {
    CIDSysInfo *csi;

    csi = NEW(1, CIDSysInfo);
    csi->registry   = strdup("Adobe");
    csi->ordering   = strdup("Identity");
    csi->supplement = 0;

    __cache->cmaps[0] = CMap_new();
    CMap_set_name(__cache->cmaps[0], "Identity-H");
    CMap_set_type(__cache->cmaps[0], CMAP_TYPE_IDENTITY);
    CMap_add_codespacerange(__cache->cmaps[0], IDENTITY_RANGE_MIN, IDENTITY_RANGE_MAX, 2);

    __cache->cmaps[1] = CMap_new();
    CMap_set_name(__cache->cmaps[1], "Identity-V");
    CMap_set_type(__cache->cmaps[1], CMAP_TYPE_IDENTITY);
    CMap_set_wmode(__cache->cmaps[1], 1);
    CMap_add_codespacerange(__cache->cmaps[1], IDENTITY_RANGE_MIN, IDENTITY_RANGE_MAX, 2);

    CMap_set_CIDSysInfo(__cache->cmaps[0], csi);
    CMap_set_CIDSysInfo(__cache->cmaps[1], csi);

    __cache->num += 2;

    RELEASE(csi->registry);
    RELEASE(csi->ordering);
    RELEASE(csi);
  }
}

CMap *
CMap_cache_get (int id)
{
  CHECK_ID(id);
  return __cache->cmaps[id];
}

int
CMap_cache_find (const char *cmap_name)
{
  int   id = 0;
  FILE *fp = NULL;

  if (!__cache)
    CMap_cache_init();
  ASSERT(__cache);

  for (id = 0; id < __cache->num; id++) {
    unsigned char *name = NULL;
    /* CMapName may be undefined when processing usecmap. */
    name = CMap_get_name(__cache->cmaps[id]);
    if (name && strcmp(cmap_name, name) == 0) {
      return id;
    }
  }

  fp = DPXFOPEN(cmap_name, DPX_RES_TYPE_CMAP);
  if (!fp)
    return -1;

  if (CMap_parse_check_sig(fp) < 0) {
    DPXFCLOSE(fp);
    return -1;
  }
     
  if (__verbose)
    MESG("(CMap:%s", cmap_name);

  if (__cache->num >= __cache->max) {
    __cache->max   += CMAP_CACHE_ALLOC_SIZE;
    __cache->cmaps = RENEW(__cache->cmaps, __cache->max, CMap *);
  }
  id = __cache->num;
  (__cache->num)++;
  __cache->cmaps[id] = CMap_new();

  if (CMap_parse(__cache->cmaps[id], fp) < 0)
    ERROR("%s: Parsing CMap file failed.", CMAP_DEBUG_STR);

  DPXFCLOSE(fp);

  if (__verbose)
    MESG(")");

  return id;
}

void
CMap_cache_close (void)
{
  if (__cache) {
    int id;
    for (id = 0; id < __cache->num; id++)
      CMap_release(__cache->cmaps[id]);
    RELEASE(__cache->cmaps);
    RELEASE(__cache);
    __cache = NULL;
  }
}
