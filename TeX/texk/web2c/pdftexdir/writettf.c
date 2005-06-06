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

$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/writettf.c#15 $
*/

#include "ptexlib.h"

static const char perforce_id[] = 
    "$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/writettf.c#15 $";

typedef signed char     TTF_CHAR;
typedef unsigned char   TTF_BYTE;
typedef signed short    TTF_SHORT;
typedef unsigned short  TTF_USHORT;
typedef signed long     TTF_LONG;
typedef unsigned long   TTF_ULONG;
typedef unsigned long   TTF_FIXED;
typedef unsigned short  TTF_FUNIT;
typedef signed short    TTF_FWORD;
typedef unsigned short  TTF_UFWORD;
typedef unsigned short  TTF_F2DOT14;

#define TTF_CHAR_SIZE    1
#define TTF_BYTE_SIZE    1
#define TTF_SHORT_SIZE   2
#define TTF_USHORT_SIZE  2
#define TTF_LONG_SIZE    4
#define TTF_ULONG_SIZE   4
#define TTF_FIXED_SIZE   4
#define TTF_FWORD_SIZE   2
#define TTF_UFWORD_SIZE  2
#define TTF_F2DOT14_SIZE 2

#define ARG_1_AND_2_ARE_WORDS       (1<<0)
#define ARGS_ARE_XY_VALUES          (1<<1)
#define ROUND_XY_TO_GRID            (1<<2)
#define WE_HAVE_A_SCALE             (1<<3)
#define RESERVED                    (1<<4)
#define MORE_COMPONENTS             (1<<5)
#define WE_HAVE_AN_X_AND_Y_SCALE    (1<<6)
#define WE_HAVE_A_TWO_BY_TWO        (1<<7)
#define WE_HAVE_INSTRUCTIONS        (1<<8)
#define USE_MY_METRICS              (1<<9)

#define get_num(t)      ((t)ttf_getnum(t##_SIZE))
#define ttf_skip(n)     ttf_getnum(n)

#define get_char()      get_num(TTF_CHAR)
#define get_byte()      get_num(TTF_BYTE)
#define get_short()     get_num(TTF_SHORT)
#define get_ushort()    get_num(TTF_USHORT)
#define get_long()      get_num(TTF_LONG)
#define get_ulong()     get_num(TTF_ULONG)
#define get_fixed()     get_num(TTF_FIXED)
#define get_funit()     get_num(TTF_FUNIT)
#define get_fword()     get_num(TTF_FWORD)
#define get_ufword()    get_num(TTF_UFWORD)
#define get_f2dot14()   get_num(TTF_F2DOT14)

#define put_num(t,n)    ((t)ttf_putnum(t##_SIZE, n))

#define put_char(n)     put_num(TTF_CHAR, n)
#define put_byte(n)     put_num(TTF_BYTE, n)
#define put_short(n)    put_num(TTF_SHORT, n)
#define put_ushort(n)   put_num(TTF_USHORT, n)
#define put_long(n)     put_num(TTF_LONG, n)
#define put_ulong(n)    put_num(TTF_ULONG, n)
#define put_fixed(n)    put_num(TTF_FIXED, n)
#define put_funit(n)    put_num(TTF_FUNIT, n)
#define put_fword(n)    put_num(TTF_FWORD, n)
#define put_ufword(n)   put_num(TTF_UFWORD, n)
#define put_f2dot14(n)  put_num(TTF_F2DOT14, n)

#define copy_byte()     put_byte(get_byte())
#define copy_char()     put_char(get_char())
#define copy_ushort()   put_ushort(get_ushort())
#define copy_short()    put_short(get_short())
#define copy_ulong()    put_ulong(get_ulong())
#define copy_long()     put_long(get_long())
#define copy_fixed()    put_fixed(get_fixed())
#define copy_funit()    put_funit(get_funit())
#define copy_fword()    put_fword(get_fword())
#define copy_ufword()   put_ufword(get_ufword())
#define copy_f2dot14()  put_f2dot14(get_f2dot14())

#define NMACGLYPHS      258
#define TABDIR_OFF      12

typedef struct {
    char tag[4];
    TTF_ULONG checksum;
    TTF_ULONG offset;
    TTF_ULONG length;
} tabdir_entry;

typedef struct {
    TTF_LONG offset;
    TTF_LONG newoffset;
    TTF_UFWORD advWidth;
    TTF_FWORD lsb;
    char *name;              /* name of glyph */
    TTF_SHORT newindex;      /* new index of glyph in output file */
    TTF_USHORT name_index;   /* index of name as read from font file */
} glyph_entry;

typedef struct {
    char *name;             /* name of glyph */
    short newindex;         /* new index of glyph in output file */
} ttfenc_entry;

typedef struct {
    TTF_USHORT platform_id;
    TTF_USHORT encoding_id;
    TTF_USHORT language_id;
    TTF_USHORT name_id;
    TTF_USHORT length;
    TTF_USHORT offset;
} name_record;

typedef struct {
    TTF_USHORT platform_id;
    TTF_USHORT encoding_id;
    TTF_ULONG  offset;
    TTF_USHORT format;
} cmap_entry;

static TTF_USHORT ntabs;
static TTF_USHORT upem;
static TTF_FIXED post_format;
static TTF_SHORT loca_format;
static TTF_ULONG last_glyf_offset;
static TTF_USHORT glyphs_count;
static TTF_USHORT new_glyphs_count;
static TTF_USHORT nhmtxs;
static TTF_USHORT new_ntabs;

