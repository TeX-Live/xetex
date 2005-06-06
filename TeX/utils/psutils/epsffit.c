/* epsffit.c
 * Copyright (C) Angus J. C. Duggan 1991-1995
 * See file LICENSE for details.
 *
 * fit epsf file into constrained size
 * Usage:
 *       epsffit [-c] [-r] [-a] [-s] llx lly urx ury [infile [outfile]]
 *               -c centres the image in the bounding box given
 *               -r rotates the image by 90 degrees anti-clockwise
 *               -a alters the aspect ratio to fit the bounding box
 *               -s adds a showpage at the end of the image
 *
 * Added filename spec (from Larry Weissman) 5 Feb 93
 * Accepts double %%BoundingBox input, outputs proper BB, 4 Jun 93. (I don't
 * like this; developers should read the Big Red Book before writing code which
 * outputs PostScript.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "psutil.h"
#include "pserror.h"
#include "patchlev.h"
#include "config.h"

#define MIN(x,y) ((x) > (y) ? (y) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

char *program;

static void usage(void)
{
   fprintf(stderr, "%s release %d patchlevel %d\n", program, RELEASE, PATCHLEVEL);
   fprintf(stderr, "Copyright (C) Angus J. C. Duggan, 1991-1995. See file LICENSE for details.\n");
   fprintf(stderr, "Usage: %s [-c] [-r] [-a] [-s] llx lly urx ury [infile [outfile]]\n",
	   program);
   exit(1);
}

void main(int argc, char **argv)
{
   int bbfound = 0;              /* %%BoundingBox: found */
   int urx, ury, llx, lly;
   int furx, fury, fllx, flly;
   int showpage = 0, centre = 0, rotate = 0, aspect = 0, maximise = 0;
   char buf[BUFSIZ];
   FILE *input;
   FILE *output;

   program = *argv++; argc--;

   while (argc > 0 && argv[0][0] == '-') {
      switch (argv[0][1]) {
      case 'c': centre = 1; break;
      case 's': showpage = 1; break;
      case 'r': rotate = 1; break;
      case 'a': aspect = 1; break;
      case 'm': maximise = 1; break;
      case 'v':
      default:  usage();
      }
      argc--;
      argv++;
   }

   if (argc < 4 || argc > 6) usage();
   fllx = atoi(argv[0]);
   flly = atoi(argv[1]);
   furx = atoi(argv[2]);
   fury = atoi(argv[3]);

   if (argc > 4) {
      if(!(input = fopen(argv[4], OPEN_READ)))
	 message(FATAL, "can't open input file %s\n", argv[4]);
   } else {
#if defined(MSDOS) || defined(WINNT) || defined(WIN32)
      int fd = fileno(stdin) ;
      if ( setmode(fd, O_BINARY) < 0 )
         message(FATAL, "can't reset stdin to binary mode\n");
#endif
      input = stdin ;
    }

   if (argc > 5) {
      if(!(output = fopen(argv[5], OPEN_WRITE)))
	 message(FATAL, "can't open output file %s\n", argv[5]);
   } else {
#if defined(MSDOS) || defined(WINNT) || defined(WIN32)
      int fd = fileno(stdout) ;
      if ( setmode(fd, O_BINARY) < 0 )
         message(FATAL, "can't reset stdout to binary mode\n");
#endif
      output = stdout ;
    }

   while (fgets(buf, BUFSIZ, input)) {
      if (buf[0] == '%' && (buf[1] == '%' || buf[1] == '!')) {
	 /* still in comment section */
	 if (!strncmp(buf, "%%BoundingBox:", 14)) {
	    double illx, illy, iurx, iury;	/* input bbox parameters */
	    if (sscanf(buf, "%%%%BoundingBox:%lf %lf %lf %lf\n",
		       &illx, &illy, &iurx, &iury) == 4) {
	       bbfound = 1;
	       llx = (int)illx;	/* accept doubles, but convert to int */
	       lly = (int)illy;
	       urx = (int)(iurx+0.5);
	       ury = (int)(iury+0.5);
	    }
	 } else if (!strncmp(buf, "%%EndComments", 13)) {
	    strcpy(buf, "\n"); /* don't repeat %%EndComments */
	    break;
	 } else fputs(buf, output);
      } else break;
   }

   if (bbfound) { /* put BB, followed by scale&translate */
      int fwidth, fheight;
      double xscale, yscale;
      double xoffset = fllx, yoffset = flly;
      double width = urx-llx, height = ury-lly;

      if (maximise)
	 if ((width > height && fury-flly > furx-fllx) ||
	     (width < height && fury-flly < furx-fllx)) 
	    rotate = 1;

      if (rotate) {
	 fwidth = fury - flly;
	 fheight = furx - fllx;
      } else {
	 fwidth = furx - fllx;
	 fheight = fury - flly;
      }

      xscale = fwidth/width;
      yscale = fheight/height;

      if (!aspect) {       /* preserve aspect ratio ? */
	 xscale = yscale = MIN(xscale,yscale);
      }
      width *= xscale;     /* actual width and height after scaling */
      height *= yscale;
      if (centre) {
	 if (rotate) {
	    xoffset += (fheight - height)/2;
	    yoffset += (fwidth - width)/2;
	 } else {
	    xoffset += (fwidth - width)/2;
	    yoffset += (fheight - height)/2;
	 }
      }
      fprintf(output, 
	      "%%%%BoundingBox: %d %d %d %d\n", (int)xoffset, (int)yoffset,
	     (int)(xoffset+(rotate ? height : width)),
	     (int)(yoffset+(rotate ? width : height)));
      if (rotate) {  /* compensate for original image shift */
	 xoffset += height + lly * yscale;  /* displacement for rotation */
	 yoffset -= llx * xscale;
      } else {
	 xoffset -= llx * xscale;
	 yoffset -= lly * yscale;
      }
      fputs("%%EndComments\n", output);
      if (showpage)
	 fputs("save /showpage{}def /copypage{}def /erasepage{}def\n", output);
      else
	 fputs("%%BeginProcSet: epsffit 1 0\n", output);
      fputs("gsave\n", output);
      fprintf(output, "%.3f %.3f translate\n", xoffset, yoffset);
      if (rotate)
	 fputs("90 rotate\n", output);
      fprintf(output, "%.3f %.3f scale\n", xscale, yscale);
      if (!showpage)
	 fputs("%%EndProcSet\n", output);
   }
   do {
      fputs(buf, output);
   } while (fgets(buf, BUFSIZ, input));
   if (bbfound) {
      fputs("grestore\n", output);
      if (showpage)
	 fputs("restore showpage\n", output); /* just in case */
   } else
      message(FATAL, "no %%%%BoundingBox:\n");

   exit(0);
}
