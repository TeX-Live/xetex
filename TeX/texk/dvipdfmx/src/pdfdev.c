/*  $Header: /home/cvsroot/dvipdfmx/src/pdfdev.c,v 1.24 2004/02/12 12:20:21 hirata Exp $
    
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "system.h"
#include "mem.h"
#include "error.h"
#include "mfileio.h"
#include "numbers.h"
#include "dvi.h"
#include "tfm.h"
#include "pdfdev.h"
#include "pdfdoc.h"
#include "pdfobj.h"
#include "pdfresource.h"
#include "type1.h"
#include "type1c.h"
#include "type0.h"
#include "cmap.h"
#include "ttf.h"
#include "pkfont.h"
#include "pdfspecial.h"
#include "pdfparse.h"
#include "tpic.h"
#include "htex.h"
#include "mpost.h"
#include "psspecial.h"
#include "colorsp.h"
#include "pdflimits.h"
#include "twiddle.h"
#include "encodings.h"
#include "agl.h"
#include "fontmap.h"

/* Internal functions */
static void dev_clear_color_stack (void);
static void dev_clear_xform_stack (void);

double hoffset = 72.0, voffset=72.0;

static double dvi2pts = 0.0;

/*
 * Acrobat doesn't seem to like coordinate systems
 * that involve scalings around 0.01, so we use
 * a scaline of 1.0.  In other words, device units = pts
 */ 

/* Following dimensions in virtual device coordinates, which are points */

static double page_width, page_height, base_page_width, base_page_height;
static int page_size_readonly;

void dev_set_page_size (double width, double height)
{
  static int first = 1;
  if (first) {
    page_width = base_page_width = width;
    page_height = base_page_height = height;
    first = 0;
  } else if (!page_size_readonly) {
    page_width  = width;
    page_height = height;
  }
}

double dev_page_width(void)
{
  page_size_readonly = 1;
  return page_width;
}

double dev_page_height(void)
{
  page_size_readonly = 1;
  return page_height;
}

double dev_base_page_width(void)
{
  return base_page_width;
}

double dev_base_page_height(void)
{
  return base_page_height;
}

static unsigned char verbose = 0;

void pdf_dev_set_verbose (void)
{
  if (verbose < 255) verbose++;
}

#define GRAPHICS_MODE 1
#define TEXT_MODE     2
#define STRING_MODE   3

int motion_state = GRAPHICS_MODE; /* Start in graphics mode */

#define FORMAT_BUF_SIZE 4096
static char format_buffer[FORMAT_BUF_SIZE];

/*
 * The coordinate system in the pdf file is setup so that 1 unit in the
 * PDF content stream's coordinate system represents 65,800 DVI units.
 * This choice was made so that a PDF coordinate represented only
 * to the hundredths place represents an exact integer number of DVI units.
 * Doing so allows relative motions in a PDF file to be known
 * precisely in DVI units, and allows us to keep track of relative motions
 * using integer arithmetic.  Relative motions in the PDF file are
 * represented in decimal with no more than two digits after the decimal
 * point.  In the PDF stream, a PDF user coordinate of 0.01 represents
 * exactly 658 DVI units.
 *
 * The "pdfdev" module is the only module that knows the
 * relationship between PDF units and true points.  It provides
 * pdf_dev_scale() to inform other modules of the scale.
 * Modules that render PDF marking operators (such as those
 * that render tpic specials or PS specials) need this value.
 * The overhead of this call is a slight performance hit for
 * rendering images, but allows dvipdfm to set text blazingly fast
 *
 * Some constants related to this representation follow:
 */

/*
 * pdf_dev_scale() returns the factor by which a PDF unit
 * must be multiplied to produce an Adobe point (big point)
 */
double pdf_dev_scale (void)
{
  return ((double)one_hundred_bp * dvi2pts / 100.);
}

/*
 * In PDF, vertical text positioning is always applied when current font is
 * vertical font. While Ascii pTeX manages current writing direction and font
 * WMode separately. The variable text_wmode keeps both writing direction and
 * WMode of current font [bit 1: WMode(font), bit 0: direction]:
 *
 * 00/11 HH/VV_MODE h(v) direction with h(v) version of a font
 * 01    HV_MODE    -90 deg. rotated
 * 10    VH_MODE    +90 deg. rotated
 *
 * In MetaPost PostScript file processing (mp_mode = 1), only HH/VV mode is
 * applied.
 */
#define HH_MODE 0
#define HV_MODE 1
#define VH_MODE 2
#define VV_MODE 3

#define WMODE_CHANGES(p,q) ((abs(p-q) % 3) ? 1 : 0)
#define VERTFONT(p) (p & 0x02)
#define VERTDIR(p)  (p & 0x01)

/*
 * Device coordinates are relative to upper left of page.  One of the
 * first things appearing in the page stream is a coordinate transformation
 * matrix that forces this to be true.  This coordinate
 * transformation is the only place where the paper size is required.
 * Unfortunately, positive is up, which doesn't agree with TeX's convention.
 */

static struct {
  struct {
    spt_t  x;
    spt_t  y;
  } origin;
  spt_t  offset;
  struct {
    int    id;
    double slant;
    double extend;
    int    wmode;
  } font;
  int    wmode;
  int    wmode_save;
  int    force_reset;
  int    mp_mode;
  int    dir;
  int    is_mb;
} text_state = {
  {0, 0},
  0,
  { 0.0, 1.0, -1, 0},
  HH_MODE, HH_MODE,
  0,
  0,
  0,
  0
};

int num_dev_fonts = 0, num_phys_fonts = 0;

#define PHYSICAL 1
#define VIRTUAL  2

/*
 * TYPE1C should be TYPE1...
 */
#define TYPE1    1
#define PK       2
#define TRUETYPE 3
#define TYPE0    4
#define TYPE1C   5

