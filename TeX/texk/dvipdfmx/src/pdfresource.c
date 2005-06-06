/*  $Header: /home/cvsroot/dvipdfmx/src/pdfresource.c,v 1.3 2003/12/07 09:01:33 hirata Exp $

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

/*
 * Currently, this is nearly useless.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "system.h"
#include "mem.h"
#include "error.h"
#include "dpxutil.h"

#include "pdfobj.h"

#include "pdfresource.h"

#define PDF_RESOURCE_DEBUG_STR "PDF"
#define PDF_RESOURCE_DEBUG     3

typedef struct
{
  char    *name;
  int      type;
#if 0
  void    *data;
#endif
  pdf_obj *obj;
  pdf_obj *ref;
} PDF_resource;

#if 0
static struct {
  char *name;
  int   id;
} pdf_resource_categories[] = {
  {"Font",       PDF_RES_TYPE_FONT},
  {"CIDFont",    PDF_RES_TYPE_CIDFONT},
  {"Encoding",   PDF_RES_TYPE_ENCODING},
  {"CMap",       PDF_RES_TYPE_CMAP},
  {NULL, 0}
};
#endif

#define CACHE_ALLOC_SIZE 16u
struct PDF_resource_cache
{
  int num;
  int max;
  PDF_resource **resources;
};

static struct PDF_resource_cache *__cache = NULL;

static PDF_resource *
PDF_resource_new (void)
{
  PDF_resource *resource;

  resource = NEW(1, PDF_resource);
  resource->name = NULL;
  resource->type = 0;
  resource->obj  = NULL;
  resource->ref  = NULL;

  return resource;
}

static void
PDF_resource_flush (PDF_resource *resource)
{
  if (resource->ref) pdf_release_obj(resource->ref);
  if (resource->obj) pdf_release_obj(resource->obj);

  resource->ref = NULL;
  resource->obj = NULL;
}

static void
PDF_resource_release (PDF_resource *resource)
{
  if (resource) {
    if (resource->ref && resource->obj)
      WARN("Trying to release un-flushed object.");
    if (resource->ref) pdf_release_obj(resource->ref);
    if (resource->obj) pdf_release_obj(resource->obj);
    if (resource->name) RELEASE(resource->name);
    RELEASE(resource);
  }
}

void
PDF_resource_init (void)
{
  if (__cache)
    ERROR("%s: Already initialized.", PDF_RESOURCE_DEBUG_STR);

  __cache = NEW(1, struct PDF_resource_cache);
  __cache->max = CACHE_ALLOC_SIZE;
  __cache->resources = NEW(__cache->max, PDF_resource *);
  __cache->num = 0;
}

void
PDF_resource_close (void)
{
  if (__cache) {
    int res_id;
    for (res_id = 0; res_id < __cache->num; res_id++) {
      PDF_resource *resource = __cache->resources[res_id];
      PDF_resource_flush(resource);
      PDF_resource_release(resource);
    }
    if (__cache->resources)
      RELEASE(__cache->resources);
    RELEASE(__cache);
  }
}

static pdf_obj *
get_resource (PDF_resource *resource)
{
  ASSERT(resource);

  /*
   * Flush object to save memory usage.
   */
  if (!resource->ref) {
    resource->ref = pdf_ref_obj(resource->obj);
    pdf_release_obj(resource->obj);
    resource->obj = NULL;
  }

  return pdf_link_obj(resource->ref);
}

pdf_obj *
PDF_defineresource (const char *res_name, pdf_obj *res_obj, int res_type)
{
  PDF_resource *resource = NULL;
  int           res_id;

  ASSERT(res_name);

  if (!__cache)
    PDF_resource_init();
  ASSERT(__cache);

  for (res_id = 0; res_id < __cache->num; res_id++) {
    resource = __cache->resources[res_id];
    if (!strcmp(res_name, resource->name) && res_type == resource->type)
      break;
  }

  if (res_id < __cache->num) {
    PDF_resource_flush(resource);
    resource->obj = res_obj;
  } else {
    if (__cache->num >= __cache->max) {
      __cache->max += CACHE_ALLOC_SIZE;
      __cache->resources = RENEW(__cache->resources, __cache->max, PDF_resource *);
    }
    resource = PDF_resource_new();
    resource->name = strdup(res_name);
    resource->type = res_type;
    resource->obj  = res_obj;

    __cache->resources[res_id] = resource;
    (__cache->num)++;
  }

  return res_obj;
}

pdf_obj *
PDF_findresource (const char *res_name, int res_type)
{
  int res_id;

  ASSERT(res_name);

  if (!__cache)
    PDF_resource_init();
  ASSERT(__cache);


  for (res_id = 0; res_id < __cache->num; res_id++) {
    PDF_resource *resource = __cache->resources[res_id];
    if (!strcmp(res_name, resource->name) && res_type == resource->type)
      return get_resource(resource);
  }

  return NULL;
}
