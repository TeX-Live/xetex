/*  $Header: /home/cvsroot/dvipdfmx/src/agl.c,v 1.7 2004/02/15 12:59:42 hirata Exp $

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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <ctype.h>

#include "system.h"
#include "mem.h"
#include "error.h"

#include "mfileio.h"
#include "pdfparse.h"
#include "dpxutil.h"

#include "unicode.h"

#include "agl.h"

#define AGL_DEBUG_STR "AGL"
#define AGL_DEBUG     5

static unsigned int __verbose = 0;

static AGList *AGList_new      (void);
static void    AGList_release  (AGList *agl);
static void    AGList_insert   (AGList *agl, AGList *glyph);
static unsigned int get_hash   (const char *key);

void
AGL_set_verbose (void)
{
  __verbose++;
}

char *
AGName_normalize (char *glyphname)
{
  /*
   * Not Implemented Yet
   *
   * This has nothing to do with the Unicode normalization forms.
   * This routine is supposed to do a conversion of glyph names such as
   * "Asmall" to "a.sc", "summationtext" to "summation.text", ... but not
   * "Aacute" to "A" + "acute".
   */

  return NULL;
}

char *
AGName_strip_suffix (char *glyphname)
{
  char *suffix, *dot;

  dot = strchr(glyphname, '.');
  if (dot) {
    if (strlen(dot) > 1) {
      suffix = NEW(strlen(dot+1)+1, char);
      strcpy(suffix, dot+1);
    } else {
      suffix = NULL;
    }
    *dot = '\0';
  } else
    suffix = NULL;

  return suffix;
}

int
AGName_is_unicode (const char *glyphname)
{
  char c;

  if (!glyphname || strlen(glyphname) < 7 ||
      strncmp(glyphname, "uni", 3) != 0) {
    return 0;
  }
  c = glyphname[3];
  if (isdigit(c) || (c >= 'A' && c <= 'F'))
    return 1;
  else
    return 0;

  return 0;
}

long
AGName_convert_uni (const char *glyphname)
{
  long  ucv = -1;
  char *p;

  if (!AGName_is_unicode(glyphname))
    return -1;

  if (strlen(glyphname) > 7 && *(glyphname+7) != '.') {
    WARN("Mapping to multiple Unicode codes not supported.");
    return -1;
  }

  p = (char *) (glyphname + 3);
  ucv = 0;
  while (*p && *p != '.') {
    if (!isdigit(*p) && (*p < 'A' || *p > 'F')) {
      WARN("Invalid char %c in Unicode glyph name %s.", *p, glyphname);
      return -1;
    }
    ucv <<= 4;
    ucv += isdigit(*p) ? *p - '0' : *p - 'A' + 10;
    p++;
  }

  if (ucv > 0xD7FFL && ucv < 0xE000L) {
    WARN("Invalid Unicode code value U+%08X.", ucv);
    ucv = -1;
  }

  return ucv;
}


#define AGL_FLAG_NEXT_IS_ALT (1 << 0)
#define AGL_FLAG_CHAR_DECOMP (1 << 1)

/*
 * The following glyphname lenght limit is taken from
 * "Glyph Names and Current Implementations":
 *   http://partners.adobe.com/asn/tech/type/glyphnamelimits.jsp
 * , Document version 1.1. Last updated January 31, 2003.
 */
#define AGL_GLYPHNAME_LEN_MAX 31

/*
 * Adobe glyph list contains 4,000+ glyph name entries...
 */
struct AGList {
  int   flags;

  char *name;
  char *suffix;

  long  code; /* -1 for composite glyph */
  struct AGList *comp;

  struct AGList *next;
};

#define IS_COMPOSITE(g) ((g) && ((g)->flags & AGL_FLAG_CHAR_DECOMP))
#define HAVE_ALTERN(g)  ((g) && ((g)->flags & AGL_FLAG_NEXT_IS_ALT))
#define HAVE_SUFFIX(g)  ((g) && ((g)->suffix))

static AGList *
AGList_new (void)
{
  AGList *agl;

  agl = NEW(1, AGList);

  agl->flags  = 0;
  agl->name   = NULL;
  agl->suffix = NULL;
  agl->code   = -1;
  agl->comp   = NULL;

  agl->next   = NULL;

  return agl;
}

