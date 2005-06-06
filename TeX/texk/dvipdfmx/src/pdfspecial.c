/*  $Header: /home/cvsroot/dvipdfmx/src/pdfspecial.c,v 1.26 2004/02/04 12:46:11 hirata Exp $

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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "system.h"
#include "mem.h"
#include "mfileio.h"
#include "numbers.h"
#include "dvi.h"
#include "pdflimits.h"
#include "pdfspecial.h"
#include "pdfobj.h"
#include "pdfdoc.h"
#include "pdfdev.h"
#include "pdfparse.h"
#include "epdf.h"
#include "mpost.h"
#include "jpegimage.h"
#include "psimage.h"

#ifdef HAVE_LIBPNG
#include "pngimage.h"
#endif /* HAVE_LIBPNG */

static void add_reference (char *name, pdf_obj *object, char *res_name);
static void release_reference (char *name);

static char    *lookup_ref_res_name (char *name);
static pdf_obj *lookup_object(char *name);

static void do_content (char **start, char *end, double x_user, double y_user);
static void do_literal (char **start, char *end, double x_user, double y_user);
static void do_fstream (char **start, char *end);

static void do_image   (char **start, char *end, double x_user, double y_user);
static void compute_scales (struct xform_info *p, int width, int height);
static void finish_image (pdf_obj *image_res, struct xform_info *p, char *res_name,
			  int *p_width, int *p_height);
static void do_bxobj (char **start, char *end, double x_user, double y_user);
static void do_exobj (char **start, char *end);
static void do_uxobj (char **start, char *end, double x_user, double y_user);

static char   ignore_colors = 0;
static double annot_grow    = 0.0;

static unsigned char verbose = 0;

void pdf_special_set_verbose(void)
{
  if (verbose < 255) verbose++;
}

void pdf_special_set_grow (double g)
{
  annot_grow = g;
}
double pdf_special_tell_grow (void)
{
  return annot_grow;
}

void pdf_special_ignore_colors(void)
{
  ignore_colors = 1;
}

static void do_bop(char **start, char *end)
{
  if (*start < end)
    pdf_doc_bop(*start, end - *start);
  *start = end;
}

static void do_eop(char **start, char *end)
{
  if (*start < end)
    pdf_doc_eop(*start, end - *start);
  *start = end;
}

pdf_obj *get_reference_lvalue(char **start, char *end)
{
  pdf_obj *result = NULL;
  char *name, *save = *start;

  skip_white (start, end);
  if ((name = parse_opt_ident(start, end)) != NULL) {
    result = lookup_object(name);
    if (result == NULL) {
      WARN("Named object \"@%s\" doesn't exist.", name);
      *start = save;
      dump(*start, end);
    }
    RELEASE (name);
  }

  return result;
}

static void modify_page_attributes (pdf_dict *dict)
{
  pdf_name *name;
  pdf_array *array;
  pdf_number *number;
  while (dict->key != NULL) {
    if ((dict->key)->type == PDF_NAME && (dict->value)->type == PDF_ARRAY) {
      name = (dict->key)->data;
      array = (dict->value)->data;
      if ((strcmp(name->name, "MediaBox") == 0 ||
           strcmp(name->name, "CropBox" ) == 0 ||
           strcmp(name->name, "BleedBox") == 0 ||
           strcmp(name->name, "TrimBox" ) == 0 ||
           strcmp(name->name, "ArtBox"  ) == 0) &&
          array->size == 4) {
        double height = (double) ROUND(dev_base_page_height()-dev_page_height(), .001);
	if ((array->values)[1]->type == PDF_NUMBER) {
          number = (array->values)[1]->data;
	  number->value += height;
        }
	if ((array->values)[3]->type == PDF_NUMBER) {
          number = (array->values)[3]->data;
	  number->value += height;
        }
      }
    }
    dict = dict->next;
  }
}

static void do_put(char **start, char *end)
{
  pdf_obj *result = NULL, *data = NULL;
  char *save = *start;
  skip_white(start, end);
  if ((result = get_reference_lvalue(start, end)) != NULL) {
    skip_white (start, end);
    save = *start;
    switch (result->type) {
    case PDF_DICT:
      if ((data = parse_pdf_dict (start, end)) != NULL &&
	  data->type == PDF_DICT) {
	pdf_merge_dict(result, data);
      } else {
	WARN("Special put:  Expecting a dictionary.");
	*start = save;
	dump(*start, end);
      }
      if (data != NULL) {
        modify_page_attributes(data->data);
	pdf_release_obj (data);
      }
      break;
    case PDF_ARRAY:
      while (*start < end && 
	     (data = parse_pdf_object (start, end)) != NULL) {
	pdf_add_array (result, data);
	skip_white(start, end);
      }
      if (*start < end) {
	WARN("Special: put: invalid object.  Rest of line ignored.");
	*start = save;
	dump(*start, end);
      }
      break;
    default:
      WARN("Special put:  Invalid destination object type.");
      break;
    }
  }
  else {
    WARN("Special put:  Nonexistent object reference.");
  }
  return;
}

/*
 * The following must be consecutive and
 * starting at 0.  Order must match
 * dimensions array below
 */

#define WIDTH  0
#define HEIGHT 1
#define DEPTH  2
#define SCALE  3
#define XSCALE 4
#define YSCALE 5
#define ROTATE 6
#define BBOX   7

static struct {
  const char *s;
  int   key;
  int   hasunits;
} dimensions[] = {
  {"width",  WIDTH,  1},
  {"height", HEIGHT, 1},
  {"depth",  DEPTH,  1},
  {"scale",  SCALE,  0},
  {"xscale", XSCALE, 0},
  {"yscale", YSCALE, 0},
  {"rotate", ROTATE, 0},
  {"bbox",   BBOX,   0}
};

struct xform_info *new_xform_info (void)
{
  struct xform_info *result;
  result = NEW (1, struct xform_info);
  result -> width = 0.0;
  result -> height = 0.0;
  result -> depth = 0.0;
  result -> scale = 0.0;
  result -> xscale = 0.0;
  result -> yscale = 0.0;
  result -> rotate = 0.0;
  result -> user_bbox = 0;
  result -> clip = 0;
 /* These next two must be initialized be cause
    they represent the reference point even
    if the user doesn't specify one.  We must
    have a reference point */
  result -> u_llx = 0.0;
  result -> u_lly = 0.0;
  return result;
}

void release_xform_info (struct xform_info *p)
{
  RELEASE (p);
}

int validate_image_xform_info (struct xform_info *p)
{
  if (p->width != 0.0)
    if (p->scale !=0.0 || p->xscale != 0.0) {
      WARN("Can't supply both width and scale.");
      return 0;
    }
  if (p->height != 0.0) 
    if (p->scale !=0.0 || p->yscale != 0.0) {
      WARN("Can't supply both height and scale.");
      return 0;
    }
  if (p->scale != 0.0)
    if (p->xscale != 0.0 || p->yscale != 0.0) {
      WARN("Can't supply overall scale along with axis scales.");
      return 0;
    }
  return 1;
}

static int parse_one_dim_word (char **start, char *end)
{
  int i;
  char *dimension_string;
  char *save = *start;
  int result = -1;
  skip_white(start, end);
  if ((dimension_string = parse_ident(start, end)) != NULL) {
    for (i=0; i<sizeof(dimensions)/sizeof(dimensions[0]); i++) {
      if (!strcmp (dimensions[i].s, dimension_string))
	break;
    }
    if (i != sizeof(dimensions)/sizeof(dimensions[0])) {
      result =  dimensions[i].key;
    } else {
      WARN("Invalid keyword \"%s\".", dimension_string);
    }
    RELEASE (dimension_string);
  }
  if (result < 0) {
    *start = save;
    WARN("Expecting a keyword here, e.g., height, width, etc.");
    dump(*start, end);
  }
  return result;
}

