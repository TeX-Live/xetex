/*  $Header: /home/cvsroot/dvipdfmx/src/pdfdoc.c,v 1.55 2008/05/22 10:08:02 matthias Exp $
 
    This is dvipdfmx, an eXtended version of dvipdfm by Mark A. Wicks.

    Copyright (C) 2008 by Jin-Hwan Cho, Matthias Franz, and Shunsaku Hirata,
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
 * TODO: Many things...
 *  {begin,end}_{bead,article}, box stack, name tree (not limited to dests)...
 */
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>

#include "system.h"
#include "mem.h"
#include "error.h"
#include "mfileio.h"

#include "numbers.h"

#include "pdfobj.h"
#include "pdfparse.h"
#include "pdfnames.h"

#include "pdfencrypt.h"

#include "pdfdev.h"
#include "pdfdraw.h"
#include "pdfcolor.h"

#include "pdfresource.h"
#include "pdffont.h"
#include "pdfximage.h"

#include "pdflimits.h"

#if HAVE_LIBPNG
#include "pngimage.h"
#endif
#include "jpegimage.h"

#include "pdfdoc.h"

#define PDFDOC_PAGES_ALLOC_SIZE   128u
#define PDFDOC_ARTICLE_ALLOC_SIZE 16
#define PDFDOC_BEAD_ALLOC_SIZE    16

static int verbose = 0;

static char  manual_thumb_enabled  = 0;
static char *thumb_basename = NULL;

void
pdf_doc_enable_manual_thumbnails (void)
{
#if HAVE_LIBPNG
  manual_thumb_enabled = 1;
#else
  WARN("Manual thumbnail is not supported without the libpng library.");
#endif
}

static pdf_obj *
read_thumbnail (const char *thumb_filename) 
{
  pdf_obj *image_ref;
  int      xobj_id;
  FILE    *fp;

  fp = MFOPEN(thumb_filename, FOPEN_RBIN_MODE);
  if (!fp) {
    WARN("Could not open thumbnail file \"%s\"", thumb_filename);
    return NULL;
  }
  if (!check_for_png(fp) && !check_for_jpeg(fp)) {
    WARN("Thumbnail \"%s\" not a png/jpeg file!", thumb_filename);
    MFCLOSE(fp);
    return NULL;
  }
  MFCLOSE(fp);

  xobj_id = pdf_ximage_findresource(thumb_filename, 0);
  if (xobj_id < 0) {
    WARN("Could not read thumbnail file \"%s\".", thumb_filename);
    image_ref = NULL;
  } else {
    image_ref = pdf_ximage_get_reference(xobj_id);
  }

  return image_ref;
}

void
pdf_doc_set_verbose (void)
{
  verbose++;
  pdf_font_set_verbose();
  pdf_color_set_verbose();
  pdf_ximage_set_verbose();
}

typedef struct pdf_form
{
  char       *ident;

  pdf_tmatrix matrix;
  pdf_rect    cropbox;

  pdf_obj    *resources;
  pdf_obj    *contents;
} pdf_form;

struct form_list_node
{
  int      q_depth;
  pdf_form form;

  struct form_list_node *prev;
};

#define USE_MY_MEDIABOX (1 << 0)
typedef struct pdf_page
{
  pdf_obj  *page_obj;
  pdf_obj  *page_ref;

  int       flags;

  double    ref_x, ref_y;
  pdf_rect  cropbox;

  pdf_obj  *resources;

  /* Contents */
  pdf_obj  *background;
  pdf_obj  *contents;

  /* global bop, background, contents, global eop */
  pdf_obj  *content_refs[4];

  pdf_obj  *annots;
  pdf_obj  *beads;
} pdf_page;

typedef struct pdf_olitem
{
  pdf_obj *dict;

  int      is_open;

  struct pdf_olitem *first;
  struct pdf_olitem *parent;

  struct pdf_olitem *next;
} pdf_olitem;

typedef struct pdf_bead
{
  char    *id;
  long     page_no;
  pdf_rect rect;
} pdf_bead;

typedef struct pdf_article
{
  char     *id;
  pdf_obj  *info;
  long      num_beads;
  long      max_beads;
  pdf_bead *beads;
} pdf_article;

struct name_dict
{
  char  *category;
  struct ht_table *data;
};


typedef struct pdf_doc
{
  struct {
    pdf_obj *dict;

    pdf_obj *viewerpref;
    pdf_obj *pagelabels;
    pdf_obj *pages;
    pdf_obj *names;
    pdf_obj *threads;
  } root;

  pdf_obj *info;

  struct {
    pdf_rect mediabox;
    pdf_obj *bop, *eop;

    long      num_entries; /* This is not actually total number of pages. */
    long      max_entries;
    pdf_page *entries;
  } pages;

  struct {
    pdf_olitem *first;
    pdf_olitem *current;
    int         current_depth;
  } outlines;

  struct {
    long         num_entries;
    long         max_entries;
    pdf_article *entries;
  } articles;

  struct name_dict *names;

  struct {
    int    outline_open_depth;
    double annot_grow;
  } opt;

  struct form_list_node *pending_forms;

} pdf_doc;
static pdf_doc pdoc;

static void
pdf_doc_init_catalog (pdf_doc *p)
{
  p->root.viewerpref = NULL;
  p->root.pagelabels = NULL;
  p->root.pages      = NULL;
  p->root.names      = NULL;
  p->root.threads    = NULL;
  
  p->root.dict = pdf_new_dict();
  pdf_set_root(p->root.dict);

  return;
}

static void
pdf_doc_close_catalog (pdf_doc *p)
{
  pdf_obj *tmp;

  if (p->root.viewerpref) {
    tmp = pdf_lookup_dict(p->root.dict, "ViewerPreferences");
    if (!tmp) {
      pdf_add_dict(p->root.dict,
                   pdf_new_name("ViewerPreferences"),
                   pdf_ref_obj (p->root.viewerpref));
    } else if (PDF_OBJ_DICTTYPE(tmp)) {
      pdf_merge_dict(p->root.viewerpref, tmp);
      pdf_add_dict(p->root.dict,
                   pdf_new_name("ViewerPreferences"),
                   pdf_ref_obj (p->root.viewerpref));
    } else { /* Maybe reference */
      /* What should I do? */
      WARN("Could not modify ViewerPreferences.");
    }
    pdf_release_obj(p->root.viewerpref);
    p->root.viewerpref = NULL;
  }

  if (p->root.pagelabels) {
    tmp = pdf_lookup_dict(p->root.dict, "PageLabels");
    if (!tmp) {
      tmp = pdf_new_dict();
      pdf_add_dict(tmp, pdf_new_name("Nums"),  pdf_link_obj(p->root.pagelabels));
      pdf_add_dict(p->root.dict,
                   pdf_new_name("PageLabels"), pdf_ref_obj(tmp));
      pdf_release_obj(tmp);
    } else { /* Maybe reference */
      /* What should I do? */
      WARN("Could not modify PageLabels.");
    }
    pdf_release_obj(p->root.pagelabels);
    p->root.pagelabels = NULL;
  }

  pdf_add_dict(p->root.dict,
               pdf_new_name("Type"), pdf_new_name("Catalog"));
  pdf_release_obj(p->root.dict);
  p->root.dict = NULL;

  return;
}

/*
 * Pages are starting at 1.
 * The page count does not increase until the page is finished.
 */
#define LASTPAGE(p)  (&(p->pages.entries[p->pages.num_entries]))
#define FIRSTPAGE(p) (&(p->pages.entries[0]))
#define PAGECOUNT(p) (p->pages.num_entries)
#define MAXPAGES(p)  (p->pages.max_entries)

static void
doc_resize_page_entries (pdf_doc *p, long size)
{
  if (size > MAXPAGES(p)) {
    long i;

    p->pages.entries = RENEW(p->pages.entries, size, struct pdf_page);
    for (i = p->pages.max_entries; i < size; i++) {
      p->pages.entries[i].page_obj   = NULL;
      p->pages.entries[i].page_ref   = NULL;
      p->pages.entries[i].flags      = 0;
      p->pages.entries[i].resources  = NULL;
      p->pages.entries[i].background = NULL;
      p->pages.entries[i].contents   = NULL;
      p->pages.entries[i].content_refs[0] = NULL; /* global bop */
      p->pages.entries[i].content_refs[1] = NULL; /* background */
      p->pages.entries[i].content_refs[2] = NULL; /* page body  */
      p->pages.entries[i].content_refs[3] = NULL; /* global eop */
      p->pages.entries[i].annots    = NULL;
      p->pages.entries[i].beads     = NULL;
    }
    p->pages.max_entries = size;
  }

  return;
}