static void
AGList_release (AGList *agl)
{
  while (agl) {
    AGList *next = agl->next;
    if (agl->name)
      RELEASE(agl->name);
    if (agl->suffix)
      RELEASE(agl->suffix);
    if (IS_COMPOSITE(agl))
      AGList_release(agl->comp);
    RELEASE(agl);
    agl = next;
  }
}

char *
AGList_get_name (AGList *agl)
{
  char *glyphname;
  int   len;

  ASSERT(agl);

  len = strlen(agl->name);
  if (agl->suffix)
    len += strlen(agl->suffix) + 1;

  glyphname = NEW(len + 1, char);
  strcpy(glyphname, agl->name);
  if (agl->suffix) {
    sprintf(glyphname, "%s.%s", agl->name, agl->suffix);
  } else {
    strcpy(glyphname, agl->suffix);
  }
  
  return glyphname;
}

long
AGList_get_code (AGList *agl)
{
  ASSERT(agl);

  return (IS_COMPOSITE(agl) ? -1 : agl->code);
}

int
AGList_is_composite (AGList *agl)
{
  ASSERT(agl);

  return (IS_COMPOSITE(agl) ? 1 : 0);
}

AGList *
AGList_next_alternative (AGList *agl)
{
  ASSERT(agl);

  return (HAVE_ALTERN(agl) ? agl->next : NULL);
}

long
AGList_sputx_UTF16BE (AGList *glyph, unsigned char **dst, unsigned char *dstend)
{
  long len;

  ASSERT(glyph);

  len = 0;
  if (IS_COMPOSITE(glyph)) {
    glyph = glyph->comp;
    while (glyph) {
      if (HAVE_ALTERN(glyph))
	ERROR("Unexpected error.");
      len += AGList_sputx_UTF16BE(glyph, dst, dstend);
      glyph = glyph->next;
    }
  } else {
    len += UC_sputx_UTF16BE(glyph->code, dst, dstend);
  }

  return len;
}

#define MATCH_SUFFIX(g,s) ((!(s) && !((g)->suffix)) || \
                           ((s) && (g)->suffix && !strcmp((s),(g)->suffix)))
#define EXACT_MATCH(g,n,s) (!strcmp((g)->name,(n)) && MATCH_SUFFIX((g),(s)))

static void
AGList_insert (AGList *agl, AGList *glyph)
{
  AGList *prev = NULL;

  ASSERT(agl && glyph);
  while (agl) {
    if (!strcmp(agl->name, glyph->name)) {
      if (MATCH_SUFFIX(agl, glyph->suffix)) {
	if (agl->code == glyph->code)
	  return;
	break;
      } else {
	if (!HAVE_ALTERN(agl))
	  break;
      }
    }
    prev = agl;
    agl  = agl->next;
  }
  if (!agl) {
    if (prev)
      prev->next = glyph;
  } else {
    glyph->flags |= AGL_FLAG_NEXT_IS_ALT;
    glyph->next  = agl;
    if (prev)
      prev->next = glyph;
  }
}

struct AGLmap {
  char    *ident;
  AGList **map;
};

#define HASH_TABLE_SIZE 503

static unsigned int
get_hash (const char *key)
{
  unsigned int h = 0;

  while (*key)
    h = 33*h + (*key++);

  return h % HASH_TABLE_SIZE;
}

AGLmap *
AGLmap_new (void)
{
  AGLmap *aglm;

  aglm = NEW(1, AGLmap);
  aglm->ident = NULL;
  aglm->map   = NEW(HASH_TABLE_SIZE, AGList *);
  memset(aglm->map, 0, HASH_TABLE_SIZE*sizeof(AGList *));

  return aglm;
}

void
AGLmap_release (AGLmap *aglm)
{
  if (aglm) {
    if (aglm->ident)
      RELEASE(aglm->ident);
    if (aglm->map) {
      int i;
      for (i = 0; i < HASH_TABLE_SIZE; i++) {
	if (aglm->map[i])
	  AGList_release(aglm->map[i]);
      }
      RELEASE(aglm->map);
    }
  }
}

static AGList *
make_composite_glyph (char **start, char *end, char *name, char *suffix, long first)
{
  AGList *agl, *curr;
  long  code;
  char *next;

  agl = AGList_new();
  agl->name   = name;
  agl->suffix = suffix;
  agl->flags |= AGL_FLAG_CHAR_DECOMP;

  agl->comp = curr = AGList_new();
  /* Glyph name unknown */
  curr->code = first;

  skip_white(start, end);
  while (*start < end && isxdigit(**start)) {
    code = strtol(*start, &next, 16);
    if (next == *start)
      break;
    curr->next = AGList_new();
    curr = curr->next;
    /* Glyph name unknown */
    curr->code = code;
    *start = next;
    if (**start == '_')
      *start += 1;
    else
      skip_white(start, end);
  }

  return agl;
}