static struct dev_font {
  char     short_name[7]; /* Needs to be big enough to hold name "Fxxx"
			     where xxx is number of largest font */
  int      used_on_this_page;
  char    *tex_name;
  spt_t    sptsize;

  int      font_id;
  pdf_obj *resource;
  int      format;
  char    *used_chars;

  double   extend;
  double   slant;
  int      remap;
  int      mapc;
  int      wmode; /* WMode, horizontal: 0, vertical: 1 */
  int      cmap;  /* cmap id */
} *dev_font = NULL;

static int max_device_fonts = 0;

void dev_fonts_need (int n)
{
  if (n > max_device_fonts) {
    max_device_fonts = MAX (max_device_fonts+MAX_FONTS, n);
    dev_font = RENEW (dev_font, max_device_fonts, struct dev_font);
  }
}

static void set_text_matrix (spt_t xpos, spt_t ypos,
                             double slant, double extend)
{
  int len = 0;

  switch(text_state.wmode) {
  case VH_MODE:
    len += sprintf (format_buffer+len, " 0 %.7g -1 %.3g ", extend, slant);
    break;
  case HV_MODE:
    len += sprintf (format_buffer+len, " 0 %.7g 1 %.3g ", -extend, -slant);
    break;
  default: /* normal mode */
    len += sprintf (format_buffer+len, " %.7g 0 %.3g 1 ", extend, slant);
  }
  len += pdf_print_bp(format_buffer+len, xpos);
  format_buffer[len++] = ' ';
  len += pdf_print_bp(format_buffer+len, ypos);
  format_buffer[len++] = ' ';
  format_buffer[len++] = 'T';
  format_buffer[len++] = 'm';

  pdf_doc_add_to_page (format_buffer, len);
  return;
}

/*
 * reset_text_state() outputs a BT and does any necessary coordinate
 * transformations to get ready to ship out text.
 */

static void reset_text_state (void)
{
  text_state.origin.x = 0;
  text_state.origin.y = 0;
  text_state.offset   = 0;

  /* We need to reset the line matrix to handle slanted fonts */
  pdf_doc_add_to_page(" BT", 3);
  /* text_slant = 0 and text_extend = 1 at top of page */
  if (text_state.force_reset ||
      (text_state.font.slant != 0.0 || text_state.font.extend != 1.0) ||
      (text_state.wmode != HH_MODE && text_state.wmode != VV_MODE))
    set_text_matrix(text_state.origin.x, text_state.origin.y,
		    text_state.font.slant, text_state.font.extend);
  text_state.force_reset = 0;
}

static void text_mode (void)
{
  switch (motion_state) {
  case STRING_MODE:
    pdf_doc_add_to_page(text_state.is_mb ? ">]TJ" : ")]TJ", 4);
  case TEXT_MODE:
    break;
  case GRAPHICS_MODE:
    reset_text_state();
    break;
  }
  motion_state      = TEXT_MODE;
  text_state.offset = 0;
}

void graphics_mode (void)
{
  switch (motion_state) {
  case GRAPHICS_MODE:
    break;
  case STRING_MODE:
    pdf_doc_add_to_page(text_state.is_mb ? ">]TJ" : ")]TJ", 4);
  case TEXT_MODE:
    pdf_doc_add_to_page(" ET", 3);
    text_state.force_reset  = 0;
    text_state.font.id = -1;
    break;
  }
  motion_state = GRAPHICS_MODE;
}

static void string_mode (spt_t xpos, spt_t ypos, double slant, double extend)
{
  spt_t delx, dely, desired_delx, desired_dely, scaled_delx, scaled_dely;

  switch (motion_state) {
  case STRING_MODE:
    break;
  case GRAPHICS_MODE:
    reset_text_state();
  case TEXT_MODE:
    if (text_state.force_reset) {
      text_state.origin.x = xpos;
      text_state.origin.y = ypos;
      set_text_matrix(xpos, ypos, slant, extend);
      pdf_doc_add_to_page(text_state.is_mb ? "[<" : "[(", 2);
      text_state.force_reset = 0;
    } else {
      register int len = 0;
      delx = xpos - text_state.origin.x;
      dely = ypos - text_state.origin.y;
      format_buffer[len++] = ' ';
      switch (text_state.wmode) { /* precompensating for line transformation matrix */
      case HV_MODE:
        desired_delx = delx;
	desired_dely = (spt_t)((dely+desired_delx*slant)/extend);
	len += pdf_print_bp(format_buffer+len, -desired_dely);
	scaled_dely = -scaled_out;
	format_buffer[len++] = ' ';
	len += pdf_print_bp(format_buffer+len, desired_delx);
	scaled_delx = scaled_out;
	break;
      case VH_MODE:
	desired_delx = delx;
	desired_dely = (spt_t)((dely+desired_delx*slant)/extend);
	len += pdf_print_bp(format_buffer+len, desired_dely);
	scaled_dely = scaled_out;
	format_buffer[len++] = ' ';
	len += pdf_print_bp(format_buffer+len, -desired_delx);
	scaled_delx = -scaled_out;
	break;
      default: /* HH and VV */
	desired_dely = dely;
	desired_delx = (spt_t)((delx-desired_dely*slant)/extend);
	len += pdf_print_bp(format_buffer+len, desired_delx);
	scaled_delx = scaled_out;
	format_buffer[len++] = ' ';
	len += pdf_print_bp(format_buffer+len, desired_dely);
	scaled_dely = scaled_out;
      }
      pdf_doc_add_to_page(format_buffer, len);
      pdf_doc_add_to_page(text_state.is_mb ? " TD[<" : " TD[(", 5);
      /* Estimate errors in DVI units */
      text_state.origin.x = xpos - (desired_delx - scaled_delx);
      text_state.origin.y = ypos - (desired_dely - scaled_dely);
      text_state.offset   = 0;
    }
    break;
  }
  motion_state = STRING_MODE;
}