static pdf_page *
doc_get_page_entry (pdf_doc *p, unsigned long page_no)
{
  pdf_page *page;

  if (page_no > 65535ul) {
    ERROR("Page number %ul too large!", page_no);
  } else if (page_no == 0) {
    ERROR("Invalid Page number %ul.", page_no);
  }

  if (page_no > MAXPAGES(p)) {
    doc_resize_page_entries(p, page_no + PDFDOC_PAGES_ALLOC_SIZE);
  }

  page = &(p->pages.entries[page_no - 1]);

  return page;
}

static void pdf_doc_init_page_tree  (pdf_doc *p, double media_width, double media_height);
static void pdf_doc_close_page_tree (pdf_doc *p);

static void pdf_doc_init_names  (pdf_doc *p);
static void pdf_doc_close_names (pdf_doc *p);

static void pdf_doc_init_docinfo  (pdf_doc *p);
static void pdf_doc_close_docinfo (pdf_doc *p);

static void pdf_doc_init_articles    (pdf_doc *p);
static void pdf_doc_close_articles   (pdf_doc *p);
static void pdf_doc_init_bookmarks   (pdf_doc *p, int bm_open_depth);
static void pdf_doc_close_bookmarks  (pdf_doc *p);

void
pdf_doc_set_bop_content (const char *content, unsigned length)
{
  pdf_doc *p = &pdoc;

  ASSERT(p);

  if (p->pages.bop) {
    pdf_release_obj(p->pages.bop);
    p->pages.bop = NULL;
  }

  if (length > 0) {
    p->pages.bop = pdf_new_stream(STREAM_COMPRESS);
    pdf_add_stream(p->pages.bop, content, length);
  } else {
    p->pages.bop = NULL;
  }

  return;
}

void
pdf_doc_set_eop_content (const char *content, unsigned length)
{
  pdf_doc *p = &pdoc;

  if (p->pages.eop) {
    pdf_release_obj(p->pages.eop);
    p->pages.eop = NULL;
  }

  if (length > 0) {
    p->pages.eop = pdf_new_stream(STREAM_COMPRESS);
    pdf_add_stream(p->pages.eop, content, length);
  } else {
    p->pages.eop = NULL;
  }

  return;
}

#ifndef HAVE_TM_GMTOFF
#ifndef HAVE_TIMEZONE

/* auxiliary function to compute timezone offset on
   systems that do not support the tm_gmtoff in struct tm,
   or have a timezone variable.  Such as i386-solaris.  */

static long
compute_timezone_offset()
{
  const time_t now = time(NULL);
  struct tm tm;
  struct tm local;
  time_t gmtoff;

  localtime_r(&now, &local);
  gmtime_r(&now, &tm);
  return (mktime(&local) - mktime(&tm));
}

#endif /* HAVE_TIMEZONE */
#endif /* HAVE_TM_GMTOFF */

/*
 * Docinfo
 */
static long
asn_date (char *date_string)
{
  long        tz_offset;
  time_t      current_time;
  struct tm  *bd_time;

  time(&current_time);
  bd_time = localtime(&current_time);

#ifdef HAVE_TM_GMTOFF
  tz_offset = bd_time->tm_gmtoff;
#else
#  ifdef HAVE_TIMEZONE
  tz_offset = -timezone;
#  else
  tz_offset = compute_timezone_offset();
#  endif /* HAVE_TIMEZONE */
#endif /* HAVE_TM_GMTOFF */

  sprintf(date_string, "D:%04d%02d%02d%02d%02d%02d%c%02ld'%02ld'",
	  bd_time->tm_year + 1900, bd_time->tm_mon + 1, bd_time->tm_mday,
	  bd_time->tm_hour, bd_time->tm_min, bd_time->tm_sec,
	  (tz_offset > 0) ? '+' : '-', labs(tz_offset) / 3600,
                                      (labs(tz_offset) / 60) % 60);

  return strlen(date_string);
}

static void
pdf_doc_init_docinfo (pdf_doc *p)
{
  p->info = pdf_new_dict();
  pdf_set_info(p->info);

  return;
}

static void
pdf_doc_close_docinfo (pdf_doc *p)
{
  pdf_obj *docinfo = p->info;

  /*
   * Excerpt from PDF Reference 4th ed., sec. 10.2.1.
   *
   * Any entry whose value is not known should be omitted from the dictionary,
   * rather than included with an empty string as its value.
   *
   * ....
   *
   * Note: Although viewer applications can store custom metadata in the document
   * information dictionary, it is inappropriate to store private content or
   * structural information there; such information should be stored in the
   * document catalog instead (see Section 3.6.1,  Document Catalog ).
   */
  const char *keys[] = {
    "Title", "Author", "Subject", "Keywords", "Creator", "Producer",
    "CreationDate", "ModDate", /* Date */
    NULL
  };
  pdf_obj *value;
  char    *banner;
  int      i;

  for (i = 0; keys[i] != NULL; i++) {
    value = pdf_lookup_dict(docinfo, keys[i]);
    if (value) {
      if (!PDF_OBJ_STRINGTYPE(value)) {
        WARN("\"%s\" in DocInfo dictionary not string type.", keys[i]);
        pdf_remove_dict(docinfo, keys[i]);
        WARN("\"%s\" removed from DocInfo.", keys[i]);
      } else if (pdf_string_length(value) == 0) {
        /* The hyperref package often uses emtpy strings. */
        pdf_remove_dict(docinfo, keys[i]);
      }
    }
  }

  banner = NEW(strlen(PACKAGE)+strlen(VERSION)+4, char);
  sprintf(banner, "%s (%s)", PACKAGE, VERSION);
  pdf_add_dict(docinfo,
               pdf_new_name("Producer"),
               pdf_new_string(banner, strlen(banner)));
  RELEASE(banner);
  
  if (!pdf_lookup_dict(docinfo, "CreationDate")) {
    char now[32];

    asn_date(now);
    pdf_add_dict(docinfo, 
                 pdf_new_name ("CreationDate"),
                 pdf_new_string(now, strlen(now)));
  }

  pdf_release_obj(docinfo);
  p->info = NULL;

  return;
}

static pdf_obj *
pdf_doc_get_page_resources (pdf_doc *p, const char *category)
{
  pdf_obj  *resources;
  pdf_page *currentpage;
  pdf_obj  *res_dict;

  if (!p || !category) {
    return NULL;
  }

  if (p->pending_forms) {
    if (p->pending_forms->form.resources) {
      res_dict = p->pending_forms->form.resources;
    } else {
      res_dict = p->pending_forms->form.resources = pdf_new_dict();
    }
  } else {
    currentpage = LASTPAGE(p);
    if (currentpage->resources) {
      res_dict = currentpage->resources;
    } else {
      res_dict = currentpage->resources = pdf_new_dict();
    }
  }
  resources = pdf_lookup_dict(res_dict, category);
  if (!resources) {
    resources = pdf_new_dict();
    pdf_add_dict(res_dict, pdf_new_name(category), resources);
  }

  return resources;
}

void
pdf_doc_add_page_resource (const char *category,
                           const char *resource_name, pdf_obj *resource_ref)
{
  pdf_doc *p = &pdoc;
  pdf_obj *resources;
  pdf_obj *duplicate;

  if (!PDF_OBJ_INDIRECTTYPE(resource_ref)) {
    WARN("Passed non indirect reference...");
    resource_ref = pdf_ref_obj(resource_ref); /* leak */
  }
  resources = pdf_doc_get_page_resources(p, category);
  duplicate = pdf_lookup_dict(resources, resource_name);
  if (duplicate && pdf_compare_reference(duplicate, resource_ref)) {
    WARN("Conflicting page resource found (page: %ld, category: %s, name: %s).",
         pdf_doc_current_page_number(), category, resource_name);
    WARN("Ignoring...");
    pdf_release_obj(resource_ref);
  } else {
    pdf_add_dict(resources, pdf_new_name(resource_name), resource_ref);
  }

  return;
}