static struct {
  const char *s;
  double units;
  int is_true_unit;
} units[] = {
  {"pt", (72.0/72.27), 0},
  {"in", (72.0), 0},
  {"cm", (72.0/2.54), 0},
  {"mm", (72.0/25.4), 0},
  {"bp", 1.0, 0},
  {"truept", (72.0/72.27), 1},
  {"truein", (72.0), 1},
  {"truecm", (72.0/2.54), 1},
  {"truemm", (72.0/25.4), 1},
  {"truebp", 1.0, 1}
};
  
double parse_one_unit (char **start, char *end)
{
  int i;
  char *unit_string = NULL, *save = *start;
  double result = -1.0;
  int errors = 0;
  skip_white(start, end);
  if ((unit_string = parse_c_ident(start, end)) != NULL) {
    for (i=0; i<sizeof(units)/sizeof(units[0]); i++) {
      if (!strcmp (units[i].s, unit_string))
	break;
    }
    if (i == sizeof(units)/sizeof(units[0])) {
      WARN("%s: Invalid unit of measurement (should be in, cm, pt, etc.).", unit_string);
      errors = 1;
    }
    if (i != sizeof(units)/sizeof(units[0]) && !units[i].is_true_unit)
      result = units[i].units;
    /* If these are "true" units, we must pre-shrink since the entire
       document is magnified */
    if (i != sizeof(units)/sizeof(units[0]) && units[i].is_true_unit)
      result = units[i].units/dvi_tell_mag();
    RELEASE (unit_string);
  }
  if (!unit_string || errors) {
    WARN("Expecting a unit here (e.g., in, cm, pt).");
    *start = save; 
    dump(*start, end);
  }
  return result;
}

static int parse_dimension (char **start, char *end,
			    struct xform_info *p)
{
  char *number_string = NULL;
  char *save = NULL;
  double units = -1.0;
  int key, error = 0;
  skip_white(start, end);
  while (*start < end && isalpha (**start)) {
    save = *start;
    if ((key = parse_one_dim_word(start, end)) >= 0) {
      skip_white(start, end);
    } else {
      WARN("Expecting a dimension/transformation keyword (e.g., width, height) here:");
      error = 1;
      break;
    }
    if (key != BBOX) { /* BBOX is handled somewhat differently than
			  all the others  */
      number_string = parse_number(start, end);
      if (key >= 0 && number_string == NULL) {
	WARN("Expecting a number following dimension/transformation keyword.");
	error = 1;
	break;
      }
      /* If we got a key and a number, see if we need a dimension also */
      if (key >= 0 && number_string != NULL && dimensions[key].hasunits) {
	skip_white(start, end);
	if ((units = parse_one_unit(start, end)) < 0.0) {
	  WARN("Expecting a dimension unit.");
	  error = 1;
	}
      }
      if (!error && key >= 0 && number_string != NULL) {
	switch (key) {
	case WIDTH:
	  if (p->width != 0.0)
	    WARN("Duplicate width specified: %s.", number_string);
	  p->width = atof (number_string)*units;
	  break;
	case HEIGHT:
	  if (p->height != 0.0)
	    WARN("Duplicate height specified: %s.", number_string);
	  p->height = atof (number_string)*units;
	  break;
	case DEPTH:
	  if (p->depth != 0.0)
	    WARN("Duplicate depth specified: %s.", number_string);
	  p->depth = atof (number_string)*units;
	  break;
	case SCALE:
	  if (p->scale != 0.0)
	    WARN("Duplicate depth specified: %s.", number_string);
	  p->scale = atof (number_string);
	  break;
	case XSCALE:
	  if (p->xscale != 0.0)
	    WARN("Duplicate xscale specified: %s.", number_string);
	  p->xscale = atof (number_string);
	  break;
	case YSCALE:
	  if (p->yscale != 0.0)
	    WARN("Duplicate yscale specified: %s.", number_string);
	  p->yscale = atof (number_string);
	  break;
	case ROTATE:
	  if (p->rotate != 0)
	    WARN("Duplicate rotation specified: %s.", number_string);
	  p->rotate = atof (number_string) * M_PI / 180.0;
	  break;
	default:
	  ERROR ("parse_dimension: Invalid key");
	}
	if (number_string != NULL) {
	  RELEASE (number_string);
	  number_string = NULL;
	}
      }
    } else { /* BBox case handled here */
      char *llx = NULL, *lly = NULL, *urx = NULL, *ury = NULL;
      if ((llx = parse_number (start, end)) &&
	  (lly = parse_number (start, end)) &&
	  (urx = parse_number (start, end)) &&
	  (ury = parse_number (start, end))) {
	p->u_llx = atof (llx);
	p->u_lly = atof (lly);
	p->u_urx = atof (urx);
	p->u_ury = atof (ury);
	p->user_bbox = 1; /* Flag to indicate that user specified a bbox */
      } else {
	WARN("Expecting four numbers following \"bbox\" specification.");
	error = 1; /* Flag error, but don't break until we get a
		      chance to free structures */
      }
      if (llx) RELEASE (llx);
      if (lly) RELEASE (lly);
      if (urx) RELEASE (urx);
      if (ury) RELEASE (ury);
      if (error) break;
    }
    skip_white(start, end);
  }
  if (error && save)
    dump (save, end);
  return !error;
}

static void do_pagesize(char **start, char *end)
{
  struct xform_info *p;
  int error = 0;
  p = new_xform_info();
  skip_white(start, end);
  if (parse_dimension(start, end, p)) {
    if (p->scale != 0.0 || p->xscale != 0.0 || p->yscale != 0.0) {
      WARN("Scale meaningless for pagesize.");
      error = 1;
    }
    if (p->width == 0.0 || p->depth + p->height == 0.0) {
      WARN("Page cannot have a zero dimension.");
      error = 1;
    }
  } else {
    WARN("Special: pagesize: Failed to find a valid set of dimensions.");
    dump (*start, end);
    error = 1;
  }
  if (!error)
    /* Since these are physical dimensions, they need to be scaled by
       mag.  "Virtual" dimensions are scaled by a transformation
       matrix.  Physical dimensions (e.g, page size and annotations)
       cannot */
    dev_set_page_size (dvi_tell_mag()*p->width, dvi_tell_mag()*(p->depth + p->height));
  release_xform_info (p);
  return;
}

