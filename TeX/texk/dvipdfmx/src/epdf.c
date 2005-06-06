/*  $Header: /home/cvsroot/dvipdfmx/src/epdf.c,v 1.6 2003/12/02 09:55:54 hirata Exp $

    This is dvipdfm, a DVI to PDF translator.
    Copyright (C) 1998, 1999 by Mark A. Wicks

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    
    The author may be contacted via the e-mail address

	mwicks@kettering.edu
*/


/*
 * Concatinating content streams are only supported for streams that only uses
 * single FlateDecode filter, i.e.,
 *
 *   /Filter /FlateDecode or /Filter [/FlateDecode]
 *
 * TrimBox, BleedBox, ArtBox, Rotate ...
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "system.h"
#include "mem.h"
#include "mfileio.h"
#include "error.h"

#if HAVE_ZLIB
#include <zlib.h>
#endif

#include "pdfobj.h"
#include "pdfdoc.h"
#include "pdfspecial.h"
#include "epdf.h"

#if HAVE_ZLIB
static int  add_stream_flate (pdf_obj *dst, char *data, long len);
#endif
static int  concat_stream    (pdf_obj *dst, pdf_obj *src);
static void print_bbox_info  (pdf_obj *rect, const char *type, pdf_obj *crop_box, struct xform_info *p);
static int  rect_equal       (pdf_obj *rect1, pdf_obj *rect2);

/*
 * From PDF Reference 4th ed.
 *
 *  MediaBox rectangle (Required; inheritable)
 *
 *   A rectangle, expressed in default user space units, defining the boundaries of the
 *   physical medium on which the page is intended to be displayed or printed.
 *
 *  CropBox rectangle (Optional; inheritable)
 *
 *    A rectangle, expressed in default user space units, defining the visible region of
 *    default user space. When the page is displayed or printed, its contents are to be
 *    clipped (cropped) to this rectangle and then imposed on the output medium in some
 *    implementation-defined manner.
 *    Default value: the value of MediaBox.
 *
 *  BleedBox rectangle (Optional; PDF 1.3)
 *
 *    A rectangle, expressed in default user space units, defining the region to which the
 *    contents of the page should be clipped when output in a production environment.
 *    Default value: the value of CropBox.
 *
 *  TrimBox rectangle (Optional; PDF 1.3)
 *
 *    A rectangle, expressed in default user space units, defining the intended dimensions
 *    of the finished page after trimming.
 *    Default value: the value of CropBox.
 *
 *  ArtBox rectangle (Optional; PDF 1.3)
 *
 *    A rectangle, expressed in default user space units, defining the extent of the page's
 *    meaningful content (including potential white space) as intended by the page's creator
 *    Default value: the value of CropBox.
 *
 *  Rotate integer (Optional; inheritable)
 *
 *    The number of degrees by which the page should be rotated clockwise when displayed or
 *    printed. The value must be a multiple of 90.
 *    Default value: 0.
 */

static int
rect_equal (pdf_obj *rect1, pdf_obj *rect2)
{
  int i;

  if (!rect1 || !rect2)
    return 0;
  for (i = 0; i < 4; i++) {
    if (pdf_number_value(pdf_get_array(rect1, i)) !=
	pdf_number_value(pdf_get_array(rect2, i)))
      return 0;
  }

  return 1;
}

static void
print_bbox_info (pdf_obj *rect, const char *type, pdf_obj *crop_box, struct xform_info *p)
{
  WARN("\"%s\" different from current CropBox found.", type);
  WARN("%s (PDF): [ %g %g %g %g ]", type,
       pdf_number_value(pdf_get_array(rect, 0)),
       pdf_number_value(pdf_get_array(rect, 1)),
       pdf_number_value(pdf_get_array(rect, 2)),
       pdf_number_value(pdf_get_array(rect, 3)));
  WARN("CropBox/MediaBox (PDF)   : [ %g %g %g %g ]",
       pdf_number_value(pdf_get_array(crop_box, 0)),
       pdf_number_value(pdf_get_array(crop_box, 1)),
       pdf_number_value(pdf_get_array(crop_box, 2)),
       pdf_number_value(pdf_get_array(crop_box, 3)));
  WARN("User-specified BoundingBox: [ %g %g %g %g ]",
       p->u_llx, p->u_lly, p->u_urx, p->u_ury);
  WARN("Clip: %s", p->clip ? "true" : "false");
}