static void
doc_flush_page (pdf_doc *p, pdf_page *page, pdf_obj *parent_ref)
{
  pdf_obj *contents_array;
  int      count;

  pdf_add_dict(page->page_obj,
               pdf_new_name("Type"), pdf_new_name("Page"));
  pdf_add_dict(page->page_obj,
               pdf_new_name("Parent"), parent_ref);

  /*
   * Clipping area specified by CropBox is affected by MediaBox which
   * might be inherit from parent node. If MediaBox of the root node
   * does not have enough size to cover all page's imaging area, using
   * CropBox here gives incorrect result.
   */
  if (page->flags & USE_MY_MEDIABOX) {
    pdf_obj *mediabox;

    mediabox = pdf_new_array();
    pdf_add_array(mediabox,
                  pdf_new_number(ROUND(page->cropbox.llx, 0.01)));
    pdf_add_array(mediabox,
                  pdf_new_number(ROUND(page->cropbox.lly, 0.01)));
    pdf_add_array(mediabox,
                  pdf_new_number(ROUND(page->cropbox.urx, 0.01)));
    pdf_add_array(mediabox,
                  pdf_new_number(ROUND(page->cropbox.ury, 0.01)));
    pdf_add_dict(page->page_obj, pdf_new_name("MediaBox"),  mediabox);
  }

  count = 0;
  contents_array = pdf_new_array();
  if (page->content_refs[0]) { /* global bop */
    pdf_add_array(contents_array, page->content_refs[0]);
    count++;
  } else if (p->pages.bop &&
             pdf_stream_length(p->pages.bop) > 0) {
    pdf_add_array(contents_array, pdf_ref_obj(p->pages.bop));
    count++;
  }
  if (page->content_refs[1]) { /* background */
    pdf_add_array(contents_array, page->content_refs[1]);
    count++;
  }
  if (page->content_refs[2]) { /* page body */
    pdf_add_array(contents_array, page->content_refs[2]);
    count++;
  }
  if (page->content_refs[3]) { /* global eop */
    pdf_add_array(contents_array, page->content_refs[3]);
    count++;
  } else if (p->pages.eop &&
             pdf_stream_length(p->pages.eop) > 0) {
    pdf_add_array(contents_array, pdf_ref_obj(p->pages.eop));
    count++;
  }

  if (count == 0) {
    WARN("Page with empty content found!!!");
  }
  page->content_refs[0] = NULL;
  page->content_refs[1] = NULL;
  page->content_refs[2] = NULL;
  page->content_refs[3] = NULL;

  pdf_add_dict(page->page_obj,
               pdf_new_name("Contents"), contents_array);


  if (page->annots) {
    pdf_add_dict(page->page_obj,
                 pdf_new_name("Annots"), pdf_ref_obj(page->annots));
    pdf_release_obj(page->annots);
  }
  if (page->beads) {
    pdf_add_dict(page->page_obj,
                 pdf_new_name("B"), pdf_ref_obj(page->beads));
    pdf_release_obj(page->beads);
  }
  pdf_release_obj(page->page_obj);
  pdf_release_obj(page->page_ref);

  page->page_obj = NULL;
  page->page_ref = NULL;
  page->annots   = NULL;
  page->beads    = NULL;

  return;
}

/* B-tree? */
#define PAGE_CLUSTER 4
static pdf_obj *
build_page_tree (pdf_doc  *p,
                 pdf_page *firstpage, long num_pages,
                 pdf_obj  *parent_ref)
{
  pdf_obj *self, *self_ref, *kids;
  long     i;

  self = pdf_new_dict();
  /*
   * This is a slight kludge which allow the subtree dictionary
   * generated by this routine to be merged with the real
   * page_tree dictionary, while keeping the indirect object
   * references right.
   */
  self_ref = parent_ref ? pdf_ref_obj(self) : pdf_ref_obj(p->root.pages);

  pdf_add_dict(self, pdf_new_name("Type"),  pdf_new_name("Pages"));
  pdf_add_dict(self, pdf_new_name("Count"), pdf_new_number((double) num_pages));

  if (parent_ref != NULL)
    pdf_add_dict(self, pdf_new_name("Parent"), parent_ref);

  kids = pdf_new_array();
  if (num_pages > 0 && num_pages <= PAGE_CLUSTER) {
    for (i = 0; i < num_pages; i++) {
      pdf_page *page;

      page = firstpage + i;
      if (!page->page_ref)
        page->page_ref = pdf_ref_obj(page->page_obj);
      pdf_add_array (kids, pdf_link_obj(page->page_ref));
      doc_flush_page(p, page, pdf_link_obj(self_ref));
    }
  } else if (num_pages > 0) {
    for (i = 0; i < PAGE_CLUSTER; i++) {
      long start, end;

      start = (i*num_pages)/PAGE_CLUSTER;
      end   = ((i+1)*num_pages)/PAGE_CLUSTER;
      if (end - start > 1) {
        pdf_obj *subtree;

        subtree = build_page_tree(p, firstpage + start, end - start,
                                  pdf_link_obj(self_ref));
        pdf_add_array(kids, pdf_ref_obj(subtree));
        pdf_release_obj(subtree);
      } else {
        pdf_page *page;

        page = firstpage + start;
        if (!page->page_ref)
          page->page_ref = pdf_ref_obj(page->page_obj);
        pdf_add_array (kids, pdf_link_obj(page->page_ref));
        doc_flush_page(p, page, pdf_link_obj(self_ref));
      }
    }
  }
  pdf_add_dict(self, pdf_new_name("Kids"), kids);
  pdf_release_obj(self_ref);

  return self;
}

static void
pdf_doc_init_page_tree (pdf_doc *p, double media_width, double media_height)
{
  /*
   * Create empty page tree.
   * The docroot.pages is kept open until the document is closed.
   * This allows the user to write to pages if he so choses.
   */
  p->root.pages = pdf_new_dict();

  p->pages.num_entries = 0;
  p->pages.max_entries = 0;
  p->pages.entries     = NULL;

  p->pages.bop = NULL;
  p->pages.eop = NULL;

  p->pages.mediabox.llx = 0.0;
  p->pages.mediabox.lly = 0.0;
  p->pages.mediabox.urx = media_width;
  p->pages.mediabox.ury = media_height;

  return;
}

static void
pdf_doc_close_page_tree (pdf_doc *p)
{
  pdf_obj *page_tree_root;
  pdf_obj *mediabox;
  long     page_no;

  /*
   * Do consistency check on forward references to pages.
   */
  for (page_no = PAGECOUNT(p) + 1; page_no <= MAXPAGES(p); page_no++) {
    pdf_page  *page;

    page = doc_get_page_entry(p, page_no);
    if (page->page_obj) {
      WARN("Nonexistent page #%ld refered.", page_no);
      pdf_release_obj(page->page_ref);
      page->page_ref = NULL;
    }
    if (page->page_obj) {
      WARN("Entry for a nonexistent page #%ld created.", page_no);
      pdf_release_obj(page->page_obj);
      page->page_obj = NULL;
    }
    if (page->annots) {
      WARN("Annotation attached to a nonexistent page #%ld.", page_no);
      pdf_release_obj(page->annots);
      page->annots = NULL;
    }
    if (page->beads) {
      WARN("Article beads attached to a nonexistent page #%ld.", page_no);
      pdf_release_obj(page->beads);
      page->beads = NULL;
    }
    if (page->resources) {
      pdf_release_obj(page->resources);
      page->resources = NULL;
    }
  }

  /*
   * Connect page tree to root node.
   */
  page_tree_root = build_page_tree(p, FIRSTPAGE(p), PAGECOUNT(p), NULL);
  pdf_merge_dict (p->root.pages, page_tree_root);
  pdf_release_obj(page_tree_root);

  /* They must be after build_page_tree() */
  if (p->pages.bop) {
    pdf_add_stream (p->pages.bop, "\n", 1);
    pdf_release_obj(p->pages.bop);
    p->pages.bop = NULL;
  }
  if (p->pages.eop) {
    pdf_add_stream (p->pages.eop, "\n", 1);
    pdf_release_obj(p->pages.eop);
    p->pages.eop = NULL;
  }

  /* Create media box at root node and let the other pages inherit it. */
  mediabox = pdf_new_array();
  pdf_add_array(mediabox, pdf_new_number(ROUND(p->pages.mediabox.llx, 0.01)));
  pdf_add_array(mediabox, pdf_new_number(ROUND(p->pages.mediabox.lly, 0.01)));
  pdf_add_array(mediabox, pdf_new_number(ROUND(p->pages.mediabox.urx, 0.01)));
  pdf_add_array(mediabox, pdf_new_number(ROUND(p->pages.mediabox.ury, 0.01)));
  pdf_add_dict(p->root.pages, pdf_new_name("MediaBox"), mediabox);

  pdf_add_dict(p->root.dict,
               pdf_new_name("Pages"),
               pdf_ref_obj (p->root.pages));
  pdf_release_obj(p->root.pages);
  p->root.pages  = NULL;

  RELEASE(p->pages.entries);
  p->pages.entries     = NULL;
  p->pages.num_entries = 0;
  p->pages.max_entries = 0;

  return;
}


#ifndef BOOKMARKS_OPEN_DEFAULT
#define BOOKMARKS_OPEN_DEFAULT 0
#endif