static void do_ann(char **start, char *end)
{
  pdf_obj *result = NULL, *rectangle = NULL;
  char *name = NULL;
  int error = 0;
  struct xform_info *p = new_xform_info();
  skip_white(start, end);
  name = parse_opt_ident(start, end);
  skip_white(start, end);
  if (parse_dimension(start, end, p)) {
    if (p->scale != 0.0 || p->xscale != 0.0 || p->yscale != 0.0) {
      WARN("Scale meaningless for annotations.");
      error = 1;
    }
#if 0
    /* Removed because ConTeXt sometimes uses an empty object. */
    if (p->width == 0.0 || p->depth + p->height == 0.0) {
      WARN("Special ann: Rectangle has a zero dimension.");
      WARN("Special ann: Annotations require both horizontal and vertical dimensions.");
      error = 1;
    }
#endif
  }
  if (!error && (result = parse_pdf_ann_dict(start, end)) != NULL) {
    double height = (double) ROUND(dev_base_page_height()-dev_page_height(), .001);
    rectangle = pdf_new_array();
    pdf_add_array (rectangle, pdf_new_number(ROUND(dev_phys_x()-annot_grow, .001)));
    pdf_add_array (rectangle, pdf_new_number(height+ROUND(dev_phys_y()-dvi_tell_mag()*p->depth-annot_grow, .001)));
    pdf_add_array (rectangle, pdf_new_number(ROUND(dev_phys_x()+dvi_tell_mag()*p->width+annot_grow, .001)));
    pdf_add_array (rectangle, pdf_new_number(height+ROUND(dev_phys_y()+dvi_tell_mag()*p->height+annot_grow, .001)));
    pdf_add_dict (result, pdf_new_name ("Rect"),
		  rectangle);
    pdf_doc_add_to_page_annots (pdf_ref_obj (result));
    /* If this object has a named reference, we file it away for
       later.  Otherwise we release it */
    if (name != NULL) {
      add_reference (name, result, NULL);
      /* An annotation is treated differently from a cos object.
	 cos objects are kept open for the user.  We forcibly
         "close" the annotation by calling release_reference */
      release_reference (name);
    } else
      pdf_release_obj (result);
  } else {
    WARN("Ignoring annotation with invalid dictionary.");
    error = 1;
  }
  release_xform_info (p);
  if (name)
    RELEASE (name);
}

pdf_obj *pending_annot_dict = NULL;
static void do_bann(char **start, char *end)
{
  int error = 0;
#ifdef MEM_DEBUG
MEM_START
#endif
  if (!pending_annot_dict) {
    skip_white(start, end);
    if ((pending_annot_dict = parse_pdf_dict(start, end)) != NULL) {
      pdf_doc_begin_annot (pending_annot_dict);
      /* If this object has a named reference, we file it away for
	 later.  Otherwise we release it */
    } else {
      WARN("Ignoring annotation with invalid dictionary.");
      error = 1;
    }
  } else {
    WARN("Can't begin an annotation when one is pending.");
  }
#ifdef MEM_DEBUG
MEM_END
#endif
  return;
}

static void do_eann(char **start, char *end)
{
#ifdef MEM_DEBUG
MEM_START
#endif
  if (pending_annot_dict) {
    pdf_doc_end_annot ();
    pdf_release_obj (pending_annot_dict);
    pending_annot_dict = NULL;
  } else {
    WARN("Tried to end an annotation without starting one!");
  }
#ifdef MEM_DEBUG
MEM_END
#endif
  return;
}

static void do_bgcolor(char **start, char *end)
{
  char *save = *start;
  pdf_obj *color;
  int error = 0;
  skip_white(start, end);
  if ((color = parse_pdf_object(start, end)) != NULL &&
      (color -> type == PDF_ARRAY ||
       color -> type == PDF_NUMBER )) {
    switch (color -> type) {
      int i;
    case PDF_ARRAY:
      for (i=0; i<5; i++) {
	if (pdf_get_array (color, i) == NULL ||
	    pdf_get_array (color, i) -> type != PDF_NUMBER)
	  break;
      }
      switch (i) {
      case 3:
	dev_bg_rgb_color (pdf_number_value (pdf_get_array (color,0)),
			  pdf_number_value (pdf_get_array (color,1)),
			  pdf_number_value (pdf_get_array (color,2)));
	break;
      case 4:
	dev_bg_cmyk_color (pdf_number_value (pdf_get_array (color,0)),
			   pdf_number_value (pdf_get_array (color,1)),
			   pdf_number_value (pdf_get_array (color,2)),
			   pdf_number_value (pdf_get_array (color,3)));
	break;
      default:
	WARN("Special: begincolor: Expecting either RGB or CMYK color array.");
	error = 1;
      }
      break;
    case PDF_NUMBER:
      dev_bg_gray (pdf_number_value (color));
    }
  } else {
    WARN("Special: background color: Expecting color specified by an array or number.");
    error = 1;
  }
  if (error) {
    *start = save;
    dump (*start, end);
  }
  if (color)
    pdf_release_obj (color);
  return;
}

static void do_bcolor(char **start, char *end)
{
  char *save = *start;
  pdf_obj *color;
  int error = 0;
#ifdef MEM_DEBUG
  MEM_START
#endif /* MEM_DEBUG */
  skip_white(start, end);
  if ((color = parse_pdf_object(start, end)) != NULL &&
      (color -> type == PDF_ARRAY ||
       color -> type == PDF_NUMBER )) {
    switch (color -> type) {
      int i;
    case PDF_ARRAY:
      for (i=0; i<5; i++) {
	if (pdf_get_array (color, i) == NULL ||
	    pdf_get_array (color, i) -> type != PDF_NUMBER)
	  break;
      }
      switch (i) {
      case 3:
	dev_begin_rgb_color (pdf_number_value (pdf_get_array (color,0)),
			     pdf_number_value (pdf_get_array (color,1)),
			     pdf_number_value (pdf_get_array (color,2)));
	break;
      case 4:
	dev_begin_cmyk_color (pdf_number_value (pdf_get_array (color,0)),
			      pdf_number_value (pdf_get_array (color,1)),
			      pdf_number_value (pdf_get_array (color,2)),
			      pdf_number_value (pdf_get_array (color,3)));
	break;
      default:
	WARN("Special: begincolor: Expecting either RGB or CMYK color array.");
	error = 1;
      }
      break;
    case PDF_NUMBER:
      dev_begin_gray (pdf_number_value (color));
    }
  } else {
    WARN("Special: Begincolor: Expecting color specified by an array or number.");
    error = 1;
  }
  if (error) {
    *start = save;
    dump (*start, end);
  }
  if (color)
    pdf_release_obj (color);
#ifdef MEM_DEBUG
  MEM_END
#endif /* MEM_DEBUG */
  return;
}

static void do_scolor(char **start, char *end)
{
  char *save = *start;
  pdf_obj *color;
  int error = 0;
#ifdef MEM_DEBUG
  MEM_START
#endif /* MEM_DEBUG */
  skip_white(start, end);
  if ((color = parse_pdf_object(start, end)) != NULL &&
      (color -> type == PDF_ARRAY ||
       color -> type == PDF_NUMBER )) {
    switch (color -> type) {
      int i;
    case PDF_ARRAY:
      for (i=0; i<5; i++) {
	if (pdf_get_array (color, i) == NULL ||
	    pdf_get_array (color, i) -> type != PDF_NUMBER)
	  break;
      }
      switch (i) {
      case 3:
	dev_set_def_rgb_color (pdf_number_value (pdf_get_array (color,0)),
			       pdf_number_value (pdf_get_array (color,1)),
			       pdf_number_value (pdf_get_array (color,2)));
	break;
      case 4:
	dev_set_def_cmyk_color (pdf_number_value (pdf_get_array (color,0)),
				pdf_number_value (pdf_get_array (color,1)),
				pdf_number_value (pdf_get_array (color,2)),
				pdf_number_value (pdf_get_array (color,3)));
	break;
      default:
	WARN("Special: begincolor: Expecting either RGB or CMYK color array.");
	error = 1;
      }
      break;
    case PDF_NUMBER:
      dev_set_def_gray (pdf_number_value (color));
    }
  } else {
    WARN("Special: Begincolor: Expecting color specified by an array or number.");
    error = 1;
  }
  if (error) {
    *start = save;
    dump (*start, end);
  }
  if (color)
    pdf_release_obj (color);
#ifdef MEM_DEBUG
  MEM_END
#endif /* MEM_DEBUG */
  return;
}

