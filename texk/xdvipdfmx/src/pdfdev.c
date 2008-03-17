/*  $Header: /home/cvsroot/dvipdfmx/src/pdfdev.c,v 1.64 2007/11/27 02:44:29 chofchof Exp $
    
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
#include <math.h>

#include "system.h"
#include "mem.h"
#include "error.h"

#include "mfileio.h"
#include "numbers.h"

#include "pdfdoc.h"
#include "pdfobj.h"

#include "pdffont.h"
#include "fontmap.h"
#include "cmap.h"
#include "pdfximage.h"

#include "pdfdraw.h"
#include "pdfcolor.h"

#include "pdflimits.h"

#include "pdfdev.h"

static int verbose = 0;

void
pdf_dev_set_verbose (void)
{
  verbose++;
}

/* Not working yet... */
double
pdf_dev_scale (void)
{
  return 1.0;
}

/*
 * Unit conversion, formatting and others.
 */

#define TEX_ONE_HUNDRED_BP 6578176
static struct {
  double dvi2pts;
  long   min_bp_val; /* Shortest resolvable distance in the output PDF.     */
  int    precision;  /* Number of decimal digits (in fractional part) kept. */
} dev_unit = {
  0.0,
  658,
  2
};


double
dev_unit_dviunit (void)
{
  return (1.0/dev_unit.dvi2pts);
}

#define DEV_PRECISION_MAX  8
static unsigned long ten_pow[10] = {
  1ul, 10ul, 100ul, 1000ul, 10000ul, 100000ul, 1000000ul, 10000000ul, 100000000ul, 1000000000ul
};

static double ten_pow_inv[10] = {
  1.0, 0.1,  0.01,  0.001,  0.0001,  0.00001,  0.000001,  0.0000001,  0.00000001,  0.000000001
};

#define bpt2spt(b) ( (spt_t) round( (b) / dev_unit.dvi2pts  ) )
#define spt2bpt(s) ( (s) * dev_unit.dvi2pts )
#define dround_at(v,p) (ROUND( (v), ten_pow_inv[(p)] ))

static int
p_itoa (long value, char *buf)
{
  int   sign, ndigits;
  char *p = buf;

  if (value < 0) {
    *p++  = '-';
    value = -value;
    sign  = 1;
  } else {
    sign  = 0;
  }

  ndigits = 0;
  /* Generate at least one digit in reverse order */
  do {
    p[ndigits++] = (value % 10) + '0';
    value /= 10;
  } while (value != 0);

  /* Reverse the digits */
  {
    int i;

    for (i = 0; i < ndigits / 2 ; i++) {
      char tmp = p[i];
      p[i] = p[ndigits-i-1];
      p[ndigits-i-1] = tmp;
    }
  }
  p[ndigits] = '\0';

  return  (sign ? ndigits + 1 : ndigits);
}

/* ... */
static int
p_dtoa (double value, int prec, char *buf)
{
  int    n;
  char  *p, *q;

  n = sprintf(buf, "%.*f", prec, value);
  /* find decimal-point */
  for (p = buf + n - 1; p > buf && *p != '.'; p--);
  if (p > buf) {
    /* chop trailing zeros */
    for (q = buf + n - 1; q > p && *q == '0'; q--) {
      *q = '\0'; n--;
    }
    /* If a decimal point appears, at least one digit appears
     * before it.
     */
    if (q == p) {
      *q = '\0'; n--;
    }
  }
  /* -0 --> 0 */
  if (n == 2 && buf[0] == '-' && buf[1] == '0') {
    buf[0] = '0'; buf[1] = '\0'; n = 1;
  }

  return  n;
}

static int
dev_sprint_bp (char *buf, spt_t value, spt_t *error)
{
  double  value_in_bp;
  double  error_in_bp;
  int     prec = dev_unit.precision;

  value_in_bp = spt2bpt(value);
  if (error) {
    error_in_bp = value_in_bp - dround_at(value_in_bp, prec);
    *error = bpt2spt(error_in_bp);
  }

  return  p_dtoa(value_in_bp, prec, buf);
}

/* They are affected by precision (set at device initialization). */
int
pdf_sprint_matrix (char *buf, const pdf_tmatrix *M)
{
  int  len;
  int  prec2 = MIN(dev_unit.precision + 2, DEV_PRECISION_MAX);
  int  prec0 = MAX(dev_unit.precision, 2);

  len  = p_dtoa(M->a, prec2, buf);
  buf[len++] = ' ';
  len += p_dtoa(M->b, prec2, buf+len);
  buf[len++] = ' ';
  len += p_dtoa(M->c, prec2, buf+len);
  buf[len++] = ' ';
  len += p_dtoa(M->d, prec2, buf+len);
  buf[len++] = ' ';
  len += p_dtoa(M->e, prec0, buf+len);
  buf[len++] = ' ';
  len += p_dtoa(M->f, prec0, buf+len);
  buf[len]   = '\0'; /* xxx_sprint_xxx NULL terminates strings. */

  return  len;
}

int
pdf_sprint_rect (char *buf, const pdf_rect *rect)
{
  int  len;

  len  = p_dtoa(rect->llx, dev_unit.precision, buf);
  buf[len++] = ' ';
  len += p_dtoa(rect->lly, dev_unit.precision, buf+len);
  buf[len++] = ' ';
  len += p_dtoa(rect->urx, dev_unit.precision, buf+len);
  buf[len++] = ' ';
  len += p_dtoa(rect->ury, dev_unit.precision, buf+len);
  buf[len]   = '\0'; /* xxx_sprint_xxx NULL terminates strings. */

  return  len;
}

int
pdf_sprint_coord (char *buf, const pdf_coord *p)
{
  int  len;

  len  = p_dtoa(p->x, dev_unit.precision, buf);
  buf[len++] = ' ';
  len += p_dtoa(p->y, dev_unit.precision, buf+len);
  buf[len]   = '\0'; /* xxx_sprint_xxx NULL terminates strings. */

  return  len;
}

int
pdf_sprint_length (char *buf, double value)
{
  int  len;

  len = p_dtoa(value, dev_unit.precision, buf);
  buf[len] = '\0'; /* xxx_sprint_xxx NULL terminates strings. */

  return  len;
}


int
pdf_sprint_number (char *buf, double value)
{
  int  len;

  len = p_dtoa(value, DEV_PRECISION_MAX, buf);
  buf[len] = '\0'; /* xxx_sprint_xxx NULL terminates strings. */

  return  len;
}


static struct
{
  /* Text composition (direction) mode is ignored (always same
   * as font's writing mode) if autorotate is unset (value zero).
   */
  int    autorotate;

  /*
   * Ignore color migrated to here. This is device's capacity.
   * colormode 0 for ignore colors
   */
  int    colormode;

} dev_param = {
  1, /* autorotate */
  1, /* colormode  */
};

/*
 * Text handling routines.
 */

/* Motion state:
 *  GRAPHICS_MODE  Initial state (not within BT/ET block nor in string)
 *  TEXT_MODE      Text section is started via BT operator but not
 *                 in string.
 *  STRING_MODE    In string. A string or array of strings are currently
 *                 in process. May started '[', '<' or '('.
 */
#define GRAPHICS_MODE  1
#define TEXT_MODE      2
#define STRING_MODE    3

static int motion_state = GRAPHICS_MODE;