static int clean_bookmarks (pdf_olitem *item);
static int flush_bookmarks (pdf_olitem *item,
                            pdf_obj *parent_ref,
                            pdf_obj *parent_dict);

static void
pdf_doc_init_bookmarks (pdf_doc *p, int bm_open_depth)
{
  pdf_olitem *item;

#define MAX_OUTLINE_DEPTH 256u
  p->opt.outline_open_depth =
    ((bm_open_depth >= 0) ?
     bm_open_depth : MAX_OUTLINE_DEPTH - bm_open_depth);

  p->outlines.current_depth = 1;

  item = NEW(1, pdf_olitem);
  item->dict    = NULL;
  item->next    = NULL;
  item->first   = NULL;
  item->parent  = NULL;
  item->is_open = 1;

  p->outlines.current = item;
  p->outlines.first   = item;

  return;
}

static int
clean_bookmarks (pdf_olitem *item)
{
  pdf_olitem *next;

  while (item) {
    next = item->next;
    if (item->dict)
      pdf_release_obj(item->dict);
    if (item->first)
      clean_bookmarks(item->first);
    RELEASE(item);
    
    item = next;
  }

  return 0;
}

static int
flush_bookmarks (pdf_olitem *node,
                 pdf_obj *parent_ref, pdf_obj *parent_dict)
{
  int         retval;
  int         count;
  pdf_olitem *item;
  pdf_obj    *this_ref, *prev_ref, *next_ref;

  ASSERT(node->dict);

  this_ref = pdf_ref_obj(node->dict);
  pdf_add_dict(parent_dict,
               pdf_new_name("First"), pdf_link_obj(this_ref));

  retval = 0;
  for (item = node, prev_ref = NULL;
       item && item->dict; item = item->next) {
    if (item->first && item->first->dict) {
      count = flush_bookmarks(item->first, this_ref, item->dict);
      if (item->is_open) {
        pdf_add_dict(item->dict,
                     pdf_new_name("Count"),
                     pdf_new_number(count));
        retval += count;
      } else {
        pdf_add_dict(item->dict,
                     pdf_new_name("Count"),
                     pdf_new_number(-count));
      }
    }
    pdf_add_dict(item->dict,
                 pdf_new_name("Parent"),
                 pdf_link_obj(parent_ref));
    if (prev_ref) {
      pdf_add_dict(item->dict,
                   pdf_new_name("Prev"),
                   prev_ref);
    }
    if (item->next && item->next->dict) {
      next_ref = pdf_ref_obj(item->next->dict);
      pdf_add_dict(item->dict,
                   pdf_new_name("Next"),
                   pdf_link_obj(next_ref));
    } else {
      next_ref = NULL;
    }

    pdf_release_obj(item->dict);
    item->dict = NULL;

    prev_ref = this_ref;
    this_ref = next_ref;
    retval++;    
  }

  pdf_add_dict(parent_dict,
               pdf_new_name("Last"),
               pdf_link_obj(prev_ref));

  pdf_release_obj(prev_ref);
  pdf_release_obj(node->dict);
  node->dict = NULL;

  return retval;
}
  
int
pdf_doc_bookmarks_up (void)
{
  pdf_doc    *p = &pdoc;
  pdf_olitem *parent, *item;

  item = p->outlines.current;
  if (!item || !item->parent) {
    WARN("Can't go up above the bookmark root node!");
    return -1;
  }
  parent = item->parent;
  item   = parent->next;
  if (!parent->next) {
    parent->next  = item = NEW(1, pdf_olitem);
    item->dict    = NULL;
    item->first   = NULL;
    item->next    = NULL;
    item->is_open = 0;
    item->parent  = parent->parent;
  }
  p->outlines.current = item;
  p->outlines.current_depth--;

  return 0;
}

int
pdf_doc_bookmarks_down (void)
{
  pdf_doc    *p = &pdoc;
  pdf_olitem *item, *first;

  item = p->outlines.current;
  if (!item->dict) {
    pdf_obj *tcolor, *action;

    WARN("Empty bookmark node!");
    WARN("You have tried to jump more than 1 level.");

    item->dict = pdf_new_dict();

#define TITLE_STRING "<No Title>"
    pdf_add_dict(item->dict,
                 pdf_new_name("Title"),
                 pdf_new_string(TITLE_STRING, strlen(TITLE_STRING)));

    tcolor = pdf_new_array();
    pdf_add_array(tcolor, pdf_new_number(1.0));
    pdf_add_array(tcolor, pdf_new_number(0.0));
    pdf_add_array(tcolor, pdf_new_number(0.0));
    pdf_add_dict (item->dict,
                  pdf_new_name("C"), pdf_link_obj(tcolor));
    pdf_release_obj(tcolor);

    pdf_add_dict (item->dict,
                  pdf_new_name("F"), pdf_new_number(1.0));

#define JS_CODE "app.alert(\"The author of this document made this bookmark item empty!\", 3, 0)"
    action = pdf_new_dict();
    pdf_add_dict(action,
                 pdf_new_name("S"), pdf_new_name("JavaScript"));
    pdf_add_dict(action, 
                 pdf_new_name("JS"), pdf_new_string(JS_CODE, strlen(JS_CODE)));
    pdf_add_dict(item->dict,
                 pdf_new_name("A"), pdf_link_obj(action));
    pdf_release_obj(action);
  }

  item->first    = first = NEW(1, pdf_olitem);
  first->dict    = NULL;
  first->is_open = 0;
  first->parent  = item;
  first->next    = NULL;
  first->first   = NULL;

  p->outlines.current = first;
  p->outlines.current_depth++;

  return 0;
}

int
pdf_doc_bookmarks_depth (void)
{
  pdf_doc *p = &pdoc;

  return p->outlines.current_depth;
}

void
pdf_doc_bookmarks_add (pdf_obj *dict, int is_open)
{
  pdf_doc    *p = &pdoc;
  pdf_olitem *item, *next;

  ASSERT(p && dict);

  item = p->outlines.current;

  if (!item) {
    item = NEW(1, pdf_olitem);
    item->parent = NULL;
    p->outlines.first = item;
  } else if (item->dict) { /* go to next item */
    item = item->next;
  }

#define BMOPEN(b,p) (((b) < 0) ? (((p)->outlines.current_depth > (p)->opt.outline_open_depth) ? 0 : 1) : (b))

#if 0
  item->dict    = pdf_link_obj(dict);
#endif
  item->dict    = dict; 
  item->first   = NULL;
  item->is_open = BMOPEN(is_open, p);

  item->next    = next = NEW(1, pdf_olitem);
  next->dict    = NULL;
  next->parent  = item->parent;
  next->first   = NULL;
  next->is_open = -1;
  next->next    = NULL;

  p->outlines.current = item;

  return;
}

static void
pdf_doc_close_bookmarks (pdf_doc *p)
{
  pdf_obj     *catalog = p->root.dict;
  pdf_olitem  *item;
  int          count;
  pdf_obj     *bm_root, *bm_root_ref;
  
  item = p->outlines.first;
  if (item->dict) {
    bm_root     = pdf_new_dict();
    bm_root_ref = pdf_ref_obj(bm_root);
    count       = flush_bookmarks(item, bm_root_ref, bm_root);
    pdf_add_dict(bm_root,
                 pdf_new_name("Count"),
                 pdf_new_number(count));
    pdf_add_dict(catalog,
                 pdf_new_name("Outlines"),
                 bm_root_ref);
    pdf_release_obj(bm_root);
  }
  clean_bookmarks(item);

  p->outlines.first   = NULL;
  p->outlines.current = NULL;
  p->outlines.current_depth = 0;

  return;
}


static const char *name_dict_categories[] = {
  "Dests", "AP", "JavaScript", "Pages",
  "Templates", "IDS", "URLS", "EmbeddedFiles",
  "AlternatePresentations", "Renditions"
};
#define NUM_NAME_CATEGORY (sizeof(name_dict_categories)/sizeof(name_dict_categories[0]))

static void
pdf_doc_init_names (pdf_doc *p)
{
  int    i;

  p->root.names   = NULL;
  
  p->names = NEW(NUM_NAME_CATEGORY + 1, struct name_dict);
  for (i = 0; i < NUM_NAME_CATEGORY; i++) {
    p->names[i].category = (char *) name_dict_categories[i];
    p->names[i].data     = NULL;
  }
  p->names[NUM_NAME_CATEGORY].category = NULL;
  p->names[NUM_NAME_CATEGORY].data     = NULL;

  return;
}