pdf_obj *
pdf_include_page (FILE *image_file, struct xform_info *p, char *res_name)
{
  pdf_obj *page_tree;
  pdf_obj *media_box, *resources, *rotate;
  pdf_obj *contents,  *contents_ref;

  /*
   * Get Page Tree.
   */
  page_tree = NULL;
  {
    pdf_obj *trailer, *catalog;

    trailer = pdf_open(image_file);
    if (!trailer)
      ERROR("Corrupt PDF file?");
    if (pdf_lookup_dict(trailer, "Encrypt")) {
      WARN("This PDF document is encrypted.");
      pdf_release_obj(trailer);
      pdf_close();
      return NULL;
    }
    catalog = pdf_deref_obj(pdf_lookup_dict(trailer, "Root"));
    if (!catalog)
      ERROR("Catalog isn't where I expect it.");
    pdf_release_obj(trailer);
    page_tree = pdf_deref_obj(pdf_lookup_dict (catalog, "Pages"));
    pdf_release_obj(catalog);
  }
  if (!page_tree)
    ERROR("Page tree not found.");

  /*
   * Seek first page. Get Media/Crop Box. Collect all Resources.
   * Media box and resources can be inherited.
   */
  media_box = rotate = resources = NULL;
  {
    pdf_obj *kids_ref, *kids;
    pdf_obj *crop_box = NULL;
    pdf_obj *tmp;

    if ((tmp = pdf_lookup_dict(page_tree, "MediaBox")))
      media_box = pdf_deref_obj(tmp);
    if ((tmp = pdf_lookup_dict(page_tree, "CropBox")))
      crop_box = pdf_deref_obj(tmp);
    if ((tmp = pdf_lookup_dict(page_tree, "Rotate")))
      rotate = pdf_deref_obj(tmp);

    if ((tmp = pdf_lookup_dict(page_tree, "Resources")))
      resources = pdf_deref_obj(tmp);

    if (!resources)
      resources = pdf_new_dict();

    while ((kids_ref = pdf_lookup_dict(page_tree, "Kids")) != NULL) {
      kids = pdf_deref_obj(kids_ref);
      pdf_release_obj(page_tree);
      page_tree = pdf_deref_obj(pdf_get_array(kids, 0));
      pdf_release_obj(kids);
      
      if ((tmp = pdf_deref_obj(pdf_lookup_dict(page_tree, "MediaBox")))) {
	if (media_box)
	  pdf_release_obj(media_box);
	media_box = tmp;
      }
      if ((tmp = pdf_deref_obj(pdf_lookup_dict(page_tree, "CropBox")))) {
	if (crop_box)
	  pdf_release_obj(crop_box);
	crop_box = tmp;
      }
      if ((tmp = pdf_deref_obj(pdf_lookup_dict(page_tree, "Rotate")))) {
	if (rotate)
	  pdf_release_obj(rotate);
	rotate = tmp;
      }
      if ((tmp = pdf_deref_obj(pdf_lookup_dict(page_tree, "Resources")))) {
	pdf_merge_dict(tmp, resources);
	pdf_release_obj(resources);
	resources = tmp;
      }
    }
    if (crop_box) {
      pdf_release_obj(media_box);
      media_box = crop_box;
    }
  }
  if (!media_box)
    ERROR("No BoundingBox information available.");

  /*
   * BleedBox, TrimBox, and ArtBox are not inheritable.
   */
  {
    pdf_obj *tmp;

    if ((tmp = pdf_deref_obj(pdf_lookup_dict(page_tree, "TrimBox")))) {
      if (!rect_equal(tmp, media_box)) {
	print_bbox_info(tmp, "TrimBox", media_box, p);
	if (media_box)
	  pdf_release_obj(media_box);
	media_box = tmp;
      }
    }
    if ((tmp = pdf_deref_obj(pdf_lookup_dict(page_tree, "BleedBox")))) {
      if (!rect_equal(tmp, media_box)) {
	print_bbox_info(tmp, "BleedBox", media_box, p);
	if (media_box)
	  pdf_release_obj(media_box);
	media_box = tmp;
      }
    }
    if ((tmp = pdf_deref_obj(pdf_lookup_dict(page_tree, "ArtBox")))) {
      if (!rect_equal(tmp, media_box)) {
	print_bbox_info(tmp, "ArtBox", media_box, p);
	if (media_box)
	  pdf_release_obj(media_box);
	media_box = tmp;
      }
    }
    if (rotate && pdf_number_value(rotate) != 0.0)
      WARN("<< /Rotate %d >> found. (Not supported yet)",  (int)pdf_number_value(rotate));

    /*
     * This gets bit confusing.  In the following code, media_box is the box the
     * image is cropped to. The bounding box is the box the image is scaled to.
     *
     * If user did not supply bounding box, use media_box (which may really be
     * cropbox) as bounding box. Set the crop box parameters in the xform_info
     * structure.
     */
    p->c_llx = pdf_number_value(pdf_get_array(media_box, 0));
    p->c_lly = pdf_number_value(pdf_get_array(media_box, 1));
    p->c_urx = pdf_number_value(pdf_get_array(media_box, 2));
    p->c_ury = pdf_number_value(pdf_get_array(media_box, 3));
    /*
     * Adjust scaling and clipping information as necessary.
     */
    pdf_scale_image(p);
    pdf_release_obj(media_box);
    media_box = pdf_new_array();
    pdf_add_array(media_box, pdf_new_number(p->c_llx));
    pdf_add_array(media_box, pdf_new_number(p->c_lly));
    pdf_add_array(media_box, pdf_new_number(p->c_urx));
    pdf_add_array(media_box, pdf_new_number(p->c_ury));
  }

  /*
   * Handle page content stream.
   * page_tree is now set to the first page.
   */
  contents_ref = contents = NULL;
  {
    contents = pdf_deref_obj(pdf_lookup_dict(page_tree, "Contents"));
    if (!contents)
      ERROR("Could not find any valid page content.");
    /*
     * Concatinate all content streams.
     */
    if (contents->type == PDF_ARRAY) {
      pdf_obj *content_seg, *content_new;
      int      idx = 0;
      content_new = pdf_new_stream(STREAM_COMPRESS);
      for (;;) {
	content_seg = pdf_deref_obj(pdf_get_array(contents, idx));
	if (!content_seg)
	  break;
	else if (content_seg->type == PDF_NULL) {
	  /* Silently ignore. */
	}  else if (content_seg->type != PDF_STREAM) {
	  WARN("Broken PDF file?");
	  pdf_release_obj(media_box);
	  pdf_release_obj(content_new);
	  pdf_close();
	  return NULL;
	} else if (concat_stream(content_new, content_seg) < 0) {
	  WARN("Could not handle content stream with multiple segment.");
	  pdf_release_obj(media_box);
	  pdf_release_obj(content_new);
	  pdf_close();
	  return NULL;
	}
	pdf_release_obj(content_seg);
	idx++;
      }
      pdf_release_obj(contents);
      contents = content_new;
    }
    if (!contents) {
      pdf_release_obj(media_box);
      pdf_close();
      return NULL;
    }
  }

  doc_make_form_xobj(contents, media_box,
		     p->user_bbox ? p->u_llx: 0.0,
		     p->user_bbox ? p->u_lly: 0.0,
		     1.0, 1.0,
		     resources, res_name);
  contents_ref = pdf_ref_obj(contents);
  pdf_release_obj(contents);
  pdf_close();

  return contents_ref;
}

