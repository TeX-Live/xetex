/*
Copyright (c) 1996-2002 Han The Thanh, <thanh@pdftex.org>

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

$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/writet3.c#14 $
*/

#include "ptexlib.h"
#include <kpathsea/tex-glyph.h>
#include <kpathsea/magstep.h>

#define T3_BUF_SIZE   1024

static const char perforce_id[] = 
    "$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/writet3.c#14 $";

/* define t3_line_ptr, t3_line_array & t3_line_limit */
typedef char t3_line_entry;
define_array(t3_line);   

FILE *t3_file;
static boolean t3_image_used;

static integer t3_char_procs[MAX_CHAR_CODE + 1];
static float t3_char_widths[MAX_CHAR_CODE + 1];
static int t3_glyph_num;
static float t3_font_scale;
static integer t3_b0, t3_b1, t3_b2, t3_b3;
static boolean is_pk_font;

#define t3_open()       \
    open_input(&t3_file, kpse_miscfonts_format, FOPEN_RBIN_MODE)
#define t3_close()      xfclose(t3_file, cur_file_name)
#define t3_getchar()    xgetc(t3_file)
#define t3_eof()        feof(t3_file)
#define t3_prefix(s)    (!strncmp(t3_line_array, s, strlen(s)))
#define t3_putchar(c)   pdfout(c)

#define t3_check_eof()                                     \
    if (t3_eof())                                          \
        pdftex_fail("unexpected end of file");

static void t3_getline(void) 
{
    int c;
restart:
    t3_line_ptr = t3_line_array;
    c = t3_getchar();
    while (!t3_eof()) {
        alloc_array(t3_line, 1, T3_BUF_SIZE);
        append_char_to_buf(c, t3_line_ptr, t3_line_array, t3_line_limit);
        if (c == 10)
            break;
        c = t3_getchar();
    }
    alloc_array(t3_line, 2, T3_BUF_SIZE);
    append_eol(t3_line_ptr, t3_line_array, T3_BUF_SIZE);
    if (t3_line_ptr - t3_line_array < 2 || *t3_line_array == '%') {
        if (!t3_eof())
            goto restart;
    }
}

static void t3_putline(void)
{
    char *p = t3_line_array;
    while (p < t3_line_ptr)
        t3_putchar(*p++);
}

static void update_bbox(integer llx, integer lly, integer urx, integer ury, boolean is_first_glyph)
{
    if (is_first_glyph) {
        t3_b0 = llx;
        t3_b1 = lly;
        t3_b2 = urx;
        t3_b3 = ury;
    }
    else {
        if (llx < t3_b0)
            t3_b0 = llx;
        if (lly < t3_b1)
            t3_b1 = lly;
        if (urx > t3_b2)
            t3_b2 = urx;
        if (ury > t3_b3)
            t3_b3 = ury;
    }
}

static void t3_write_glyph(internalfontnumber f)
{
    static char t3_begin_glyph_str[] = "\\pdfglyph";
    static char t3_end_glyph_str[] = "\\endglyph";
    int glyph_index;
    int width, height, depth, llx, lly, urx, ury;
    char *p;
    t3_getline();
    if (t3_prefix(t3_begin_glyph_str)) {
        if (sscanf(t3_line_array + strlen(t3_begin_glyph_str) + 1,
                   "%i %i %i %i %i %i %i %i =", &glyph_index,
                   &width, &height, &depth, &llx, &lly, &urx, &ury) != 8) {
            remove_eol(p, t3_line_array);
            pdftex_warn("invalid glyph preamble: `%s'", t3_line_array);
            return;
        }
        if (glyph_index < fontbc[f] || glyph_index > fontec[f])
            return;
    }
    else
        return;
    if (!pdfcharmarked(f, glyph_index)) {
        while (!t3_prefix(t3_end_glyph_str)) {
            t3_check_eof();
            t3_getline();
        }
        return;
    }
    update_bbox(llx, lly, urx, ury, t3_glyph_num == 0);
    t3_glyph_num++;
    pdfnewdict(0, 0);
    t3_char_procs[glyph_index] = objptr;
    if (width == 0) 
        t3_char_widths[glyph_index] = 
            (getcharwidth(f, glyph_index)/t3_font_scale) / pdffontsize[f];
    else
        t3_char_widths[glyph_index] = width;
    pdfbeginstream();
    t3_getline();
    pdf_printf("%i 0 %i %i %i %i d1\nq\n", 
               (int)t3_char_widths[glyph_index], (int)llx,
               (int)lly, (int)urx, (int)ury);
    while (!t3_prefix(t3_end_glyph_str)) {
        t3_check_eof();
        if (t3_prefix("BI"))
            t3_image_used = true;
        t3_putline();
        t3_getline();
    }
    pdf_puts("Q\n");
    pdfendstream();
}