int
pdf_doc_add_names (const char *category,
                   const void *key, int keylen, pdf_obj *value)
{
  pdf_doc *p = &pdoc;
  int      i;

  for (i = 0; p->names[i].category != NULL; i++) {
    if (!strcmp(p->names[i].category, category)) {
      break;
    }
  }
  if (p->names[i].category == NULL) {
    WARN("Unknown name dictionary category \"%s\".", category);
    return -1;
  }
  if (!p->names[i].data) {
    p->names[i].data = pdf_new_name_tree();
  }

  return pdf_names_add_object(p->names[i].data, key, keylen, value);
}

static void
pdf_doc_close_names (pdf_doc *p)
{
  pdf_obj  *tmp;
  int       i;

  for (i = 0; p->names[i].category != NULL; i++) {
    if (p->names[i].data) {
      pdf_obj  *name_tree;

      name_tree = pdf_names_create_tree(p->names[i].data);
      if (!p->root.names) {
        p->root.names = pdf_new_dict();
      }
      pdf_add_dict(p->root.names,
                   pdf_new_name(p->names[i].category), pdf_ref_obj(name_tree));
      pdf_release_obj(name_tree);
      pdf_delete_name_tree(&p->names[i].data);
    }
  }

  if (p->root.names) {
    tmp = pdf_lookup_dict(p->root.dict, "Names");
    if (!tmp) {
      pdf_add_dict(p->root.dict,
                   pdf_new_name("Names"),
                   pdf_ref_obj (p->root.names));
    } else if (PDF_OBJ_DICTTYPE(tmp)) {
      pdf_merge_dict(p->root.names, tmp);
      pdf_add_dict(p->root.dict,
                   pdf_new_name("Names"),
                   pdf_ref_obj (p->root.names));
    } else { /* Maybe reference */
      /* What should I do? */
      WARN("Could not modify Names dictionary.");
    }
    pdf_release_obj(p->root.names);
    p->root.names = NULL;
  }

  RELEASE(p->names);
  p->names = NULL;

  return;
}


void
pdf_doc_add_annot (unsigned page_no, const pdf_rect *rect, pdf_obj *annot_dict)
{
  pdf_doc  *p = &pdoc;
  pdf_page *page;
  pdf_obj  *rect_array;
  double    annot_grow = p->opt.annot_grow;

  page = doc_get_page_entry(p, page_no);
  if (!page->annots)
    page->annots = pdf_new_array();

#if 1
  {
    pdf_rect  mediabox;

    pdf_doc_get_mediabox(page_no, &mediabox);
    if (rect->llx < mediabox.llx ||
        rect->urx > mediabox.urx ||
       rect->lly < mediabox.lly ||
       rect->ury > mediabox.ury) {
      WARN("Annotation out of page boundary.");
      WARN("Current page's MediaBox: [%g %g %g %g]",
           mediabox.llx, mediabox.lly, mediabox.urx, mediabox.ury);
      WARN("Annotation: [%g %g %g %g]",
           rect->llx, rect->lly, rect->urx, rect->ury);
      WARN("Maybe incorrect paper size specified.");
    }
    if (rect->llx > rect->urx || rect->lly > rect->ury) {
      WARN("Rectangle with negative width/height: [%g %g %g %g]",
           rect->llx, rect->lly, rect->urx, rect->ury);
    }
  }
#endif

  rect_array = pdf_new_array();
  pdf_add_array(rect_array, pdf_new_number(ROUND(rect->llx - annot_grow, 0.001)));
  pdf_add_array(rect_array, pdf_new_number(ROUND(rect->lly - annot_grow, 0.001)));
  pdf_add_array(rect_array, pdf_new_number(ROUND(rect->urx + annot_grow, 0.001)));
  pdf_add_array(rect_array, pdf_new_number(ROUND(rect->ury + annot_grow, 0.001)));
  pdf_add_dict (annot_dict, pdf_new_name("Rect"), rect_array);

  pdf_add_array(page->annots, pdf_ref_obj(annot_dict));

  return;
}


/*
 * PDF Article Thread
 */
static void
pdf_doc_init_articles (pdf_doc *p)
{
  p->root.threads = NULL;

  p->articles.num_entries = 0;
  p->articles.max_entries = 0;
  p->articles.entries     = NULL;

  return;
}

void
pdf_doc_begin_article (const char *article_id, pdf_obj *article_info)
{
  pdf_doc     *p = &pdoc;
  pdf_article *article;

  if (article_id == NULL || strlen(article_id) == 0)
    ERROR("Article thread without internal identifier.");

  if (p->articles.num_entries >= p->articles.max_entries) {
    p->articles.max_entries += PDFDOC_ARTICLE_ALLOC_SIZE;
    p->articles.entries = RENEW(p->articles.entries,
                                p->articles.max_entries, struct pdf_article);
  }
  article = &(p->articles.entries[p->articles.num_entries]);

  article->id = NEW(strlen(article_id)+1, char);
  strcpy(article->id, article_id);
  article->info = article_info;
  article->num_beads = 0;
  article->max_beads = 0;
  article->beads     = NULL;

  p->articles.num_entries++;

  return;
}

void
pdf_doc_end_article (const char *article_id)
{
  return; /* no-op */
}

static pdf_bead *
find_bead (pdf_article *article, const char *bead_id)
{
  pdf_bead *bead;
  long      i;

  bead = NULL;
  for (i = 0; i < article->num_beads; i++) {
    if (!strcmp(article->beads[i].id, bead_id)) {
      bead = &(article->beads[i]);
      break;
    }
  }

  return bead;
}

void
pdf_doc_add_bead (const char *article_id,
                  const char *bead_id, long page_no, const pdf_rect *rect)
{
  pdf_doc     *p = &pdoc;
  pdf_article *article;
  pdf_bead    *bead;
  long         i;

  if (!article_id) {
    ERROR("No article identifier specified.");
  }

  article = NULL;
  for (i = 0; i < p->articles.num_entries; i++) {
    if (!strcmp(p->articles.entries[i].id, article_id)) {
      article = &(p->articles.entries[i]);
      break;
    }
  }
  if (!article) {
    ERROR("Specified article thread that doesn't exist.");
    return;
  }

  bead = bead_id ? find_bead(article, bead_id) : NULL;
  if (!bead) {
    if (article->num_beads >= article->max_beads) {
      article->max_beads += PDFDOC_BEAD_ALLOC_SIZE;
      article->beads = RENEW(article->beads,
                             article->max_beads, struct pdf_bead);
      for (i = article->num_beads; i < article->max_beads; i++) {
        article->beads[i].id = NULL;
        article->beads[i].page_no = -1;
      }
    }
    bead = &(article->beads[article->num_beads]);
    if (bead_id) {
      bead->id = NEW(strlen(bead_id)+1, char);
      strcpy(bead->id, bead_id);
    } else {
      bead->id = NULL;
    }
    article->num_beads++;
  }
  bead->rect.llx = rect->llx;
  bead->rect.lly = rect->lly;
  bead->rect.urx = rect->urx;
  bead->rect.ury = rect->ury;
  bead->page_no  = page_no;

  return;
}

static pdf_obj *
make_article (pdf_doc *p,
              pdf_article *article,
              const char **bead_ids, int num_beads,
              pdf_obj *article_info)
{
  pdf_obj *art_dict;
  pdf_obj *first, *prev, *last;
  long     i, n;

  if (!article)
    return NULL;

  art_dict = pdf_new_dict();
  first = prev = last = NULL;
  /*
   * The bead_ids represents logical order of beads in an article thread.
   * If bead_ids is not given, we create an article thread in the order of
   * beads appeared.
   */
  n = bead_ids ? num_beads : article->num_beads;
  for (i = 0; i < n; i++) {
    pdf_bead *bead;

    bead = bead_ids ? find_bead(article, bead_ids[i]) : &(article->beads[i]);
    if (!bead || bead->page_no < 0) {
      continue;
    }
    last = pdf_new_dict();
    if (prev == NULL) {
      first = last;
      pdf_add_dict(first,
                   pdf_new_name("T"), pdf_ref_obj(art_dict));
    } else {
      pdf_add_dict(prev,
                   pdf_new_name("N"), pdf_ref_obj(last));
      pdf_add_dict(last,
                   pdf_new_name("V"), pdf_ref_obj(prev));
      /* We must link first to last. */
      if (prev != first)
        pdf_release_obj(prev);
    }

    /* Realize bead now. */
    {
      pdf_page *page;
      pdf_obj  *rect;

      page = doc_get_page_entry(p, bead->page_no);
      if (!page->beads) {
        page->beads = pdf_new_array();
      }
      pdf_add_dict(last, pdf_new_name("P"), pdf_link_obj(page->page_ref));
      rect = pdf_new_array();
      pdf_add_array(rect, pdf_new_number(ROUND(bead->rect.llx, 0.01)));
      pdf_add_array(rect, pdf_new_number(ROUND(bead->rect.lly, 0.01)));
      pdf_add_array(rect, pdf_new_number(ROUND(bead->rect.urx, 0.01)));
      pdf_add_array(rect, pdf_new_number(ROUND(bead->rect.ury, 0.01)));
      pdf_add_dict (last, pdf_new_name("R"), rect);
      pdf_add_array(page->beads, pdf_ref_obj(last));
    }

    prev = last;
  }

  if (first && last) {
    pdf_add_dict(last,
                 pdf_new_name("N"), pdf_ref_obj(first));
    pdf_add_dict(first,
                 pdf_new_name("V"), pdf_ref_obj(last));
    if (first != last) {
      pdf_release_obj(last);
    }
    pdf_add_dict(art_dict,
                 pdf_new_name("F"), pdf_ref_obj(first));
    /* If article_info is supplied, we override article->info. */
    if (article_info) {
      pdf_add_dict(art_dict,
                   pdf_new_name("I"), article_info);
    } else if (article->info) {
      pdf_add_dict(art_dict,
                   pdf_new_name("I"), pdf_ref_obj(article->info));
      pdf_release_obj(article->info);
      article->info = NULL; /* We do not write as object reference. */
    }
    pdf_release_obj(first);
  } else {
    pdf_release_obj(art_dict);
    art_dict = NULL;
  }

  return art_dict;
}