#define FORMAT_BUF_SIZE 4096
static char format_buffer[FORMAT_BUF_SIZE];

/*
 * In PDF, vertical text positioning is always applied when current font
 * is vertical font. While ASCII pTeX manages current writing direction
 * and font's WMode separately.
 *
 * 00/11 WMODE_HH/VV  h(v) font, h(v) direction.
 * 01    WMODE_HV    -90 deg. rotated
 * 10    WMODE_VH    +90 deg. rotated
 *
 * In MetaPost PostScript file processing (mp_mode = 1), only HH/VV mode
 * is applied.
 */
#define TEXT_WMODE_HH 0
#define TEXT_WMODE_HV 1
#define TEXT_WMODE_VH 2
#define TEXT_WMODE_VV 3

#define ANGLE_CHANGES(m1,m2) ((abs((m1)-(m2)) % 3) == 0 ? 0 : 1)
#define ROTATE_TEXT(m)       ((m) != TEXT_WMODE_HH && (m) != TEXT_WMODE_VV)

static struct {

  /* Current font.
   * This is index within dev_fonts.
   */
  int       font_id;

  /* Dvipdfmx does compression of text by doing text positioning
   * in relative motion and uses string array [(foo) -250 (bar)]
   * with kerning (negative kern is used for white space as suited
   * for TeX). This is offset within current string.
   */
  spt_t     offset;

  /* This is reference point of strings.
   * It may include error correction induced by rounding.
   */
  spt_t     ref_x;
  spt_t     ref_y;

  /* Using text raise and leading is highly recommended for
   * text extraction to work properly. But not implemented yet.
   * We can't do consice output for \TeX without this.
   */
  spt_t     raise;    /* unused */
  spt_t     leading;  /* unused */

  /* This is not always text matrix but rather font matrix.
   * We do not use horizontal scaling in PDF text state parameter
   * since they always apply scaling in fixed direction regardless
   * of writing mode.
   */
  struct {
    double  slant;
    double  extend;
    int     rotate; /* TEXT_WMODE_XX */
  } matrix;

  /* Fake bold parameter:
   * If bold_param is positive, use text rendering mode
   * fill-then-stroke with stroking line width specified
   * by bold_param.
   */
  double    bold_param;

  /* Text composition (direction) mode. */
  int       dir_mode;

  /* internal */

  /* Flag indicating text matrix to be forcibly reset.
   * Enabled if synthetic font features (slant, extend, etc)
   * are used for current font or when text rotation mode
   * changes.
   */
  int       force_reset;

  /* This information is duplicated from dev[font_id].format.
   * Set to 1 if font is composite (Type0) font.
   */
  int       is_mb;
} text_state = {
  -1,            /* font   */
  0,             /* offset */
  0, 0,          /* ref_x, ref_y   */
  0, 0,          /* raise, leading */
  {0.0, 1.0, 0},

  0.0,  /* Experimental boldness param */

  0,    /* dir_mode      */

  /* internal */
  0,    /* force_reset   */
  0     /* is_mb         */
};

#define PDF_FONTTYPE_SIMPLE    1
#define PDF_FONTTYPE_BITMAP    2
#define PDF_FONTTYPE_COMPOSITE 3

struct dev_font {
  /* Needs to be big enough to hold name "Fxxx"
   * where xxx is number of largest font
   */
  char     short_name[7];      /* Resource name */
  int      used_on_this_page;

  char    *tex_name;  /* String identifier of this font */
  spt_t    sptsize;   /* Point size */

  /* Returned values from font/encoding layer:
   *
   * The font_id and enc_id is font and encoding (CMap) identifier
   * used in pdf_font or encoding/cmap layer.
   * The PDF object "resource" is an indirect reference object
   * pointing font resource of this font. The used_chars is somewhat
   * misleading, this is actually used_glyphs in CIDFont for Type0
   * and is 65536/8 bytes binary data with each bits representing
   * whether the glyph is in-use or not. It is 256 char array for
   * simple font.
   */
  int      font_id;
  int      enc_id;
#ifdef XETEX
  unsigned short *ft_to_gid;
#endif

  /* if >= 0, index of a dev_font that really has the resource and used_chars */
  int      real_font_index;

  pdf_obj *resource;
  char    *used_chars;

  /* Font format:
   * simple, composite or bitmap.
   */
  int      format;

  /* Writing mode:
   * Non-zero for vertical. Duplicated from CMap.
   */
  int      wmode;

  /* Syntetic Font:
   *
   * We use text matrix for creating extended or slanted font,
   * but not with font's FontMatrix since TrueType and Type0
   * font don't support them.
   */
  double   extend;
  double   slant;
  double   bold;  /* Boldness prameter */

  /* Compatibility */
  int      remap; /* Obsolete */
  int      mapc;  /* Nasty workaround for Omega */

  /* There are no font metric format supporting four-bytes
   * charcter code. So we should provide an option to specify
   * UCS group and plane.
   */
  int      ucs_group;
  int      ucs_plane;

  int      is_unicode;
};
static struct dev_font *dev_fonts = NULL;

static int num_dev_fonts   = 0;
static int max_dev_fonts   = 0;
static int num_phys_fonts  = 0;

#define CURRENTFONT() ((text_state.font_id < 0) ? NULL : &(dev_fonts[text_state.font_id]))
#define GET_FONT(n)   (&(dev_fonts[(n)]))


static void
dev_set_text_matrix (spt_t xpos, spt_t ypos, double slant, double extend, int rotate)
{
  pdf_tmatrix tm;
  int         len = 0;

  /* slant is negated for vertical font so that right-side
   * is always lower. */
  switch (rotate) {
  case TEXT_WMODE_VH:
    /* Vertical font */
    tm.a =  slant ;   tm.b =  1.0;
    tm.c = -extend;   tm.d =  0.0   ;
    break;
  case TEXT_WMODE_HV:
    /* Horizontal font */
    tm.a =  0.0;    tm.b = -extend;
    tm.c =  1.0;    tm.d = -slant ;
    break;
  case TEXT_WMODE_HH:
    /* Horizontal font */
    tm.a =  extend; tm.b =  0.0;
    tm.c =  slant ; tm.d =  1.0;
    break;
  case TEXT_WMODE_VV:
    /* Vertical font */
    tm.a =  1.0; tm.b =  -slant;
    tm.c =  0.0; tm.d =   extend;
    break;
  }
  tm.e = xpos * dev_unit.dvi2pts;
  tm.f = ypos * dev_unit.dvi2pts;

  format_buffer[len++] = ' ';
  len += pdf_sprint_matrix(format_buffer+len, &tm);
  format_buffer[len++] = ' ';
  format_buffer[len++] = 'T';
  format_buffer[len++] = 'm';

  pdf_doc_add_page_content(format_buffer, len);

  text_state.ref_x = xpos;
  text_state.ref_y = ypos;
  text_state.matrix.slant  = slant;
  text_state.matrix.extend = extend;
  text_state.matrix.rotate = rotate;
}

/*
 * reset_text_state() outputs a BT and does any necessary coordinate
 * transformations to get ready to ship out text.
 */