/*
 * The purpose of the following routine is to force a Tf only
 * when it's actually necessary.  This became a problem when the
 * VF code was added.  The VF spec says to instantiate the
 * first font contained in the VF file before drawing a virtual
 * character.  However, that font may not be used for
 * many characters (e.g. small caps fonts).  For this reason, 
 * dev_select_font() should not force a "physical" font selection.
 * This routine prevents a PDF Tf font selection until there's
 * really a character in that font.
 */

static void dev_set_font (int font_id)
{
  struct dev_font *font;
  int len = 0, prev_wmode = text_state.wmode;

  font = &(dev_font[font_id]);
  text_mode();

  text_state.is_mb = (font->format == TYPE0) ? 1 : 0;
  if (font->wmode == 0) {
    if (text_state.mp_mode)
      text_state.wmode = HH_MODE;
    else
      text_state.wmode &= 1;
  } else if (font->wmode == 1) {
    if (text_state.mp_mode)
      text_state.wmode = VV_MODE;
    else
      text_state.wmode |= 2;
  }
  len  = sprintf(format_buffer, "/%s ",   font->short_name);
  len += pdf_print_bp(format_buffer+len, font->sptsize);
  format_buffer[len++] = ' ';
  format_buffer[len++] = 'T';
  format_buffer[len++] = 'f';
  if (font->slant  != text_state.font.slant  ||
      font->extend != text_state.font.extend ||
      WMODE_CHANGES(prev_wmode, text_state.wmode)) {
    text_state.font.slant  = font->slant;
    text_state.font.extend = font->extend;
    text_state.force_reset = 1;
  }
  pdf_doc_add_to_page (format_buffer, len);

  if (!font->resource) {
    switch (font->format) {
    case TYPE0:
      font->resource   = type0_font_resource(font->font_id);
      font->used_chars = type0_font_used(font->font_id);
      break;
    case TYPE1:
      font->resource   = type1_font_resource(font->font_id);
      font->used_chars = type1_font_used(font->font_id);
      break;
    case TYPE1C:
      font->resource   = type1c_font_resource(font->font_id);
      font->used_chars = type1c_font_used(font->font_id);
      break;
    case TRUETYPE:
      font->resource   = ttf_font_resource(font->font_id);
      font->used_chars = ttf_font_used(font->font_id);
      break;
    case PK:
      font->resource   = pk_font_resource(font->font_id);
      font->used_chars = pk_font_used(font->font_id);
      break;
    default:
      ERROR("Impossible font format in dev_locate_font().");
    }
  }
  if (!font->used_on_this_page) { 
    pdf_doc_add_to_page_fonts(font->short_name, pdf_link_obj(font->resource));
    font->used_on_this_page = 1;
  }

  text_state.font.id = font_id;

  return;
}

#define WBUF_SIZE 1024
void dev_set_string (spt_t xpos, spt_t ypos, unsigned char *s,
                     int length, spt_t width, int font_id, int ctype)
{
  int i, len = 0;
  spt_t kern = 0;
  spt_t text_xorigin = text_state.origin.x;
  spt_t text_yorigin = text_state.origin.y;
  
  /* Force a Tf since we are actually trying to write a character */
  if (font_id != text_state.font.id)
    dev_set_font(font_id);

  if (dev_font[font_id].format == TYPE0) {
    unsigned char wbuf1[WBUF_SIZE], wbuf2[WBUF_SIZE];
    if (ctype == 1 && dev_font[font_id].mapc >= 0) {
      /* Translate single-byte chars to double byte code space */
      for (i = 0; i < length; i++) {
	wbuf1[i*2]   = (unsigned char) dev_font[font_id].mapc;
	wbuf1[i*2+1] = s[i];
      }
      length *= 2;
      s       = wbuf1;
    }
    /*
     * Font is double-byte font. Output is assumed to be 16-bit fixed length encoding.
     * TODO: A character decomposed to multiple characters.
     */
    if (dev_font[font_id].cmap >= 0) {
      unsigned char *inbuf = s, *outbuf = wbuf2;
      long inbytesleft = length, outbytesleft = WBUF_SIZE;
      CMap *cmap = CMap_cache_get(dev_font[font_id].cmap);
      CMap_decode(cmap, (const unsigned char **) &inbuf, &inbytesleft, &outbuf, &outbytesleft);
      if (inbytesleft != 0)
	ERROR("Code conversion failed. (%d bytes remains)", inbytesleft, length);
      length = WBUF_SIZE - outbytesleft;
      s      = wbuf2;
    }
    if (dev_font[font_id].used_chars != NULL) {
      for (i = 0; i < length; i += 2)
	add_to_used_chars2(dev_font[font_id].used_chars, (s[i] << 8)| s[i+1]);
    }
  } else if (dev_font[font_id].used_chars != NULL) {
    /* Record characters used for partial font embedding.
     * Fonts without pfbs don't get counted and have used_chars set to null. */
    if (dev_font[font_id].remap)
      for (i = 0; i < length; i++)
        (dev_font[font_id].used_chars)[twiddle(s[i])] = 1;
    else 
      for (i = 0; i < length; i++)
        (dev_font[font_id].used_chars)[s[i]] = 1;
  }

  /*
   * Kern is in units of character units, i.e., 1000 = 1 em.
   *
   * The following formula is of the form a*x/b where a, x, and b are
   * signed long integers.  Since in integer arithmetic (a*x) could overflow
   * and a*(x/b) would not be accurate, we use floating point
   * arithmetic rather than trying to do this all with integer arithmetic.
   */
  if (VERTDIR(text_state.wmode)) {
    kern = (spt_t)((1000.0/dev_font[font_id].extend*(ypos-text_yorigin+text_state.offset))/dev_font[font_id].sptsize);
    if (text_state.force_reset ||
	labs(xpos-text_xorigin) > centi_min_bp_val ||
	abs(kern) > 32000) { /* Some PDF Readers fail on large kerns */
      text_mode();
      kern = 0;
    }
  } else {
    kern = (spt_t)((1000.0/dev_font[font_id].extend*(text_xorigin+text_state.offset-xpos))/dev_font[font_id].sptsize);
    if (text_state.force_reset ||
	labs(ypos-text_yorigin) > centi_min_bp_val ||
	abs(kern) > 32000) { /* Some PDF Readers fail on large kerns */
      text_mode();
      kern = 0;
    } 
  }

  if (motion_state != STRING_MODE)
    string_mode(xpos, ypos, dev_font[font_id].slant, dev_font[font_id].extend);
  else if (kern != 0) {
    text_state.offset -=
      (spt_t)(kern*dev_font[font_id].extend*(dev_font[font_id].sptsize/1000.0));
    /* Same issues as earlier. Use floating point for simplicity.
     * This routine needs to be fast, so we don't call sprintf() or
     * strcpy(). */
    format_buffer[len++] = text_state.is_mb ? '>' : ')';
    if (VERTFONT(text_state.wmode)) /* WMode == 1 */
      len += pdf_print_int(format_buffer+len, -kern);
    else
      len += pdf_print_int(format_buffer+len, kern);
    format_buffer[len++] = text_state.is_mb ? '<' : '(';
    pdf_doc_add_to_page(format_buffer, len);
    len = 0;
  }

  if (!text_state.is_mb)
    len += pdfobj_escape_str (format_buffer+len, FORMAT_BUF_SIZE-len, s,
			      length, dev_font[font_id].remap);
  else { /* hex string */
    if ((len + 2*length) >= FORMAT_BUF_SIZE)
      ERROR("[dev] set_string: Buffer overflow.\n");
    while (length-- > 0) {
      int first, second;
      first = (*s >> 4) & 0x0f;
      second = *s & 0x0f;
      format_buffer[len++] = ((first >= 10) ? first + 'W' : first + '0');
      format_buffer[len++] = ((second >= 10) ? second + 'W' : second + '0');
      s++;
    }
  }
  pdf_doc_add_to_page(format_buffer, len);

  text_state.offset += width;
}