static void do_bgray(char **start, char *end)
{
  char *number_string;
  skip_white(start, end);
  if ((number_string = parse_number (start, end)) != NULL) {
    dev_begin_gray (atof (number_string));
    RELEASE (number_string);
  } else {
    WARN("Special: begingray: Expecting a numerical grayscale specification.");
  }
  return;
}

static void do_ecolor(void)
{
  dev_end_color();
}

static void do_egray(void)
{
  dev_end_color();
}

static void do_bxform (char **start, char *end, double x_user, double y_user)
{
  int error = 0;
  char *save = *start;
  struct xform_info *p = new_xform_info();

  skip_white(start, end);
  if (parse_dimension(start, end, p)) {
    if (!validate_image_xform_info(p)) {
      WARN("Specified dimensions are inconsistent.");
      WARN("Special will be ignored.");
      error = 1;
    }
    if (p->width != 0.0 || p->height != 0.0 || p->depth != 0.0)
      WARN("[special] bxform: width, height, and depth are meaningless. These will be ignored.");
    if (p->scale != 0.0) {
      p->xscale = p->scale;
      p->yscale = p->scale;
    }
    if (p->xscale == 0.0)
      p->xscale = 1.0;
    if (p->yscale == 0.0)
      p->yscale = 1.0;
  } else {
    WARN("Error in transformation parameters.");
    error = 1;
  }
  if (!error) {
    dev_begin_xform(p->xscale, p->yscale, p->rotate, x_user, y_user);
  } else {
    *start = save;
    dump(*start, end);
  }
  release_xform_info(p);
}

static void do_exform(void)
{
#ifdef MEM_DEBUG
MEM_START
#endif
  dev_end_xform();
#ifdef MEM_DEBUG
MEM_END
#endif
}

static void do_tounicode (char **start, char *end)
{
  char *cmap_name;

  skip_white(start, end);
  /*
   * This should be PDF name or PDF string.
   */
  cmap_name = parse_ident(start, end);
  set_tounicode_cmap(cmap_name);
  RELEASE(cmap_name);
}

static void do_outline(char **start, char *end)
{
  pdf_obj *level = NULL, *result;
  int error = 0;
  char *save; 
  static int lowest_level = 255;
  skip_white(start, end);
  save = *start; 
  if ((level = parse_pdf_object(start, end)) != NULL &&
      level -> type == PDF_NUMBER) {
    /* Make sure we know where the starting level is */
    if ( (int) pdf_number_value (level) < lowest_level)
      lowest_level = (int) pdf_number_value (level);

    if ((result = parse_pdf_out_dict(start, end)) != NULL) {
      pdf_doc_change_outline_depth
	((int)pdf_number_value(level)-lowest_level+1);
      pdf_doc_add_outline (result);
    } else {
      WARN("Ignoring invalid dictionary.");
      error = 1;
    }
  } else {
    WARN("Special: outline: Expecting number for object level.");
    *start = save;
    error = 1;
  }
  if (error)
    dump (*start, end);
  if (level)
    pdf_release_obj (level);
  return;
}

static void do_article(char **start, char *end)
{
  char *name = NULL, *save = *start;
  int error = 0;
  pdf_obj *info_dict = NULL;
  skip_white (start, end);
  if (*((*start)++) != '@' || (name = parse_ident(start, end)) == NULL) {
    WARN("Article name expected.");
    *start = save;
    dump (*start, end);
    error = 1;
  }
  if (!error && (info_dict = parse_pdf_dict(start, end)) == NULL) {
    WARN("Ignoring invalid dictionary.");
    error = 1;
  }
  if (!error) {
    pdf_doc_start_article (name, pdf_link_obj(info_dict));
    add_reference (name, info_dict, NULL);
  }
  if (name)
    RELEASE (name);
  return;
}

static void do_bead(char **start, char *end)
{
  pdf_obj *bead_dict, *rectangle, *article, *info_dict = NULL;
  int error = 0;
  char *name = NULL, *save = *start;
  struct xform_info *p;
  p = new_xform_info();
  skip_white(start, end);
  if (*((*start)++) != '@' || (name = parse_ident(start, end)) == NULL) {
    WARN("Article reference expected.");
    WARN("Which article does this go with?");
    error = 1;
  }
  /* If okay so far, try to get a bounding box */
  if (!error) {
    skip_white(start, end);
    if (!parse_dimension(start, end, p)) {
      WARN("Special: thread: Error in bounding box specification for this bead.");
      error = 1;
    }
    if (p->scale != 0.0 || p->xscale != 0.0 || p->yscale != 0.0) {
      WARN("Scale meaningless for annotations.");
      error = 1;
    }
    if (p->width == 0.0 || p->depth + p->height == 0.0) {
      WARN("Special thread: Rectangle has a zero dimension.");
      error = 1;
    }
  }
  if (!error) {
    skip_white (start, end);
    if (**start == '<') {
      if ((info_dict = parse_pdf_dict (start, end)) ==
	NULL) {
	WARN("Special: thread: Error in dictionary.");
	error = 1;
      }
    } else
      info_dict = pdf_new_dict();
  }
  if (!error && name && info_dict) {
    /* Does this article exist yet */
    if ((article = lookup_object (name)) == NULL) {
      pdf_doc_start_article (name, pdf_link_obj (info_dict));
      add_reference (name, info_dict, NULL);
    } else {
      pdf_merge_dict (article, info_dict);
      pdf_release_obj (info_dict);
      info_dict = NULL;
    }
    bead_dict = pdf_new_dict ();
    rectangle = pdf_new_array();
    {
    double height = (double) ROUND(dev_base_page_height()-dev_page_height(), .001);
    pdf_add_array (rectangle, pdf_new_number(ROUND(dev_phys_x(), .001)));
    pdf_add_array (rectangle, pdf_new_number(height+ROUND(dev_phys_y()-p->depth, .001)));
    pdf_add_array (rectangle, pdf_new_number(ROUND(dev_phys_x()+p->width, .001)));
    pdf_add_array (rectangle, pdf_new_number(height+ROUND(dev_phys_y()+p->height, .001)));
    }
    pdf_add_dict (bead_dict, pdf_new_name ("R"),
		  rectangle);
    pdf_add_dict (bead_dict, pdf_new_name ("P"),
		  pdf_doc_this_page_ref());
    pdf_doc_add_bead (name, bead_dict);
  }
  release_xform_info(p);
  if (name != NULL) {
    RELEASE (name);
  }
  if (error) {
    *start = save;
    dump (*start, end);
  }
  return;
}

#define MAX_IMAGES 5

typedef struct {
  char *fname;
  int width, height;
  unsigned long rnum;
  pdf_obj *object;
  double dpi_min;
} DPX_IMAGE;

static DPX_IMAGE *dpx_images = NULL;
static int num_dpx_images = 0, max_dpx_images = 0;

static DPX_IMAGE *get_dpx_image (char *fname)
{
  int i;

  for (i = 0; i < num_dpx_images; i++)
    if (strcmp(fname, dpx_images[i].fname) == 0) /* found */
      return dpx_images + i;
  return NULL;
}

void release_dpx_images (void)
{
  int i;

  for (i = 0; i < num_dpx_images; i++)
    if (dpx_images[i].fname) free(dpx_images[i].fname);
  if (dpx_images) free(dpx_images);
}