/* 
 * reindexing glyphs is a bit unclear: `glyph_tab' contains glyphs in
 * original order in font file, `ttfenc_tab' is the new encoding vector and
 * `glyph_index' is the new order of glyphs. So n-th glyph in new order is
 * located at `glyph_tab[glyph_index[n]]'. The field `newindex' of entries in
 * both `glyph_tab' and `ttfenc_tab' contains the index of the corresponding
 * glyph...
 *
 */

static glyph_entry *glyph_tab;
static short *glyph_index;
static int ncmaptabs;
static cmap_entry *cmap_tab = NULL;
static name_record *name_tab;
static int name_record_num;
static char *name_buf;
static int name_buf_size;
static tabdir_entry *dir_tab;
static char *glyph_name_buf;
static TTF_ULONG checksum;
static TTF_ULONG tab_length;
static TTF_ULONG tmp_ulong;
static TTF_ULONG checkSumAdjustment_offset;
static FILE *ttf_file;
static ttfenc_entry ttfenc_tab[MAX_CHAR_CODE + 1];

integer ttf_length;
#define ttf_putchar     fb_putchar
#define ttf_offset      fb_offset
#define ttf_seek_outbuf fb_seek

#define INFILE ttf_file

#define ttf_open()      \
    open_input(&ttf_file, kpse_truetype_format, FOPEN_RBIN_MODE)
#define ttf_close()     xfclose(ttf_file, cur_file_name)
#define ttf_getchar()   xgetc(ttf_file)
#define ttf_eof()       feof(ttf_file)

#include "macnames.c"

static const char *newtabnames[] = {
    "OS/2",
    "PCLT",
    "cmap",
    "cvt ",
    "fpgm",
    "glyf",
    "head",
    "hhea",
    "hmtx",
    "loca",
    "maxp",
    "name",
    "post",
    "prep"
};

#define DEFAULT_NTABS       14

static unsigned char ttf_addchksm(unsigned char b)
{
    tmp_ulong = (tmp_ulong << 8) + b;
    tab_length++;
    if (tab_length % 4 == 0) {
        checksum += tmp_ulong;
        tmp_ulong = 0;
    }
    return b;
}

static TTF_ULONG ttf_getchksm(void)
{
    while (tab_length % 4 != 0)
        ttf_putchar(ttf_addchksm(0)); /* ttf_addchksm updates tab_length */
    return checksum;
}

static long ttf_putnum(int s, long n)
{
    long i = n;
    char buf[TTF_LONG_SIZE + 1], *p = buf;
    while (s-- > 0) {
        *p++ = i & 0xFF;
        i >>= 8;
    }
    p--;
    while (p >= buf)
        ttf_putchar(ttf_addchksm(*p--));
    return n;
}

static long ttf_getnum(int s)
{
    long i = 0;
    int c;
    while (s > 0) {
        if ((c = ttf_getchar()) < 0)
            pdftex_fail("unexpected EOF");
        i = (i << 8) + c;
        s--;
    }
    return i;
}

static long ttf_funit(long n)
{
    if (n < 0)
        return -((-n/upem)*1000 + ((-n%upem)*1000)/upem);
    else
        return (n/upem)*1000 + ((n%upem)*1000)/upem;
}

static void ttf_ncopy(int n)
{
    while (n-- > 0)
        copy_byte();
}

static tabdir_entry *ttf_name_lookup(const char *s, boolean required)
{
    tabdir_entry *tab;
    for (tab = dir_tab; tab - dir_tab < ntabs; tab++)
        if (strncmp(tab->tag, s, 4) == 0)
            break;
    if (tab - dir_tab == ntabs) {
        if (required)
            pdftex_fail("can't find table `%s'", s);
        else
            tab = NULL;
    }
    return tab;
}

static tabdir_entry *ttf_seek_tab(const char *name, TTF_LONG offset)
{
    tabdir_entry *tab = ttf_name_lookup(name, true);
    xfseek(INFILE, tab->offset + offset, SEEK_SET, cur_file_name);
    return tab;
}

static void ttf_seek_off(TTF_LONG offset)
{
    xfseek(INFILE, offset, SEEK_SET, cur_file_name);
}

static void ttf_copy_encoding(void)
{
    int i;
    char **glyph_names = (fm_cur->encoding)->glyph_names;
    ttfenc_entry *e = ttfenc_tab;
    pdfmarkchar(tex_font, 'a'); /* workaround for a bug of AcroReader 4.0 */
    for (i = 0; i <= MAX_CHAR_CODE; i++, e++) {
        if (pdfcharmarked(tex_font, i))
            e->name = glyph_names[i];
        else
            e->name = (char*) notdef;
    }
}

#define ttf_append_byte(B)\
do {\
    if (name_tab[i].platform_id == 3)\
        *q++ = 0;\
    *q++ = B;\
} while (0)

static void ttf_read_name(void)
{
    int i;
    tabdir_entry *tab = ttf_seek_tab("name", TTF_USHORT_SIZE);
    char *p;
    name_record_num = get_ushort();
    name_tab = xtalloc(name_record_num, name_record);
    name_buf_size = tab->length - 
         (3*TTF_USHORT_SIZE + name_record_num*6*TTF_USHORT_SIZE);
    name_buf = xtalloc(name_buf_size, char);
    ttf_skip(TTF_USHORT_SIZE);
    for (i = 0; i < name_record_num; i++) {
        name_tab[i].platform_id = get_ushort();
        name_tab[i].encoding_id = get_ushort();
        name_tab[i].language_id = get_ushort();
        name_tab[i].name_id = get_ushort();
        name_tab[i].length = get_ushort();
        name_tab[i].offset = get_ushort();
    }
    for (p = name_buf; p - name_buf < name_buf_size; p++)
         *p = get_char();
}

