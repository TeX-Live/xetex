/*  $Header: /home/cvsroot/dvipdfmx/src/pdfnames.c,v 1.3 2008/06/18 15:11:45 matthias Exp $

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

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "system.h"
#include "mem.h"
#include "error.h"
#include "numbers.h"

/* Hash */
#include "dpxutil.h"

#include "pdfobj.h"

#include "pdfnames.h"

struct obj_data
{
  pdf_obj *object_ref;
  pdf_obj *object;
  int      reserve; /* 1 if object is not actually defined. */
};

char *
printable_key (const char *key, int keylen)
{
#define MAX_KEY 32
  static char pkey[MAX_KEY+4];
  int    i, len;
  unsigned char hi, lo;

  for (i = 0, len = 0;
       i < keylen && len < MAX_KEY; i++) {
    if (isprint(key[i])) {
      pkey[len++] = key[i];
    } else {
      hi = (key[i] >> 4) & 0xff;
      lo =  key[i] & 0xff;
      pkey[len++] = '#';
      pkey[len++] = (hi < 10) ? hi + '0' : (hi - 10) + 'A';
      pkey[len++] = (lo < 10) ? lo + '0' : (lo - 10) + 'A';
    }
  }
  pkey[len] = '\0';

  return (char *) pkey;
}

struct ht_table *
pdf_new_name_tree (void)
{
  struct ht_table *names;

  names = NEW(1, struct ht_table);
  ht_init_table(names);

  return names;
}

static void
flush_objects (struct ht_table *ht_tab)
{
  struct ht_iter iter;

  if (ht_set_iter(ht_tab, &iter) >= 0) {
    do {
      char  *key;
      int    keylen;
      struct obj_data *value;

      key   = ht_iter_getkey(&iter, &keylen);
      value = ht_iter_getval(&iter);
      if (value->reserve) {
	WARN("Unresolved object reference \"%s\" found!!!",
	     printable_key(key, keylen));
      }
      if (value->object) {
	pdf_release_obj(value->object);
      }
      if (value->object_ref) {
	pdf_release_obj(value->object_ref);
      }
      value->object     = NULL;
      value->object_ref = NULL;
      value->reserve    = 0;
    } while (ht_iter_next(&iter) >= 0);
    ht_clear_iter(&iter);
  }
}

static void CDECL
hval_free (void *hval)
{
  struct obj_data *value;

  value = (struct obj_data *) hval;
  if (value->object)
    pdf_release_obj(value->object);
  if (value->object_ref)
    pdf_release_obj(value->object_ref);

  value->object     = NULL;
  value->object_ref = NULL;
  value->reserve    = 0;

  RELEASE(value);

  return;
}

void
pdf_delete_name_tree (struct ht_table **names)
{
  ASSERT(names && *names);

  flush_objects (*names);
  ht_clear_table(*names, hval_free);
  RELEASE(*names);
  *names = NULL;
}

int
pdf_names_add_object (struct ht_table *names,
		      const void *key, int keylen, pdf_obj *object)
{
  struct obj_data *value;

  ASSERT(names && object);

  if (!key || keylen < 1) {
    WARN("Null string used for name tree key.");
    return -1;
  }

  value = ht_lookup_table(names, key, keylen);
  if (!value) {
    value = NEW(1, struct obj_data);
    value->object     = object;
    value->object_ref = NULL;
    value->reserve    = 0;
    ht_append_table(names, key, keylen, value);
  } else {
    if (value->reserve) {
      /* null object is used for undefined objects */
      pdf_copy_object(value->object, object);
      pdf_release_obj(object); /* PLEASE FIX THIS!!! */
    } else {
      if (value->object || value->object_ref) {
        if (pdf_obj_get_verbose()) {
	  WARN("Object reference with key \"%s\" is in use.", printable_key(key, keylen));
        }
	pdf_release_obj(object);
	return -1;
      } else {
	value->object = object;
      }
    }
    value->reserve = 0;
  }

  return 0;
}