pdf_obj *embed_image (char *filename, struct xform_info *p,
		     double x_user, double y_user, char *objname) 
{
  pdf_obj *result = NULL;
  char *kpse_file_name;
  FILE *image_file;
  static char res_name[16];
  static unsigned long next_image = 1;
  DPX_IMAGE *image_obj = NULL;
  int width = 0, height = 0;

  MESG("(IMAGE:");
  if ((image_obj = get_dpx_image(filename))) {
    sprintf(res_name, "Im%ld", image_obj->rnum);
    MESG("%s", image_obj->fname);
    if (verbose)
      MESG("[%s]", res_name);
    if (image_obj->width == 0 || image_obj->height == 0)
      pdf_scale_image(p);
    else /* BITMAP images, JPEG and PNG */
      compute_scales(p, image_obj->width, image_obj->height);
  } else if ((kpse_file_name = kpse_find_pict(filename)) &&
             (image_file = MFOPEN(kpse_file_name, FOPEN_RBIN_MODE))) {
    sprintf(res_name, "Im%ld", next_image);
    MESG("%s", kpse_file_name);
    if (check_for_jpeg(image_file)) {
      MESG("[JPEG]");
      result = jpeg_start_image(image_file);
      if (result) finish_image(result, p, res_name, &width, &height);
    }
#ifdef HAVE_LIBPNG
    else if (check_for_png(image_file)) {
      MESG("[PNG]");
      result = start_png_image(image_file, NULL);
      if (result) finish_image(result, p, res_name, &width, &height);
    }
#endif
    else if (check_for_pdf(image_file)) {
      MESG("[PDF]");
      result = pdf_include_page(image_file, p, res_name);
    }
    else if (check_for_mp(image_file)) {
      MESG("[MPS]");
      result = mp_include(image_file, p, res_name, x_user, y_user);
    }
    /* Make sure we check for PS *after* checking for MP since
       MP is a special case of PS */
    else if (check_for_ps(image_file)) {
      MESG("[PS]");
      result = ps_include(kpse_file_name, p, res_name, x_user, y_user);
    }
    else {
      MESG("[UNKNOWN]");
      result = ps_include(kpse_file_name, p, res_name, x_user, y_user);
    }
    MFCLOSE(image_file);
    if (verbose)
      MESG("[%s]", res_name);
    if (result == NULL) {
      WARN("pdf: image inclusion failed for (%s).", filename);
      MESG(")");
      return NULL;
    }
  } else {
    WARN("Error locating or opening file (%s)", filename);
    MESG(")");
    return NULL;
  }

  if (image_obj == NULL) { /* new image */
    if (num_dpx_images >= max_dpx_images) {
      max_dpx_images += MAX_IMAGES;
      dpx_images = RENEW(dpx_images, max_dpx_images, DPX_IMAGE);
    }
    image_obj = dpx_images + num_dpx_images;
    image_obj->fname = NEW(strlen(filename)+1, char);
    strcpy(image_obj->fname, filename);
    image_obj->width = width;
    image_obj->height = height;
    image_obj->rnum = next_image++;
    image_obj->object = pdf_ref_obj(result);
    num_dpx_images++;
  }

  /* Put reference to object on page */
  pdf_doc_add_to_page_xobjects(res_name, pdf_link_obj(image_obj->object));
  pdf_doc_add_to_page(" q", 2);
  /* Handle the conversion to PDF stream units here */
  if (dev_wmode() == 1) /* rotated 90 deg. in vertical writing mode */
    add_xform_matrix(x_user, y_user, p->xscale/pdf_dev_scale(), p->yscale/pdf_dev_scale(), p->rotate - 0.50 * M_PI);
  else
    add_xform_matrix(x_user, y_user, p->xscale/pdf_dev_scale(), p->yscale/pdf_dev_scale(), p->rotate);
  if (p->depth != 0.0)
    add_xform_matrix(0.0, -p->depth, 1.0, 1.0, 0.0);
  sprintf(work_buffer, " /%s Do Q", res_name);
  pdf_doc_add_to_page(work_buffer, strlen(work_buffer));

  if (image_obj && verbose) {
    if (image_obj->width > 0 && image_obj->height > 0)
      MESG("[DPI:%dx%d]",
	   (int)(72*image_obj->width/(p->xscale/pdf_dev_scale())),
	   (int)(72*image_obj->height/(p->yscale/pdf_dev_scale())));
  }

  if (objname) {
    add_reference(objname, pdf_link_obj(image_obj->object), res_name);
    /* Read the explanation for the next line in do_ann() */
    release_reference(objname);
  }
  MESG(")");

  return result;
}

static void do_image (char **start, char *end, double x_user, double y_user)
{
  char *filename = NULL, *objname = NULL, *save;
  pdf_obj *filestring = NULL, *result = NULL;
  int error = 0;
  struct xform_info *p;
#ifdef MEM_DEBUG
MEM_START
#endif
  skip_white(start, end);
  objname = parse_opt_ident(start, end);
  p = new_xform_info();
  skip_white(start, end);
  save = *start;
  if (!parse_dimension(start, end, p)) {
    WARN("Error in dimensions of encapsulated image.");
    error = 1;
  }
  skip_white(start, end);
  if (!error && (filestring = parse_pdf_string(start, end)) == NULL) {
    WARN("Missing filename.");
    error = 1;
  }
  if (!error) {
    filename = pdf_string_value(filestring);
  }
  if (!error && !validate_image_xform_info (p)) {
    WARN("Specified dimensions are inconsistent.");
    dump (save, end);
    error = 1;
  }
  if (!error)
    result = embed_image (filename, p, x_user, y_user, objname);
  else
    dump (save, end);
  if (p)
    release_xform_info (p);
  if (objname)
    RELEASE (objname);
  if (filestring)
    pdf_release_obj (filestring);
  if (error)
    WARN("Image special ignored.");
  if (result)
    pdf_release_obj (result);
  return;
#ifdef MEM_DEBUG
MEM_END
#endif
}

static void do_dest(char **start, char *end)
{
  pdf_obj *name;
  pdf_obj *array;
#ifdef MEM_DEBUG
MEM_START
#endif
  skip_white(start, end);
  if ((name = parse_pdf_string(start, end)) == NULL) {
    WARN("PDF string expected and not found.");
    WARN("Special dest: ignored.");
    dump(*start, end);
    return;
  }
  if ((array = parse_pdf_object(start, end)) == NULL) {
    pdf_release_obj (name);
    return;
  }
  pdf_doc_add_dest (pdf_obj_string_value(name),
		    pdf_obj_string_length(name),
		    pdf_ref_obj (array));
  pdf_release_obj (name);
  pdf_release_obj (array);
#ifdef MEM_DEBUG
MEM_END
#endif
}

static void do_docinfo(char **start, char *end)
{
  pdf_obj *result;
  if ((result = parse_pdf_info_dict(start, end)) != NULL) {
    pdf_doc_merge_with_docinfo (result);
  } else {
    WARN("Special: docinfo: Dictionary expected and not found.");
    dump (*start, end);
  }
  pdf_release_obj (result);
  return;
}

static void do_docview(char **start, char *end)
{
  pdf_obj *result;
  if ((result = parse_pdf_dict(start, end)) != NULL) {
    pdf_doc_merge_with_catalog (result);
  } else {
    WARN("Special: docview: Dictionary expected and not found.");
    dump (*start, end);
  }
  pdf_release_obj (result);
  return;
}


static void do_close(char **start, char *end)
{
  char *name;
  skip_white(start, end);
  if ((name = parse_opt_ident(start, end)) != NULL) {
    release_reference (name);
    RELEASE (name);
  }
  return;
}