static void ttf_read_mapx(void)
{
    int i;
    glyph_entry *glyph;
    ttf_seek_tab("maxp", TTF_FIXED_SIZE);
    glyph_tab = xtalloc(1 + (glyphs_count = get_ushort()), glyph_entry);
    for (glyph = glyph_tab; glyph - glyph_tab < glyphs_count; glyph++) {
        glyph->newindex = -1;
        glyph->newoffset = 0;
        glyph->name_index = 0;
        glyph->name = (char*) notdef;
    }
    glyph_index = xtalloc(glyphs_count, short);
    glyph_index[0] = 0; /* index of ".notdef" glyph */
    glyph_index[1] = 1; /* index of ".null" glyph */
}

static void ttf_read_head(void)
{
    ttf_seek_tab("head", 2*TTF_FIXED_SIZE + 2*TTF_ULONG_SIZE + TTF_USHORT_SIZE);
    upem = get_ushort();
    ttf_skip(16);
    font_keys[FONTBBOX1_CODE].value.num = ttf_funit(get_fword());
    font_keys[FONTBBOX2_CODE].value.num = ttf_funit(get_fword());
    font_keys[FONTBBOX3_CODE].value.num = ttf_funit(get_fword());
    font_keys[FONTBBOX4_CODE].value.num = ttf_funit(get_fword());
    font_keys[FONTBBOX1_CODE].valid = true;
    font_keys[FONTBBOX2_CODE].valid = true;
    font_keys[FONTBBOX3_CODE].valid = true;
    font_keys[FONTBBOX4_CODE].valid = true;
    ttf_skip(2*TTF_USHORT_SIZE + TTF_SHORT_SIZE);
    loca_format = get_short();
}

static void ttf_read_hhea(void)
{
    ttf_seek_tab("hhea", TTF_FIXED_SIZE);
    font_keys[ASCENT_CODE].value.num = ttf_funit(get_fword());
    font_keys[DESCENT_CODE].value.num = ttf_funit(get_fword());
    font_keys[ASCENT_CODE].valid = true;
    font_keys[DESCENT_CODE].valid = true;
    ttf_skip(TTF_FWORD_SIZE + TTF_UFWORD_SIZE + 3*TTF_FWORD_SIZE + 8*TTF_SHORT_SIZE);
    nhmtxs = get_ushort();
}

static void ttf_read_pclt(void)
{
    if (ttf_name_lookup("PCLT", false) == NULL)
        return;
    ttf_seek_tab("PCLT", TTF_FIXED_SIZE + TTF_ULONG_SIZE + TTF_USHORT_SIZE);
    font_keys[XHEIGHT_CODE].value.num = ttf_funit(get_ushort());
    ttf_skip(2*TTF_USHORT_SIZE);
    font_keys[CAPHEIGHT_CODE].value.num = ttf_funit(get_ushort());
    font_keys[XHEIGHT_CODE].valid = true;
    font_keys[CAPHEIGHT_CODE].valid = true;
}

static void ttf_read_hmtx(void)
{
    glyph_entry *glyph;
    TTF_UFWORD last_advWidth;
    ttf_seek_tab("hmtx", 0);
    for (glyph = glyph_tab; glyph - glyph_tab < nhmtxs; glyph++) {
        glyph->advWidth = get_ufword();
        glyph->lsb = get_ufword();
    }
    if (nhmtxs < glyphs_count) {
        last_advWidth = glyph[-1].advWidth;
        for (;glyph - glyph_tab <  glyphs_count; glyph++) {
            glyph->advWidth = last_advWidth;
            glyph->lsb = get_ufword();
        }
    }
}

static void ttf_read_post(void)
{
    int k, nnames;
    long length;
    long int_part, frac_part;
    int sign = 1;
    TTF_FIXED italic_angle;
    char *p;
    glyph_entry *glyph;
    tabdir_entry *tab = ttf_seek_tab("post", 0);
    post_format = get_fixed();
    italic_angle = get_fixed();
    int_part = italic_angle >> 16;
    if (int_part > 0x7FFF) { /* a negative number */
        int_part = 0x10000 - int_part;
        sign = -1;
    }
    frac_part = italic_angle % 0x10000;
    font_keys[ITALIC_ANGLE_CODE].value.num = sign*(int_part + frac_part*1.0/0x10000);
    font_keys[ITALIC_ANGLE_CODE].valid = true;
    if (glyph_tab == NULL)
        return; /* being called from writeotf() */
    ttf_skip(2*TTF_FWORD_SIZE + 5*TTF_ULONG_SIZE);
    switch (post_format) {
    case 0x10000:
        for (glyph = glyph_tab; glyph - glyph_tab < NMACGLYPHS; glyph++) {
            glyph->name = (char*)mac_glyph_names[glyph - glyph_tab];
            glyph->name_index = glyph - glyph_tab;
        }
        break;
    case 0x20000:
        nnames = get_ushort(); /* some fonts have this value different from nglyphs */
        for (glyph = glyph_tab; glyph - glyph_tab < nnames; glyph++)
            glyph->name_index = get_ushort();
        length = tab->length - (xftell(INFILE, cur_file_name) - tab->offset);
        glyph_name_buf = xtalloc(length, char);
        for (p = glyph_name_buf; p - glyph_name_buf < length;) {
            for (k = get_byte(); k > 0; k--)
                *p++ = get_char();
            *p++ = 0;
        }
        for (glyph = glyph_tab; glyph - glyph_tab < nnames; glyph++) {
            if (glyph->name_index < NMACGLYPHS)
                glyph->name = (char*)mac_glyph_names[glyph->name_index];
            else {
                p = glyph_name_buf;
                k = glyph->name_index - NMACGLYPHS;
                for (; k > 0; k--)
                    p = strend(p) + 1;
                glyph->name = p;
            }
        }
        break;
    case 0x00030000:
        for (glyph = glyph_tab; glyph - glyph_tab < NMACGLYPHS; glyph++) {
            glyph->name_index = glyph - glyph_tab;
        }
        break;
    default:
        pdftex_fail("unsupported format (%.8X) of `post' table", (unsigned int) post_format);
    }
}