static integer get_pk_font_scale(internalfontnumber f)
{
    return 
        dividescaled(pkscalefactor, 
            dividescaled(pdffontsize[f], onehundredbp, fixeddecimaldigits + 2),
            0);
}

static integer pk_char_width(internalfontnumber f, scaled w)
{
    return 
        dividescaled(dividescaled(w, pdffontsize[f], 7),
                     get_pk_font_scale(f), 0);
}

scaled getpkcharwidth(internalfontnumber f, scaled w)
{
    return (get_pk_font_scale(f)/100000.0)*
           (pk_char_width(f, w)/100.0)*
           pdffontsize[f];
}

static boolean writepk(internalfontnumber f)
{
    kpse_glyph_file_type font_ret;
    integer llx, lly, urx, ury;
    integer cw, rw, i, j;
    halfword *row;
    char *name;
    chardesc cd;
    boolean is_null_glyph, check_preamble;
    integer dpi;
    int e;
    dpi = kpse_magstep_fix(
             round(fixedpkresolution*(((float)pdffontsize[f])/fontdsize[f])),
             fixedpkresolution, NULL);
    cur_file_name = makecstring(fontname[f]);
    name = kpse_find_pk(cur_file_name, (unsigned)dpi, &font_ret);
    if (name == NULL ||
        !FILESTRCASEEQ(cur_file_name, font_ret.name) ||
        !kpse_bitmap_tolerance((float)font_ret.dpi, (float)dpi)) {
        pdftex_warn("Font %s at %i not found", cur_file_name, (int)dpi);
        cur_file_name = NULL;
        return false;
    }
    t3_file = xfopen(name, FOPEN_RBIN_MODE);
    t3_image_used = true;
    is_pk_font = true;
    tex_printf(" <%s", (char *)name);
    cd.rastersize = 256;
    cd.raster = xtalloc(cd.rastersize, halfword);
    check_preamble = true;
    while (readchar(check_preamble, &cd) != 0) {
        check_preamble = false;
        if (!pdfcharmarked(f, cd.charcode))
            continue;
        t3_char_widths[cd.charcode] = 
            pk_char_width(f, getcharwidth(f, cd.charcode));
        if (cd.cwidth < 1 || cd.cheight < 1) {
            cd.xescape = cd.cwidth = round(t3_char_widths[cd.charcode]/100.0);
            cd.cheight = 1;
            cd.xoff = 0;
            cd.yoff = 0;
            is_null_glyph = true;
        }
        else
            is_null_glyph = false;
        llx = -cd.xoff;
        lly = cd.yoff - cd.cheight + 1;
        urx = cd.cwidth + llx + 1;
        ury = cd.cheight + lly;
        update_bbox(llx, lly, urx, ury, t3_glyph_num == 0);
        t3_glyph_num++;
        pdfnewdict(0, 0);
        t3_char_procs[cd.charcode] = objptr;
        pdfbeginstream();
        pdfprintreal(t3_char_widths[cd.charcode], 2);
        pdf_printf(" 0 %i %i %i %i d1\n", 
                    (int)llx, (int)lly, (int)urx, (int)ury);
        if (is_null_glyph)
            goto end_stream;
        pdf_printf("q\n%i 0 0 %i %i %i cm\nBI\n", (int)cd.cwidth,
                   (int)cd.cheight, (int)llx, (int)lly);
        pdf_printf("/W %i\n/H %i\n", 
                   (int)cd.cwidth, (int)cd.cheight);
        pdf_puts("/IM true\n/BPC 1\n/D [1 0]\nID ");
        cw = (cd.cwidth + 7)/8;
        rw = (cd.cwidth + 15)/16;
        row = cd.raster;
        for (i = 0; i < cd.cheight; i++) {
            for (j = 0; j < rw - 1; j++) {
                pdfout(*row/256);
                pdfout(*row%256);
                row++;
            }
            pdfout(*row/256);
            if (2*rw == cw)
                pdfout(*row%256);
            row++;
        }
        pdf_puts("\nEI\nQ\n");
end_stream:
        pdfendstream();
    }
    xfree(cd.raster);
    cur_file_name = NULL;
    return true;
}