void dev_init (double scale, double x_offset, double y_offset)
{
  dvi2pts = scale;
  hoffset = x_offset;
  voffset = y_offset;

  graphics_mode();
  dev_clear_color_stack();
  dev_clear_xform_stack();

  PDF_resource_init();
  Encoding_cache_init();
  AGLmap_cache_init();
  CMap_cache_init();
  Type1CFont_cache_init();
  TTFont_cache_init();
  Type0Font_cache_init();
}

void dev_close (void)
{
  /* Set page origin now that user has had plenty of time to set page size. */
  pdf_doc_set_origin((double) hoffset, (double) dev_base_page_height()-voffset);
}

void dev_add_comment (char *comment)
{
  pdf_doc_creator (comment);
}

/* BOP, EOP, and FONT section.
 * BOP and EOP manipulate some of the same data structures
 * as the font stuff. */ 

#define GRAY 1
#define RGB 2
#define CMYK 3
struct color {
  int colortype;
  double c1, c2, c3, c4;
} colorstack[MAX_COLORS], background = {GRAY, 1.0, 1.0, 1.0, 1.0},
    default_color = {GRAY, 0.0, 0.0, 0.0, 0.0};

#include "colors.h"

struct color color_by_name (char *s) 
{
  int i;
  struct color result;
  for (i=0; i<sizeof(colors_by_name)/sizeof(colors_by_name[0]); i++) {
    if (!strcmp (s, colors_by_name[i].name)) {
      break;
    }
  }
  if (i == sizeof(colors_by_name)/sizeof(colors_by_name[0])) {
    WARN("Color \"%s\" no known.  Using \"Black\" instead.", s);
    result = default_color;
  } else {
    result = colors_by_name[i].color;
  }
  return result;
}

static int num_colors = 0;

static void fill_page (void)
{
  if (background.colortype == GRAY && background.c1 == 1.0)
    return;
  switch (background.colortype) {
  case GRAY:
    sprintf (format_buffer, " q 0 w %.3f g %.3f G", background.c1, background.c1);
    break;
  case RGB:
    sprintf (format_buffer, " q 0 w %.3f %.3f %.3f rg %.3f %.3f %.3f RG",
	     background.c1, background.c2, background.c3,
	     background.c1, background.c2, background.c3);
    break;
  case CMYK:
    sprintf (format_buffer, " q 0 w %.3f %.3f %.3f %.3f k %.3f %.3f %.3f %.3f K ",
	     background.c1, background.c2, background.c3, background.c4,
	     background.c1, background.c2, background.c3, background.c4);
    break;
  }
  pdf_doc_this_bop (format_buffer, strlen(format_buffer));
  sprintf (format_buffer,
	   " 0 0 m %.2f 0 l %.2f %.2f l 0 %.2f l b Q ",
	   dev_page_width(), dev_page_width(), dev_page_height(),
	   dev_page_height());
  pdf_doc_this_bop (format_buffer, strlen(format_buffer));
  return;
}

void dev_bg_rgb_color (double r, double g, double b)
{
  background.colortype = RGB;
  background.c1 = r;
  background.c2 = g;
  background.c3 = b;
  return;
}

void dev_bg_cmyk_color (double c, double m, double y, double k)
{
  background.colortype = CMYK;
  background.c1 = c;
  background.c2 = m;
  background.c3 = y;
  background.c4 = k;
  return;
}