static void ttf_read_loca(void)
{
    glyph_entry *glyph;
    ttf_seek_tab("loca", 0);
    if (loca_format != 0)
        for (glyph = glyph_tab; glyph - glyph_tab < glyphs_count + 1; glyph++)
            glyph->offset = get_ulong();
    else
        for (glyph = glyph_tab; glyph - glyph_tab < glyphs_count + 1; glyph++)
            glyph->offset = get_ushort() << 1;
}

static void ttf_read_tabdir()
{
    int i;
    tabdir_entry *tab;
/*     if ((version= get_fixed()) != 0x00010000) */
/*         pdftex_fail("unsupport version 0x%.8X; can handle only version 1.0", (int)version); */
    ttf_skip(TTF_FIXED_SIZE); /* ignore the sfnt number */
    dir_tab = xtalloc(ntabs = get_ushort(), tabdir_entry);
    ttf_skip(3*TTF_USHORT_SIZE);
    for (tab = dir_tab; tab - dir_tab < ntabs; tab++) {
        for (i = 0; i < 4; i++)
            tab->tag[i] = get_char();
        tab->checksum = get_ulong();
        tab->offset = get_ulong();
        tab->length = get_ulong();
    }
}

static void ttf_read_font(void)
{
    ttf_read_tabdir();
    if (ttf_name_lookup("PCLT", false) == NULL)
        new_ntabs--;
    if (ttf_name_lookup("fpgm", false) == NULL)
        new_ntabs--;
    if (ttf_name_lookup("cvt ", false) == NULL)
        new_ntabs--;
    if (ttf_name_lookup("prep", false) == NULL)
        new_ntabs--;
    ttf_read_mapx();
    ttf_read_head();
    ttf_read_hhea();
    ttf_read_pclt();
    ttf_read_hmtx();
    ttf_read_post();
    ttf_read_loca();
    ttf_read_name();
}

static void ttf_reset_chksm(tabdir_entry *tab)
{
    checksum = 0;
    tab_length = 0;
    tmp_ulong = 0;
    tab->offset = ttf_offset();
    if (tab->offset % 4 != 0)
        pdftex_warn("offset of `%4.4s' is not a multiple of 4", tab->tag);
}

extern char *fb_array;

static void ttf_set_chksm(tabdir_entry *tab)
{
    tab->length = ttf_offset() - tab->offset;
    tab->checksum = ttf_getchksm();
}

static void ttf_copytab(const char *name)
{
    long i;
    tabdir_entry *tab = ttf_seek_tab(name, 0);
    ttf_reset_chksm(tab);
    for (i = tab->length; i > 0; i--)
        copy_char();
    ttf_set_chksm(tab);
}

#define BYTE_ENCODING_LENGTH  \
    ((MAX_CHAR_CODE + 1)*TTF_BYTE_SIZE + 3*TTF_USHORT_SIZE)

static void ttf_byte_encoding(void)
{
    ttfenc_entry *e;
    put_ushort(0);  /* format number (0: byte encoding table) */
    put_ushort(BYTE_ENCODING_LENGTH); /* length of table */
    put_ushort(0);  /* version number */
    for (e = ttfenc_tab; e - ttfenc_tab <= MAX_CHAR_CODE; e++)
        if (e->newindex < 256) {
            put_byte(e->newindex);
        }
        else {
            if (e->name != notdef)
                pdftex_warn("glyph `%s' has been mapped to `%s' in `ttf_byte_encoding' cmap table",
                     e->name, notdef);
            put_byte(0); /* notdef */
        }
}

#define TRIMMED_TABLE_MAP_LENGTH (TTF_USHORT_SIZE*(5 + (MAX_CHAR_CODE + 1)))

static void ttf_trimmed_table_map(void)
{
    ttfenc_entry *e;
    put_ushort(6);  /* format number (6): trimmed table mapping */
    put_ushort(TRIMMED_TABLE_MAP_LENGTH);
    put_ushort(0);  /* version number (0) */
    put_ushort(0);  /* first character code */
    put_ushort(MAX_CHAR_CODE + 1);  /* number of character code in table */
    for (e = ttfenc_tab; e - ttfenc_tab <= MAX_CHAR_CODE; e++)
        put_ushort(e->newindex);
}

