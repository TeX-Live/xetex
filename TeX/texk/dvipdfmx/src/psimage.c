/*  $Header: /home/cvsroot/dvipdfmx/src/psimage.c,v 1.7 2004/01/30 18:34:23 hirata Exp $

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

#include "system.h"
#include "error.h"
#include "mem.h"
#include "mfileio.h"
#include "pdfobj.h"
#include "psimage.h"
#include "epdf.h"

static char * distiller_template = NULL;

void set_distiller_template (char *s) 
{
  distiller_template = NEW (strlen(s)+1, char);
  strcpy (distiller_template, s);
  return;
}

#define need(n) { unsigned k=(n); \
                 if (size+k>max_size) { \
                     max_size += k+128; \
                     result=RENEW(result,max_size,char); \
                       }}

static char *last_dot (char *s)
{
  char *end;
  end = s+strlen(s);
  while (--end > s) {
    if (*end == '.')
      return end;
  }
  return NULL;
}

#ifdef HAVE_SYSTEM  /* No need to build a command line if we don't
		       have system() */
static char *build_command_line (char *psname, char *pdfname)
{
  char *result = NULL, *current;
  int size = 0, max_size = 0;
  if (distiller_template) {
    need(strlen(distiller_template)+1);
    for (current =distiller_template; *current != 0; current ++) {
      if (*current == '%') {
	switch (*(++current)) {
	case 'o': /* Output file name */
	  need(strlen(pdfname));
	  strcpy (result+size, pdfname);
	  size+=strlen(pdfname);
	  break;
	case 'i': /* Input filename */
	  need(strlen(psname));
	  strcpy (result+size, psname);
	  size+=strlen(psname);
	  break;
	case 'b': 
	  {
	    char *last;
	    need(strlen(psname));
	    if ((last = last_dot (psname))) {
	      strncpy (result+size, psname, last-psname);
	      size += last-psname;
	    } else {
	      strcpy (result+size, psname);
	      size += strlen(psname);
	    }
	  }
	case 0:
	  break;
	case '%':
	  result[size++] = '%';
	}
      } else {
	result[size++] = *current;
      }
      result[size] = 0;
    }
  } else {
    WARN("Config file contains no template to perform PS -> PDF conversion.");
  }
  return result;
}
#endif

pdf_obj *ps_include (char *file_name, 
		     struct xform_info *p,
		     char *res_name, double x_user, double y_user)
{
#ifdef HAVE_SYSTEM
#if defined(HAVE_MKSTEMP) && !defined(MIKTEX)
#define USE_MKSTEMP 1
#endif
  pdf_obj *result = NULL;
  char *tmp, *cmd;
  FILE *pdf_file = NULL;
#ifdef MIKTEX
  char szTmp[_MAX_PATH];
#endif /* MIKETE */

  /* Get a full qualified tmp name */
#if   defined(MIKTEX)
  miktex_create_temp_file_2 (0, szTmp);
  tmp = szTmp;
#elif defined(USE_MKSTEMP)
  /*
   * mkstemp creates a template file with mode 0600.
   */
#define TMPDIR_DEFAULT "/tmp"
#define TEMPLATE "/dvipdfmx.XXXXXX"
  {
    char *tmpdir;
    int   fd;
#ifdef HAVE_GETENV
    tmpdir = getenv("TMPDIR");
    if (!tmpdir)
      tmpdir = (char *)TMPDIR_DEFAULT;
#else
    tmpdir = (char *)TMPDIR_DEFAULT;
#endif
    tmp = NEW(strlen(tmpdir)+strlen(TEMPLATE)+1, char);
    strcpy(tmp, tmpdir);
    strcat(tmp, TEMPLATE);
    fd = mkstemp(tmp);
    if (fd == -1) {
      WARN("Could not create file \"%s\".", tmp);
      RELEASE(tmp);
      return NULL;
    }
    close(fd);
  }
#else
  tmp = tmpnam (NULL);
#endif
  if ((cmd = build_command_line (file_name, tmp))) {
    if (!system (cmd) && (pdf_file = MFOPEN (tmp, FOPEN_RBIN_MODE))) {
      result = pdf_include_page (pdf_file, p, res_name);
    } else {
      WARN("Conversion via ->%s<- failed.", cmd);
    }
    if (pdf_file) {
      MFCLOSE (pdf_file);
#ifndef USE_MKSTEMP
      remove (tmp);
#endif
    }
    RELEASE (cmd);
  }
#ifdef USE_MKSTEMP
  remove(tmp);
  RELEASE(tmp);
#endif

  return result;
#else
  WARN("Cannot include PS/EPS files unless you have and enable system() command.");
  return NULL;
#endif /* HAVE_SYSTEM */
}

int check_for_ps (FILE *image_file) 
{
  rewind (image_file);
  mfgets (work_buffer, WORK_BUFFER_SIZE, image_file);
  if (!strncmp (work_buffer, "%!", 2))
    return 1;
  return 0;
}

void psimage_close(void)
{
  if (distiller_template)
    RELEASE (distiller_template);
}