void dev_bg_gray (double value)
{
  background.colortype = GRAY;
  background.c1 = value;
  return;
}

void dev_bg_named_color (char *s)
{
  struct color color = color_by_name (s);
  switch (color.colortype) {
  case GRAY:
    dev_bg_gray (color.c1);
    break;
  case RGB:
    dev_bg_rgb_color (color.c1, color.c2, color.c3);
    break;
  case CMYK:
    dev_bg_cmyk_color (color.c1, color.c2, color.c3, color.c4);
    break;
  }
  return;
}

static void dev_clear_color_stack (void)
{
  num_colors = 0;
  return;
}
static void dev_set_color (struct color color)
{
  switch (color.colortype) {
    int len;
  case RGB:
    len = sprintf (format_buffer, " %.2f %.2f %.2f",
		   color.c1,
		   color.c2,
		   color.c3);
    pdf_doc_add_to_page (format_buffer, len);
    pdf_doc_add_to_page (" rg", 3);
    pdf_doc_add_to_page (format_buffer, len);
    pdf_doc_add_to_page (" RG", 3);
    break;
  case CMYK:
    len = sprintf (format_buffer, " %.2f %.2f %.2f %.2f",
		   color.c1,
		   color.c2,
		   color.c3,
		   color.c4);
    pdf_doc_add_to_page (format_buffer, len);
    pdf_doc_add_to_page (" k", 2);
    pdf_doc_add_to_page (format_buffer, len);
    pdf_doc_add_to_page (" K ", 3);
    break;
  case GRAY:
    len = sprintf (format_buffer, " %.2f", color.c1);
    pdf_doc_add_to_page (format_buffer, len);
    pdf_doc_add_to_page (" g", 2);
    pdf_doc_add_to_page (format_buffer, len);
    pdf_doc_add_to_page (" G", 2);
    break;
  default:
    ERROR ("\n**Fatal: Invalid default color item.\n");
  }
}


void dev_do_color (void) 
{
  if (num_colors == 0) {
    dev_set_color (default_color);
  } else {
    dev_set_color (colorstack[num_colors-1]);
  }
  return;
}

void dev_set_def_rgb_color (double r, double g, double b)
{
  default_color.c1 = r;
  default_color.c2 = g;
  default_color.c3 = b;
  default_color.colortype = RGB;
  dev_do_color();
  return;
}

void dev_set_def_gray (double g) 
{
  default_color.c1 = g;
  default_color.colortype = GRAY;
  dev_do_color();
  return;
}

void dev_set_def_named_color (char *s)
{
  struct color color = color_by_name (s);
  switch (color.colortype) {
  case GRAY:
    dev_set_def_gray (color.c1);
    break;
  case RGB:
    dev_set_def_rgb_color (color.c1, color.c2, color.c3);
    break;
  case CMYK:
    dev_set_def_cmyk_color (color.c1, color.c2, color.c3, color.c4);
    break;
  }
  return;
}

void dev_set_def_cmyk_color (double c, double m, double y, double k)
{
  default_color.c1 = c;
  default_color.c2 = m;
  default_color.c3 = y;
  default_color.c4 = k;
  default_color.colortype = CMYK;
  dev_do_color();
  return;
}

void dev_begin_named_color (char *s)
{
  struct color color = color_by_name (s);
  switch (color.colortype) {
  case GRAY:
    dev_begin_gray (color.c1);
    break;
  case RGB:
    dev_begin_rgb_color (color.c1, color.c2, color.c3);
    break;
  case CMYK:
    dev_begin_cmyk_color (color.c1, color.c2, color.c3, color.c4);
    break;
  }
  return;
}

void dev_begin_rgb_color (double r, double g, double b)
{
  if (num_colors >= MAX_COLORS) {
    WARN("Exceeded depth of color stack.");
    return;
  }
  colorstack[num_colors].c1 = r;
  colorstack[num_colors].c2 = g;
  colorstack[num_colors].c3 = b;
  colorstack[num_colors].colortype = RGB;
  num_colors+= 1;
  dev_do_color();
}

void dev_begin_cmyk_color (double c, double m, double y, double k)
{
  if (num_colors >= MAX_COLORS) {
    WARN("Exceeded depth of color stack.");
    return;
  }
  colorstack[num_colors].c1 = c;
  colorstack[num_colors].c2 = m;
  colorstack[num_colors].c3 = y;
  colorstack[num_colors].c4 = k;
  colorstack[num_colors].colortype = CMYK;
  num_colors+= 1;
  dev_do_color();
}

void dev_begin_gray (double value)
{
  if (num_colors >= MAX_COLORS) {
    WARN("Exceeded depth of color stack.");
    return;
  }
  colorstack[num_colors].c1 = value;
  colorstack[num_colors].colortype = GRAY;
  num_colors+= 1;
  dev_do_color();
}

void dev_end_color (void)
{
  if (num_colors <= 0) {
    WARN("End color with no corresponding begin color.");
    return;
  }
  num_colors -= 1;
  dev_do_color();
}

static int num_transforms = 0;

static void dev_clear_xform_stack (void)
{
  num_transforms = 0;
}

void dev_begin_xform (double xscale, double yscale, double rotate,
                      double x_user, double y_user)
{
  int len;

  if (num_transforms < MAX_TRANSFORMS) {
    double c = ROUND (cos(rotate), 1e-5);
    double s = ROUND (sin(rotate), 1e-5);
    len = sprintf(work_buffer, " q %g %g %g %g ",
                  xscale * c, xscale * s, -yscale * s, yscale * c);
    len += pdf_print_pt(work_buffer+len,
                  (1.0 - xscale * c) * x_user + (yscale * s) * y_user);
    work_buffer[len++] = ' ';
    len += pdf_print_pt(work_buffer+len,
                  (-xscale * s) * x_user + (1.0 - yscale * c) * y_user);
    len += sprintf(work_buffer+len, " cm");
    pdf_doc_add_to_page(work_buffer, len);
    num_transforms += 1;
  } else
    WARN("Exceeded depth of transformation stack.");
}