static void do_obj (char **start, char *end)
{
  register pdf_obj *result;
  register char *name;

  skip_white(start, end);
  if ((name = parse_opt_ident(start, end))) {
    if ((result = parse_pdf_object(start, end)))
      add_reference(name, result, NULL);
    else
      WARN("Special object: Could not find an object for (%s).", name);
    RELEASE (name);
    return;
  }
  WARN("Special object: Could not find a reference name.");
}

static void do_content(char **start, char *end, double x_user, double y_user)
{
  int len;
  if (*start < end) {
    len = sprintf (work_buffer, " q 1 0 0 1 %.2f %.2f cm ", x_user, y_user);
    pdf_doc_add_to_page (work_buffer, len);
    pdf_doc_add_to_page (*start, end-*start);
    pdf_doc_add_to_page (" Q", 2);
  }
  *start = end;
  return;
}

static void do_literal(char **start, char *end, double x_user, double y_user)
{
  skip_white(start, end);
  if (*start + 7 <= end && strncmp(*start, "reverse", 7) == 0) {
    x_user *= -1.; y_user *= -1.; *start += 7;
  }
  if (*start + 6 > end || strncmp(*start, "direct", 6)) {
    pdf_doc_add_to_page(work_buffer, sprintf(work_buffer, " 1 0 0 1 %.2f %.2f cm ", x_user, y_user));
  } else
    *start += 6;
  if (*start < end)
    pdf_doc_add_to_page(*start, end-*start);
  *start = end;
  return;
}

/*
 * FSTREAM: Create a PDF stream object from an existing file.
 *
 *  pdf: fstream @objname (filename) [PDF_DICT]
 */
static void do_fstream (char **start, char *end)
{
  pdf_obj *result;
  pdf_obj *tmp;
  char    *name, *filename;

  skip_white(start, end);
  if ((name = parse_opt_ident(start, end)) == NULL) {
    WARN("Could not find a reference name. PDF special ignored.");
    return;
  }

  /*
   * Filename: We do not use kpathseach here.
   */
  skip_white(start, end);
  if ((tmp = parse_pdf_string(start, end)) == NULL ||
      (filename = pdf_string_value(tmp)) == NULL) {
    WARN("Missing filename.");
    RELEASE(name);
    return;
  }
  MESG("(FILE:%s", filename);
  /*
   * Create stream object from file.
   */
  result = pdf_new_stream(STREAM_COMPRESS);
  {
    FILE *fp;
    long  len;
    char *fullname;

    fullname = kpse_find_file(filename, kpse_program_binary_format, 0);
    if (!fullname || !(fp = MFOPEN(fullname, FOPEN_RBIN_MODE))) {
      WARN("Could not find/open file \"%s\".", filename);
      RELEASE(name);
      pdf_release_obj(result);
      return;
    }
    if (verbose > 1)
      MESG("[%s]", fullname);
    while ((len = fread(work_buffer, sizeof(char), WORK_BUFFER_SIZE, fp)) > 0)
      pdf_add_stream(result, work_buffer, len);
    MFCLOSE(fp);
  }

  pdf_release_obj(tmp);
  /*
   * Optional dict.
   *
   *  TODO: check Length, Filter...
   */
  skip_white(start, end);
  if ((tmp = parse_pdf_dict(start, end)) != NULL) {
    pdf_obj *sdict = pdf_stream_dict(result);
    pdf_merge_dict(sdict, tmp);
    pdf_release_obj(tmp);
  }

  add_reference(name, result, NULL);
  RELEASE(name);

  MESG(")");

  return;
}

static int is_pdf_special (char **start, char *end)
{
  skip_white(start, end);
  if (end-*start >= strlen ("pdf:") &&
      !strncmp (*start, "pdf:", strlen("pdf:"))) {
    *start += strlen("pdf:");
    return 1;
  }
  return 0;
}

#define ANN     1
#define OUTLINE 2
#define ARTICLE 3
#define DEST    4
#define DOCINFO 7
#define DOCVIEW 8
#define OBJ     9
#define CONTENT 10
#define PUT     11
#define CLOSE   12
#define BOP     13
#define EOP     14
#define BEAD    15
#define EPDF    16
#define IMAGE   17
#define BCOLOR  18
#define ECOLOR  19
#define BGRAY   20
#define EGRAY   21
#define BGCOLOR 22
#define BXFORM  23
#define EXFORM  24
#define PAGE_SIZE 25
#define BXOBJ   26
#define EXOBJ   27
#define UXOBJ   28
#define SCOLOR  29
#define BANN    30
#define EANN    31
#define LINK_ANNOT   32
#define NOLINK_ANNOT 33
#define TOUNICODE 34
#define LITERAL 35
#define FSTREAM 36

static struct pdfmark
{
  const char *string;
  int value;
} pdfmarks[] = {
  {"ann", ANN},
  {"annot", ANN},
  {"annotate", ANN},
  {"annotation", ANN},
  {"out", OUTLINE},
  {"outline", OUTLINE},
  {"art", ARTICLE},
  {"article", ARTICLE},
  {"bead", BEAD},
  {"thread", BEAD},
  {"dest", DEST},
  {"docinfo", DOCINFO},
  {"docview", DOCVIEW},
  {"obj", OBJ},
  {"object", OBJ},
  {"content", CONTENT},
  {"put", PUT},
  {"close", CLOSE},
  {"bop", BOP},
  {"eop", EOP},
  {"epdf", EPDF},
  {"image", IMAGE},
  {"img", IMAGE},
  {"bc", BCOLOR},
  {"bcolor", BCOLOR},
  {"begincolor", BCOLOR},
  {"link", LINK_ANNOT},
  {"nolink", NOLINK_ANNOT},
  {"sc", SCOLOR},
  {"scolor", SCOLOR},
  {"setcolor", SCOLOR},
  {"ec", ECOLOR},
  {"ecolor", ECOLOR},
  {"endcolor", ECOLOR},
  {"bg", BGRAY},
  {"bgray", BGRAY},
  {"begingray", BGRAY},
  {"eg", EGRAY},
  {"egray", EGRAY},
  {"endgray", EGRAY},
  {"bgcolor", BGCOLOR},
  {"bgc", BGCOLOR},
  {"bbc", BGCOLOR},
  {"bbg", BGCOLOR},
  {"pagesize", PAGE_SIZE},
  {"beginann", BANN},
  {"bann", BANN},
  {"bannot", BANN},
  {"eann", EANN},
  {"endann", EANN},
  {"eannot", EANN},
  {"begintransform", BXFORM},
  {"begintrans", BXFORM},
  {"btrans", BXFORM},
  {"bt", BXFORM},
  {"endtransform", EXFORM},
  {"endtrans", EXFORM},
  {"etrans", EXFORM},
  {"et", EXFORM},
  {"beginxobj", BXOBJ},
  {"bxobj", BXOBJ},
  {"endxobj", EXOBJ},
  {"exobj", EXOBJ},
  {"usexobj", UXOBJ},
  {"uxobj", UXOBJ},
  {"tounicode", TOUNICODE},
  {"literal", LITERAL},
  {"fstream", FSTREAM}
};

static int parse_pdfmark (char **start, char *end)
{
  char *save;
  int i;
  if (verbose > 2) {
    MESG("\nparse_pdfmark:");
    dump (*start, end);
  }
  skip_white(start, end);
  if (*start >= end) {
    WARN("Special ignored...no pdfmark found.");
    return -1;
  }
  
  save = *start;
  while (*start < end && isalpha (**start))
    (*start)++;
  for (i=0; i<sizeof(pdfmarks)/sizeof(struct pdfmark); i++) {
    if (*start-save == strlen (pdfmarks[i].string) &&
	!strncmp (save, pdfmarks[i].string,
		  strlen(pdfmarks[i].string)))
      return pdfmarks[i].value;
  }
  *start = save;
  WARN("Expecting pdfmark (and didn't find one).");
  dump(*start, end);
  return -1;
}

