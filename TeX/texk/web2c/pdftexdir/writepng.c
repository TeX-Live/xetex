/*
Copyright (c) 1996-2004 Han The Thanh, <thanh@pdftex.org>

This file is part of pdfTeX.

pdfTeX is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

pdfTeX is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pdfTeX; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/writepng.c#13 $
*/

#include "ptexlib.h"
#include "image.h"

static const char perforce_id[] = 
    "$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/writepng.c#13 $";

void read_png_info(integer img)
{
    FILE *png_file = xfopen(img_name(img), FOPEN_RBIN_MODE);
    if ((png_ptr(img) = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
        NULL, NULL, NULL)) == NULL)
        pdftex_fail("libpng: png_create_read_struct() failed");
    if ((png_info(img) = png_create_info_struct(png_ptr(img))) == NULL)
        pdftex_fail("libpng: png_create_info_struct() failed");
    if (setjmp(png_ptr(img)->jmpbuf))
        pdftex_fail("libpng: internal error");
    png_init_io(png_ptr(img), png_file);
    png_read_info(png_ptr(img), png_info(img));
    if (png_info(img)->color_type & PNG_COLOR_MASK_ALPHA)
        png_set_strip_alpha(png_ptr(img));
    if (png_info(img)->bit_depth == 16)
        png_set_strip_16(png_ptr(img));
    png_read_update_info(png_ptr(img), png_info(img));
    img_width(img) =  png_info(img)->width;
    img_height(img) =  png_info(img)->height;
    if (png_info(img)->valid & PNG_INFO_pHYs) {
        img_xres(img) = 
            round(0.0254*png_get_x_pixels_per_meter(png_ptr(img), png_info(img)));
        img_yres(img) =
            round(0.0254*png_get_y_pixels_per_meter(png_ptr(img), png_info(img)));
    }
    switch (png_info(img)->color_type) {
    case PNG_COLOR_TYPE_PALETTE:
        img_color(img) = IMAGE_COLOR_C | IMAGE_COLOR_I;
        break;
    case PNG_COLOR_TYPE_GRAY:
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        img_color(img) = IMAGE_COLOR_B;
        break;
    case PNG_COLOR_TYPE_RGB:
    case PNG_COLOR_TYPE_RGB_ALPHA:
        img_color(img) = IMAGE_COLOR_C;
        break;
    default:
        pdftex_fail("unsupported type of color_type <%i>", png_info(img)->color_type);
    }
}

void write_png(integer img)
{
    int i, j, k, l;
    integer palette_objnum = 0;
    png_bytep row, r, *rows;
    pdf_puts("/Type /XObject\n/Subtype /Image\n");
    pdf_printf("/Width %i\n/Height %i\n/BitsPerComponent %i\n",
               (int)png_info(img)->width,
               (int)png_info(img)->height,
               (int)png_info(img)->bit_depth);
    pdf_puts("/ColorSpace ");
    switch (png_info(img)->color_type) {
    case PNG_COLOR_TYPE_PALETTE:
        pdfcreateobj(0, 0);
        palette_objnum = objptr;
        pdf_printf("[/Indexed /DeviceRGB %i %i 0 R]\n",
                   (int)(png_info(img)->num_palette - 1),
                   (int)palette_objnum);
        break;
    case PNG_COLOR_TYPE_GRAY:
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        pdf_puts("/DeviceGray\n");
        break;
    case PNG_COLOR_TYPE_RGB:
    case PNG_COLOR_TYPE_RGB_ALPHA:
        pdf_puts("/DeviceRGB\n");
        break;
    default:
        pdftex_fail("unsupported type of color_type <%i>", png_info(img)->color_type);
    }
    pdfbeginstream();
    if (png_info(img)->interlace_type == PNG_INTERLACE_NONE) {
        row = xtalloc(png_info(img)->rowbytes, png_byte);
        for (i = 0; i < (int)png_info(img)->height; i++) {
            png_read_row(png_ptr(img), row, NULL);
	    r = row;
	    k = png_info(img)->rowbytes;
	    while(k > 0) {
		l = (k > pdfbufsize)? pdfbufsize : k;
		pdfroom(l);
		for (j = 0; j < l; j++)
		    pdfbuf[pdfptr++] = *r++;
		k -= l;
	    }
        }
        xfree(row);
    }
    else {
        if (png_info(img)->height*png_info(img)->rowbytes >= 10240000L)
            pdftex_warn("large interlaced PNG might cause out of memory (use non-interlaced PNG to fix this)");
        rows = xtalloc(png_info(img)->height, png_bytep);
        for (i = 0; i < png_info(img)->height; i++)
            rows[i] = xtalloc(png_info(img)->rowbytes, png_byte);
        png_read_image(png_ptr(img), rows);
        for (i = 0; i < (int)png_info(img)->height; i++) {
            row = rows[i];
	    k = png_info(img)->rowbytes;
	    while(k > 0) {
		l = (k > pdfbufsize)? pdfbufsize : k;
		pdfroom(l);
		for (j = 0; j < l; j++)
		    pdfbuf[pdfptr++] = *row++;
		k -= l;
	    }
            xfree(rows[i]);
        }
        xfree(rows);
    }
    pdfendstream();
    if (palette_objnum > 0) {
        pdfbegindict(palette_objnum);
        pdfbeginstream();
        for (i = 0; i < png_info(img)->num_palette; i++) {
            pdfroom(3);
            pdfbuf[pdfptr++] = png_info(img)->palette[i].red;
            pdfbuf[pdfptr++] = png_info(img)->palette[i].green;
            pdfbuf[pdfptr++] = png_info(img)->palette[i].blue;
        }
        pdfendstream();
    }
    pdfflush();
}