void dev_end_xform (void)
{
  if (num_transforms > 0) {
    pdf_doc_add_to_page(" Q", 2);
    num_transforms--;
    /* Unfortunately, the following two lines are necessary in case of
       a font or color change inside of the save/restore pair. Anything
       that was done there must be redone, so in effect, we make no
       assumptions about what fonts. We act like we are starting a new page */
    dev_reselect_font();
    dev_do_color();
  } else
    WARN("End transform with no corresponding begin.");
}

int dev_xform_depth (void)
{
  return num_transforms;
}

void dev_close_all_xforms (int depth)
{
  if (num_transforms > depth) {
    WARN("Closing pending transformations at end of page/XObject.");
    while (num_transforms > depth) {
      num_transforms -= 1;
      pdf_doc_add_to_page (" Q", 2);
    }
    dev_reselect_font();
    dev_do_color();
  }
}

/* The following routine is here for forms.  Since
   a form is self-contained, it will need its own Tf command
   at the beginningg even if it is continuing to set type
   in the current font.  This routine simply forces reinstantiation
   of the current font. */
void dev_reselect_font(void)
{
  int i;

  for (i=0; i<num_dev_fonts; i++)
    dev_font[i].used_on_this_page = 0;

  text_state.font.id     = -1;
  text_state.font.slant  = 0.0;
  text_state.font.extend = 1.0;
}

static void bop_font_reset(void)
{
  dev_reselect_font();
}

void dev_bop (void)
{
  page_size_readonly = 0;
  pdf_doc_new_page ();
  graphics_mode();

  text_state.mp_mode      = 0;
  text_state.force_reset  = 0;
  text_state.is_mb        = 0;

  bop_font_reset();
  /* This shouldn't be necessary because line widths are now
     explicitly set for each rule */
  /*  pdf_doc_add_to_page ("0 w", 3); */
  dev_do_color(); /* Set text color since new page loses color state */
}

void dev_eop (void)
{
  graphics_mode();
  dev_close_all_xforms(0);
  fill_page();
  pdf_doc_finish_page ();
  /* Finish any pending PS specials */
  mp_eop_cleanup();
}