struct named_reference 
{
  char *name;
  char *res_name;
  pdf_obj *object_ref;
  pdf_obj *object;
} *named_references = NULL;

static unsigned long number_named_references = 0, max_named_objects = 0;

static void add_reference (char *name, pdf_obj *object, char *res_name)
{
  register int i;

  if (number_named_references >= max_named_objects) {
    max_named_objects += NAMED_OBJ_ALLOC_SIZE;
    named_references = RENEW (named_references, max_named_objects,
			      struct named_reference);
  }

  for (i = 0; i < number_named_references; i++)
    if (!strcmp(named_references[i].name, name))
      break;

  if (i == number_named_references) {
    named_references[i].name = NEW (strlen(name)+1, char);
    strcpy(named_references[i].name, name);
    number_named_references++;
  }
  if (res_name != NULL && strlen(res_name) > 0) {
    named_references[i].res_name = NEW(strlen(name)+1, char);
    strcpy(named_references[i].res_name, res_name);
  } else
    named_references[i].res_name = NULL;
  named_references[i].object_ref = pdf_ref_obj(object);
  named_references[i].object = object;
}

/* The following routine returns copies, not the original object */
pdf_obj *lookup_reference(char *name)
{
  int i;
  /* First check for builtins first */
  if (!strcmp (name, "ypos")) {
    return pdf_new_number(ROUND(dev_phys_y(), .001));
  }
  if (!strcmp (name, "xpos")) {
    return pdf_new_number(ROUND(dev_phys_x(), .001));
  }
  if (!strcmp (name, "thispage")) {
    return pdf_doc_this_page_ref();
  }
  if (!strcmp (name, "prevpage")) {
    return pdf_doc_prev_page_ref();
  }
  if (!strcmp (name, "nextpage")) {
    return pdf_doc_next_page_ref();
  }
  if (!strcmp (name, "pages")) {
    return pdf_ref_obj (pdf_doc_page_tree());
  }
  if (!strcmp (name, "names")) {
    return pdf_ref_obj (pdf_doc_names());
  }
  if (!strcmp (name, "resources")) {
    return pdf_ref_obj (pdf_doc_current_page_resources());
  }
  if (!strcmp (name, "catalog")) {
    return pdf_ref_obj (pdf_doc_catalog());
  }

  if (strlen (name) > 4 &&
      !strncmp (name, "page", 4) &&
      is_an_int (name+4)) {
    return pdf_doc_ref_page(atoi (name+4));
  }
  for (i=0; i<number_named_references; i++) {
    if (!strcmp (named_references[i].name, name)) {
      break;
    }
  }
  if (i == number_named_references)
    add_reference(name, pdf_new_null(), NULL);
  return (pdf_link_obj(named_references[i].object_ref));
}

static pdf_obj *lookup_object(char *name)
{
  int i;
  if (!strcmp (name, "thispage")) {
    return pdf_doc_this_page();
  }
  if (!strcmp (name, "pages")) {
    return pdf_doc_page_tree();
  }
  if (!strcmp (name, "names")) {
    return pdf_doc_names();
  }
  if (!strcmp (name, "resources")) {
    return pdf_doc_current_page_resources();
  }
  if (!strcmp (name, "catalog")) {
    return pdf_doc_catalog();
  }

  for (i=0; i<number_named_references; i++) {
    if (!strcmp (named_references[i].name, name)) {
      break;
    }
  }
  if (i == number_named_references)
    return NULL;
  else if (named_references[i].object == NULL)
    WARN("lookup_object: Referenced object not defined or already closed.");
  return (named_references[i].object);
}

static char *lookup_ref_res_name(char *name)
{
  int i;
  for (i=0; i<number_named_references; i++) {
  if (named_references[i].name != NULL)
    if (named_references[i].name != NULL &&
	!strcmp (named_references[i].name, name)) {
      break;
    }
  }
  if (i == number_named_references)
    return NULL;
  if (named_references[i].res_name == NULL)
    WARN("lookup_object: Referenced object not useable as a form!");
  return (named_references[i].res_name);
}

static void release_reference (char *name)
{
  int i;
  for (i=0; i<number_named_references; i++) {
    if (!strcmp (named_references[i].name, name)) {
      break;
    }
  }
  if (i == number_named_references) {
    WARN("release_reference: tried to release nonexistent reference.");
    return;
  }
  if (named_references[i].object != NULL) {
    pdf_release_obj (named_references[i].object);
    named_references[i].object = NULL;
  }
  else
    WARN("release_reference: @%s: trying to close an object twice?", name);
}


void pdf_finish_specials (void)
{
  int i;
  /* Flush out any pending objects that weren't properly closeed.
     Threads never get closed.  Is this a bug? */
  for (i=0; i<number_named_references; i++) {
    pdf_release_obj (named_references[i].object_ref);
    if (named_references[i].object != NULL) {
      pdf_release_obj (named_references[i].object);
      named_references[i].object = NULL;
    }
    if (named_references[i].res_name != NULL) {
      RELEASE (named_references[i].res_name);
      named_references[i].res_name = NULL;
    }
    RELEASE (named_references[i].name);
  }
  if (number_named_references > 0)
    RELEASE (named_references);
}

int pdf_parse_special (char *buffer, UNSIGNED_QUAD size, double x_user, double y_user)
{
  char *start = buffer, *end = buffer + size;

  if (is_pdf_special(&start, end)) {
    switch (parse_pdfmark(&start, end)) {
    case ANN:
      do_ann(&start, end);
      break;
    case BANN:
      do_bann(&start, end);
      break;
    case LINK_ANNOT:
      dev_link_annot(1);
      break;
    case NOLINK_ANNOT:
      dev_link_annot(0);
      break;
    case EANN:
      do_eann(&start, end);
      break;
    case OUTLINE:
      do_outline(&start, end);
      break;
    case ARTICLE:
      do_article(&start, end);
      break;
    case BEAD:
      do_bead(&start, end);
      break;
    case DEST:
      do_dest(&start, end);
      break;
    case DOCINFO:
      do_docinfo(&start, end);
      break;
    case DOCVIEW:
      do_docview(&start, end);
      break;
    case OBJ:
      do_obj(&start, end);
      break;
    case CONTENT:
      do_content(&start, end, x_user, y_user);
      break;
    case PUT:
      do_put(&start, end);
      break;
    case CLOSE:
      do_close(&start, end);
      break;
    case BOP:
      do_bop(&start, end);
      break;
    case EOP:
      do_eop(&start, end);
      break;
    case EPDF:
      do_image(&start, end, x_user, y_user);
      break;
    case IMAGE:
      do_image(&start, end, x_user, y_user);
      break;
    case BGCOLOR:
      if (!ignore_colors) do_bgcolor(&start, end);
      break;
    case SCOLOR:
      if (!ignore_colors) do_scolor(&start, end);
      break;
    case BCOLOR:
      if (!ignore_colors) do_bcolor(&start, end);
      break;
    case ECOLOR:
      if (!ignore_colors) do_ecolor();
      break;
    case BGRAY:
      do_bgray(&start, end);
      break;
    case EGRAY:
      do_egray();
      break;
    case BXFORM:
      do_bxform(&start, end, x_user, y_user);
      break;
    case EXFORM:
      do_exform();
      break;
    case PAGE_SIZE:
      do_pagesize(&start, end);
      break;
    case BXOBJ:
      do_bxobj(&start, end, x_user, y_user);
      break;
    case EXOBJ:
      do_exobj(&start, end);
      break;
    case UXOBJ:
      do_uxobj(&start, end, x_user, y_user);
      break;
    case TOUNICODE:
      do_tounicode(&start, end);
      break;
    case LITERAL:
      do_literal(&start, end, x_user, y_user);
      break;
    case FSTREAM:
      do_fstream(&start, end);
      break;
    default:
      dump(start, end);
      WARN("Invalid pdf special ignored.");
      break;
    }
    skip_white(&start, end);
    if (start < end) {
      WARN("Unparsed material at end of special ignored.");
      dump(start, end);
    }
    return 1;
  } else
    return 0;
}