static void
dump_list (AGList *agl)
{
  while (agl) {
    MESG("%s", agl->name ? agl->name : "UNKNOWN");
    if (agl->suffix)
      MESG(".%s", agl->suffix);
    if (IS_COMPOSITE(agl))
      MESG("[COMPOSITE]");
    if (HAVE_ALTERN(agl))
      MESG("[ALTERNATE]");
    if (IS_COMPOSITE(agl)) {
      MESG(" ==> ");
      dump_list(agl->comp);
    } else {
      if (agl->code > 0xFFFFL)
	MESG(": %08X", agl->code);
      else
	MESG(": %04X", agl->code);
      MESG(";");
    }
    if (HAVE_ALTERN(agl))
      MESG(" ");
    agl = agl->next;
  }
}

/*
 * format unused.
 */
#define WBUF_SIZE 1024
int
AGLmap_read (AGLmap *aglm, FILE *fp, int format)
{
  int   count = 0;
  char  wbuf[WBUF_SIZE];
  char *start = NULL, *end, *next;

  ASSERT(aglm);

  while ((start = mfgets(wbuf, WBUF_SIZE, fp)) != NULL) {
    AGList *agl;
    char   *name, *suffix;
    long    code;

    end = start + strlen(start);
    skip_white(&start, end);
    /* Need table version check. */
    if (!start || *start == '#') {
      continue;
    }
    next = strchr(start, ';');
    if (!next || next == start) {
      continue;
    }
    name   = parse_ident(&start, next);
    suffix = AGName_strip_suffix(name);
    start = next+1;
    code  = strtol(start, &next, 16);
    if (next == start) {
      WARN("Invalid AGL entry (ignored): %s", wbuf);
      if (name)   RELEASE(name);
      if (suffix) RELEASE(suffix);
      continue;
    }
    start = next;
    skip_white(&start, end);
    if (isxdigit(*start)) { /* Decomposition */
      agl = make_composite_glyph(&start, end, name, suffix, code);
    } else {
      agl = AGList_new();
      agl->name   = name;
      agl->suffix = suffix;
      agl->code   = code;
    }
    {
      unsigned int idx = get_hash(name);
      if (!aglm->map[idx]) {
	aglm->map[idx] = agl;
      } else {
	AGList_insert(aglm->map[idx], agl);
      }
      if (__verbose > AGL_DEBUG) {
	MESG("%s: ", AGL_DEBUG_STR);
	dump_list(agl);
	MESG("\n");
      }
    }
    count++;
  }

  if (__verbose > AGL_DEBUG)
    MESG("\n%s: %d glyph list entries found.\n", AGL_DEBUG_STR, count);

  return count;
}

AGList *
AGLmap_lookup (AGLmap *aglm, const char *glyphname, const char *suffix)
{
  unsigned int hkey = 0;
  AGList *agl;

  if (!aglm)
    return NULL;
  
  hkey = get_hash(glyphname);
  agl = (aglm->map)[hkey];
  while (agl) {
    if (EXACT_MATCH(agl, glyphname, suffix))
      break;
    agl = agl->next;
  }

  return agl;
}