static void
reset_text_state (void)
{
  /*
   * We need to reset the line matrix to handle slanted fonts.
   */
  pdf_doc_add_page_content(" BT", 3);
  /*
   * text_state.matrix is identity at top of page.
   * This sometimes write unnecessary "Tm"s when transition from
   * GRAPHICS_MODE to TEXT_MODE occurs.
   */
  if (text_state.force_reset ||
      text_state.matrix.slant  != 0.0 ||
      text_state.matrix.extend != 1.0 ||
      ROTATE_TEXT(text_state.matrix.rotate)) {
    dev_set_text_matrix(0, 0,
                        text_state.matrix.slant,
                        text_state.matrix.extend,
                        text_state.matrix.rotate);
  }
  text_state.ref_x = 0;
  text_state.ref_y = 0;
  text_state.offset   = 0;
  text_state.force_reset = 0;
}

static void
text_mode (void)
{
  switch (motion_state) {
  case TEXT_MODE:
    break;
  case STRING_MODE:
    pdf_doc_add_page_content(text_state.is_mb ? ">]TJ" : ")]TJ", 4);
    break;
  case GRAPHICS_MODE:
    reset_text_state();
    break;
  }
  motion_state      = TEXT_MODE;
  text_state.offset = 0;
}

void
graphics_mode (void)
{
  switch (motion_state) {
  case GRAPHICS_MODE:
    break;
  case STRING_MODE:
    pdf_doc_add_page_content(text_state.is_mb ? ">]TJ" : ")]TJ", 4);
    /* continue */
  case TEXT_MODE:
    pdf_doc_add_page_content(" ET", 3);
    text_state.force_reset =  0;
    text_state.font_id     = -1;
    break;
  }
  motion_state = GRAPHICS_MODE;
}

static void
start_string (spt_t xpos, spt_t ypos, double slant, double extend, int rotate)
{
  spt_t delx, dely, error_delx, error_dely;
  spt_t desired_delx, desired_dely;
  int   len = 0;

  delx = xpos - text_state.ref_x;
  dely = ypos - text_state.ref_y;
  /*
   * Precompensating for line transformation matrix.
   *
   * Line transformation matrix L for horizontal font in horizontal
   * mode and it's inverse I is
   *
   *          | e  0|          | 1/e  0|
   *   L_hh = |     | , I_hh = |       |
   *          | s  1|          |-s/e  1|
   *
   * For vertical font in vertical mode,
   *
   *          | 1  -s|          | 1  s/e|
   *   L_vv = |      | , I_vv = |       |
   *          | 0   e|          | 0  1/e|
   *
   * For vertical font in horizontal mode,
   *
   *          | s   1|          | 0  1|
   *   L_vh = |      | = L_vv x |     |
   *          |-e   0|          |-1  0|
   *
   *          | 0  -1|
   *   I_vh = |      | x I_vv
   *          | 1   0|
   *
   * For horizontal font in vertical mode,
   *
   *          | 0  -e|          | 0  -1|
   *   L_hv = |      | = L_hh x |      |
   *          | 1  -s|          | 1   0|
   *
   *          | 0   1|
   *   I_hv = |      | x I_hh
   *          |-1   0|
   *
   */
  switch (rotate) {
  case TEXT_WMODE_VH:
    /* Vertical font in horizontal mode: rot = +90
     *                           | 0  -1/e|
     * d_user =  d x I_vh = d x  |        |
     *                           | 1   s/e|
     */
    desired_delx = dely;
    desired_dely = (spt_t) (-(delx - dely*slant)/extend);

    /* error_del is in device space
     *
     *               | 0  1|
     *  e = e_user x |     | = (-e_user_y, e_user_x)
     *               |-1  0|
     *
     * We must care about rotation here but not extend/slant...
     * The extend and slant actually is font matrix.
     */
    format_buffer[len++] = ' ';
    len += dev_sprint_bp(format_buffer+len, desired_delx, &error_dely);
    format_buffer[len++] = ' ';
    len += dev_sprint_bp(format_buffer+len, desired_dely, &error_delx);
    error_delx = -error_delx;
    break;
  case TEXT_WMODE_HV:
    /* Horizontal font in vertical mode: rot = -90
     *
     *                         |-s/e  1|
     * d_user = d x I_hv = d x |       |
     *                         |-1/e  0|
     */
    desired_delx = (spt_t)(-(dely + delx*slant)/extend);
    desired_dely = delx;

    /*
     * e = (e_user_y, -e_user_x)
     */
    format_buffer[len++] = ' ';
    len += dev_sprint_bp(format_buffer+len, desired_delx, &error_dely);
    format_buffer[len++] = ' ';
    len += dev_sprint_bp(format_buffer+len, desired_dely, &error_delx);
    error_dely = -error_dely;
    break;
  case TEXT_WMODE_HH:
    /* Horizontal font in horizontal mode:
     *                         | 1/e    0|
     * d_user = d x I_hh = d x |         |
     *                         |-s/e    1|
     */
    desired_delx = (spt_t)((delx - dely*slant)/extend);
    desired_dely = dely;

    format_buffer[len++] = ' ';
    len += dev_sprint_bp(format_buffer+len, desired_delx, &error_delx);
    format_buffer[len++] = ' ';
    len += dev_sprint_bp(format_buffer+len, desired_dely, &error_dely);
    break;
  case TEXT_WMODE_VV:
    /* Vertical font in vertical mode:
     *                         | 1  s/e|
     * d_user = d x I_vv = d x |       |
     *                         | 0  1/e|
     */
    desired_delx = delx;
    desired_dely = (spt_t)((dely + delx*slant)/extend);

    format_buffer[len++] = ' ';
    len += dev_sprint_bp(format_buffer+len, desired_delx, &error_delx);
    format_buffer[len++] = ' ';
    len += dev_sprint_bp(format_buffer+len, desired_dely, &error_dely);
    break;
  }
  pdf_doc_add_page_content(format_buffer, len);
  /*
   * dvipdfm wrongly using "TD" in place of "Td".
   * The TD operator set leading, but we are not using T* etc.
   */
  pdf_doc_add_page_content(text_state.is_mb ? " Td[<" : " Td[(", 5);

  /* Error correction */
  text_state.ref_x = xpos - error_delx;
  text_state.ref_y = ypos - error_dely;

  text_state.offset   = 0;
}