/* Compute a transformation matrix
   transformations are applied in the following
   order: scaling, rotate, displacement. */
void add_xform_matrix (double xoff, double yoff,
		       double xscale, double yscale,
		       double rotate) 
{
  double c, s;
  c = ROUND(cos(rotate),1e-5);
  s = ROUND(sin(rotate),1e-5);
  sprintf (work_buffer, " %g %g %g %g %.2f %.2f cm",
	   c*xscale, s*xscale, -s*yscale, c*yscale, xoff, yoff);
  pdf_doc_add_to_page (work_buffer, strlen(work_buffer));
}


void pdf_scale_image (struct xform_info *p)
{
  double xscale = 1.0, yscale = 1.0, nat_width, nat_height;
  if (p->user_bbox) {  /* Did user override natural bounding box */
    nat_width  = p->u_urx - p->u_llx;
    nat_height = p->u_ury - p->u_lly;
  } else { 	       /* If not, use media width and height */
    nat_width  = p->c_urx - p->c_llx;
    nat_height = p->c_ury - p->c_lly;
    p->u_llx = 0.0;    /* Initialize u_llx and u_lly because they
    p->u_lly = 0.0;       are used to set the origin within the crop
			  area */
  }
  if (p->clip && p->user_bbox) {  /* Clip to user specified bbox? */
    p->c_urx = p->u_urx;
    p->c_ury = p->u_ury;
    p->c_llx = p->u_llx;
    p->c_lly = p->u_lly;
  }
  if (p->scale != 0) {
    xscale = p->scale;
    yscale = p->scale;
  }
  if (p->xscale != 0) {
    xscale = p->xscale;
  }
  if (p->yscale != 0) {
    yscale = p->yscale;
  }
  if (p->width != 0.0 && nat_width != 0.0) {
    xscale = p->width/nat_width;
    if (p->height == 0.0)
      yscale = xscale;
  }
  if (p->height != 0.0 && nat_height != 0.0) {
    yscale = p->height/nat_height;
    if (p->width == 0.0)
      xscale = yscale;
  }
  /* We overwrite p->xscale and p->yscale to pass values back to
     caller to user */
  p->xscale = xscale;
  p->yscale = yscale;
  return;
}

/* Compute new xscale and yscale based on user specified values
   and image dimensions (recall that PDF changes dimensions of all
   images to 1x1 :-( */
static void compute_scales (struct xform_info *p, int width, int height)
{
  /* Following routine sets xscale and yscale to a fraction of their natural values */
  p->c_llx = 0;
  p->c_lly = 0;
  p->c_urx = width*(72.0/100.0);
  p->c_ury = height*(72.0/100.0);
  if (p->user_bbox) {
    WARN("Ignoring user specified bounding box for raster image.");
    p->user_bbox = 0;
  }
  pdf_scale_image(p);
  /* Since bitmapped images are always 1x1 in PDF, we must rescale again */
  p->xscale *= width*(72.0/100.0);
  p->yscale *= height*(72.0/100.0);
}

static void finish_image (pdf_obj *image_res, struct xform_info *p, char *res_name, int *p_width, int *p_height)
{
  pdf_obj *image_dict;

  image_dict = pdf_stream_dict (image_res);
  pdf_add_dict (image_dict, pdf_new_name ("Name"),
		pdf_new_name (res_name));
  pdf_add_dict (image_dict, pdf_new_name ("Type"),
		pdf_new_name ("XObject"));
  pdf_add_dict (image_dict, pdf_new_name ("Subtype"),
		pdf_new_name ("Image"));

  *p_width = (int)pdf_number_value(pdf_lookup_dict(image_dict, "Width"));
  *p_height = (int)pdf_number_value(pdf_lookup_dict(image_dict, "Height"));
  compute_scales(p, *p_width, *p_height);
}

static void do_bxobj (char **start, char *end, double x_user, double y_user)
{
  char *objname = NULL;
  static unsigned long num_forms = 0;
  pdf_obj *xobject = NULL;
  struct xform_info *p = NULL;
  int errors = 0;
  skip_white(start, end);
  /* If there's an object name, check dimensions */
  if ((objname = parse_opt_ident(start, end)) != NULL) {
    p = new_xform_info ();
    skip_white(start, end);
    if (!parse_dimension(start, end, p)) {
      WARN("Failed to find a valid dimension.");
      errors = 1;
    }
    if (p->scale != 0.0 || p->xscale != 0.0 || p->yscale != 0.0) {
      WARN("Scale information is meaningless for form xobjects.");
      errors = 1;
    }
#if 0
    /* Removed because ConTeXt sometimes uses an empty object. */
    if (p->width == 0.0 || p->depth+p->height == 0.0) {
      WARN("Special: bxobj: Bounding box has a zero dimension.");
      WARN("width:%g, height:%g, depth:%g", p->width, p->height, p->depth);
      errors = 1;
    }
#endif

    /* If there's an object name and valid dimension, add it to the
       tables */
    if (!errors) {
      char *res_name;
      sprintf (work_buffer, "Fm%ld", ++num_forms);
      res_name = NEW (strlen(work_buffer)+1, char);
      strcpy (res_name, work_buffer);
      xobject = begin_form_xobj (x_user, y_user,
				 x_user, y_user-p->depth,
				 x_user+p->width, y_user+p->height,
				 res_name);
      add_reference (objname, xobject, res_name);
      RELEASE (res_name);
    /* Next line has Same explanation as for do_ann. */
      release_reference (objname);
    }
    release_xform_info (p);
    RELEASE (objname);
  }
  else {
    WARN("Special: beginxobj:  A form XObject must be named.");
  }
  return;
}


static void do_exobj (char **start, char *end)
{
  skip_white(start, end);
  end_form_xobj(*start < end ? parse_pdf_dict(start, end) : NULL);
}

static void do_uxobj (char **start, char *end, double x_user, double y_user)
{
  char *objname, *res_name = NULL;
  pdf_obj *xobj_res = NULL;
  skip_white (start, end);
  if (((objname = parse_opt_ident(start, end)) != NULL) &&
      ((res_name = lookup_ref_res_name (objname)) != NULL) &&
      ((xobj_res = lookup_reference (objname)) != NULL)) {
    sprintf (work_buffer, " q 1 0 0 1 %.2f %.2f cm /%s Do Q", x_user, y_user, res_name);
    pdf_doc_add_to_page (work_buffer, strlen(work_buffer));
    pdf_doc_add_to_page_xobjects (res_name, xobj_res);
  }
  if (objname != NULL && (res_name == NULL || xobj_res == NULL)) {
    WARN("Special: usexobj:  Specified XObject \"%s\" doesn't exist.", objname);
  }
  if (objname != NULL)
    RELEASE (objname);
  return;
}