long
AGLmap_decode_UTF16BE (AGLmap *aglm, const char *glyphstr,
		       unsigned char **dst, unsigned char *dstend, int *fail_count)
{
  long  len;
  int   count;
  char *cur, *end;

  ASSERT(aglm);

  cur = (char *) glyphstr;
  end = (char *) glyphstr + strlen(glyphstr);

  len = 0; count = 0;
  while (cur < end) {
    char   *name, *suffix, *next;
    long    ucv;
    AGList *agl;
    char tmp[AGL_GLYPHNAME_LEN_MAX+1];

    next = strchr(cur, '_');
    if (!next)
      next = end;
    if (next > cur + AGL_GLYPHNAME_LEN_MAX) {
      WARN("Glyph name too long.");
      count++;
      continue;
    }

    memcpy(tmp, cur, (int) (next - cur));
    tmp[(int) (next-cur)] = '\0';
    name = tmp;
    suffix = strchr(tmp, '.');
    if (suffix) {
      *suffix = '\0';
      suffix++;
    }

    if (AGName_is_unicode(name)) {
      if (suffix)
	WARN("Suffix in Unicode glyph name \"%s.%s\" ignored.", name, suffix);
      ucv = AGName_convert_uni(name);
      if (!UC_is_valid(ucv)) {
	WARN("Glyph \"%s\" mapped to invalid code U+%08X.", name, ucv);
	count++;
	continue;
      } else if ((ucv >= 0x00E000L && ucv <= 0x00F8FFL) ||
	       (ucv >= 0x0F0000L && ucv <= 0x0FFFFDL) ||
	       (ucv >= 0x100000L && ucv <= 0x10FFFDL)) {
	if (__verbose)
	  WARN("Glyph \"%s\" mapped to PUA U+%08X.", name, ucv);
      }
      len += UC_sputx_UTF16BE(ucv, dst, dstend);
    } else if ((agl = AGLmap_lookup(aglm, name, suffix)) != NULL) {
      len += AGList_sputx_UTF16BE(agl, dst, dstend);
    } else {
      if (__verbose) {
	if (suffix)
	  WARN("No reliable Unicode mapping available for glyph \"%s.%s\".", name, suffix);
	else
	  WARN("No reliable Unicode mapping available for glyph \"%s\".", name);
      }
      count++;
    }
    cur = next + 1;
  }

  if (fail_count)
    *fail_count = count;
  return len;
}

#define CACHE_ALLOC_SIZE 8u
#define CHECK_ID(n) do {\
                        if (! __cache)\
                           ERROR("%s: Not initialized.", AGL_DEBUG_STR);\
                        if ((n) < 0 || (n) >= __cache->num)\
                           ERROR("%s: Invalid ID %d", AGL_DEBUG_STR, (n));\
                    } while (0)

struct AGLmapCache
{
  int      num;
  int      max;
  AGLmap **maps;
};

static struct AGLmapCache *__cache = NULL;

void
AGLmap_cache_init (void)
{
  if (__cache)
    ERROR("%s: Already initialized.", AGL_DEBUG_STR);

  __cache = NEW(1, struct AGLmapCache);
  __cache->max  = CACHE_ALLOC_SIZE;
  __cache->num  = 0;
  __cache->maps = NEW(__cache->max, struct AGLmap *);
}

AGLmap *
AGLmap_cache_get (int id)
{
  CHECK_ID(id);
  return __cache->maps[id];
}

int
AGLmap_cache_find (const char *name)
{
  int     id;
  AGLmap *aglm;
  char   *fullname;
  FILE   *fp;

  if (!__cache)
    AGLmap_cache_init();
  ASSERT(__cache);

  for (id = 0; id < __cache->num; id++) {
    aglm = __cache->maps[id];
    if (!strcmp(name, aglm->ident))
      return id;
  }

  fullname = kpse_find_file(name, kpse_program_text_format, 0);
  if (!fullname) {
    char *altname = NEW(strlen(name)+strlen(".txt")+1, char);
    sprintf(altname, "%s.txt", name);
    fullname = kpse_find_file(altname, kpse_program_text_format, 0);
    RELEASE(altname);
  }

  if (!fullname || !(fp = MFOPEN(fullname, FOPEN_R_MODE)))
    return -1;

  if (__verbose) {
    MESG("(AGL:%s", name);
    if (__verbose > 1)
      MESG("[%s]", fullname);
  }

  aglm = AGLmap_new();
  if (AGLmap_read(aglm, fp, 0) <= 0) {
    AGLmap_release(aglm);
    MFCLOSE(fp);
    return -1;
  }
  MFCLOSE(fp);

  if (__cache->num >= __cache->max) {
    __cache->max += CACHE_ALLOC_SIZE;
    __cache->maps = RENEW(__cache->maps, __cache->max, struct AGLmap *);
  }

  aglm->ident = strdup(name);
  id = __cache->num;
  __cache->maps[id] = aglm;
  (__cache->num)++;

  if (__verbose)
    MESG(")");

  return id;
}

void
AGLmap_cache_close (void)
{
  if (__cache) {
    int id;
    for (id = 0; id < __cache->num; id++)
      AGLmap_release(__cache->maps[id]);
    RELEASE(__cache->maps);
    RELEASE(__cache);
    __cache = NULL;
  }
}