int dev_locate_font (const char *tex_name, spt_t ptsize, int map_id)
{
  int i, this_font;

  if (ptsize == 0)
    ERROR ("\n**Fatal: locate_dev_font() called with the zero ptsize.\n");

  for (i = 0; i < num_dev_fonts; i++) {
    if (!strcmp(tex_name, dev_font[i].tex_name)) {
      /* Must match in name and size to resolve to the same device font. */
      if (ptsize == dev_font[i].sptsize)
        return i;
      /* Scaleable fonts must match in name; however, this routine must
         return a different id if the ptsize is different. */
      else if (dev_font[i].format != PK)
        break;
    }
  }

  /*
   * Make sure we have room for a new one, even though we
   * may not actually create one
   */
  dev_fonts_need(num_dev_fonts + 1);
  this_font = num_dev_fonts;

  if (i < num_dev_fonts) {
    /*
     * A previously existing physical font can be used; however, this
     * routine must return a distinct ID if the ptsizes are different.
     * Copy the information from the previous record to the new record.
     */
    strcpy(dev_font[this_font].short_name, dev_font[i].short_name);
    /*
     * The value in used_on_this_page will be incorrect if the font
     * has already been used on a page in a different point size.
     * It's too hard to do right.  The only negative consequence is
     * that there will be an attempt to add the resource to the page
     * resource dict. However, the second attempt will do nothing.
     */
    dev_font[this_font].used_on_this_page = 0;
    dev_font[this_font].tex_name = NEW (strlen(tex_name)+1, char);
    strcpy(dev_font[this_font].tex_name, tex_name);
    dev_font[this_font].sptsize  = ptsize;
    /*
     * These two fonts are treated as having the same physical "used_chars".
     */
    dev_font[this_font].used_chars = dev_font[i].used_chars;
    dev_font[this_font].resource = dev_font[i].resource;
    dev_font[this_font].format   = dev_font[i].format;
    dev_font[this_font].font_id  = dev_font[i].font_id;
    dev_font[this_font].extend = dev_font[i].extend;
    dev_font[this_font].slant  = dev_font[i].slant;
    dev_font[this_font].remap  = dev_font[i].remap;
    dev_font[this_font].mapc   = dev_font[i].mapc;
    dev_font[this_font].wmode  = dev_font[i].wmode;
    dev_font[this_font].cmap   = dev_font[i].cmap;
  } else {
    /*
     * There is no physical font we can use.
     */
    int font_id = -1, font_format = -1;
    int tfm_id = -1,  encoding_id = -1, cmap_id = -1;
    int wmode = 0, remap = 0, mapc = -1;
    double extend, slant;
    char *font_name, *enc_name;
    char  short_name[7];

    /*
     * Get appropriate info from map file. (PK fonts at two different
     * point sizes would be looked up twice unecessarily.)
     */
    font_name = (map_id < 0 ? (char *)tex_name : fontmap_font_name(map_id));
    enc_name = fontmap_enc_name(map_id);
    extend   = fontmap_extend(map_id);
    slant    = fontmap_slant(map_id);
    remap    = fontmap_remap(map_id);
    mapc     = fontmap_mapc (map_id);

    if (verbose > 1) {
      MESG("\n");
      if (map_id >= 0) {
	MESG("fontmap: %s -> %s", tex_name, font_name);
	if (enc_name)
	  MESG("(%s)", enc_name);
	if (extend != 1.0)
	  MESG("[extend=%g]", extend);
	if (slant != 0.0)
	  MESG("[slant=%g]", slant);
	if (remap)
	  MESG("[remap]");
	if (mapc >= 0)
	  MESG("[map=<%02x>]", mapc);
	MESG("\n");
      } else
	MESG("fontmap: %s (no map)\n", tex_name);
    }

    /*
     * If this font has an encoding specified on the record,
     * check whether it is CMap, and otherwise get its encoding ID.
     * We assume that an encoding file without extension is a CMap.
     * The default encoding_id is -1.
     */
    if (enc_name) {
      if (strstr(enc_name, ".enc") == NULL) {
        cmap_id = CMap_cache_find(enc_name);
	if (cmap_id >= 0) {
	  CMap *cmap;
	  int   cmap_type, minbytein;
	  cmap      = CMap_cache_get(cmap_id);
	  cmap_type = CMap_get_type(cmap);
	  minbytein = CMap_get_profile(cmap, CMAP_PROF_TYPE_INBYTES_MIN);
	  /*
	   * Check for output encoding.
	   */
	  if (cmap_type != CMAP_TYPE_IDENTITY    &&
	      cmap_type != CMAP_TYPE_CODE_TO_CID &&
	      cmap_type != CMAP_TYPE_TO_UNICODE) {
	    WARN("Only 16-bit encoding supported for output encoding.");
	  }
	  /*
	   * Turn on map option.
	   */
	  if (minbytein == 2 && mapc < 0) {
	    if (verbose > 3) {
	      MESG("\n");
	      MESG("Input encoding \"%s\" requires at least 2 bytes.", CMap_get_name(cmap));
	      MESG("\n");
	      MESG("The -m <00> option will be assumed for \"%s\".", font_name);
	      MESG("\n");
	    }
	    mapc = 0;
	  }
	}
      }
      if (cmap_id < 0) {
        encoding_id = Encoding_cache_find(enc_name);
	if (encoding_id < 0) {
	  ERROR("Could not find encoding file \"%s\".", enc_name);
	}
      }
    }

    tfm_id = tfm_open(tex_name, 0); /* need not exist */
    /*
     * We assume, for now that we will find this as a physical font,
     * as opposed to a vf, so we need a device name to tell the
     * lower-level routines what we want this to be called.  We'll
     * blast this name away later if we don't need it.
     */
    short_name[0] = 'F';
    pdf_print_int(short_name+1, num_phys_fonts+1);

    if (cmap_id >= 0) {
      /*
       * Composite Font
       */
      if ((font_id = type0_font(font_name, tfm_id,
				short_name, cmap_id, remap)) >= 0) {
	font_format = TYPE0;
        wmode       = type0_font_wmode(font_id);
	remap       = 0;
      } else
	return -1;
    } else {
      /*
       * Simple Font
       */
#ifdef OPENTYPE_PREFERED
      if ((font_id = type1c_font(font_name, tfm_id,
				 short_name, encoding_id, remap)) >= 0)
	font_format = TYPE1C;
      else if ((font_id = ttf_font(font_name, tfm_id,
				   short_name, encoding_id, remap)) >= 0)
	font_format = TRUETYPE;
      else if ((font_id = type1_font(font_name, tfm_id,
				     short_name, encoding_id, remap)) >= 0)
	font_format = TYPE1;
#else
      if ((font_id = type1_font(font_name, tfm_id,
				short_name, encoding_id, remap)) >= 0)
	font_format = TYPE1;
      else if ((font_id = type1c_font(font_name, tfm_id,
				      short_name, encoding_id, remap)) >= 0)
	font_format = TYPE1C;
      else if ((font_id = ttf_font(font_name, tfm_id,
				   short_name, encoding_id, remap)) >= 0)
	font_format = TRUETYPE;
#endif /* OPENTYPE_PREFERED */
      else if ((font_id = pk_font(font_name, ptsize * dvi2pts,
				  tfm_id, short_name)) >=0)
	font_format = PK;
      else
	return -1;
    }

    /*
     * This is a new physical font and we found a physical font we can use.
     */
    strcpy(dev_font[this_font].short_name, short_name);
    dev_font[this_font].used_on_this_page = 0;

    dev_font[this_font].tex_name = NEW (strlen(tex_name)+1, char);
    strcpy(dev_font[this_font].tex_name, tex_name);
    dev_font[this_font].sptsize  = ptsize;

    dev_font[this_font].format   = font_format;
    dev_font[this_font].font_id  = font_id;
    dev_font[this_font].resource = NULL;
    dev_font[this_font].wmode    = wmode;
    dev_font[this_font].cmap     = cmap_id;

    dev_font[this_font].used_chars = NULL;

    if (wmode && slant != 0.0) {
      WARN("Slant option ignored for vertical fonts.");
      slant = 0.0;
    }
    dev_font[this_font].slant  = slant;
    dev_font[this_font].extend = extend;
    dev_font[this_font].remap  = remap;
    dev_font[this_font].mapc   = mapc;
  }

  num_phys_fonts++;
  num_dev_fonts++;

  return this_font;
}
  
void dev_close_all_fonts(void)
{
  int i;
  for (i=0; i<num_dev_fonts; i++) {
    if (verbose > 2 && !dev_font[i].resource)
      MESG("\nFont \"%s\" (id=%d) not used at all.\n", dev_font[i].tex_name, i);
    RELEASE(dev_font[i].tex_name);
  }
  if (dev_font)
    RELEASE (dev_font);

  /* Release all map entries */
  release_map_record();

  /* Close the various font handlers */
  type1_close_all();
  pk_close_all();

  Type1CFont_cache_close();
  TTFont_cache_close();
  Type0Font_cache_close();

  AGLmap_cache_close();
  CMap_cache_close();
  Encoding_cache_close();
  PDF_resource_close();
}

