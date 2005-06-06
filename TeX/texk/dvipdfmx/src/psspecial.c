/*  $Header: /home/cvsroot/dvipdfmx/src/psspecial.c,v 1.5 2004/01/30 18:34:23 hirata Exp $
    
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
#include "system.h"
#include "mem.h"
#include "mfileio.h"
#include "psspecial.h"
#include "pdfparse.h"
#include "pdfspecial.h"
#include "psimage.h"
#include "mpost.h"
#include "pdfdoc.h"

#define HOFFSET 1
#define VOFFSET 2
#define HSIZE   3
#define VSIZE   4
#define HSCALE  5
#define VSCALE  6
#define ANGLE   7
#define CLIP    8
#define LLX     9
#define LLY    10
#define URX    11
#define URY    12
#define RWI    13
#define RHI    14

static struct keys
{
  const char *key;
  int id;
} keys[] = {
  {"hoffset", HOFFSET},
  {"voffset", VOFFSET},
  {"hsize", HSIZE},
  {"vsize", VSIZE},
  {"hscale", HSCALE},
  {"vscale", VSCALE},
  {"angle", ANGLE},
  {"clip", CLIP},
  {"llx", LLX},
  {"lly", LLY},
  {"urx", URX},
  {"ury", URY},
  {"rwi", RWI},
  {"rhi", RHI},
};

static int parse_psfile (char **start, char *end, double x_user, double y_user) 
{
  char *key, *val, *filename = NULL;
  double hoffset = 0.0, voffset = 0.0;
  double hsize = 0.0, vsize = 0.0;
  int error = 0;
  struct xform_info *p = new_xform_info();
  skip_white(start, end);
  parse_key_val (start, end, &key, &val);
  if (key && val) {
    filename = val;
    RELEASE (key);
    skip_white (start, end);
    while (*start < end) {
      parse_key_val (start, end, &key, &val);
      if (key) {
	int i;
	for (i=0; i<sizeof(keys)/sizeof(keys[0]); i++) {
	  if (!strcmp (key, keys[i].key))
	    break;
	}
	if (i == sizeof(keys)/sizeof(keys[0])) {
	  fprintf (stderr, "\nUnknown key in special: %s\n", key);
	  break;
	}
	if (val) {
	  if (is_a_number(val)) {
	    switch (keys[i].id) {
	    case HOFFSET:
	      hoffset = atof (val);
	      break;
	    case VOFFSET:
	      voffset = atof (val);
	      break;
	    case HSIZE:
	      hsize = atof (val);
	      break;
	    case VSIZE:
	      vsize = atof (val);
	      break;
	    case HSCALE:
	      p -> xscale = atof(val)/100.0;
	      break;
	    case VSCALE:
	      p -> yscale = atof(val)/100.0;
	      break;
	    case ANGLE:
	      p -> rotate = atof(val)*M_PI/180.0;
	      break;
	    case LLX:
	      p -> user_bbox = 1;
	      p -> u_llx = atof(val);
	      break;
	    case LLY:
	      p -> user_bbox = 1;
	      p -> u_lly = atof(val);
	      break;
	    case URX:
	      p -> user_bbox = 1;
	      p -> u_urx = atof(val);
	      break;
	    case URY:
	      p -> user_bbox = 1;
	      p -> u_ury = atof(val);
	      break;
	    case RWI:
	      p -> width = atof(val)/10.0;
	      break;
	    case RHI:
	      p -> height = atof(val)/10.0;
	      break;
	    default:
	      if (keys[i].id == CLIP) {
		fprintf (stderr, "\nPSfile key \"clip\" takes no value\n");
	      } else
		fprintf (stderr, "\nPSfile key \"%s=%s\" not recognized\n",
			 key, val);
	      error = 1;
	    }
	  } else {
	    fprintf (stderr, "\nPSfile key \"%s\" assigned nonnumeric value\n", key);
	    error = 1;
	  }
	  RELEASE (val);
	} else {  /* Keywords without values */
	  switch (keys[i].id) {
	  case CLIP:
	    p -> clip  = 1;
	    break;
	  default:
	    fprintf (stderr, "\nPSfile key \"%s\" needs a value\n",
		     key);
	    error = 1;
	  }
	}
	RELEASE (key);
      } else {
	fprintf (stderr, "\nInvalid keyword in PSfile special\n");
	dump (*start, end);
	break;
      }
      skip_white (start, end);
    } /* If here and *start == end we got something */
    if (*start == end && validate_image_xform_info (p)) {
      pdf_obj *result;
      result = embed_image (filename, p, x_user, y_user, NULL);
      if (result)
	pdf_release_obj (result);
    }
  } else {
    fprintf (stderr, "\nPSfile special has no filename\n");
    error = 1;
  }
  if (filename)
    RELEASE (filename);
  release_xform_info (p);
  return !error;
}