static void
string_mode (spt_t xpos, spt_t ypos, double slant, double extend, int rotate)
{
  switch (motion_state) {
  case STRING_MODE:
    break;
  case GRAPHICS_MODE:
    reset_text_state();
    /* continue */
  case TEXT_MODE:
    if (text_state.force_reset) {
      dev_set_text_matrix(xpos, ypos, slant, extend, rotate);
      pdf_doc_add_page_content(text_state.is_mb ? "[<" : "[(", 2);
      text_state.force_reset = 0;
    } else {
      start_string(xpos, ypos, slant, extend, rotate);
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

static int
dev_set_font (int font_id)
{
  struct dev_font *font;
  struct dev_font *real_font;
  int    text_rotate;
  double font_scale;
  int    len;
  int    vert_dir, vert_font;

  /* text_mode() must come before text_state.is_mb is changed. */
  text_mode();

  font = GET_FONT(font_id);
  ASSERT(font); /* Caller should check font_id. */

  if (font->real_font_index >= 0)
    real_font = GET_FONT(font->real_font_index);
  else
    real_font = font;

  text_state.is_mb = (font->format == PDF_FONTTYPE_COMPOSITE) ? 1 : 0;

  vert_font  = font->wmode ? 1 : 0;
  if (dev_param.autorotate) {
    vert_dir = text_state.dir_mode ? 1 : 0;
  } else {
    vert_dir = vert_font;
  }
  text_rotate = (vert_font << 1)|vert_dir;

  if (font->slant  != text_state.matrix.slant  ||
      font->extend != text_state.matrix.extend ||
      ANGLE_CHANGES(text_rotate, text_state.matrix.rotate)) {
    text_state.force_reset = 1;
  }
  text_state.matrix.slant  = font->slant;
  text_state.matrix.extend = font->extend;
  text_state.matrix.rotate = text_rotate;

  if (!real_font->resource) {
    real_font->resource   = pdf_get_font_reference(real_font->font_id);
    real_font->used_chars = pdf_get_font_usedchars(real_font->font_id);
  }

  if (!real_font->used_on_this_page) { 
    pdf_doc_add_page_resource("Font",
                              real_font->short_name,
                              pdf_link_obj(real_font->resource));
    real_font->used_on_this_page = 1;
  }

  font_scale = (double) font->sptsize * dev_unit.dvi2pts;
  len  = sprintf(format_buffer, " /%s", real_font->short_name); /* space not necessary. */
  format_buffer[len++] = ' ';
  len += p_dtoa(font_scale, MIN(dev_unit.precision+1, DEV_PRECISION_MAX), format_buffer+len);
  format_buffer[len++] = ' ';
  format_buffer[len++] = 'T';
  format_buffer[len++] = 'f';
  pdf_doc_add_page_content(format_buffer, len);

  if (font->bold > 0.0 || font->bold != text_state.bold_param) {
    if (font->bold <= 0.0)
      len = sprintf(format_buffer, " 0 Tr");
    else
      len = sprintf(format_buffer, " 2 Tr %.6f w", font->bold); /* _FIXME_ */
    pdf_doc_add_page_content(format_buffer, len);
  }
  text_state.bold_param = font->bold;

  text_state.font_id    = font_id;

  return  0;
}


/* Access text state parameters.
 */
int
pdf_dev_currentfont (void)
{
  return text_state.font_id;
}

double
pdf_dev_get_font_ptsize (int font_id)
{
  struct dev_font *font;

  font = GET_FONT(font_id);
  if (font) {
    return font->sptsize * dev_unit.dvi2pts;
  }

  return 1.0;
}

int
pdf_dev_get_font_wmode (int font_id)
{
  struct dev_font *font;

  font = GET_FONT(font_id);
  if (font) {
    return font->wmode;
  }

  return 0;
}

static unsigned char sbuf0[FORMAT_BUF_SIZE];
static unsigned char sbuf1[FORMAT_BUF_SIZE];

static int
handle_remap (unsigned char **str_ptr, int length)
{
  unsigned char *p;
  int  i;

  p = *str_ptr;

#define twiddle(n) ( \
    ( \
     ((n) <= 9)  ? \
        ((n) + 161) : \
           (((n) <= 32)  ? \
           ((n) + 163) : \
           (((n) == 127) ? 196: (n)) \
        ) \
    ) \
)

  for (i = 0; i < length; i++) {
    sbuf0[i] = twiddle(p[i]);
  }

  *str_ptr = sbuf0;
  return 0;
}

static int
handle_multibyte_string (struct dev_font *font,
                         unsigned char **str_ptr, int *str_len, int ctype)
{
  unsigned char *p;
  int            i, length;

  p      = *str_ptr;
  length = *str_len;

#ifdef XETEX
  if (ctype == -1) { /* freetype glyph indexes */
    if (font->ft_to_gid) {
      /* convert freetype glyph indexes to physical GID */
      unsigned char *inbuf = p;
      unsigned char *outbuf = sbuf0;
      for (i = 0; i < length; i += 2) {
        unsigned int gid;
        gid = *inbuf++ << 8;
        gid += *inbuf++;
        gid = font->ft_to_gid[gid];
        *outbuf++ = gid >> 8;
        *outbuf++ = gid & 0xff;
      }
      p = sbuf0;
      length = outbuf - sbuf0;
    }
  }
  else
#endif
  /* _FIXME_ */
  if (font->is_unicode) { /* UCS-4 */
    if (ctype == 1) {
      if (length * 4 >= FORMAT_BUF_SIZE) {
        WARN("Too long string...");
        return -1;
      }
      for (i = 0; i < length; i++) {
        sbuf1[i*4  ] = font->ucs_group;
        sbuf1[i*4+1] = font->ucs_plane;
        sbuf1[i*4+2] = '\0';
        sbuf1[i*4+3] = p[i];
      }
      length *= 4;
    } else if (ctype == 2) {
      if (length * 2 >= FORMAT_BUF_SIZE) {
        WARN("Too long string...");
        return -1;
      }
      for (i = 0; i < length; i += 2) {
        sbuf1[i*2  ] = font->ucs_group;
        sbuf1[i*2+1] = font->ucs_plane;
        sbuf1[i*2+2] = p[i];
        sbuf1[i*2+3] = p[i+1];
      }
      length *= 2;
    }
    p = sbuf1;
  } else if (ctype == 1 && font->mapc >= 0) {
    /* Omega workaround...
     * Translate single-byte chars to double byte code space.
     */
    if (length * 2 >= FORMAT_BUF_SIZE) {
      WARN("Too long string...");
      return -1;
    }
    for (i = 0; i < length; i++) {
      sbuf1[i*2  ] = (font->mapc & 0xff);
      sbuf1[i*2+1] = p[i];
    }
    length *= 2;
    p       = sbuf1;
  }

  /*
   * Font is double-byte font. Output is assumed to be 16-bit fixed length
   * encoding.
   * TODO: A character decomposed to multiple characters.
   */
  if (ctype != -1 && font->enc_id >= 0) {
    unsigned char *inbuf, *outbuf;
    long           inbytesleft, outbytesleft;
    CMap          *cmap;

    cmap         = CMap_cache_get(font->enc_id);
    inbuf        = p;
    outbuf       = sbuf0;
    inbytesleft  = length;
    outbytesleft = FORMAT_BUF_SIZE;

    CMap_decode(cmap,
                (const unsigned char **) &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    if (inbytesleft != 0) {
      WARN("CMap conversion failed. (%d bytes remains)", inbytesleft);
      return -1;
    }
    length  = FORMAT_BUF_SIZE - outbytesleft;
    p       = sbuf0;
  }

  *str_ptr = p;
  *str_len = length;
  return 0;
}

/*
 * ctype:
#ifdef XETEX
 *  -1 input string contains 2-byte Freetype glyph index values
#endif
 *  0  byte-width of char can be variable and input string
 *     is properly encoded.
 *  n  Single character cosumes n bytes in input string.
 *
 * _FIXME_
 * -->
 * selectfont(font_name, point_size) and show_string(pos, string)
 */
void
pdf_dev_set_string (spt_t xpos, spt_t ypos,
                    const void *instr_ptr, int instr_len,
                    spt_t width,
                    int   font_id, int ctype)
{
  struct dev_font *font;
  struct dev_font *real_font;
  unsigned char   *str_ptr; /* Pointer to the reencoded string. */
  int              length, i, len = 0;
  spt_t            kern, delh, delv;
  spt_t            text_xorigin;
  spt_t            text_yorigin;

  if (font_id < 0 || font_id >= num_dev_fonts) {
    ERROR("Invalid font: %d (%d)", font_id, num_dev_fonts);
    return;
  }
  if (font_id != text_state.font_id) {
    dev_set_font(font_id);
  }

  font = CURRENTFONT();
  if (!font) {
    ERROR("Currentfont not set.");
    return;
  }

  if (font->real_font_index >= 0)
    real_font = GET_FONT(font->real_font_index);
  else
    real_font = font;

  text_xorigin = text_state.ref_x;
  text_yorigin = text_state.ref_y;

  str_ptr = (unsigned char *) instr_ptr;
  length  = instr_len;

  if (font->format == PDF_FONTTYPE_COMPOSITE) {
    if (handle_multibyte_string(font, &str_ptr, &length, ctype) < 0) {
      ERROR("Error in converting input string...");
      return;
    }
    if (real_font->used_chars != NULL) {
      for (i = 0; i < length; i += 2)
        add_to_used_chars2(real_font->used_chars,
                           (unsigned short) (str_ptr[i] << 8)|str_ptr[i+1]);
    }
  } else {
    if (font->remap)
      handle_remap(&str_ptr, length); /* length unchanged. */
    if (real_font->used_chars != NULL) {
      for (i = 0; i < length; i++)
        real_font->used_chars[str_ptr[i]] = 1;
    }
  }

  /*
   * Kern is in units of character units, i.e., 1000 = 1 em.
   *
   * Positive kern means kerning (reduce excess white space).
   *
   * The following formula is of the form a*x/b where a, x, and b are signed long
   * integers.  Since in integer arithmetic (a*x) could overflow and a*(x/b) would
   * not be accurate, we use floating point arithmetic rather than trying to do
   * this all with integer arithmetic.
   *
   * 1000.0 / (font->extend * font->sptsize) is caluculated each times...
   * Is accuracy really a matter? Character widths are always rounded to integer
   * (in 1000 units per em) but dvipdfmx does not take into account of this...
   */

  if (text_state.dir_mode) {
    /* Top-to-bottom */
    delh = ypos - text_yorigin + text_state.offset;
    delv = xpos - text_xorigin;
  } else {
    /* Left-to-right */
    delh = text_xorigin + text_state.offset - xpos;
    delv = ypos - text_yorigin;
  }

  /* White-space more than 3em is not considered as a part of single text.
   * So we will break string mode in that case.
   * Dvipdfmx spend most of time processing strings with kern = 0 (but far
   * more times in font handling).
   * You may want to use pre-calculated value for WORD_SPACE_MAX.
   * More text compression may be possible by replacing kern with space char
   * when -kern is equal to space char width.
   */
#define WORD_SPACE_MAX(f) (spt_t) (3.0 * (f)->extend * (f)->sptsize)

  if (text_state.force_reset ||
      labs(delv) > dev_unit.min_bp_val ||
      labs(delh) > WORD_SPACE_MAX(font)) {
    text_mode();
    kern = 0;
  } else {
    kern = (spt_t) (1000.0 / font->extend * delh / font->sptsize);
  }

  /* Inaccucary introduced by rounding of character width appears within
   * single text block. There are point_size/1000 rounding error per character.
   * If you really care about accuracy, you should compensate this here too.
   */
  if (motion_state != STRING_MODE)
    string_mode(xpos, ypos,
                font->slant, font->extend, text_state.matrix.rotate);
  else if (kern != 0) {
    /*
     * Same issues as earlier. Use floating point for simplicity.
     * This routine needs to be fast, so we don't call sprintf() or strcpy().
     */
    text_state.offset -= 
      (spt_t) (kern * font->extend * (font->sptsize / 1000.0));
    format_buffer[len++] = text_state.is_mb ? '>' : ')';
    if (font->wmode)
      len += p_itoa(-kern, format_buffer + len);
    else {
      len += p_itoa( kern, format_buffer + len);
    }
    format_buffer[len++] = text_state.is_mb ? '<' : '(';
    pdf_doc_add_page_content(format_buffer, len);
    len = 0;
  }

  if (text_state.is_mb) {
    if (FORMAT_BUF_SIZE - len < 2 * length)
      ERROR("Buffer overflow...");
    for (i = 0; i < length; i++) {
      int first, second;

      first  = (str_ptr[i] >> 4) & 0x0f;
      second = str_ptr[i] & 0x0f;
      format_buffer[len++] = ((first >= 10)  ? first  + 'W' : first  + '0');
      format_buffer[len++] = ((second >= 10) ? second + 'W' : second + '0');
    }
  } else {
    len += pdfobj_escape_str(format_buffer + len,
                             FORMAT_BUF_SIZE - len, str_ptr, length);
  }
  /* I think if you really care about speed, you should avoid memcopy here. */
  pdf_doc_add_page_content(format_buffer, len);

  text_state.offset += width;
}

void
pdf_init_device (double dvi2pts, int precision, int black_and_white)
{
  if (precision < 0 ||
      precision > DEV_PRECISION_MAX)
    WARN("Number of decimal digits out of range [0-%d].",
         DEV_PRECISION_MAX);

  if (precision < 0) {
    dev_unit.precision  = 0;
  } else if (precision > DEV_PRECISION_MAX) {
    dev_unit.precision  = DEV_PRECISION_MAX;
  } else {
    dev_unit.precision  = precision;
  }
  dev_unit.dvi2pts      = dvi2pts;
  dev_unit.min_bp_val   = (long) ROUND(1.0/(ten_pow[dev_unit.precision]*dvi2pts), 1);
  if (dev_unit.min_bp_val < 0)
    dev_unit.min_bp_val = -dev_unit.min_bp_val;

  dev_param.colormode = (black_and_white ? 0 : 1);

  graphics_mode();
  pdf_color_clear_stack();
  pdf_dev_init_gstates();

  num_dev_fonts = 0;
  max_dev_fonts = 0;
  dev_fonts     = NULL;
}

void
pdf_close_device (void)
{
  if (dev_fonts) {
    int    i;

    for (i = 0; i < num_dev_fonts; i++) {
      if (dev_fonts[i].tex_name)
        RELEASE(dev_fonts[i].tex_name);
      if (dev_fonts[i].resource)
        pdf_release_obj(dev_fonts[i].resource);
      dev_fonts[i].tex_name = NULL;
      dev_fonts[i].resource = NULL;
    }
    RELEASE(dev_fonts);
  }
  dev_fonts     = NULL;
  num_dev_fonts = 0;
  max_dev_fonts = 0;

  pdf_dev_clear_gstates();
}

/*
 * BOP, EOP, and FONT section.
 * BOP and EOP manipulate some of the same data structures
 * as the font stuff.
 */
void
pdf_dev_reset_fonts (void)
{
  int  i;

  for (i = 0; i < num_dev_fonts; i++) {
    dev_fonts[i].used_on_this_page = 0;
  }

  text_state.font_id       = -1;

  text_state.matrix.slant  = 0.0;
  text_state.matrix.extend = 1.0;
  text_state.matrix.rotate = TEXT_WMODE_HH;

  text_state.bold_param    = 0.0;

  text_state.is_mb         = 0;
}

void
pdf_dev_reset_color(void)
{
  pdf_color *sc, *fc;

  if (pdf_dev_get_param(PDF_DEV_PARAM_COLORMODE)) {
    pdf_color_get_current(&sc, &fc);
    pdf_dev_set_strokingcolor(sc);
    pdf_dev_set_nonstrokingcolor(fc);
  }
  return;
}

static int
color_to_string (pdf_color *color, char *buffer)
{
  int i, len = 0;

  for (i = 0; i < color->num_components; i++) {
    len += sprintf(format_buffer+len, " %g", ROUND(color->values[i], 0.001));
  }
  return len;
}

void
pdf_dev_set_color (pdf_color *color)
{
  int len;

  if (!pdf_dev_get_param(PDF_DEV_PARAM_COLORMODE)) {
    WARN("Ignore color option was set. Just ignore.");
    return;
  } else if (!(color && pdf_color_is_valid(color))) {
    WARN("No valid color is specified. Just ignore.");
    return;
  }

  graphics_mode();
  len = color_to_string(color, format_buffer);
  format_buffer[len++] = ' ';
  switch (color->num_components) {
  case  3:
    format_buffer[len++] = 'R';
    format_buffer[len++] = 'G';
    break;
  case  4:
    format_buffer[len++] = 'K';
    break;
  case  1:
    format_buffer[len++] = 'G';
    break;
  default: /* already verified the given color */
    break;
  }
  strncpy(format_buffer+len, format_buffer, len);
  len = len << 1;
  switch (color->num_components) {
  case  3:
    format_buffer[len-2] = 'r';
    format_buffer[len-1] = 'g';
    break;
  case  4:
    format_buffer[len-1] = 'k';
    break;
  case  1:
    format_buffer[len-1] = 'g';
  break;
  default: /* already verified the given color */
    break;
  }
  pdf_doc_add_page_content(format_buffer, len);
  return;
}

void
pdf_dev_set_strokingcolor (pdf_color *color)
{
  int len;

  if (!pdf_dev_get_param(PDF_DEV_PARAM_COLORMODE)) {
    WARN("Ignore color option was set. Just ignore.");
    return;
  } else if (!(color && pdf_color_is_valid(color))) {
    WARN("No valid color is specified. Just ignore.");
    return;
  }

  graphics_mode();
  len = color_to_string(color, format_buffer);
  format_buffer[len++] = ' ';
  switch (color->num_components) {
  case  3:
    format_buffer[len++] = 'R';
    format_buffer[len++] = 'G';
    break;
  case  4:
    format_buffer[len++] = 'K';
    break;
  case  1:
    format_buffer[len++] = 'G';
    break;
  default: /* already verified the given color */
    break;
  }
  pdf_doc_add_page_content(format_buffer, len);
  return;
}

void
pdf_dev_set_nonstrokingcolor (pdf_color *color)
{
  int len;

  if (!pdf_dev_get_param(PDF_DEV_PARAM_COLORMODE)) {
    WARN("Ignore color option was set. Just ignore.");
    return;
  } else if (!(color && pdf_color_is_valid(color))) {
    WARN("No valid color is specified. Just ignore.");
    return;
  }

  graphics_mode();
  len = color_to_string(color, format_buffer);
  format_buffer[len++] = ' ';
  switch (color->num_components) {
  case  3:
    format_buffer[len++] = 'r';
    format_buffer[len++] = 'g';
    break;
  case  4:
    format_buffer[len++] = 'k';
    break;
  case  1:
    format_buffer[len++] = 'g';
    break;
  default: /* already verified the given color */
    break;
  }
  pdf_doc_add_page_content(format_buffer, len);
  return;
}

/* Not working */
void
pdf_dev_set_origin (double phys_x, double phys_y)
{
  pdf_tmatrix M0, M1;

  pdf_dev_currentmatrix(&M0);
  pdf_dev_currentmatrix(&M1);
  pdf_invertmatrix(&M1);
  M0.e = phys_x; M0.f = phys_y;
  pdf_concatmatrix(&M1, &M0);

  pdf_dev_concat(&M1);
}

void
pdf_dev_bop (const pdf_tmatrix *M)
{
  graphics_mode();

  text_state.force_reset  = 0;

  pdf_dev_gsave();
  pdf_dev_concat(M);

  pdf_dev_reset_fonts();
  pdf_dev_reset_color();
}

void
pdf_dev_eop (void)
{
  int  depth;

  graphics_mode();

  depth = pdf_dev_current_depth();
  if (depth != 1) {
    WARN("Unbalenced q/Q nesting...: %d", depth);
    pdf_dev_grestore_to(0);
  } else {
    pdf_dev_grestore();
  }
}

static void
print_fontmap (const char *font_name, fontmap_rec *mrec)
{
  if (!mrec)
    return;

  MESG("\n");

  MESG("fontmap: %s -> %s", font_name, mrec->font_name);
  if (mrec->enc_name)
    MESG("(%s)",  mrec->enc_name);
  if (mrec->opt.extend != 1.0)
    MESG("[extend:%g]", mrec->opt.extend);
  if (mrec->opt.slant  != 0.0)
    MESG("[slant:%g]",  mrec->opt.slant);
  if (mrec->opt.bold   != 0.0) 
    MESG("[bold:%g]",   mrec->opt.bold);
  if (mrec->opt.flags & FONTMAP_OPT_REMAP) 
    MESG("[remap]");
  if (mrec->opt.flags & FONTMAP_OPT_NOEMBED)
    MESG("[noemb]");
  if (mrec->opt.mapc >= 0)
    MESG("[map:<%02x>]", mrec->opt.mapc);
  if (mrec->opt.charcoll)  
    MESG("[csi:%s]",     mrec->opt.charcoll);
  if (mrec->opt.index) 
    MESG("[index:%d]",   mrec->opt.index);

  switch (mrec->opt.style) {
  case FONTMAP_STYLE_BOLD:
    MESG("[style:bold]");
    break;
  case FONTMAP_STYLE_ITALIC:
    MESG("[style:italic]");
    break;
  case FONTMAP_STYLE_BOLDITALIC:
    MESG("[style:bolditalic]");
    break;
  }
  MESG("\n");

}

/* _FIXME_
 * Font is identified with font_name and point_size as in DVI here.
 * However, except for PDF_FONTTYPE_BITMAP, we can share the 
 * short_name, resource and used_chars between multiple instances
 * of the same font at different sizes.
 */
int
pdf_dev_locate_font (const char *font_name, spt_t ptsize)
{
  int              i;
  fontmap_rec     *mrec;
  struct dev_font *font;

  if (!font_name)
    return  -1;

  if (ptsize == 0) {
    ERROR("pdf_dev_locate_font() called with the zero ptsize.");
    return -1;
  }

  for (i = 0; i < num_dev_fonts; i++) {
    if (strcmp(font_name, dev_fonts[i].tex_name) == 0) {
      if (ptsize == dev_fonts[i].sptsize)
        return i; /* found a dev_font that matches the request */
      if (dev_fonts[i].format != PDF_FONTTYPE_BITMAP)
        break; /* new dev_font will share pdf resource with /i/ */
    }
  }
    
  /*
   * Make sure we have room for a new one, even though we may not
   * actually create one.
   */
  if (num_dev_fonts >= max_dev_fonts) {
    max_dev_fonts += 16;
    dev_fonts      = RENEW(dev_fonts, max_dev_fonts, struct dev_font);
  }

  font = &dev_fonts[num_dev_fonts];

  /* New font */
  mrec = pdf_lookup_fontmap_record(font_name);

  if (verbose > 1)
    print_fontmap(font_name, mrec);

  font->font_id = pdf_font_findresource(font_name, ptsize * dev_unit.dvi2pts, mrec);
  if (font->font_id < 0)
    return  -1;

  /* We found device font here. */
  if (i < num_dev_fonts) {
    font->real_font_index = i;
    strcpy(font->short_name, dev_fonts[i].short_name);
  }
  else {
    font->real_font_index = -1;
    font->short_name[0] = 'F';
    p_itoa(num_phys_fonts + 1, &font->short_name[1]); /* NULL terminated here */
    num_phys_fonts++;
  }

  font->used_on_this_page = 0;

  font->tex_name = NEW(strlen(font_name) + 1, char);
  strcpy(font->tex_name, font_name);
  font->sptsize  = ptsize;

  switch (pdf_get_font_subtype(font->font_id)) {
  case PDF_FONT_FONTTYPE_TYPE3:
    font->format = PDF_FONTTYPE_BITMAP;
    break;
  case PDF_FONT_FONTTYPE_TYPE0:
    font->format = PDF_FONTTYPE_COMPOSITE;
    break;
  default:
    font->format = PDF_FONTTYPE_SIMPLE;
    break;
  }

  font->wmode      = pdf_get_font_wmode   (font->font_id);
  font->enc_id     = pdf_get_font_encoding(font->font_id);
#ifdef XETEX
  font->ft_to_gid  = pdf_get_font_ft_to_gid(font->font_id);
#endif

  font->resource   = NULL; /* Don't ref obj until font is actually used. */  
  font->used_chars = NULL;

  font->extend     = 1.0;
  font->slant      = 0.0;
  font->bold       = 0.0;
  font->remap      = 0;
  font->mapc       = -1;
  font->is_unicode = 0;
  font->ucs_group  = 0;
  font->ucs_plane  = 0;

  if (mrec) {
    font->extend = mrec->opt.extend;
    font->slant  = mrec->opt.slant;
    font->bold   = mrec->opt.bold;
    font->remap  = (int) (mrec->opt.flags & FONTMAP_OPT_REMAP);
    if (mrec->opt.mapc >= 0)
      font->mapc = (mrec->opt.mapc >> 8) & 0xff;
    else {
      font->mapc = -1;
    }
    if (mrec->enc_name &&
        !strcmp(mrec->enc_name, "unicode")) {
      font->is_unicode   = 1;
      if (mrec->opt.mapc >= 0) {
        font->ucs_group  = (mrec->opt.mapc >> 24) & 0xff;
        font->ucs_plane  = (mrec->opt.mapc >> 16) & 0xff;
      } else {
        font->ucs_group  = 0;
        font->ucs_plane  = 0;
      }
    } else {
      font->is_unicode   = 0;
    }
  }

  return  num_dev_fonts++;
}


/* This does not remember current stroking width. */
static int
dev_sprint_line (char *buf, spt_t width,
                 spt_t p0_x, spt_t p0_y, spt_t p1_x, spt_t p1_y)
{
  int    len = 0;
  double w;

  w = width * dev_unit.dvi2pts;

  len += p_dtoa(w, MIN(dev_unit.precision+1, DEV_PRECISION_MAX), buf+len);
  buf[len++] = ' ';
  buf[len++] = 'w';
  buf[len++] = ' ';
  len += dev_sprint_bp(buf+len, p0_x, NULL);
  buf[len++] = ' ';
  len += dev_sprint_bp(buf+len, p0_y, NULL);
  buf[len++] = ' ';
  buf[len++] = 'm';
  buf[len++] = ' ';
  len += dev_sprint_bp(buf+len, p1_x, NULL);
  buf[len++] = ' ';
  len += dev_sprint_bp(buf+len, p1_y, NULL);
  buf[len++] = ' ';
  buf[len++] = 'l';
  buf[len++] = ' ';
  buf[len++] = 'S';

  return len;
}

/* Not optimized. */
#define PDF_LINE_THICKNESS_MAX 5.0
void
pdf_dev_set_rule (spt_t xpos, spt_t ypos, spt_t width, spt_t height)
{
  int    len = 0;
  double width_in_bp;

  graphics_mode();

  format_buffer[len++] = ' ';
  format_buffer[len++] = 'q';
  format_buffer[len++] = ' ';
  /* Don't use too thick line. */
  width_in_bp = ((width < height) ? width : height) * dev_unit.dvi2pts;
  if (width_in_bp < 0.0 || /* Shouldn't happen */
      width_in_bp > PDF_LINE_THICKNESS_MAX) {
    pdf_rect rect;

    rect.llx =  dev_unit.dvi2pts * xpos;
    rect.lly =  dev_unit.dvi2pts * ypos;
    rect.urx =  dev_unit.dvi2pts * width;
    rect.ury =  dev_unit.dvi2pts * height;
    len += pdf_sprint_rect(format_buffer+len, &rect);
    format_buffer[len++] = ' ';
    format_buffer[len++] = 'r';
    format_buffer[len++] = 'e';
    format_buffer[len++] = ' ';
    format_buffer[len++] = 'f';
  } else {
    if (width > height) {
      /* NOTE:
       *  A line width of 0 denotes the thinnest line that can be rendered at
       *  device resolution. See, PDF Reference Manual 4th ed., sec. 4.3.2,
       *  "Details of Graphics State Parameters", p. 185.
       */
      if (height < dev_unit.min_bp_val) {
        WARN("Too thin line: height=%ld (%g bp)", height, width_in_bp);
        WARN("Please consider using \"-d\" option.");
      }
      len += dev_sprint_line(format_buffer+len,
                             height,
                             xpos,
                             ypos + height/2,
                             xpos + width,
                             ypos + height/2);
    } else {
      if (width < dev_unit.min_bp_val) {
        WARN("Too thin line: width=%ld (%g bp)", width, width_in_bp);
        WARN("Please consider using \"-d\" option.");
      }
      len += dev_sprint_line(format_buffer+len,
                             width,
                             xpos + width/2,
                             ypos,
                             xpos + width/2,
                             ypos + height);
    }
  }
  format_buffer[len++] = ' ';
  format_buffer[len++] = 'Q';
  pdf_doc_add_page_content(format_buffer, len);
}

/* Rectangle in device space coordinate. */
void
pdf_dev_set_rect (pdf_rect *rect,
                  spt_t x_user, spt_t y_user,
                  spt_t width,  spt_t height, spt_t depth)
{
  double      dev_x, dev_y;
  pdf_coord   p0, p1, p2, p3;
  double      min_x, min_y, max_x, max_y;

  dev_x = x_user * dev_unit.dvi2pts;
  dev_y = y_user * dev_unit.dvi2pts;
  if (text_state.dir_mode) {
    p0.x = dev_x - dev_unit.dvi2pts * depth;
    p0.y = dev_y - dev_unit.dvi2pts * width;
    p1.x = dev_x + dev_unit.dvi2pts * height;
    p1.y = p0.y;
    p2.x = p1.x;
    p2.y = dev_y;
    p3.x = p0.x;
    p3.y = p2.y;
  } else {
    p0.x = dev_x;
    p0.y = dev_y - dev_unit.dvi2pts * depth;
    p1.x = dev_x + dev_unit.dvi2pts * width;
    p1.y = p0.y;
    p2.x = p1.x;
    p2.y = dev_y + dev_unit.dvi2pts * height;
    p3.x = p0.x;
    p3.y = p2.y;
  }

  pdf_dev_transform(&p0, NULL); /* currentmatrix */
  pdf_dev_transform(&p1, NULL);
  pdf_dev_transform(&p2, NULL);
  pdf_dev_transform(&p3, NULL);

  min_x = MIN(p0.x , p1.x);
  min_x = MIN(min_x, p2.x);
  min_x = MIN(min_x, p3.x);

  max_x = MAX(p0.x , p1.x);
  max_x = MAX(max_x, p2.x);
  max_x = MAX(max_x, p3.x);

  min_y = MIN(p0.y , p1.y);
  min_y = MIN(min_y, p2.y);
  min_y = MIN(min_y, p3.y);

  max_y = MAX(p0.y , p1.y);
  max_y = MAX(max_y, p2.y);
  max_y = MAX(max_y, p3.y);

  rect->llx = min_x;
  rect->lly = min_y;
  rect->urx = max_x;
  rect->ury = max_y;

  return;
}

int
pdf_dev_get_dirmode (void)
{
  return text_state.dir_mode;
}

void
pdf_dev_set_dirmode (int text_dir)
{
  struct dev_font *font;
  int text_rotate;
  int vert_dir, vert_font;

  font = CURRENTFONT();

  vert_font = (font && font->wmode) ? 1 : 0;
  if (dev_param.autorotate) {
    vert_dir = text_dir ? 1 : 0;
  } else {
    vert_dir = vert_font;
  }
  text_rotate = (vert_font << 1)|vert_dir;

  if (font &&
      ANGLE_CHANGES(text_rotate, text_state.matrix.rotate)) {
    text_state.force_reset = 1;
  }

  text_state.matrix.rotate = text_rotate;
  text_state.dir_mode      = text_dir;
}

static void
dev_set_param_autorotate (int auto_rotate)
{
  struct dev_font *font;
  int    text_rotate, vert_font, vert_dir;

  font = CURRENTFONT();

  vert_font = (font && font->wmode) ? 1 : 0;
  if (auto_rotate) {
    vert_dir = text_state.dir_mode ? 1 : 0;
  } else {
    vert_dir = vert_font;
  }
  text_rotate = (vert_font << 1)|vert_dir;

  if (ANGLE_CHANGES(text_rotate, text_state.matrix.rotate)) {
    text_state.force_reset = 1;
  }
  text_state.matrix.rotate = text_rotate;
  dev_param.autorotate     = auto_rotate;
}

int
pdf_dev_get_param (int param_type)
{
  int value = 0;

  switch (param_type) {
  case PDF_DEV_PARAM_AUTOROTATE:
    value = dev_param.autorotate;
    break;
  case PDF_DEV_PARAM_COLORMODE:
    value = dev_param.colormode;
    break;
  default:
    ERROR("Unknown device parameter: %d", param_type);
  }

  return value;
}

void
pdf_dev_set_param (int param_type, int value)
{
  switch (param_type) {
  case PDF_DEV_PARAM_AUTOROTATE:
    dev_set_param_autorotate(value);
    break;
  case PDF_DEV_PARAM_COLORMODE:
    dev_param.colormode = value; /* 0 for B&W */
    break;
  default:
    ERROR("Unknown device parameter: %d", param_type);
  }

  return;
}


int
pdf_dev_put_image (int             id,
                   transform_info *p,
                   double          ref_x,
                   double          ref_y)
{
  char        *res_name;
  pdf_tmatrix  M;
  pdf_rect     r;
  int          len = 0;

  pdf_copymatrix(&M, &(p->matrix));
  M.e += ref_x; M.f += ref_y;
  /* Just rotate by -90, but not tested yet. Any problem if M has scaling? */
  if (dev_param.autorotate &&
      text_state.dir_mode) {
    double tmp;
    tmp = -M.a; M.a = M.b; M.b = tmp;
    tmp = -M.c; M.c = M.d; M.d = tmp;
  }

  graphics_mode();
  pdf_dev_gsave();

  pdf_dev_concat(&M);

  pdf_ximage_scale_image(id, &M, &r, p);
  pdf_dev_concat(&M);

  /* Clip */
  if (p->flags & INFO_DO_CLIP) {
#if  0
    pdf_dev_newpath();
    pdf_dev_moveto(r.llx, r.lly);
    pdf_dev_lineto(r.urx, r.lly);
    pdf_dev_lineto(r.urx, r.ury);
    pdf_dev_lineto(r.llx, r.ury);
    pdf_dev_closepath();
    pdf_dev_clip();
    pdf_dev_newpath();
#else
    pdf_dev_rectclip(r.llx, r.lly, r.urx - r.llx, r.ury - r.lly);
#endif
  }

  res_name = pdf_ximage_get_resname(id);
  len = sprintf(work_buffer, " /%s Do", res_name);
  pdf_doc_add_page_content(work_buffer, len);

  pdf_dev_grestore();

  pdf_doc_add_page_resource("XObject",
                            res_name,
                            pdf_ximage_get_reference(id));

  if (dvi_is_tracking_boxes()) {
    pdf_tmatrix P;
    int i;
    pdf_rect rect;
    pdf_coord corner[4];

    pdf_dev_set_rect(&rect, 65536 * ref_x, 65536 * ref_y,
	65536 * (r.urx - r.llx), 65536 * (r.ury - r.lly), 0);

    corner[0].x = rect.llx; corner[0].y = rect.lly;
    corner[1].x = rect.llx; corner[1].y = rect.ury;
    corner[2].x = rect.urx; corner[2].y = rect.ury;
    corner[3].x = rect.urx; corner[3].y = rect.lly;

    pdf_copymatrix(&P, &(p->matrix));
    for (i = 0; i < 4; ++i) {
      corner[i].x -= rect.llx;
      corner[i].y -= rect.lly;
      pdf_dev_transform(&(corner[i]), &P);
      corner[i].x += rect.llx;
      corner[i].y += rect.lly;
    }

    rect.llx = corner[0].x;
    rect.lly = corner[0].y;
    rect.urx = corner[0].x;
    rect.ury = corner[0].y;
    for (i = 0; i < 4; ++i) {
      if (corner[i].x < rect.llx)
	rect.llx = corner[i].x;
      if (corner[i].x > rect.urx)
	rect.urx = corner[i].x;
      if (corner[i].y < rect.lly)
	rect.lly = corner[i].y;
      if (corner[i].y > rect.ury)
	rect.ury = corner[i].y;
    }

    pdf_doc_expand_box(&rect);
  }

  return 0;
}


void
transform_info_clear (transform_info *info)
{
  /* Physical dimensions */
  info->width    = 0.0;
  info->height   = 0.0;
  info->depth    = 0.0;

  info->bbox.llx = 0.0;
  info->bbox.lly = 0.0;
  info->bbox.urx = 0.0;
  info->bbox.ury = 0.0;

  /* Transformation matrix */
  pdf_setmatrix(&(info->matrix), 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);

  info->flags    = 0;
}