static void
clean_article (pdf_article *article)
{
  if (!article)
    return;
    
  if (article->beads) {
    long  i;

    for (i = 0; i < article->num_beads; i++) {
      if (article->beads[i].id)
        RELEASE(article->beads[i].id);
    }
    RELEASE(article->beads);
    article->beads = NULL;
  }
    
  if (article->id)
    RELEASE(article->id);
  article->id = NULL;
  article->num_beads = 0;
  article->max_beads = 0;

  return;
}

static void
pdf_doc_close_articles (pdf_doc *p)
{
  int  i;

  for (i = 0; i < p->articles.num_entries; i++) {
    pdf_article *article;

    article = &(p->articles.entries[i]);
    if (article->beads) {
      pdf_obj *art_dict;

      art_dict = make_article(p, article, NULL, 0, NULL);
      if (!p->root.threads) {
        p->root.threads = pdf_new_array();
      }
      pdf_add_array(p->root.threads, pdf_ref_obj(art_dict));
      pdf_release_obj(art_dict);
    }
    clean_article(article);
  }
  RELEASE(p->articles.entries);
  p->articles.entries = NULL;
  p->articles.num_entries = 0;
  p->articles.max_entries = 0;

  if (p->root.threads) {
    pdf_add_dict(p->root.dict,
                 pdf_new_name("Threads"),
                 pdf_ref_obj (p->root.threads));
    pdf_release_obj(p->root.threads);
    p->root.threads = NULL;
  }

  return;
}

/* page_no = 0 for root page tree node. */
void
pdf_doc_set_mediabox (unsigned page_no, const pdf_rect *mediabox)
{
  pdf_doc  *p = &pdoc;
  pdf_page *page;

  if (page_no == 0) {
    p->pages.mediabox.llx = mediabox->llx;
    p->pages.mediabox.lly = mediabox->lly;
    p->pages.mediabox.urx = mediabox->urx;
    p->pages.mediabox.ury = mediabox->ury;
  } else {
    page = doc_get_page_entry(p, page_no);
    page->cropbox.llx = mediabox->llx;
    page->cropbox.lly = mediabox->lly;
    page->cropbox.urx = mediabox->urx;
    page->cropbox.ury = mediabox->ury;
    page->flags |= USE_MY_MEDIABOX;
  }

  return;
}

void
pdf_doc_get_mediabox (unsigned page_no, pdf_rect *mediabox)
{
  pdf_doc  *p = &pdoc;
  pdf_page *page;

  if (page_no == 0) {
    mediabox->llx = p->pages.mediabox.llx;
    mediabox->lly = p->pages.mediabox.lly;
    mediabox->urx = p->pages.mediabox.urx;
    mediabox->ury = p->pages.mediabox.ury;
  } else {
    page = doc_get_page_entry(p, page_no);
    if (page->flags & USE_MY_MEDIABOX) {
      mediabox->llx = page->cropbox.llx;
      mediabox->lly = page->cropbox.lly;
      mediabox->urx = page->cropbox.urx;
      mediabox->ury = page->cropbox.ury;
    } else {
      mediabox->llx = p->pages.mediabox.llx;
      mediabox->lly = p->pages.mediabox.lly;
      mediabox->urx = p->pages.mediabox.urx;
      mediabox->ury = p->pages.mediabox.ury;
    }
  }

  return;
}

pdf_obj *
pdf_doc_current_page_resources (void)
{
  pdf_obj  *resources;
  pdf_doc  *p = &pdoc;
  pdf_page *currentpage;

  if (p->pending_forms) {
    if (p->pending_forms->form.resources) {
      resources = p->pending_forms->form.resources;
    } else {
      resources = p->pending_forms->form.resources = pdf_new_dict();
    }
  } else {
    currentpage = LASTPAGE(p);
    if (currentpage->resources) {
      resources = currentpage->resources;
    } else {
      resources = currentpage->resources = pdf_new_dict();
    }
  }

  return resources;
}

pdf_obj *
pdf_doc_get_dictionary (const char *category)
{
  pdf_doc *p    = &pdoc;
  pdf_obj *dict = NULL;

  ASSERT(category);

  if (!strcmp(category, "Names")) {
    if (!p->root.names)
      p->root.names = pdf_new_dict();
    dict = p->root.names;
  } else if (!strcmp(category, "Pages")) {
    if (!p->root.pages)
      p->root.pages = pdf_new_dict();
    dict = p->root.pages;
  } else if (!strcmp(category, "Catalog")) {
    if (!p->root.dict)
      p->root.dict = pdf_new_dict();
    dict = p->root.dict;
  } else if (!strcmp(category, "Info")) {
    if (!p->info)
      p->info = pdf_new_dict();
    dict = p->info;
  } else if (!strcmp(category, "@THISPAGE")) {
    /* Sorry for this... */
    pdf_page *currentpage;

    currentpage = LASTPAGE(p);
    dict =  currentpage->page_obj;
  }

  if (!dict) {
    ERROR("Document dict. \"%s\" not exist. ", category);
  }

  return dict;
}

long
pdf_doc_current_page_number (void)
{
  pdf_doc *p = &pdoc;

  return (long) (PAGECOUNT(p) + 1);
}

pdf_obj *
pdf_doc_ref_page (unsigned long page_no)
{
  pdf_doc  *p = &pdoc;
  pdf_page *page;

  page = doc_get_page_entry(p, page_no);
  if (!page->page_obj) {
    page->page_obj = pdf_new_dict();
    page->page_ref = pdf_ref_obj(page->page_obj);
  }

  return pdf_link_obj(page->page_ref);
}

pdf_obj *
pdf_doc_get_reference (const char *category)
{
  pdf_obj *ref = NULL;
  long     page_no;

  ASSERT(category);

  page_no = pdf_doc_current_page_number();
  if (!strcmp(category, "@THISPAGE")) {
    ref = pdf_doc_ref_page(page_no);
  } else if (!strcmp(category, "@PREVPAGE")) {
    if (page_no <= 1) {
      ERROR("Reference to previous page, but no pages have been completed yet.");
    }
    ref = pdf_doc_ref_page(page_no - 1);
  } else if (!strcmp(category, "@NEXTPAGE")) {
    ref = pdf_doc_ref_page(page_no + 1);
  }

  if (!ref) {
    ERROR("Reference to \"%s\" not exist. ", category);
  }

  return ref;
}

static void
pdf_doc_new_page (pdf_doc *p)
{
  pdf_page *currentpage;

  if (PAGECOUNT(p) >= MAXPAGES(p)) {
    doc_resize_page_entries(p, MAXPAGES(p) + PDFDOC_PAGES_ALLOC_SIZE);
  }

  /*
   * This is confusing. pdf_doc_finish_page() have increased page count!
   */
  currentpage = LASTPAGE(p);
  /* Was this page already instantiated by a forward reference to it? */
  if (!currentpage->page_ref) {
    currentpage->page_obj = pdf_new_dict();
    currentpage->page_ref = pdf_ref_obj(currentpage->page_obj);
  }

  currentpage->background = NULL;
  currentpage->contents   = pdf_new_stream(STREAM_COMPRESS);
  currentpage->resources  = pdf_new_dict();

  currentpage->annots = NULL;
  currentpage->beads  = NULL;

  return;
}