#define SEG_MAP_DELTA_LENGTH ((16 + (MAX_CHAR_CODE + 1))*TTF_USHORT_SIZE)

static void ttf_seg_map_delta(void)
{
    ttfenc_entry *e;
    put_ushort(4);  /* format number (4: segment mapping to delta values) */
    put_ushort(SEG_MAP_DELTA_LENGTH);
    put_ushort(0);  /* version number */
    put_ushort(4);  /* 2*segCount */
    put_ushort(4);  /* searchRange */
    put_ushort(1);  /* entrySelector */
    put_ushort(0);  /* rangeShift */
    put_ushort(0xF0FF); /* endCount[0] */
    put_ushort(0xFFFF); /* endCount[1] */
    put_ushort(0); /* reversedPad */
    put_ushort(0xF000); /* startCount[0] */
    put_ushort(0xFFFF); /* startCount[1] */
    put_ushort(0); /* idDelta[0] */
    put_ushort(1); /* idDelta[1] */
    put_ushort(2*TTF_USHORT_SIZE); /* idRangeOffset[0] */
    put_ushort(0); /* idRangeOffset[1] */
    for (e = ttfenc_tab; e - ttfenc_tab <= MAX_CHAR_CODE; e++)
        put_ushort(e->newindex);
}

#define CMAP_ENTRY_LENGTH (2*TTF_USHORT_SIZE + TTF_ULONG_SIZE)

static void ttf_select_cmap(void)
{
    if (cmap_tab == NULL)
        cmap_tab = xtalloc(2, cmap_entry);
    cmap_tab[0].platform_id  = 1; /* Macintosh */
    cmap_tab[0].encoding_id  = 0; /* Symbol; ignore code page */
    cmap_tab[0].format = new_glyphs_count < 256 ? 0  /* byte encoding */
                                                : 6; /* trimmed table mapping */
    cmap_tab[1].platform_id  = 3; /* Microsoft */
    cmap_tab[1].encoding_id  = 0; /* Symbol; ignore code page */
    cmap_tab[1].format       = 4; /* segment mapping to delta */
    ncmaptabs = 2;
}

static void ttf_write_cmap(void)
{
    cmap_entry *ce;
    long offset;
    tabdir_entry *tab = ttf_name_lookup("cmap", true);
    ttf_select_cmap();
    ttf_reset_chksm(tab);
    put_ushort(0);  /* table version number (0) */
    put_ushort(ncmaptabs);  /* number of encoding tables */
    offset = 2*TTF_USHORT_SIZE + ncmaptabs*CMAP_ENTRY_LENGTH;
    for (ce = cmap_tab; ce - cmap_tab < ncmaptabs; ce++) {
        ce->offset = offset;
        switch (ce->format) {
        case 0: 
            offset +=  BYTE_ENCODING_LENGTH;
            break;
        case 4: 
            offset +=  SEG_MAP_DELTA_LENGTH;
            break;
        case 6: 
            offset +=  TRIMMED_TABLE_MAP_LENGTH;
            break;
        default:
            pdftex_fail("invalid format (it should not have happened)");
        }
        put_ushort(ce->platform_id);
        put_ushort(ce->encoding_id);
        put_ulong(ce->offset);
    }
    for (ce = cmap_tab; ce - cmap_tab < ncmaptabs; ce++) {
        switch (ce->format) {
        case 0: 
            ttf_byte_encoding();
            break;
        case 4: 
            ttf_seg_map_delta();
            break;
        case 6: 
            ttf_trimmed_table_map();
            break;
        }
    }
    ttf_set_chksm(tab);
}

static void ttf_write_name(void)
{
    int i;
    char *p;
    tabdir_entry *tab = ttf_name_lookup("name", true);
    ttf_reset_chksm(tab);
    put_ushort(0); /* Format selector */
    put_ushort(name_record_num);
    put_ushort(3*TTF_USHORT_SIZE + name_record_num*6*TTF_USHORT_SIZE);
    for (i = 0; i < name_record_num; i++) {
        put_ushort(name_tab[i].platform_id);
        put_ushort(name_tab[i].encoding_id);
        put_ushort(name_tab[i].language_id);
        put_ushort(name_tab[i].name_id);
        put_ushort(name_tab[i].length);
        put_ushort(name_tab[i].offset);
    }
    for (p = name_buf; p - name_buf < name_buf_size; p++)
         put_char(*p);
    ttf_set_chksm(tab);
}

static void ttf_write_dirtab(void)
{
    tabdir_entry *tab;
    TTF_ULONG i, k;
    char *p;
    integer save_offset = ttf_offset();
    ttf_seek_outbuf(TABDIR_OFF);
    if (is_subsetted(fm_cur)) {
        for (i = 0; i < DEFAULT_NTABS; i++) {
            tab = ttf_name_lookup(newtabnames[i], false);
            if (tab == NULL)
                continue;
            for (k = 0; k < 4; k++)
               put_char(tab->tag[k]);
            put_ulong(tab->checksum);
            put_ulong(tab->offset);
            put_ulong(tab->length);
        }
    }
    else {
        for (tab = dir_tab; tab - dir_tab < ntabs; tab++) {
            for (k = 0; k < 4; k++)
               put_char(tab->tag[k]);
            put_ulong(tab->checksum);
            put_ulong(tab->offset);
            put_ulong(tab->length);
        }
    }
    /* adjust checkSumAdjustment */
    tmp_ulong = 0;
    checksum = 0;
    for (p = fb_array, i = 0; i < save_offset;) {
        tmp_ulong = (tmp_ulong << 8) + *p++;
        i++;
        if (i % 4 == 0) {
            checksum += tmp_ulong;
            tmp_ulong = 0;
        }
    }
    if (i % 4 != 0) {
        pdftex_warn("font length is not a multiple of 4 (%li)", i);
        checksum <<= 8*(4 - i % 4);
    }
    k = 0xB1B0AFBA - checksum;
    ttf_seek_outbuf(checkSumAdjustment_offset);
    put_ulong(k);
    ttf_seek_outbuf(save_offset);
}