void writet3(int objnum, internalfontnumber f)
{
    static char t3_font_scale_str[] = "\\pdffontscale";
    int i, e;
    int first_char, last_char;
    integer pk_font_scale;
    boolean is_notdef;
    t3_glyph_num = 0;
    t3_image_used = false;
    for (i = 0; i <= MAX_CHAR_CODE; i++) {
        t3_char_procs[i] = 0;
        t3_char_widths[i] = 0;
    }
    packfilename(fontname[f], getnullstr(), maketexstring(".pgc"));
    cur_file_name = makecstring(makenamestring());
    is_pk_font = false;
    if (!t3_open()) {
        if (writepk(f))
            goto write_font_dict;
        else {
            cur_file_name = NULL;
            return;
        }
    }
    tex_printf("<%s", nameoffile + 1);
    t3_getline();
    if (!t3_prefix(t3_font_scale_str) ||
        sscanf(t3_line_array + strlen(t3_font_scale_str) + 1, "%g", &t3_font_scale) < 1 ||
        t3_font_scale <= 0 || t3_font_scale > 1000 ) {
        pdftex_warn("missing or invalid font scale");
        t3_close();
        cur_file_name = NULL;
        return;
    }
    while (!t3_eof())
        t3_write_glyph(f);
write_font_dict:
    for (i = fontbc[f]; i <= fontec[f]; i++)
        if (pdfcharmarked(f, i))
            break;
    first_char = i;
    for (i = fontec[f]; i > first_char; i--)
        if (pdfcharmarked(f, i))
            break;
    last_char = i;
    pdfbegindict(objnum); /* Type 3 font dictionary */
    pdf_puts("/Type /Font\n/Subtype /Type3\n");
    pdf_printf("/Name /F%i\n", (int)f);
    if (pdffontattr[f] != getnullstr()) {
        pdfprint(pdffontattr[f]);
        pdf_puts("\n");
    }
    if (is_pk_font) {
        pk_font_scale = get_pk_font_scale(f);
        pdf_puts("/FontMatrix [");
        pdfprintreal(pk_font_scale, 5);
        pdf_puts(" 0 0 ");
        pdfprintreal(pk_font_scale, 5);
        pdf_puts(" 0 0]\n") ;
    }
    else
        pdf_printf("/FontMatrix [%g 0 0 %g 0 0]\n", 
                   (double)t3_font_scale, (double)t3_font_scale);
    pdf_printf("/%s [ %i %i %i %i ]\n", 
               font_keys[FONTBBOX1_CODE].pdfname, 
               (int)t3_b0, (int)t3_b1, 
               (int)t3_b2, (int)t3_b3);
    pdf_printf("/Resources << /ProcSet [ /PDF %s] >>\n", 
               t3_image_used ? "/ImageB " : "");
    pdf_printf("/FirstChar %i\n/LastChar %i\n", first_char, last_char);
    pdf_printf("/Widths %i 0 R\n/Encoding %i 0 R\n/CharProcs %i 0 R\n", 
               (int)(objptr + 1), (int)(objptr + 2),
               (int)(objptr + 3));
    pdfenddict();
    pdfnewobj(0, 0); /* chars width array */
    pdf_puts("[");
    if (is_pk_font)
        for (i = first_char; i <= last_char; i++) {
            pdfprintreal(t3_char_widths[i], 2);
            pdf_puts(" ");
        }
    else
        for (i = first_char; i <= last_char; i++)
            pdf_printf("%i ", (int)t3_char_widths[i]);
    pdf_puts("]\n");
    pdfendobj();
    pdfnewdict(0, 0); /* encoding dictionary */
    pdf_printf("/Type /Encoding\n/Differences [%i", first_char);
    if (t3_char_procs[first_char] == 0) {
        pdf_printf("/%s", notdef);
        is_notdef = true;
    }
    else {
        pdf_printf("/a%i", first_char);
        is_notdef = false;
    }
    for (i = first_char + 1; i <= last_char; i++) {
        if (t3_char_procs[i] == 0) {
            if (!is_notdef) {
                pdf_printf(" %i/%s", i, notdef);
                is_notdef = true;
            }
        }
        else {
            if (is_notdef) {
                pdf_printf(" %i", i);
                is_notdef = false;
            }
            pdf_printf("/a%i", i);
        }
    }
    pdf_puts("]\n");
    pdfenddict();
    pdfnewdict(0, 0); /* CharProcs dictionary */
    for (i = first_char; i <= last_char; i++)
        if (t3_char_procs[i] != 0)
            pdf_printf("/a%i %i 0 R\n", (int)i, (int)t3_char_procs[i]);
    pdfenddict();
    t3_close();
    tex_printf(">");
    cur_file_name = NULL;
}