/* This only closes contents and resources. */
static void
pdf_doc_finish_page (pdf_doc *p)
{
  pdf_page *currentpage;

  if (p->pending_forms) {
    ERROR("A pending form XObject at the end of page.");
  }

  currentpage = LASTPAGE(p);
  if (!currentpage->page_obj)
    currentpage->page_obj = pdf_new_dict();

  /*
   * Make Contents array.
   */

  /*
   * Global BOP content stream.
   * pdf_ref_obj() returns reference itself when the object is
   * indirect reference, not reference to the indirect reference.
   * We keep bop itself but not reference to it since it is
   * expected to be small.
   */
  if (p->pages.bop &&
      pdf_stream_length(p->pages.bop) > 0) {
    currentpage->content_refs[0] = pdf_ref_obj(p->pages.bop);
  } else {
    currentpage->content_refs[0] = NULL;
  }
  /*
   * Current page background content stream.
   */
  if (currentpage->background) {
    if (pdf_stream_length(currentpage->background) > 0) {
      currentpage->content_refs[1] = pdf_ref_obj(currentpage->background);
      pdf_add_stream (currentpage->background, "\n", 1);
    }
    pdf_release_obj(currentpage->background);
    currentpage->background = NULL;
  } else {
    currentpage->content_refs[1] = NULL;
  }

  /* Content body of current page */
  currentpage->content_refs[2] = pdf_ref_obj(currentpage->contents);
  pdf_add_stream (currentpage->contents, "\n", 1);
  pdf_release_obj(currentpage->contents);
  currentpage->contents = NULL;

  /*
   * Global EOP content stream.
   */
  if (p->pages.eop &&
      pdf_stream_length(p->pages.eop) > 0) {
    currentpage->content_refs[3] = pdf_ref_obj(p->pages.eop);
  } else {
    currentpage->content_refs[3] = NULL;
  }

  /*
   * Page resources.
   */
  if (currentpage->resources) {
    pdf_obj *procset;
    /*
     * ProcSet is obsolete in PDF-1.4 but recommended for compatibility.
     */

    procset = pdf_new_array ();
    pdf_add_array(procset, pdf_new_name("PDF"));
    pdf_add_array(procset, pdf_new_name("Text"));
    pdf_add_array(procset, pdf_new_name("ImageC"));
    pdf_add_array(procset, pdf_new_name("ImageB"));
    pdf_add_array(procset, pdf_new_name("ImageI"));
    pdf_add_dict(currentpage->resources, pdf_new_name("ProcSet"), procset);

    pdf_add_dict(currentpage->page_obj,
                 pdf_new_name("Resources"),
                 pdf_ref_obj(currentpage->resources));
    pdf_release_obj(currentpage->resources);
    currentpage->resources = NULL;
  }

  if (manual_thumb_enabled) {
    char    *thumb_filename;
    pdf_obj *thumb_ref;

    thumb_filename = NEW(strlen(thumb_basename)+7, char);
    sprintf(thumb_filename, "%s.%ld",
            thumb_basename, (p->pages.num_entries % 99999) + 1L);
    thumb_ref = read_thumbnail(thumb_filename);
    RELEASE(thumb_filename);
    if (thumb_ref)
      pdf_add_dict(currentpage->page_obj, pdf_new_name("Thumb"), thumb_ref);
  }

  p->pages.num_entries++;

  return;
}


static pdf_color bgcolor = { 1, { 1.0 } };
void
pdf_doc_set_bgcolor (const pdf_color *color)
{
  if (color)
    memcpy(&bgcolor, color, sizeof(pdf_color));
  else { /* as clear... */
    pdf_color_graycolor(&bgcolor, 1.0);
  }
}

static void
doc_fill_page_background (pdf_doc *p)
{
  pdf_page  *currentpage;
  pdf_rect   r;
  int        cm;
  pdf_obj   *saved_content;

  cm = pdf_dev_get_param(PDF_DEV_PARAM_COLORMODE);
  if (!cm || pdf_color_is_white(&bgcolor)) {
    return;
  }

  pdf_doc_get_mediabox(pdf_doc_current_page_number(), &r);

  currentpage = LASTPAGE(p);
  ASSERT(currentpage);

  if (!currentpage->background)
    currentpage->background = pdf_new_stream(STREAM_COMPRESS);

  saved_content = currentpage->contents;
  currentpage->contents = currentpage->background;

  pdf_dev_gsave();
  //pdf_color_push();
  pdf_dev_set_nonstrokingcolor(&bgcolor);
  pdf_dev_rectfill(r.llx, r.lly, r.urx - r.llx, r.ury - r.lly);
  //pdf_color_pop();
  pdf_dev_grestore();

  currentpage->contents = saved_content;

  return;
}

void
pdf_doc_begin_page (double scale, double x_origin, double y_origin)
{
  pdf_doc     *p = &pdoc;
  pdf_tmatrix  M;

  M.a = scale; M.b = 0.0;
  M.c = 0.0  ; M.d = scale;
  M.e = x_origin;
  M.f = y_origin;

  /* pdf_doc_new_page() allocates page content stream. */
  pdf_doc_new_page(p);
  pdf_dev_bop(&M);

  return;
}

void
pdf_doc_end_page (void)
{
  pdf_doc *p = &pdoc;

  pdf_dev_eop();
  doc_fill_page_background(p);

  pdf_doc_finish_page(p);

  return;
}

void
pdf_doc_add_page_content (const char *buffer, unsigned length)
{
  pdf_doc  *p = &pdoc;
  pdf_page *currentpage;

  if (p->pending_forms) {
    pdf_add_stream(p->pending_forms->form.contents, buffer, length);
  } else {
    currentpage = LASTPAGE(p);
    pdf_add_stream(currentpage->contents, buffer, length);
  }

  return;
}

static char *doccreator = NULL; /* Ugh */

void
pdf_open_document (const char *filename,
		   int do_encryption,
                   double media_width, double media_height,
                   double annot_grow_amount, int bookmark_open_depth)
{
  pdf_doc *p = &pdoc;

  pdf_out_init(filename, do_encryption);

  pdf_doc_init_catalog(p);

  p->opt.annot_grow = annot_grow_amount;
  p->opt.outline_open_depth = bookmark_open_depth;

  pdf_init_resources();
  pdf_init_colors();
  pdf_init_fonts();
  /* Thumbnail want this to be initialized... */
  pdf_init_images();

  pdf_doc_init_docinfo(p);
  if (doccreator) {
    pdf_add_dict(p->info,
                 pdf_new_name("Creator"),
                 pdf_new_string(doccreator, strlen(doccreator)));
    RELEASE(doccreator); doccreator = NULL;
  }

  pdf_doc_init_bookmarks(p, bookmark_open_depth);
  pdf_doc_init_articles (p);
  pdf_doc_init_names    (p);
  pdf_doc_init_page_tree(p, media_width, media_height);

  if (do_encryption) {
    pdf_obj *encrypt = pdf_encrypt_obj();
    pdf_set_encrypt(encrypt, pdf_enc_id_array());
    pdf_release_obj(encrypt);
  }

  /* Create a default name for thumbnail image files */
  if (manual_thumb_enabled) {
    if (strlen(filename) > 4 &&
        !strncmp(".pdf", filename + strlen(filename) - 4, 4)) {
      thumb_basename = NEW(strlen(filename)-4+1, char);
      strncpy(thumb_basename, filename, strlen(filename)-4);
      thumb_basename[strlen(filename)-4] = 0;
    } else {
      thumb_basename = NEW(strlen(filename)+1, char);
      strcpy(thumb_basename, filename);
    }
  }

  p->pending_forms = NULL;
   
  return;
}

void
pdf_doc_set_creator (const char *creator)
{
  if (!creator ||
      creator[0] == '\0')
    return;

  doccreator = NEW(strlen(creator)+1, char);
  strcpy(doccreator, creator); /* Ugh */
}


void
pdf_close_document (void)
{
  pdf_doc *p = &pdoc;

  /*
   * Following things were kept around so user can add dictionary items.
   */
  pdf_doc_close_articles (p);
  pdf_doc_close_names    (p);
  pdf_doc_close_bookmarks(p);
  pdf_doc_close_page_tree(p);
  pdf_doc_close_docinfo  (p);

  pdf_doc_close_catalog  (p);

  pdf_close_images();
  pdf_close_fonts ();
  pdf_close_colors();

  pdf_close_resources(); /* Should be at last. */

  pdf_out_flush();

  if (thumb_basename)
    RELEASE(thumb_basename);

  return;
}