void dev_rule (spt_t xpos, spt_t ypos, spt_t width, spt_t height)
{
  int len = 0;
  spt_t w, p1, p2, p3, p4;

  graphics_mode();
  if (width> height) { /* Horizontal stroke */
    w = height;
    p1 = xpos;
    p2 = ypos + height / 2;
    p3 = xpos + width;
    p4 = ypos + height / 2;
  } else { /* Vertical stroke */
    w = width;
    p1 = xpos + width / 2;
    p2 = ypos;
    p3 = xpos + width / 2;
    p4 = ypos + height;
  }
  /* This needs to be quick */
  format_buffer[len++] = ' ';
  len += pdf_print_bp(format_buffer+len, w);
  format_buffer[len++] = ' ';
  format_buffer[len++] = 'w';
  format_buffer[len++] = ' ';
  len += pdf_print_bp(format_buffer+len, p1);
  format_buffer[len++] = ' ';
  len += pdf_print_bp(format_buffer+len, p2);
  format_buffer[len++] = ' ';
  format_buffer[len++] = 'm';
  format_buffer[len++] = ' ';
  len += pdf_print_bp(format_buffer+len, p3);
  format_buffer[len++] = ' ';
  len += pdf_print_bp(format_buffer+len, p4);
  format_buffer[len++] = ' ';
  format_buffer[len++] = 'l';
  format_buffer[len++] = ' ';
  format_buffer[len++] = 'S';
  pdf_doc_add_to_page(format_buffer, len);
}

/* The following routines tell the coordinates in true Adobe points
   with the coordinate system having its origin at the bottom
   left of the page. */

double dev_phys_x (void)
{
  return dvi_dev_xpos()*dvi_tell_mag() + hoffset;
}

double dev_phys_y (void)
{
  return dev_page_height() + dvi_tell_mag()*dvi_dev_ypos() -voffset;
}

static int src_special (char *buffer, UNSIGNED_QUAD size) {
  char *start = buffer;
  char *end = buffer + size;
  int result = 0;
  skip_white (&start, end);
  if ((start+3 < end) &&
      (!strncmp ("src:", start, 4)))
    result = 1;
  return result;
}

void dev_do_special (void *buffer, UNSIGNED_QUAD size, spt_t x_user, 
		     spt_t y_user)
{
  double dev_xuser, dev_yuser;
  dev_xuser = (double)x_user / (double)one_hundred_bp * 100.;
  dev_yuser = (double)(-y_user) / (double)one_hundred_bp * 100.;
  graphics_mode();
  if (!pdf_parse_special(buffer, size, dev_xuser, dev_yuser) &&
      !tpic_parse_special (buffer, size, dev_xuser, dev_yuser) &&
      !htex_parse_special (buffer, size) &&
      !color_special (buffer, size) &&
      !ps_parse_special (buffer, size, dev_xuser, dev_yuser) &&
      !src_special (buffer, size)) {
    WARN("Unrecognized special ignored.");
    if (verbose)
      dump(buffer, ((char *)buffer)+size);
  }
}

static unsigned dvi_stack_depth = 0;
static int dvi_tagged_depth = -1;
static unsigned char link_annot = 1;

void dev_link_annot (unsigned char flag)
{
  link_annot = flag;
}

void dev_stack_depth (unsigned int depth)
{
  /* If decreasing below tagged_depth */
  if (link_annot && 
      dvi_stack_depth == dvi_tagged_depth &&
      depth == dvi_tagged_depth - 1) {
  /* See if this appears to be the end of a "logical unit"
     that's been broken.  If so, flush the logical unit */
    pdf_doc_flush_annot();
  }
  dvi_stack_depth = depth;
  return;
}

/* The following routines setup and tear down a callback at
   a certain stack depth.  This is used to handle broken (linewise)
   links */

void dev_tag_depth (void)
{
  dvi_tagged_depth = dvi_stack_depth;
  dvi_compute_boxes (1);
  return;
}

void dev_untag_depth (void)
{
  dvi_tagged_depth = -1;
  dvi_compute_boxes (0);
  return;
}

void dev_expand_box (spt_t width, spt_t height, spt_t depth)
{
  double phys_width, phys_height, phys_depth, scale;
  if (link_annot && dvi_stack_depth >= dvi_tagged_depth) {
    scale = dvi2pts*dvi_tell_mag();
    phys_width = scale*width;
    phys_height = scale*height;
    phys_depth = scale*depth;
    if (! VERTDIR(text_state.wmode)) {
      pdf_doc_expand_box (dev_phys_x(), dev_phys_y()-phys_depth,
			  dev_phys_x()+phys_width,
			  dev_phys_y()+phys_height);
    } else {
      pdf_doc_expand_box (dev_phys_x()-phys_depth, dev_phys_y()-phys_width,
			  dev_phys_x()+phys_height,
			  dev_phys_y());
    }
  }
}

int dev_wmode (void)
{
  return VERTDIR(text_state.wmode);
}

void dev_set_wmode (int dir)
{
  int prev_wmode = text_state.wmode;

  if (dir) {
    text_state.wmode |= 1;
  } else {
    text_state.wmode &= 2;
  }
  if (WMODE_CHANGES(prev_wmode, text_state.wmode))
    text_state.force_reset = 1;
}

void dev_start_mp_mode (void) /* MetaPost mode (or PostScript mode) */
{
  text_state.wmode_save = text_state.wmode;
  text_state.wmode      = HH_MODE;
  text_state.mp_mode    = 1;
}

void dev_end_mp_mode (void)
{
  text_state.wmode   = text_state.wmode_save;
  text_state.mp_mode = 0;
}
