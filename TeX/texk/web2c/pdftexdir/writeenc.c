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

$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/writeenc.c#14 $
*/

#include "ptexlib.h"

static const char perforce_id[] = 
    "$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/writeenc.c#14 $";

/* define enc_ptr, enc_array & enc_limit */
define_array(enc);   

void read_enc(enc_entry *e)
{
    assert(e != NULL);
    if (e->loaded)
        return;
    load_enc(e->name, e->glyph_names);
    e->loaded = true;
}

/* write_enc is used to write either external encoding (given in map file) or
 * internal encoding (read from the font file); when glyph_names is NULL
 * the 2nd argument is a pointer to the encoding entry; otherwise the 3rd is 
 * the object number of the Encoding object
 */
void write_enc(char **glyph_names, enc_entry *e, integer eobjnum)
{
    boolean is_notdef;
    int i;
    char **g;
    if (glyph_names == NULL) {
        assert(e != NULL);
        if (e->objnum != 0) /* the encoding has been written already */
            return;
        pdfnewdict(0, 0);
        e->objnum = objptr;
        g = e->glyph_names;
    }
    else {
        pdfbegindict(eobjnum);
        g = glyph_names;
    }
    pdf_printf("/Type /Encoding\n/Differences [ 0 /%s", g[0]);
    is_notdef = (g[0] == notdef);
    for (i = 1; i <= MAX_CHAR_CODE; i++) {
        if (g[i] == notdef) {
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
            pdf_printf("/%s", g[i]);
        }
    }
    pdf_puts("]\n");
    pdfenddict();
}

enc_entry *add_enc(char *s) /* built-in encodings have s == NULL */
{
    int i;
    enc_entry *e;
    if (enc_array != NULL && s != NULL) {
        for (e = enc_array; e < enc_ptr; ++e)
            if (e->name != NULL) /* don't check for built-in encodings */
                if  (strcmp(s, e->name) == 0)
                    return e;
    }
    alloc_array(enc, 1, SMALL_ARRAY_SIZE);
    if (s != NULL)
        enc_ptr->name = xstrdup(s);
    else
        enc_ptr->name = NULL;
    enc_ptr->loaded = false;
    enc_ptr->updated = false;
    enc_ptr->firstfont = getnullfont();
    enc_ptr->objnum = 0;
    enc_ptr->glyph_names = xtalloc(MAX_CHAR_CODE + 1, char *);
    for (i = 0; i <= MAX_CHAR_CODE; i++)
        enc_ptr->glyph_names[i] = (char*) notdef;
    return enc_ptr++;
}

/* get encoding for map entry fm. When encoding vector is not given, try to
 * get it from T1 font file, in this case t1_read_enc sets the font being
 * reencoded, so next calls for the same entry doesn't cause reading the font
 * again
 */
boolean get_enc(fm_entry *fm)
{
    int i;
    char **glyph_names;
    if (is_reencoded(fm)) { /* external encoding vector available */
        read_enc(fm->encoding);
        return true;
    }
    if (!is_t1fontfile(fm)) /* get built-in encoding for T1 fonts only */
        return false;
    if (t1_read_enc(fm)) { /* encoding read into t1_builtin_glyph_names */
        fm->encoding = add_enc(NULL);
        glyph_names = (fm->encoding)->glyph_names;
        for (i = 0; i <= MAX_CHAR_CODE; i++)
            glyph_names[i] = t1_builtin_glyph_names[i];
        (fm->encoding)->loaded = true;
        return true;
    }
    return false;
}

/* check whether an encoding contains indexed glyph in form "/index123" */
/* boolean indexed_enc(fm_entry *fm) */
/* { */
/*     char **s = enc_array[fm->encoding].glyph_names; */
/*     int i, n; */
/*     for (i = 0; i <= MAX_CHAR_CODE; i++, s++) */
/*         if (*s != NULL && *s != notdef &&  */
/*             sscanf(*s,  INDEXED_GLYPH_PREFIX "%i", &n) == 1) */
/*                 return true; */
/*     return false; */
/* } */

void setcharmap(internalfontnumber f)
{
    fm_entry *fm;
    enc_entry *e;
    char **glyph_names;
    int i, k;
    if (pdfmovechars == 0 || fontbc[f] > 32 || !hasfmentry(f))
        return;
    if (fontec[f] < 128) {
        for (i = fontbc[f]; i <= 32; i++)
            pdfcharmap[f][i] = i + MOVE_CHARS_OFFSET;
        return;
    }
    fm = (fm_entry *) pdffontmap[f];
    if (pdfmovechars == 1 || !get_enc(fm))
        return;
    e = fm->encoding;
    if (e->firstfont != getnullfont()) {
        for (i = fontbc[f]; i <= 32; i++)
            pdfcharmap[f][i] = pdfcharmap[e->firstfont][i];
        return;
    }
    e->firstfont = f;
    glyph_names = e->glyph_names;
    for (i = 32, k = MAX_CHAR_CODE; i >= fontbc[f] && k > 127; i--) {
        if (glyph_names[i] == notdef)
            continue;
        while (glyph_names[k] != notdef && k > 127)
            k--;
        if (k < 128)
            return;
        glyph_names[k] = glyph_names[i];
        glyph_names[i] = (char*) notdef;
        pdfcharmap[f][i] = k;
    }
}

void enc_free()
{
    enc_entry *e;
    int k;
    for (e = enc_array; e < enc_ptr; e++) {
        xfree(e->name);
        if (e->loaded) { /* encoding has been loaded */
            for (k = 0; k <= MAX_CHAR_CODE; k++)
                if (e->glyph_names[k] != notdef)
                    xfree(e->glyph_names[k]);
        }
        xfree(e->glyph_names);
    }
    xfree(enc_array);
}