/*
 * All this routine does is give the form a name and add a unity scaling matrix.
 * It fills in required fields.  The caller must initialize the stream.
 */
void
pdf_doc_make_xform (pdf_obj     *xform,
                    pdf_rect    *bbox,
                    pdf_tmatrix *matrix,
                    pdf_obj     *resources)
{
  pdf_obj *xform_dict;
  pdf_obj *tmp;

  xform_dict = pdf_stream_dict(xform);
  pdf_add_dict(xform_dict,
               pdf_new_name("Type"),     pdf_new_name("XObject"));
  pdf_add_dict(xform_dict,
               pdf_new_name("Subtype"),  pdf_new_name("Form"));
  pdf_add_dict(xform_dict,
               pdf_new_name("FormType"), pdf_new_number(1.0));

  if (!bbox)
    ERROR("No BoundingBox supplied.");

  tmp = pdf_new_array();
  pdf_add_array(tmp, pdf_new_number(ROUND(bbox->llx, .001)));
  pdf_add_array(tmp, pdf_new_number(ROUND(bbox->lly, .001)));
  pdf_add_array(tmp, pdf_new_number(ROUND(bbox->urx, .001)));
  pdf_add_array(tmp, pdf_new_number(ROUND(bbox->ury, .001)));
  pdf_add_dict(xform_dict, pdf_new_name("BBox"), tmp);

  if (matrix) {
    tmp = pdf_new_array();
    pdf_add_array(tmp, pdf_new_number(ROUND(matrix->a, .00001)));
    pdf_add_array(tmp, pdf_new_number(ROUND(matrix->b, .00001)));
    pdf_add_array(tmp, pdf_new_number(ROUND(matrix->c, .00001)));
    pdf_add_array(tmp, pdf_new_number(ROUND(matrix->d, .00001)));
    pdf_add_array(tmp, pdf_new_number(ROUND(matrix->e, .001  )));
    pdf_add_array(tmp, pdf_new_number(ROUND(matrix->f, .001  )));
    pdf_add_dict(xform_dict, pdf_new_name("Matrix"), tmp);
  }

  pdf_add_dict(xform_dict, pdf_new_name("Resources"), resources);

  return;
}

/*
 * begin_form_xobj creates an xobject with its "origin" at
 * xpos and ypos that is clipped to the specified bbox. Note
 * that the origin is not the lower left corner of the bbox.
 */
int
pdf_doc_begin_grabbing (const char *ident,
                        double ref_x, double ref_y, const pdf_rect *cropbox)
{
  int         xobj_id = -1;
  pdf_doc    *p = &pdoc;
  pdf_form   *form;
  struct form_list_node *fnode;
  xform_info  info;

  fnode = NEW(1, struct form_list_node);

  fnode->prev    = p->pending_forms;
  fnode->q_depth = pdf_dev_current_depth();
  form           = &fnode->form;

  /*
  * The reference point of an Xobject is at the lower left corner
  * of the bounding box.  Since we would like to have an arbitrary
  * reference point, we use a transformation matrix, translating
  * the reference point to (0,0).
  */

  form->matrix.a = 1.0; form->matrix.b = 0.0;
  form->matrix.c = 0.0; form->matrix.d = 1.0;
  form->matrix.e = -ref_x;
  form->matrix.f = -ref_y;

  form->cropbox.llx = ref_x + cropbox->llx;
  form->cropbox.lly = ref_y + cropbox->lly;
  form->cropbox.urx = ref_x + cropbox->urx;
  form->cropbox.ury = ref_y + cropbox->ury;

  form->contents  = pdf_new_stream(STREAM_COMPRESS);
  form->resources = pdf_new_dict();

  pdf_ximage_init_form_info(&info);

  info.matrix.a = 1.0; info.matrix.b = 0.0;
  info.matrix.c = 0.0; info.matrix.d = 1.0;
  info.matrix.e = -ref_x;
  info.matrix.f = -ref_y;

  info.bbox.llx = cropbox->llx;
  info.bbox.lly = cropbox->lly;
  info.bbox.urx = cropbox->urx;
  info.bbox.ury = cropbox->ury;

  /* Use reference since content itself isn't available yet. */
  xobj_id = pdf_ximage_defineresource(ident,
                                      PDF_XOBJECT_TYPE_FORM,
                                      &info, pdf_ref_obj(form->contents));

  p->pending_forms = fnode;

  /*
   * Make sure the object is self-contained by adding the
   * current font and color to the object stream.
   */
  pdf_dev_reset_fonts();
  pdf_dev_reset_color();

  return xobj_id;
}

void
pdf_doc_end_grabbing (void)
{
  pdf_form *form;
  pdf_obj  *procset;
  pdf_doc  *p = &pdoc;
  struct form_list_node *fnode;

  if (!p->pending_forms) {
    WARN("Tried to close a nonexistent form XOject.");
    return;
  }
  
  fnode = p->pending_forms;
  form  = &fnode->form;

  pdf_dev_grestore_to(fnode->q_depth);

  /*
   * ProcSet is obsolete in PDF-1.4 but recommended for compatibility.
   */
  procset = pdf_new_array();
  pdf_add_array(procset, pdf_new_name("PDF"));
  pdf_add_array(procset, pdf_new_name("Text"));
  pdf_add_array(procset, pdf_new_name("ImageC"));
  pdf_add_array(procset, pdf_new_name("ImageB"));
  pdf_add_array(procset, pdf_new_name("ImageI"));
  pdf_add_dict (form->resources, pdf_new_name("ProcSet"), procset);

  pdf_doc_make_xform(form->contents,
                     &form->cropbox, &form->matrix,
                     pdf_ref_obj(form->resources));
  pdf_release_obj(form->resources);
  pdf_release_obj(form->contents);

  p->pending_forms = fnode->prev;

  /* Here we do not need pdf_dev_reset_color(). */
  pdf_dev_reset_fonts();

  RELEASE(fnode);

  return;
}

static struct
{
  int      dirty;
  pdf_obj *annot_dict;
  pdf_rect rect;
} breaking_state = {0, NULL, {0.0, 0.0, 0.0, 0.0}};

static void
reset_box (void)
{
  breaking_state.rect.llx = 10000.0; /* large value */
  breaking_state.rect.lly = 10000.0; /* large value */
  breaking_state.rect.urx = 0.0;     /* small value */
  breaking_state.rect.ury = 0.0;     /* small value */
  breaking_state.dirty    = 0;
}

void
pdf_doc_begin_annot (pdf_obj *dict)
{
  breaking_state.annot_dict = dict;
  reset_box();
}

void
pdf_doc_end_annot (void)
{
  pdf_doc_break_annot();
  breaking_state.annot_dict = NULL;
}

void
pdf_doc_break_annot (void)
{
  if (breaking_state.dirty) {
    pdf_obj  *annot_dict;

    /* Copy dict */
    annot_dict = pdf_new_dict();
    pdf_merge_dict(annot_dict, breaking_state.annot_dict);
    pdf_doc_add_annot(pdf_doc_current_page_number(),
                      &(breaking_state.rect), annot_dict);
    pdf_release_obj(annot_dict);
  }
  reset_box();
}

void
pdf_doc_expand_box (const pdf_rect *rect)
{
  breaking_state.rect.llx = MIN(breaking_state.rect.llx, rect->llx);
  breaking_state.rect.lly = MIN(breaking_state.rect.lly, rect->lly);
  breaking_state.rect.urx = MAX(breaking_state.rect.urx, rect->urx);
  breaking_state.rect.ury = MAX(breaking_state.rect.ury, rect->ury);
  breaking_state.dirty    = 1;
}

/* This should be number tree */
void
pdf_doc_set_pagelabel (long  pg_start,
                       const char *type,
                       const void *prefix, int prfx_len, long start)
{
  pdf_doc *p = &pdoc;
  pdf_obj *label_dict;

  if (!p->root.pagelabels)
    p->root.pagelabels = pdf_new_array();

  label_dict = pdf_new_dict();
  if (!type || type[0] == '\0') /* Set back to default. */
    pdf_add_dict(label_dict, pdf_new_name("S"),  pdf_new_name("D"));
  else {
    if (type)
      pdf_add_dict(label_dict, pdf_new_name("S"), pdf_new_name(type));
    if (prefix && prfx_len > 0)
      pdf_add_dict(label_dict,
                   pdf_new_name("P"),
                   pdf_new_string(prefix, prfx_len));
    if (start != 1)
      pdf_add_dict(label_dict,
                   pdf_new_name("St"), pdf_new_number(start));
  }

  pdf_add_array(p->root.pagelabels, pdf_new_number(pg_start));
  pdf_add_array(p->root.pagelabels, label_dict);

  return;
}