int
pdf_names_add_reference (struct ht_table *names,
			 const void *key, int keylen, pdf_obj *object_ref)
{
  struct obj_data *value;

  ASSERT(names);

  if (!PDF_OBJ_INDIRECTTYPE(object_ref)) {
    WARN("Invalid type: @%s is not reference...",
	 printable_key(key, keylen));
    return -1;
  }

  value = ht_lookup_table(names, key, keylen);
  if (!value) {
    value = NEW(1, struct obj_data);
    value->object     = NULL;
    value->object_ref = object_ref;
    value->reserve = 0;
    ht_append_table(names, key, keylen, value);
  } else {
    if (value->object || value->object_ref) {
      WARN("Object reference \"%s\" is in use.",
	   printable_key(key, keylen));
      WARN("Please close it before redefining.");
      return -1;
    } else {
      value->object     = NULL;
      value->object_ref = object_ref;
    }
    value->reserve = 0;
  }

  return 0;
}

/*
 * The following routine returns copies, not the original object.
 */
pdf_obj *
pdf_names_lookup_reference (struct ht_table *names,
			    const void *key, int keylen)
{
  struct obj_data *value;

  ASSERT(names);

  value = ht_lookup_table(names, key, keylen);
  /* Reserve object label */
  if (!value) {
    value = NEW(1, struct obj_data);
    value->object     = pdf_new_null(); /* dummy */
    value->object_ref = NULL;
    value->reserve    = 1;
    ht_append_table(names, key, keylen, value);
  }

  if (!value->object_ref) {
    if (value->object)
      value->object_ref = pdf_ref_obj(value->object);
    else {
      WARN("Object @%s not defined or already closed.",
	   printable_key(key, keylen));
    }
  }

  return pdf_link_obj(value->object_ref);
}

pdf_obj *
pdf_names_lookup_object (struct ht_table *names,
			 const void *key, int keylen)
{
  struct obj_data *value;

  ASSERT(names);

  value = ht_lookup_table(names, key, keylen);
  if (!value)
    return NULL;
  else if (!value->object) {
    WARN("Object @%s not defined or already closed.",
	 printable_key(key, keylen));
  }

  return value->object;
}

int
pdf_names_close_object (struct ht_table *names,
			const void *key, int keylen)
{
  struct obj_data *value;

  ASSERT(names);

  value = ht_lookup_table(names, key, keylen);
  if (!value) {
    WARN("Tried to release nonexistent reference: %s",
	 printable_key(key, keylen));
    return -1;
  }

  if (value->object) {
    pdf_release_obj(value->object);
    value->object = NULL;
  } else {
    WARN("Trying to close object @%s twice?",
	 printable_key(key, keylen));
    return -1;
  }

  return 0;
}

struct named_object
{
  char    *key;
  int      keylen;
  pdf_obj *value;
};

static int CDECL
cmp_key (const void *d1, const void *d2)
{
  struct named_object *sd1, *sd2;
  int    keylen, cmp;

  sd1 = (struct named_object *) d1;
  sd2 = (struct named_object *) d2;

  if (!sd1->key)
    cmp = -1;
  else if (!sd2->key)
    cmp =  1;
  else {
    keylen = MIN(sd1->keylen, sd2->keylen);
    cmp    = memcmp(sd1->key, sd2->key, keylen);
    if (!cmp) {
      cmp = sd1->keylen - sd2->keylen;
    }
  }

  return cmp;
}

