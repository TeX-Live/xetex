/*  $Header: /home/cvsroot/dvipdfmx/src/jpegimage.c,v 1.2 2003/12/07 11:28:27 hirata Exp $

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

/*
 * JPEG Support.
 *
 *  Adobe Photoshop has a problem that it writes inverted CMYK data in JPEG files.
 */

#include "system.h"
#include "error.h"
#include "mem.h"

#include "mfileio.h"
#include "numbers.h"

#include "pdfobj.h"

#include "jpegimage.h"

#define JPEG_DEBUG_STR "JPEG"
#define JPEG_DEBUG     3

/*
 * #ifdef HAVE_LIBJPEG
 * #include <jpeglib.h>
 * #endif
 */

typedef struct xi_info_struct
{
  long width;
  long height;
  int  bpc;
  int  components;
  int  invert;
  long min_dpi; /* NOT USED YET */
} xi_info;

/*
 * JPEG Markers
 */
#define SOF0	0xc0
#define SOF1	0xc1
#define SOF2	0xc2
#define SOF3	0xc3
#define SOF5	0xc5
#define SOF6	0xc6
#define SOF7	0xc7
#define SOF9	0xc9
#define SOF10	0xca
#define SOF11	0xcb
#define SOF13	0xcd
#define SOF14	0xce
#define SOF15	0xcf
#define SOI	0xd8
#define EOI	0xd9
#define SOS	0xda
#define COM	0xfe
/*
 * Application-Specific JPEG Markers
 *  0xe0: used by JFIF
 *  0xee: used by Adobe
 */
#define APP0    0xe0
#define APPE    0xee

static int read_header (xi_info *info, FILE *fp);
static int get_marker  (unsigned char *marker, FILE *fp);

int
check_for_jpeg (FILE *fp)
{
  unsigned char jpeg_sig[2];

  rewind(fp);
  if (fread(jpeg_sig, sizeof(unsigned char), 2, fp) != 2)
    return 0;
  else if (jpeg_sig[0] != 0xff || jpeg_sig[1] != SOI)
    return 0;

  return 1;
}

pdf_obj *
jpeg_start_image (FILE *fp)
{
  pdf_obj *xobj;
  pdf_obj *dict;
  xi_info  info;

  if (!check_for_jpeg(fp) || read_header(&info, fp) < 0) {
    WARN("%s: Not a JPEG file?", JPEG_DEBUG_STR);
    return NULL;
  }

  /*
   * JPEG image is DCTEncode'd.
   */
  xobj = pdf_new_stream(0);
  dict = pdf_stream_dict(xobj);
  pdf_add_dict(dict, pdf_new_name("Width"),  pdf_new_number(info.width));
  pdf_add_dict(dict, pdf_new_name("Height"), pdf_new_number(info.height));
  pdf_add_dict(dict, pdf_new_name("BitsPerComponent"), pdf_new_number(info.bpc));
  switch (info.components) {
  case 1:
    pdf_add_dict(dict, pdf_new_name("ColorSpace"), pdf_new_name("DeviceGray"));
    break;
  case 3:
    pdf_add_dict(dict, pdf_new_name("ColorSpace"), pdf_new_name("DeviceRGB"));
    break;
  case 4:
    pdf_add_dict(dict, pdf_new_name("ColorSpace"), pdf_new_name("DeviceCMYK"));
    break;
  default:
    WARN("%s: Number of color component not 1/3/4: %d", JPEG_DEBUG_STR, info.components);
    pdf_release_obj(xobj);
    return NULL;
  }

  /*
   * From jpeg2ps
   */
  if (info.invert) {
    pdf_obj *tmp;
    int      i;
    tmp = pdf_new_array();
    for (i = 0; i < info.components; i++) {
      pdf_add_array(tmp, pdf_new_number(1.0));
      pdf_add_array(tmp, pdf_new_number(0.0));
    }
    pdf_add_dict(dict, pdf_new_name("Decode"), tmp);
  }

  pdf_add_dict(dict, pdf_new_name("Filter"), pdf_new_name("DCTDecode"));

  rewind(fp);
  {
    long length;
    while ((length = fread(work_buffer, sizeof (char), WORK_BUFFER_SIZE, fp)) > 0)
      pdf_add_stream(xobj, work_buffer, length);
  }

  return xobj;
}

#ifdef DEBUG
/*
 * DEBUG
 */
static unsigned short dump_APP0 (FILE *fp, unsigned short length);
static unsigned short dump_JFIF (FILE *fp, unsigned short length);
static unsigned short dump_JFXX (FILE *fp, unsigned short length);

static unsigned short
dump_APP0 (FILE *fp, unsigned short length)
{
  unsigned short len;
  /*
   * 'J' 'F' 'I' 'F' '\0'
   * 'J' 'F' 'X' 'X' '\0'
   */
  char ident[5];

  if ((len = fread(ident, sizeof(char), 5, fp)) != 5 ||
      ident[4] != '\0')
    return len;

  if (!strcmp(ident, "JFIF"))
    len += dump_JFIF(fp, length-len);
  else if (!strcmp(ident, "JFXX"))
    len += dump_JFXX(fp, length-len);

  return len;
}

