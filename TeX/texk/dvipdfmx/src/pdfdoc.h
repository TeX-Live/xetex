/*  $Header: /home/cvsroot/dvipdfmx/src/pdfdoc.h,v 1.7 2004/01/30 18:34:22 hirata Exp $

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

#ifndef _PDFDOC_H_
#define _PDFDOC_H_

#include "pdfobj.h"

extern void pdf_doc_new_page (void);
extern void pdf_doc_finish_page (void);
extern pdf_obj *pdf_doc_this_page_ref (void);
extern pdf_obj *pdf_doc_this_page (void);
extern pdf_obj *pdf_doc_page_tree (void);
extern pdf_obj *pdf_doc_catalog (void);
extern void pdf_doc_creator (const char *s);
extern pdf_obj *pdf_doc_names (void);
extern pdf_obj *pdf_doc_next_page_ref (void);
extern pdf_obj *pdf_doc_prev_page_ref (void);
extern pdf_obj *pdf_doc_ref_page (unsigned long page_no);

extern void pdf_doc_add_to_page_resources (const char *name, pdf_obj *resources);
extern void pdf_doc_add_to_page_fonts (const char *name, pdf_obj *font);
extern void pdf_doc_add_to_page_xobjects (const char *name, pdf_obj *xobject);
extern pdf_obj *pdf_doc_current_page_resources(void);

extern void pdf_doc_add_to_page_annots (pdf_obj *annot);

extern void pdf_doc_add_dest (const char *name, unsigned length, pdf_obj *array);

extern void pdf_doc_start_article (const char *name, pdf_obj *info);
extern void pdf_doc_add_bead (const char *name, pdf_obj *partial_dict);

extern void pdf_doc_merge_with_docinfo (pdf_obj *dictionary);
extern void pdf_doc_merge_with_catalog (pdf_obj *dictionary);

extern void pdf_doc_add_to_page (const char *buffer, unsigned length);

extern void pdf_doc_change_outline_depth (int new_depth);
extern void pdf_doc_add_outline (pdf_obj *dict);

extern void pdf_doc_init (const char *filename);
extern void pdf_doc_close (void);

extern void pdf_doc_comment (const char *comment);
     
extern void pdf_doc_set_verbose(void);

extern void pdf_doc_bop (const char *string, unsigned length);
extern void pdf_doc_eop (const char *string, unsigned length);
extern void pdf_doc_set_origin (double x, double y);
extern void pdf_doc_this_bop (const char *string, unsigned length);
extern void pdf_doc_this_eop (const char *string, unsigned length);

extern void doc_make_form_xobj (pdf_obj *stream, pdf_obj *bbox,
				double refptx, double refpty,
				double xscale, double yscale,
				pdf_obj *resources, char *form_name);
extern pdf_obj *begin_form_xobj (double xpos, double ypos,
				 double bbllx, double bblly,
				 double bburx, double bbury, char *form_name);
extern void end_form_xobj (pdf_obj *resources);
extern void pdf_doc_enable_thumbnails (void);
extern void pdf_doc_begin_annot (pdf_obj *dict);
extern void pdf_doc_set_box (void);
extern void pdf_doc_end_annot (void);
extern void pdf_doc_flush_annot (void);
extern void pdf_doc_expand_box (double llx, double lly, double urx,
				double ury);

#endif /* _PDFDOC_H_ */