static void ttf_write_glyf(void)
{
    short *id, k;
    TTF_USHORT idx;
    TTF_USHORT flags;
    tabdir_entry *tab = ttf_name_lookup("glyf", true);
    long glyf_offset = tab->offset;
    long new_glyf_offset = ttf_offset();
    ttf_reset_chksm(tab);
    for (id = glyph_index; id - glyph_index < new_glyphs_count; id++) {
        glyph_tab[*id].newoffset = ttf_offset() - new_glyf_offset;
        if (glyph_tab[*id].offset != glyph_tab[*id + 1].offset) {
            ttf_seek_off(glyf_offset + glyph_tab[*id].offset);
            k = copy_short();
            ttf_ncopy(4*TTF_FWORD_SIZE);
            if (k < 0) {
                do {
                    flags = copy_ushort();
                    idx = get_ushort();
                    if (glyph_tab[idx].newindex < 0) {
                        glyph_tab[idx].newindex = new_glyphs_count;
                        glyph_index[new_glyphs_count++] = idx;
                        /* 
                            N.B.: Here we change `new_glyphs_count',
                            which appears in the condition of the `for' loop
                        */
                    }
                    put_ushort(glyph_tab[idx].newindex);
                    if (flags & ARG_1_AND_2_ARE_WORDS)
                        ttf_ncopy(2*TTF_SHORT_SIZE);
                    else
                        ttf_ncopy(TTF_USHORT_SIZE);
                    if (flags & WE_HAVE_A_SCALE)
                        ttf_ncopy(TTF_F2DOT14_SIZE);
                    else if (flags & WE_HAVE_AN_X_AND_Y_SCALE)
                        ttf_ncopy(2*TTF_F2DOT14_SIZE);
                    else if (flags & WE_HAVE_A_TWO_BY_TWO)
                        ttf_ncopy(4*TTF_F2DOT14_SIZE);
                } while (flags & MORE_COMPONENTS);
                if (flags & WE_HAVE_INSTRUCTIONS)
                    ttf_ncopy(copy_ushort());
            }
            else
                ttf_ncopy(glyph_tab[*id + 1].offset - glyph_tab[*id].offset - 
                    TTF_USHORT_SIZE - 4*TTF_FWORD_SIZE);
        }
    }
    last_glyf_offset = ttf_offset() - new_glyf_offset;
    ttf_set_chksm(tab);
}

static void ttf_reindex_glyphs(void)
{
    ttfenc_entry *e;
    glyph_entry *glyph;
    int i;
    /* 
     * reindexing glyphs: we append index of used glyphs to `glyph_index'
     * while going through `ttfenc_tab'. After appending a new entry to
     * `glyph_index' we set field `newindex' of corresponding entries in both
     * `glyph_tab' and `ttfenc_tab' to the newly created index
     * 
     */
    for (e = ttfenc_tab; e - ttfenc_tab <= MAX_CHAR_CODE; e++) {
        e->newindex = 0; /* index of ".notdef" glyph */
        if (e->name != notdef) {
            if (sscanf(e->name,  INDEXED_GLYPH_PREFIX "%i", &i) == 1) {
                if (i < 0 || i >= glyphs_count)
                    pdftex_fail("`%s' out of valid range (0..%i)",
                                e->name, glyphs_count);
                glyph = glyph_tab + i;
            }
            else {
                for (glyph = glyph_tab; glyph - glyph_tab < glyphs_count; glyph++)
                    if (glyph->name != notdef && strcmp(glyph->name, e->name) == 0)
                            break;
            }
            if (glyph - glyph_tab < glyphs_count) {
                if (glyph->newindex < 0) {
                    glyph_index[new_glyphs_count] = glyph - glyph_tab;
                    glyph->newindex = new_glyphs_count;
                    new_glyphs_count++;
                }
                e->newindex = glyph->newindex;
            }
            else
                pdftex_warn("glyph `%s' not found", e->name);
        }
    }
}
 
static void ttf_write_head()
{
    tabdir_entry *tab;
    tab = ttf_seek_tab("head", 0);
    ttf_reset_chksm(tab);
    ttf_ncopy(2*TTF_FIXED_SIZE);
    checkSumAdjustment_offset = ttf_offset();
    put_ulong(0);
    ttf_skip(TTF_ULONG_SIZE); /* skip checkSumAdjustment */
    ttf_ncopy(TTF_ULONG_SIZE + 2*TTF_USHORT_SIZE + 16 + 
        4*TTF_FWORD_SIZE + 2*TTF_USHORT_SIZE + TTF_SHORT_SIZE);
    if (is_subsetted(fm_cur))  {
        put_short(loca_format);
        put_short(0);
    }
    else
        ttf_ncopy(2*TTF_SHORT_SIZE);
    ttf_set_chksm(tab);
}
 