static unsigned short
dump_JFIF (FILE *fp, unsigned short length)
{
  unsigned char major, minor;
  unsigned char units;
  unsigned short density_x, density_y;
  unsigned char thumbpc_x, thumbpc_y;

  major = get_unsigned_byte(fp);
  minor = get_unsigned_byte(fp);
  units = get_unsigned_byte(fp);
  density_x = get_unsigned_pair(fp);
  density_y = get_unsigned_pair(fp);
  thumbpc_x = get_unsigned_byte(fp);
  thumbpc_y = get_unsigned_byte(fp);

  MESG("\n");
  MESG("JFIF\n");
  MESG("  Version: %u.%u\n", major, minor);
  MESG("  Units  :");
  switch (units) {
  case 0:
    MESG("None (aspect ratio only)");
    break;
  case 1:
    MESG("Dots per inch");
    break;
  case 2:
    MESG("Dots per cm");
    break;
  default:
    MESG("Unknown");
  }
  MESG("\n");
  MESG("  Density: %u %u\n", density_x, density_y);
  MESG("  Thumbnail: %ux%u pixels\n", thumbpc_x, thumbpc_y);
  MESG("  Thumbnail Data:\n");
  {
    long i;
    for (i = 0; i < thumbpc_x*thumbpc_y; i++) {
      if (i % thumbpc_x == 0)
	MESG("\n    ");
      MESG("%X", get_unsigned_byte(fp));
      MESG("%X", get_unsigned_byte(fp));
      MESG("%X", get_unsigned_byte(fp));
    }
  }
  MESG("\n");

  return 9 + 3*thumbpc_x*thumbpc_y;
}

static unsigned short
dump_JFXX (FILE *fp, unsigned short length)
{
  unsigned char extension_code;

  extension_code = get_unsigned_byte(fp);
  MESG("\n");
  MESG("JFXX\n");
  MESG("  Extension Code: ");
  switch (extension_code) {
  case 0x10:
    MESG("Thumbnail coded using JPEG");
    break;
  case 0x11:
    MESG("Thumbnail stored using 1 byte/pixel");
    break;
  case 0x13:
    MESG("Thumbnail stored using 3 bytes/pixel");
    break;
  default:
    MESG("Unknown");
  }
  MESG("\n");
  {
    unsigned short i;
    for (i = 0; i < length - 1; i++)
      get_unsigned_byte(fp);
  }
  MESG("Thumbnail: << %u bytes skipped. >>", length-1);
  MESG("\n");

  return length;
}
#endif /* DEBUG */

static int
get_marker (unsigned char *marker, FILE *fp)
{
  unsigned char c;

  c = get_unsigned_byte(fp);
  if (c != 0xff)
    return -1;

  for (;;) {
    if ((c = get_unsigned_byte(fp)) != 0xff) {
      *marker = c;
      return 0;
    }
  }

  return -1;
}

static int
read_header (xi_info *info, FILE *fp)
{
  unsigned char  marker;
  unsigned short length;
  int found_SOFn, is_Adobe_ext;

  found_SOFn = 0; is_Adobe_ext = 0; marker = 0;
  while (!found_SOFn && get_marker(&marker, fp) >= 0) {
    length = get_unsigned_pair(fp) - 2;
    switch (marker) {
    case SOF0:  case SOF1:  case SOF2:  case SOF3:
    case SOF5:  case SOF6:  case SOF7:  case SOF9:
    case SOF10: case SOF11: case SOF13: case SOF14:
    case SOF15:
      info->bpc    = (int)  get_unsigned_byte(fp);
      info->height = (long) get_unsigned_pair(fp);
      info->width  = (long) get_unsigned_pair(fp);
      info->components = (int) get_unsigned_byte(fp);
      found_SOFn = 1;
      break;
    case APPE:
      {
	char sig[5];
	if (fread(sig, sizeof(char), 5, fp) != 5)
	  return -1;
	if (!memcmp(sig, "Adobe", 5))
	  is_Adobe_ext = 1;
	length -= 5;
	while (length-- > 0)
	  get_unsigned_byte(fp);
      }
      break;
#ifdef DEBUG
    case APP0:
      length -= dump_APP0(fp, length);
      while (length-- > 0)
	get_unsigned_byte(fp);
      break;
#endif /* DEBUG */
    default:
      while (length-- > 0)
	get_unsigned_byte(fp);
    }
  }

  /*
   * From jpeg2ps
   */
  if (found_SOFn && is_Adobe_ext && info->components == 4) {
    WARN("%s: Found Adobe CMYK JPEG.", JPEG_DEBUG_STR);
    info->invert = 1;
  } else
    info->invert = 0;

  return (found_SOFn ? 0 : -1);
}