#define NAME_CLUSTER 4
static pdf_obj *
build_name_tree (struct named_object *first, long num_leaves, int is_root)
{
  pdf_obj *result;
  int      i;

  result = pdf_new_dict();
  /*
   * According to PDF Refrence, Third Edition (p.101-102), a name tree
   * always has exactly one root node, which contains a SINGLE entry:
   * either Kids or Names but not both. If the root node has a Names
   * entry, it is the only node in the tree. If it has a Kids entry,
   * then each of the remaining nodes is either an intermediate node,
   * containing a Limits entry and a Kids entry, or a leaf node,
   * containing a Limits entry and a Names entry.
   */
  if (!is_root) {
    struct named_object *last;
    pdf_obj *limits;

    limits = pdf_new_array();
    last   = &first[num_leaves - 1];
    pdf_add_array(limits, pdf_new_string(first->key, first->keylen));
    pdf_add_array(limits, pdf_new_string(last->key , last->keylen ));
    pdf_add_dict (result, pdf_new_name("Limits"),    limits);
  }

  if (num_leaves > 0 &&
      num_leaves <= 2 * NAME_CLUSTER) {
    pdf_obj *names;

    /* Create leaf nodes. */
    names = pdf_new_array();
    for (i = 0; i < num_leaves; i++) {
      struct named_object *cur;

      cur = &first[i];
      pdf_add_array(names, pdf_new_string(cur->key, cur->keylen));
      switch (PDF_OBJ_TYPEOF(cur->value)) {
      case PDF_ARRAY:
      case PDF_DICT:
      case PDF_STREAM:
      case PDF_STRING:
	pdf_add_array(names, pdf_ref_obj(cur->value));
	break;
      case PDF_OBJ_INVALID:
	ERROR("Invalid object...: %s", printable_key(cur->key, cur->keylen));
	break;
      default:
	pdf_add_array(names, pdf_link_obj(cur->value));
	break;
      }
      pdf_release_obj(cur->value);
      cur->value = NULL;
    }
    pdf_add_dict(result, pdf_new_name("Names"), names);
  } else if (num_leaves > 0) {
    pdf_obj *kids;

    /* Intermediate node */
    kids = pdf_new_array();
    for (i = 0; i < NAME_CLUSTER; i++) {
      pdf_obj *subtree;
      long     start, end;

      start = (i*num_leaves) / NAME_CLUSTER;
      end   = ((i+1)*num_leaves) / NAME_CLUSTER;
      subtree = build_name_tree(&first[start], (end - start), 0);
      pdf_add_array  (kids, pdf_ref_obj(subtree));
      pdf_release_obj(subtree);
    }
    pdf_add_dict(result, pdf_new_name("Kids"), kids);
  }

  return result;
}

static struct named_object *
flat_table (struct ht_table *ht_tab, long *num_entries)
{
  struct named_object *objects;
  struct ht_iter       iter;
  long   count;

  ASSERT(ht_tab);

  *num_entries = count = ht_tab->count;
  objects = NEW(count, struct named_object);
  if (ht_set_iter(ht_tab, &iter) >= 0) {
    do {
      char  *key;
      int    keylen;
      struct obj_data *value;

      count--;
      key   = ht_iter_getkey(&iter, &keylen);
      value = ht_iter_getval(&iter);
      if (value->reserve) {
	WARN("Named object \"%s\" not defined!!!",
	     printable_key(key, keylen));
	WARN("Replacing with null.");
	objects[count].key    = (char *) key;
	objects[count].keylen = keylen;
	objects[count].value  = pdf_new_null();
      } else if (value->object_ref) {
	objects[count].key    = (char *) key;
	objects[count].keylen = keylen;
	objects[count].value  = pdf_link_obj(value->object_ref);
      } else if (value->object) {
	objects[count].key    = (char *) key;
	objects[count].keylen = keylen;
	objects[count].value  = pdf_link_obj(value->object);
      } else {
	WARN("Named object \"%s\" not defined!!!",
	     printable_key(key, keylen));
	WARN("Replacing with null.");
	objects[count].key    = (char *) key;
	objects[count].keylen = keylen;
	objects[count].value  = pdf_new_null();
      }
    } while (ht_iter_next(&iter) >= 0 && count > 0);
    ht_clear_iter(&iter);
  }

  return objects;
}

pdf_obj *
pdf_names_create_tree (struct ht_table *names)
{
  pdf_obj *name_tree;
  struct   named_object *flat;
  long     count;

  flat = flat_table(names, &count);
  if (!flat)
    name_tree = NULL;
  else {
    if (count < 1)
      name_tree = NULL;
    else {
      qsort(flat, count, sizeof(struct named_object), cmp_key);
      name_tree = build_name_tree(flat, count, 1);
    }
    RELEASE(flat);
  }

  return name_tree;
}