static void ttf_write_hhea(void)
{
    tabdir_entry *tab;
    tab = ttf_seek_tab("hhea", 0);
    ttf_reset_chksm(tab);
    ttf_ncopy(TTF_FIXED_SIZE + 3*TTF_FWORD_SIZE + TTF_UFWORD_SIZE + 3*TTF_FWORD_SIZE + 8*TTF_SHORT_SIZE);
    put_ushort(new_glyphs_count);
    ttf_set_chksm(tab);
}

static void ttf_write_htmx(void)
{
    short *id;
    tabdir_entry *tab = ttf_seek_tab("hmtx", 0);
    ttf_reset_chksm(tab);
    for (id = glyph_index; id - glyph_index < new_glyphs_count; id++) {
        put_ufword(glyph_tab[*id].advWidth);
        put_ufword(glyph_tab[*id].lsb);
    }
    ttf_set_chksm(tab);
}

static void ttf_write_loca(void)
{
    short *id;
    tabdir_entry *tab = ttf_seek_tab("loca", 0);
    ttf_reset_chksm(tab);
    loca_format = 0;
    if (last_glyf_offset >= 0x00020000 || (last_glyf_offset & 1))
        loca_format = 1;
    else
        for (id = glyph_index; id - glyph_index < new_glyphs_count; id++)
            if (glyph_tab[*id].newoffset & 1) {
                loca_format = 1;
                break;
            }
    if (loca_format != 0) {
        for (id = glyph_index; id - glyph_index < new_glyphs_count; id++)
            put_ulong(glyph_tab[*id].newoffset);
        put_ulong(last_glyf_offset);
    }
    else {
        for (id = glyph_index; id - glyph_index < new_glyphs_count; id++)
            put_ushort(glyph_tab[*id].newoffset/2);
        put_ushort(last_glyf_offset/2);
    }
    ttf_set_chksm(tab);
}

static void ttf_write_mapx(void)
{
    tabdir_entry *tab = ttf_seek_tab("maxp", TTF_FIXED_SIZE + TTF_USHORT_SIZE);
    ttf_reset_chksm(tab);
    put_fixed(0x00010000);
    put_ushort(new_glyphs_count);
    ttf_ncopy(13*TTF_USHORT_SIZE);
    ttf_set_chksm(tab);
}

static void ttf_write_OS2(void)
{
    tabdir_entry *tab = ttf_seek_tab("OS/2", 0);
    TTF_USHORT version;
    ttf_reset_chksm(tab);
    version = get_ushort();
    if (version != 0x0000 && version != 0x0001 && version != 0x0002)
        pdftex_fail("unknown verssion of OS/2 table (%.4X)", version);
    put_ushort(0x0001); /* fix version to 1*/
    ttf_ncopy(2*TTF_USHORT_SIZE + 13*TTF_SHORT_SIZE + 10*TTF_BYTE_SIZE);
    ttf_skip(4*TTF_ULONG_SIZE); /* ulUnicodeRange 1--4 */
    put_ulong(0x00000003); /* Basic Latin + Latin-1 Supplement (0x0000--0x00FF) */
    put_ulong(0x10000000); /* Private Use (0xE000--0xF8FF) */
    put_ulong(0x00000000);
    put_ulong(0x00000000);
    ttf_ncopy(4*TTF_CHAR_SIZE + TTF_USHORT_SIZE); /* achVendID + fsSelection */
    ttf_skip(2*TTF_USHORT_SIZE);
    put_ushort(0x0000); /* usFirstCharIndex */
    put_ushort(0xF0FF); /* usLastCharIndex */
    ttf_ncopy(5*TTF_USHORT_SIZE);
    /* for version 0 the OS/2 table ends here, the rest is for version 1 */ 
    put_ulong(0x80000000); /* Symbol Character Set---don't use any code page */
    put_ulong(0x00000000);
    ttf_set_chksm(tab);
}

static boolean unsafe_name(char *s)
{
    const char **p;
    for (p = ambiguous_names; *p != NULL; p++)
        if (strcmp(s, *p) == 0)
            return true;
    return false;
}

static void ttf_write_post(void)
{
    tabdir_entry *tab = ttf_seek_tab("post", TTF_FIXED_SIZE);
    glyph_entry *glyph;
    char *s;
    short *id;
    int l;
    ttf_reset_chksm(tab);
    if (!write_ttf_glyph_names || post_format == 0x00030000) {
        put_fixed(0x00030000);
        ttf_ncopy(TTF_FIXED_SIZE + 2*TTF_FWORD_SIZE + 5*TTF_ULONG_SIZE);
    }
    else {
        put_fixed(0x00020000);
        ttf_ncopy(TTF_FIXED_SIZE + 2*TTF_FWORD_SIZE + 5*TTF_ULONG_SIZE);
        put_ushort(new_glyphs_count);
        k = 0;
        for (id = glyph_index; id - glyph_index < new_glyphs_count; id++) {
            glyph = glyph_tab + *id;
            if (glyph->name_index >= NMACGLYPHS || unsafe_name(glyph->name))
                glyph->name_index = NMACGLYPHS + k++;
            put_ushort(glyph->name_index);
        }
        for (id = glyph_index; id - glyph_index < new_glyphs_count; id++) {
            glyph = glyph_tab + *id;
            if (glyph->name_index >= NMACGLYPHS) {
                s = glyph->name;
                l = strlen(s);
                put_byte(l);
                while (l-- > 0)
                    put_char(*s++);
            }
        }
    }
    ttf_set_chksm(tab);
}

