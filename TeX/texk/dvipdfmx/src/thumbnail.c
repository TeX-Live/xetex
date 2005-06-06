/*  $Header: /home/cvsroot/dvipdfmx/src/thumbnail.c,v 1.6 2004/01/30 18:34:24 hirata Exp $

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
#ifndef MIKTEX
#include <kpathsea/c-ctype.h>
#endif /* ! MIKTEX */

#include "system.h"
#include "mfileio.h"
#include "mem.h"
#include "pdfobj.h"
#include "thumbnail.h"

#ifdef HAVE_LIBPNG
#include "pngimage.h"

#define TMP "/tmp"

static char *guess_name (const char *thumb_filename)
{
  /* Build path name for anticipated thumbnail image */
  char *tmpdir, *tmpname;
#ifdef MIKTEX
  if (!(tmpdir = getenv ("MIKTEX_TMP"))
      && !(tmpdir = getenv ("MIKTEX_TEMP"))
      && !(tmpdir = getenv ("TMP"))
      && !(tmpdir = getenv ("TEMP")))
    tmpdir = TMP;
#else /* ! MIKTEX */
  if (!(tmpdir = getenv ("TMP")) &&
      !(tmpdir = getenv ("TEMP"))) 
    tmpdir = (char *)TMP;
#endif /* ! MIKTEX */
  tmpname = NEW (strlen(tmpdir)+strlen(thumb_filename)+strlen(DIR_SEP_STRING)+1,
		 char);
  strcpy (tmpname, tmpdir);
  if (!IS_DIR_SEP (tmpname[strlen(tmpname)-1])) {
    strcat (tmpname, DIR_SEP_STRING);
  }
  strcat (tmpname, thumb_filename);
  return tmpname;
}

static char thumbnail_remove_opt = 0;
void thumb_remove(void)
{
  thumbnail_remove_opt = 1;
}

pdf_obj *do_thumbnail (const char *thumb_filename) 
{
  pdf_obj *image_stream = NULL, *image_ref = NULL;
  int found_in_cwd = 0;
  FILE *thumb_file;
  char *guess_filename = NULL;
  guess_filename = guess_name (thumb_filename);
  if ((thumb_file = MFOPEN (thumb_filename, FOPEN_RBIN_MODE))) {
    found_in_cwd = 1;
  } else if (!(thumb_file = MFOPEN (guess_filename, FOPEN_RBIN_MODE))) {
    fprintf (stderr, "\nNo thumbnail file\n");
    return NULL;
  }
  if (!check_for_png (thumb_file)) {
    fprintf (stderr, "\nThumbnail not a png file! Skipping\n");
    return NULL;
  }
  rewind (thumb_file);

  if ((image_stream = start_png_image (thumb_file, NULL))) {
    image_ref = pdf_ref_obj (image_stream);
    pdf_release_obj (image_stream);
  } else {
    image_ref = NULL;
  }
  if (thumbnail_remove_opt && found_in_cwd) 
    remove (thumb_filename);
  else if (thumbnail_remove_opt)
    remove (guess_filename);
  if (guess_filename)
    RELEASE (guess_filename);
  MFCLOSE (thumb_file);
  return image_ref;
}

#endif /* HAVE_LIBPNG */