static void do_texfig (char **start, char *end)
{
  char *filename;
  struct xform_info *p;
  skip_white (start, end);
  if (*start < end && (filename = parse_val_ident (start, end))) {
    p = new_xform_info (); /* Leave this empty */
    p-> yscale = -1;
    if (validate_image_xform_info (p)) {
      pdf_obj *result;
      result = embed_image (filename, p, 0.0, 0.0, NULL);
      if (result)
	pdf_release_obj (result);
    }
    release_xform_info (p);
    RELEASE (filename);
  } else {
    fprintf (stderr, "Expecting filename here:\n");
    dump (*start, end);
  }
}


int ps_parse_special (char *buffer, UNSIGNED_QUAD size, double x_user,
		      double y_user)
{
  char *start = buffer, *end;
  static int block_pending = 0;
  static double pending_x=0.0, pending_y=0.0;
  int result = 0;
  end = buffer + size;
  skip_white (&start, end);
  if (!strncmp (start, "PSfile", strlen("PSfile")) ||
      !strncmp (start, "psfile", strlen("PSfile"))) {
    result = 1; /* This means it is a PSfile special, not that it was
		   successful */
    parse_psfile(&start, end, x_user, y_user);
  } else if (!strncmp (start, "ps::[begin]", strlen("ps::[begin]"))) {
    start += strlen("ps::[begin]");
    block_pending = 1;
    pending_x = x_user;
    pending_y = y_user;
    result = 1; /* Likewise */
    do_raw_ps_special (&start, end, 1, x_user, y_user);
  } else if (!strncmp (start, "ps::[end]", strlen("ps::[end]"))) {
    if (block_pending) {
      start += strlen("ps::[end]");
      do_raw_ps_special (&start, end, 1, pending_x, pending_y);
      block_pending = 0;
    } else {
      fprintf (stderr, "\nps::[end] without ps::[begin] ignored.\n");
    }
    result = 1; /* Likewise */
  } else if (!strncmp (start, "ps: plotfile", strlen("ps: plotfile"))) {
    /* This is a bizarre, ugly  special case.. Not really postscript
       code */
    start += strlen ("ps: plotfile");
    result = 1;
    do_texfig (&start, end);
  } else if (!strncmp (start, "ps::", strlen("ps::")) ||
	     !strncmp (start, "PS::", strlen("PS::"))) {
    /* dvipdfm doesn't distinguish between ps:: and ps: */
    start += 4;
    result = 1; /* Likewise */
    do_raw_ps_special (&start, end, 1, 
		       block_pending?pending_x:x_user, block_pending?pending_y:y_user);
  } else if (!strncmp (start, "ps:", strlen("ps:")) ||
	     !strncmp (start, "PS:", strlen("PS:"))) {
    start += 3;
    result = 1; /* Likewise */
    do_raw_ps_special (&start, end, 1,
		       block_pending?pending_x:x_user, block_pending?pending_y:y_user);
  } else if (!strncmp (start, "postscriptbox", strlen("postscriptbox"))) {
    char filename[256];
    double width, height;

    if (sscanf (start+13, "{%lfpt}{%lfpt}{%256[^}]}",
		&width, &height, filename) == 3) {
      struct xform_info *p = new_xform_info();

      p -> width = width*72/72.27;
      p -> height = height*72/72.27;

      {
	FILE *image_file;
	char *kpse_file_name;
	char buf[256], *pos;
	if ((kpse_file_name = kpse_find_pict (filename)) &&
	    (image_file = MFOPEN (kpse_file_name, FOPEN_R_MODE))) {
	  while (fgets (buf, 256, image_file)) {
	    if ((pos = strstr(buf, "%%BoundingBox:" )) != NULL){
	      double llx, lly, urx, ury;

	      while (*pos++ != ':' );
	      if (sscanf (pos, "%lf %lf %lf %lf",
			  &llx, &lly, &urx, &ury) == 4) {
		p -> u_llx = llx;
		p -> u_lly = lly;
		p -> u_urx = urx;
		p -> u_ury = ury;
		p -> user_bbox = 1;
		break;
	      }
	    }
	  }
	  MFCLOSE (image_file);
	}
      }
             
      if(p -> user_bbox && validate_image_xform_info (p)) {
	pdf_obj *embeded;
	embeded = embed_image (filename, p, x_user, y_user, NULL);
	if (embeded)
	  pdf_release_obj (embeded);
	result = 1;
      }
      
      release_xform_info (p);
    }
  }

  return result;
}