static void ttf_init_font(int n)
{
    int i, k;
    for (i = 1, k = 0; i <= n; i <<= 1, k++);
    put_fixed(0x00010000); /* font version */
    put_ushort(n); /* number of tables */
    put_ushort(i << 3); /* search range */
    put_ushort(k - 1); /* entry selector */
    put_ushort((n<<4) - (i<<3)); /* range shift */
    ttf_seek_outbuf(TABDIR_OFF + n*4*TTF_ULONG_SIZE);
}

static void ttf_subset_font(void)
{
    ttf_init_font(new_ntabs);
    if (ttf_name_lookup("PCLT", false) != NULL)
        ttf_copytab("PCLT");
    if (ttf_name_lookup("fpgm", false) != NULL)
        ttf_copytab("fpgm");
    if (ttf_name_lookup("cvt ", false) != NULL)
        ttf_copytab("cvt ");
    if (ttf_name_lookup("prep", false) != NULL)
        ttf_copytab("prep");
    ttf_reindex_glyphs();
    ttf_write_glyf();
    ttf_write_loca();
    ttf_write_OS2();
    ttf_write_head();
    ttf_write_hhea();
    ttf_write_htmx();
    ttf_write_mapx();
    ttf_write_name();
    ttf_write_post();
    ttf_write_cmap();
    ttf_write_dirtab();
}

static void ttf_copy_font(void)
{
    tabdir_entry *tab;
    ttf_init_font(ntabs);
    for (tab = dir_tab; tab - dir_tab < ntabs; tab++) {
        if (strncmp(tab->tag, "head", 4) == 0)
            ttf_write_head();
        else
            ttf_copytab(tab->tag);
    }
    ttf_write_dirtab();
}

void writettf()
{
    set_cur_file_name(fm_cur->ff_name);
    if (is_subsetted(fm_cur) && !(is_reencoded(fm_cur))) {
        pdftex_warn("encoding vector required for TrueType font subsetting");
        cur_file_name = NULL;
        return;
    }
    if (!ttf_open()) {
        pdftex_fail("cannot open TrueType font file for reading");
    }
    cur_file_name = (char*)nameoffile + 1;
    if (!is_included(fm_cur))
        tex_printf("{%s", cur_file_name);
    else if (is_subsetted(fm_cur))
        tex_printf("<%s", cur_file_name);
    else
        tex_printf("<<%s", cur_file_name);
    fontfile_found = true;
    new_glyphs_count = 2;
    new_ntabs = DEFAULT_NTABS;
    dir_tab = NULL;
    glyph_tab = NULL;
    glyph_index = NULL;
    glyph_name_buf = NULL;
    name_tab = NULL;
    name_buf = NULL;
    ttf_read_font();
    if (is_included(fm_cur)) {
        pdfsaveoffset = pdfoffset();
        pdfflush();
        if (is_subsetted(fm_cur)) {
            ttf_copy_encoding();
            ttf_subset_font();
        }
        else
            ttf_copy_font();
        ttf_length = ttf_offset();
    }
    xfree(dir_tab);
    xfree(glyph_tab);
    xfree(glyph_index);
    xfree(glyph_name_buf);
    xfree(name_tab);
    xfree(name_buf);
    ttf_close();
    if (!is_included(fm_cur))
        tex_printf("}");
    else if (is_subsetted(fm_cur))
        tex_printf(">");
    else
        tex_printf(">>");
    cur_file_name = NULL;
}

void writeotf()
{
    tabdir_entry *tab;
    long i;
    set_cur_file_name(fm_cur->ff_name);
    if (!open_input(&ttf_file, kpse_type1_format, FOPEN_RBIN_MODE)) {
        pdftex_fail("cannot open OpenType font file for reading");
    }
    cur_file_name = (char*)nameoffile + 1;
    tex_printf("<<%s", cur_file_name);
    fontfile_found = true;
    dir_tab = NULL;
    glyph_tab = NULL;
    ttf_read_tabdir();
    /* read font parameters */
    if (ttf_name_lookup("head", false) != NULL)
        ttf_read_head();
    if (ttf_name_lookup("hhea", false) != NULL)
        ttf_read_hhea();
    if (ttf_name_lookup("PCLT", false) != NULL)
        ttf_read_pclt();
    if (ttf_name_lookup("post", false) != NULL)
        ttf_read_post();
    /* copy font file */
    tab = ttf_seek_tab("CFF ", 0);
    for (i = tab->length; i > 0; i--)
        copy_char();
    xfree(dir_tab);
    ttf_close();
    tex_printf(">>");
    cur_file_name = NULL;
}

/*
 To calculate the checkSum for the 'head' table which itself includes the
   checkSumAdjustment entry for the entire font, do the following:

     * Set the checkSumAdjustment to 0.
     * Calculate the checksum for all the tables including the 'head' table
       and enter that value into the table directory.
     * Calculate the checksum for the entire font.
     * Subtract that value from the hex value B1B0AFBA.
     * Store the result in checkSumAdjustment.

   The checkSum for the 'head table which includes the checkSumAdjustment
   entry for the entire font is now incorrect. That is not a problem. Do not
   change it. An application attempting to verify that the 'head' table has
   not changed should calculate the checkSum for that table by not including
   the checkSumAdjustment value, and compare the result with the entry in the
   table directory.

   The table directory also includes the offset of the associated tagged
   table from the beginning of the font file and the length of that table.


 */