#define WBUF_SIZE 4096
#if HAVE_ZLIB
static int
add_stream_flate (pdf_obj *dst, char *data, long len)
{
  z_stream z;
  char     wbuf[WBUF_SIZE];

  z.zalloc = Z_NULL; z.zfree = Z_NULL; z.opaque = Z_NULL;

  z.next_in  = data; z.avail_in  = len;
  z.next_out = wbuf; z.avail_out = WBUF_SIZE;

  if (inflateInit(&z) != Z_OK) {
    WARN("inflateInit() failed.");
    return -1;
  }

  for (;;) {
    int status;
    status = inflate(&z, Z_NO_FLUSH);
    if (status == Z_STREAM_END)
      break;
    else if (status != Z_OK) {
      WARN("inflate() failed. Broken PDF file?");
      inflateEnd(&z);
      return -1;
    }

    if (z.avail_out == 0) {
      pdf_add_stream(dst, wbuf, WBUF_SIZE);
      z.next_out  = wbuf;
      z.avail_out = WBUF_SIZE;
    }
  }

  if (WBUF_SIZE - z.avail_out > 0)
    pdf_add_stream(dst, wbuf, WBUF_SIZE - z.avail_out);

  return (inflateEnd(&z) == Z_OK ? 0 : -1);
}
#endif

static int
concat_stream (pdf_obj *dst, pdf_obj *src)
{
  pdf_stream *stream;
  pdf_obj    *dict, *filter;

  if (!dst || !src ||
      dst->type != PDF_STREAM || src->type != PDF_STREAM)
    ERROR("Invalid type.");

  stream = (pdf_stream *)src->data;
  dict   = pdf_stream_dict(src);

  if (pdf_lookup_dict(dict, "DecodeParms")) {
    WARN("DecodeParams not supported.");
    return -1;
  }

  filter = pdf_lookup_dict(dict, "Filter");
  if (!filter) {
    pdf_add_stream(dst, stream->stream, stream->stream_length);
    return 0;
#if HAVE_ZLIB
  } else {
    char *filter_name;
    if (filter->type == PDF_NAME) {
      filter_name = pdf_name_value(filter);
      if (filter_name && !strcmp(filter_name, "FlateDecode"))
	return add_stream_flate(dst, stream->stream, stream->stream_length);
      else {
	WARN("DecodeFilter \"%s\" not supported.", filter_name);
	return -1;
      }
    } else if (filter->type == PDF_ARRAY) {
      if (((pdf_array *)filter->data)->size > 1) {
	WARN("Multiple DecodeFilter not supported.");
	return -1;
      } else {
	filter_name = pdf_name_value(pdf_get_array(filter, 0));
	if (filter_name && !strcmp(filter_name, "FlateDecode"))
	  return add_stream_flate(dst, stream->stream, stream->stream_length);
	else {
	  WARN("DecodeFilter \"%s\" not supported.", filter_name);
	  return -1;
	}
      }
    } else
      ERROR("Broken PDF file?");
#endif /* HAVE_ZLIB */
  }

  return -1;
}
