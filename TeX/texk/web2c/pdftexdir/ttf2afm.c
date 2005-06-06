/*
Copyright (c) 1996-2003 Han The Thanh, <thanh@pdftex.org>

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

$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/ttf2afm.c#16 $
*/

/*
#include <kpathsea/c-auto.h>
#include <kpathsea/c-fopen.h>
#include <kpathsea/c-limits.h>
#include <kpathsea/c-memstr.h>
#include <kpathsea/c-proto.h>
#include <kpathsea/c-std.h>
#include <kpathsea/c-unistd.h>
#include <kpathsea/c-vararg.h>
#include <kpathsea/getopt.h>
*/
#include <kpathsea/kpathsea.h>
#include <time.h>
#include <pdftexdir/ptexmac.h>

static const char perforce_id[] = 
    "$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/ttf2afm.c#16 $";

typedef unsigned char   TTF_BYTE;
typedef signed char     TTF_CHAR;
typedef unsigned short  TTF_USHORT;
typedef signed short    TTF_SHORT;
typedef unsigned long   TTF_ULONG;
typedef signed long     TTF_LONG;
typedef unsigned long   TTF_FIXED;
typedef unsigned short  TTF_FUNIT;
typedef signed short    TTF_FWORD;
typedef unsigned short  TTF_UFWORD;
typedef unsigned short  TTF_F2DOT14;

typedef struct {
    char tag[4];
    TTF_ULONG checksum;
    TTF_ULONG offset;
    TTF_ULONG length;
} dirtab_entry;

typedef struct {
    TTF_ULONG wx;
    const char *name;
    TTF_USHORT index;
    TTF_LONG bbox[4];
    TTF_LONG offset;
    char found;
} mtx_entry;

typedef struct _kern_entry {
    TTF_FWORD value;
    TTF_USHORT adjacent;
    struct _kern_entry *next;
} kern_entry;

typedef struct {
    TTF_USHORT platform_id;
    TTF_USHORT encoding_id;
    TTF_ULONG  offset;
} cmap_entry;

typedef struct {
    TTF_USHORT endCode;
    TTF_USHORT startCode;
    TTF_USHORT idDelta;
    TTF_USHORT idRangeOffset;
} seg_entry;


#define TTF_BYTE_SIZE       1
#define TTF_CHAR_SIZE       1
#define TTF_USHORT_SIZE     2
#define TTF_SHORT_SIZE      2
#define TTF_ULONG_SIZE      4
#define TTF_LONG_SIZE       4
#define TTF_FIXED_SIZE      4
#define TTF_FWORD_SIZE      2
#define TTF_UFWORD_SIZE     2
#define TTF_F2DOT14_SIZE    2

#define get_type(t)         ((t)getnum(t##_SIZE))

#define get_byte()      get_type(TTF_BYTE)
#define get_char()      get_type(TTF_CHAR)
#define get_ushort()    get_type(TTF_USHORT)
#define get_short()     get_type(TTF_SHORT)
#define get_ulong()     get_type(TTF_ULONG)
#define get_long()      get_type(TTF_LONG)
#define get_fixed()     get_type(TTF_FIXED)
#define get_funit()     get_type(TTF_FUNIT)
#define get_fword()     get_type(TTF_FWORD)
#define get_ufword()    get_type(TTF_UFWORD)
#define get_f2dot14()   get_type(TTF_F2DOT14)

#define NTABS           24

/*
#define append_char_to_buf(c, p, buf, buf_size) do {       \
    if (c == 9)                                            \
        c = 32;                                            \
    if (c == 13 || c == EOF)                               \
        c = 10;                                            \
    if (c != ' ' || (p > buf && p[-1] != 32)) {            \
        check_buf(p - buf, buf_size);                      \
        *p++ = c;                                          \
    }                                                      \
} while (0)

#define append_eol(p, buf, buf_size) do {                  \
    if (p - buf > 1 && p[-1] != 10) {                      \
        check_buf(p - buf, buf_size);                      \
        *p++ = 10;                                         \
    }                                                      \
    if (p - buf > 2 && p[-2] == 32) {                      \
        p[-2] = 10;                                        \
        p--;                                               \
    }                                                      \
    *p = 0;                                                \
} while (0)

#define read_field(r, q, buf) do {                         \
    for (q = buf; *r != 32 && *r != 10; *q++ = *r++);      \
    *q = 0;                                                \
    if (*r == 32)                                          \
        r++;                                               \
} while (0)
*/

#define ttf_alloc(n, t) ((t *) xmalloc ((n) * sizeof (t)))
#define pdftex_fail     ttf_fail

#define NMACGLYPHS      258
#define MAX_CHAR_CODE   255
#define ENC_BUF_SIZE    1024
#define INDEXED_GLYPH_PREFIX    "index"

#define enc_getchar()   xgetc(encfile)
#define enc_eof()       feof(encfile)

#define ttf_skip(n)         getnum(n)
#define print_str(S)    if (S != NULL) fprintf(outfile, #S " %s\n", S)
#define print_dimen(N)  if (N != 0) fprintf(outfile, #N " %i\n", (int)get_ttf_funit(N))

#define get_ttf_funit(n) \
    (n < 0 ? -((-n/upem)*1000 + ((-n%upem)*1000)/upem) :\
    ((n/upem)*1000 + ((n%upem)*1000)/upem))

char *FontName = NULL;
char *FullName = NULL;
char *FamilyName = NULL;
char *Notice = NULL;
TTF_LONG ItalicAngle = 0;
TTF_LONG IsFixedPitch = 0;
TTF_LONG FontBBox1 = 0;
TTF_LONG FontBBox2 = 0;
TTF_LONG FontBBox3 = 0;
TTF_LONG FontBBox4 = 0;
TTF_LONG UnderlinePosition = 0;
TTF_LONG UnderlineThickness = 0;
TTF_LONG CapHeight = 0;
TTF_LONG XHeight = 0;
TTF_LONG Ascender = 0;
TTF_LONG Descender = 0;

char *cur_file_name = NULL;
char *b_name = NULL;
FILE *fontfile, *encfile, *outfile = NULL;
char enc_line[ENC_BUF_SIZE];
int print_all = 0;      /* print all glyphs? */
int print_index = 0;    /* print glyph names by index? */
int read_index = 0;     /* read names in encoding file as index? */
int print_cmap = 0;
char *null_name = "UNDEFINED";

TTF_USHORT upem;
TTF_USHORT ntabs;
int nhmtx;
int post_format;
int loca_format;
int nglyphs;
int nkernpairs = 0;
int names_count = 0;
char *ps_glyphs_buf = NULL;
dirtab_entry *dir_tab;
mtx_entry *mtx_tab;
kern_entry *kern_tab;
char *enc_names[MAX_CHAR_CODE + 1];

#include "macnames.c"

void ttf_fail(char *fmt,...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\nError: ttf2afm");
    if (cur_file_name)
        fprintf(stderr, "(file %s)", cur_file_name);
    fprintf(stderr, ": ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(-1);
}

void warn(char *fmt,...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "\nWarning: ttf2afm");
    if (cur_file_name)
        fprintf(stderr, "(file %s)", cur_file_name);
    fprintf(stderr, ": ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}


/*
void *xmalloc(unsigned long size)
{
    void *p = (void *)malloc(size);
    if (p == NULL)
        ttf_fail("malloc() failed (%lu bytes)", (unsigned long)size);
    return p;
}

FILE *xfopen(char *name, char *mode)
{
    FILE *f = fopen(name, mode);
    if (f == NULL)
        ttf_fail("fopen() failed");
    return f;
}

void xfclose(FILE *f)
{
  if (fclose (f) != NULL)
        ttf_fail("fclose() failed");
}

long xftell(FILE *f)
{
    long offset = ftell(f);
    if (offset < 0)
        ttf_fail("ftell() failed");
    return offset;
}

char *xstrdup(char *s)
{
    char *p = ttf_alloc(strlen(s) + 1, char);
    return strcpy(p, s);
}

*/

int xgetc(FILE *stream)
{
    int c = getc(stream);
    if (c < 0 && c != EOF)
        ttf_fail("getc() failed");
    return c;
}

long getnum(int s)
{
    long i = 0;
    int c;
    while (s > 0) {
        if ((c = xgetc(fontfile)) < 0)
            ttf_fail("unexpected EOF");
        i = (i << 8) + c;
        s--;
    }
    return i;
}

dirtab_entry *name_lookup(char *s)
{
    dirtab_entry *p;
    for (p = dir_tab; p - dir_tab < ntabs; p++)
        if (strncmp(p->tag, s, 4) == 0)
            break;
    if (p - dir_tab == ntabs)
        p = NULL;
    return p;
}

void seek_tab(char *name, TTF_LONG offset)
{
    dirtab_entry *p = name_lookup(name);
    if (p == NULL)
        ttf_fail("can't find table `%s'", name);
    if (fseek(fontfile, p->offset + offset, SEEK_SET) < 0)
        ttf_fail("fseek() failed while reading `%s' table", name);
}

void seek_off(char *name, TTF_LONG offset)
{
    if (fseek(fontfile, offset, SEEK_SET) < 0)
        ttf_fail("fseek() failed while reading `%s' table", name);
}

void store_kern_value(TTF_USHORT i, TTF_USHORT j, TTF_FWORD v)
{
    kern_entry *pk;
    for (pk = kern_tab + i; pk->next != NULL; pk = pk->next);
    pk->next = ttf_alloc(1, kern_entry);
    pk = pk->next;
    pk->next = NULL;
    pk->adjacent = j;
    pk->value = v;
}

TTF_FWORD get_kern_value(TTF_USHORT i, TTF_USHORT j)
{
    kern_entry *pk;
    for (pk = kern_tab + i; pk->next != NULL; pk = pk->next)
        if (pk->adjacent == j)
            return pk->value;
    return 0;
}

void free_tabs()
{
    int i;
    kern_entry *p, *q, *r;
    xfree(ps_glyphs_buf);
    xfree(dir_tab);
    xfree(mtx_tab);
    for (i = 0; i <= MAX_CHAR_CODE; i++)
        if (enc_names[i] != notdef)
            free(enc_names[i]);
    for (p = kern_tab; p - kern_tab < nglyphs; p++)
        if (p->next != NULL) {
            for (q = p->next; q != NULL; q = r) {
                r = q->next;
                xfree(q);
            }
        }
    xfree(kern_tab);
}

void enc_getline()
{
    char *p;
    int c;
restart:
    if (enc_eof())
        ttf_fail("unexpected end of file");
    p = enc_line;
    do {
        c = enc_getchar();
        append_char_to_buf(c, p, enc_line, ENC_BUF_SIZE);
    } while (c != 10);
    append_eol(p, enc_line, ENC_BUF_SIZE);
    if (p - enc_line <= 2 || *enc_line == '%')
        goto restart;
}

void read_encoding(char *encname)
{
    char buf[ENC_BUF_SIZE], *q, *r;
    int i;
    cur_file_name = encname;
    if ((encfile = kpse_open_file(encname, kpse_enc_format)) == NULL)
        ttf_fail("can't open encoding file for reading");
    enc_getline();
    if (*enc_line != '/' || (r = strchr(enc_line, '[')) == 0)
        ttf_fail("invalid encoding vector: name or `[' missing:\n%s", enc_line);
    for (i = 0; i <= MAX_CHAR_CODE; i++)
        enc_names[i] = (char*) notdef;
    if (r[1] == 32)
        r += 2;
    else
        r++;
    for (;;) {
        while (*r == '/') {
            for (q = buf, r++; *r != ' ' && *r != 10 && *r != ']' && *r != '/'; *q++ = *r++);
            *q = 0;
            if (*r == ' ')
                r++;
            if (strcmp(buf, notdef) != 0)
                enc_names[names_count] = xstrdup(buf);
            if (names_count++ > MAX_CHAR_CODE)
                ttf_fail("encoding vector contains more than %i names",
                     (int)(MAX_CHAR_CODE + 1));
        }
        if (*r != 10 && *r != '%')
            if (strncmp(r, "] def", strlen("] def")) == 0)
                goto done;
            else
                ttf_fail("invalid encoding vector: a name or `] def' expected:\n%s", enc_line);
        enc_getline();
        r = enc_line;
    }
done:
    xfclose(encfile, cur_file_name);
}

void read_font()
{
    long i, j, k, l, n, m, platform_id, encoding_id;
    TTF_FWORD kern_value;
    char buf[1024], *p;
    dirtab_entry *pd;
    kern_entry *pk;
    mtx_entry *pm;
    ttf_skip(TTF_FIXED_SIZE);
    ntabs = get_ushort();
    ttf_skip(3*TTF_USHORT_SIZE);
    dir_tab = ttf_alloc(ntabs, dirtab_entry);
    for (pd = dir_tab; pd - dir_tab < ntabs; pd++) {
        pd->tag[0] = get_char();
        pd->tag[1] = get_char();
        pd->tag[2] = get_char();
        pd->tag[3] = get_char();
        ttf_skip(TTF_ULONG_SIZE);
        pd->offset = get_ulong();
        pd->length = get_ulong();
    }
    seek_tab("head", 2*TTF_FIXED_SIZE + 2*TTF_ULONG_SIZE + TTF_USHORT_SIZE);
    upem = get_ushort();
    ttf_skip(16);
    FontBBox1 = get_fword();
    FontBBox2 = get_fword();
    FontBBox3 = get_fword();
    FontBBox4 = get_fword();
    ttf_skip(TTF_USHORT_SIZE);
    ttf_skip(TTF_USHORT_SIZE + TTF_SHORT_SIZE);
    loca_format = get_short();
    seek_tab("maxp", TTF_FIXED_SIZE);
    nglyphs = get_ushort();
    mtx_tab = ttf_alloc(nglyphs + 1, mtx_entry);
    for (pm = mtx_tab; pm - mtx_tab < nglyphs + 1; pm++) {
        pm->name = NULL; /* notdef */
        pm->found = 0;
    }
    seek_tab("hhea", TTF_FIXED_SIZE);
    Ascender = get_fword();
    Descender = get_fword();
    ttf_skip(TTF_FWORD_SIZE + TTF_UFWORD_SIZE + 3*TTF_FWORD_SIZE + 8*TTF_SHORT_SIZE);
    nhmtx = get_ushort();
    seek_tab("hmtx", 0);
    for (pm = mtx_tab; pm - mtx_tab < nhmtx; pm++) {
        pm->wx = get_ufword();
        ttf_skip(TTF_FWORD_SIZE);
    }
    i = pm[-1].wx;
    for (; pm - mtx_tab < nglyphs; pm++)
        pm->wx = i;
    seek_tab("post", 0);
    post_format = get_fixed();
    ItalicAngle = get_fixed();
    UnderlinePosition = get_fword();
    UnderlineThickness = get_fword();
    IsFixedPitch = get_ulong();
    ttf_skip(4*TTF_ULONG_SIZE);
    switch (post_format) {
    case 0x00010000:
        for (pm = mtx_tab; pm - mtx_tab < NMACGLYPHS; pm++)
            pm->name = mac_glyph_names[pm - mtx_tab];
        break;
    case 0x00020000:
        l = get_ushort(); /* some fonts have this value different from nglyphs */
        for (pm = mtx_tab; pm - mtx_tab < l; pm++)
            pm->index = get_ushort();
        if ((pd = name_lookup("post")) == NULL)
            ttf_fail("can't find table `post'");
        n = pd->length - (xftell(fontfile, cur_file_name) - pd->offset);
        ps_glyphs_buf = ttf_alloc(n + 1, char);
        for (m = 0, p = ps_glyphs_buf; p - ps_glyphs_buf < n;) {
            for (i = get_byte(); i > 0; i--)
                *p++ = get_char();
            *p++ = 0;
            m++;

        }
        for (pm = mtx_tab; pm - mtx_tab < l; pm++) {
            if (pm->index < NMACGLYPHS)
                pm->name = mac_glyph_names[pm->index];
            else {
                k = pm->index - NMACGLYPHS;
                if (k < m) {
                    for (p = ps_glyphs_buf; k > 0; k--)
                        p = (char *)strend(p) + 1;
                    pm->name = p;
                }
                else {
                    pm->name = NULL; /* index out of valid range, fix name to notdef */
                }
            }
        }
        break;
    case 0x00030000:
        if (print_index == 0) {
            warn("no names available in `post' table, printing by index forced");
            print_index = 2;
        }
        break;
    default:
        ttf_fail("unsupported format (%.8X) of `post' table", post_format);
    }
    seek_tab("loca", 0);
    for (pm = mtx_tab; pm - mtx_tab < nglyphs; pm++)
        pm->offset = (loca_format == 1 ? get_ulong() : get_ushort() << 1);
    if ((pd = name_lookup("glyf")) == NULL)
        ttf_fail("can't find table `glyf'");
    for (n = pd->offset, pm = mtx_tab; pm - mtx_tab < nglyphs; pm++)
        if (pm->offset != (pm + 1)->offset) {
            seek_off("glyf", n + pm->offset);
            ttf_skip(TTF_SHORT_SIZE);
            pm->bbox[0] = get_fword();
            pm->bbox[1] = get_fword();
            pm->bbox[2] = get_fword();
            pm->bbox[3] = get_fword();
        }
        else { /* get the BBox from .notdef */
            pm->bbox[0] = mtx_tab[0].bbox[0];
            pm->bbox[1] = mtx_tab[0].bbox[1];
            pm->bbox[2] = mtx_tab[0].bbox[2];
            pm->bbox[3] = mtx_tab[0].bbox[3];
        }
    seek_tab("name", TTF_USHORT_SIZE);
    i = ftell(fontfile);
    n = get_ushort();
    j = get_ushort() + i - TTF_USHORT_SIZE;
    i += 2*TTF_USHORT_SIZE;
    while (n-- > 0) {
        seek_off("name", i);
        platform_id = get_ushort();
        encoding_id = get_ushort();
        get_ushort(); /* skip language_id */
        k = get_ushort();
        l = get_ushort();
        if ((platform_id == 1 && encoding_id == 0) &&
            (k == 0 || k == 1 || k == 4 || k == 6)) {
            seek_off("name", j + get_ushort());
            for (p = buf; l-- > 0; p++)
                *p = get_char();
            *p++ = 0;
            p = xstrdup(buf);
            switch (k) {
            case 0:  Notice = p; break;
            case 1:  FamilyName = p; break;
            case 4:  FullName = p; break;
            case 6:  FontName = p; break;
            }
            if (Notice != NULL && FamilyName != NULL && FullName != NULL && FontName != NULL)
                break;
        }
        i += 6*TTF_USHORT_SIZE;
    }
    if ((pd = name_lookup("PCLT")) != NULL) {
        seek_off("PCLT", pd->offset + TTF_FIXED_SIZE + TTF_ULONG_SIZE + TTF_USHORT_SIZE);
        XHeight = get_ushort();
        ttf_skip(2*TTF_USHORT_SIZE);
        CapHeight = get_ushort();
    }
    if ((pd = name_lookup("kern")) == NULL)
        return;
    kern_tab = ttf_alloc(nglyphs, kern_entry);
    for (pk = kern_tab; pk - kern_tab < nglyphs; pk++) {
        pk->next = NULL;
        pk->value = 0;
    }
    seek_off("kern", pd->offset + TTF_USHORT_SIZE);
    for (n = get_ushort(); n > 0; n--) {
        ttf_skip(2*TTF_USHORT_SIZE);
        k = get_ushort();
        if (!(k & 1) || (k & 2) || (k & 4))
            return;
        if (k >> 8 != 0) {
            warn("warning: only format 0 supported of `kern' \
                 subtables, others are ignored\n");
            continue;
        }
        k = get_ushort();
        ttf_skip(3*TTF_USHORT_SIZE);
        while (k-- > 0) {
            i = get_ushort();
            j = get_ushort();
            kern_value = get_fword();
            if (kern_value != 0) {
                store_kern_value(i, j, kern_value);
                nkernpairs++;
            }
        }
    }
}

int null_glyph(const char *s)
{
    if (s != NULL &&
        (strcmp(s, ".null") == 0 ||
         strcmp(s, ".notdef") == 0))
        return 1;
    return 0;
}

#define fix_glyph_name(s)   ((s) != NULL ? (s) : null_name)
#define dont_print(s)       (!print_all && !print_index && null_glyph(s))
#define glyph_found(i)       (print_all || mtx_tab[i].found)

void print_glyph_name(FILE *f, int i)
{
    const char *s = mtx_tab[i].name;
    switch (print_index) {
    case 0:
        fprintf(f, fix_glyph_name(s));
        break;
    case 1:
        fprintf(f, "%s%i", INDEXED_GLYPH_PREFIX, i);
        break;
    case 2:
        if (i < 0x0100)
            fprintf(f, "%s0x%.2X", INDEXED_GLYPH_PREFIX, i);
        else
            fprintf(f, "%s0x%.4X", INDEXED_GLYPH_PREFIX, i);
        break;
    }
}

void print_afm(char *date, char *fontname)
{
    int i, ncharmetrics;
    mtx_entry *pm;
    int new_nkernpairs;
    short mtx_index[MAX_CHAR_CODE + 1], *idx;
    char **pe;
    kern_entry *pk, *qk;
    char buf[20];
    fprintf(outfile, "Comment Converted at %s by ttf2afm from font file `%s'", date, fontname);
    fputs("\nStartFontMetrics 2.0\n", outfile);
    print_str(FontName);
    print_str(FullName);
    print_str(FamilyName);
    print_str(Notice);
    fprintf(outfile, "ItalicAngle %i", (int)(ItalicAngle/0x10000));
    if (ItalicAngle%0x10000 > 0)
        fprintf(outfile, ".%i", (int)((ItalicAngle%0x10000)*1000)/0x10000);
    fputs("\n", outfile);
    fprintf(outfile, "IsFixedPitch %s\n", IsFixedPitch ? "true" : "false");
    fprintf(outfile, "FontBBox %i %i %i %i\n", 
            (int)get_ttf_funit(FontBBox1),
            (int)get_ttf_funit(FontBBox2),
            (int)get_ttf_funit(FontBBox3),
            (int)get_ttf_funit(FontBBox4));
    print_dimen(UnderlinePosition);
    print_dimen(UnderlineThickness);
    print_dimen(CapHeight);
    print_dimen(XHeight);
    print_dimen(Ascender);
    print_dimen(Descender);
    ncharmetrics = nglyphs;
    if (!print_all && !print_index)
        for (pm = mtx_tab; pm - mtx_tab < nglyphs; pm++)
            if (null_glyph(pm->name))
                ncharmetrics--;
    if (names_count == 0) { /* external encoding vector not given */
        fprintf(outfile, "\nStartCharMetrics %u\n", ncharmetrics);
        for (pm = mtx_tab; pm - mtx_tab < nglyphs; pm++) {
            if (dont_print(pm->name))
                continue;
            pm->found = 1;
            fprintf(outfile, "C -1 ; WX %i ; N ", (int)get_ttf_funit(pm->wx));
            print_glyph_name(outfile, pm - mtx_tab);
            fprintf(outfile, " ; B %i %i %i %i ;\n", 
                       (int)get_ttf_funit(pm->bbox[0]),
                       (int)get_ttf_funit(pm->bbox[1]),
                       (int)get_ttf_funit(pm->bbox[2]),
                       (int)get_ttf_funit(pm->bbox[3]));
        }
    }
    else { /* external encoding vector given */
        for (idx = mtx_index; idx - mtx_index <= MAX_CHAR_CODE; *idx++ = 0);
        if (!print_all)
            ncharmetrics = 0;
        for (pe = enc_names; pe - enc_names < names_count; pe++) {
            if (*pe == notdef)
                continue;
            if (read_index) {
                if (sscanf(*pe, INDEXED_GLYPH_PREFIX "%i", &i) == 1) {
                    if (i < 0 || i > nglyphs)
                        ttf_fail("`%s' out of valid range (0..%i)",
                             *pe, nglyphs);
                    pm = mtx_tab + i;
                }
                else
                    ttf_fail("`%s<num>' expected instead of `%s'", 
                         INDEXED_GLYPH_PREFIX, *pe);
            }
            else 
                for (pm = mtx_tab; pm - mtx_tab < nglyphs; pm++)
                    if (pm->name != NULL && strcmp(*pe, pm->name) == 0)
                            break;
            if (pm - mtx_tab < nglyphs) {
                mtx_index[pe - enc_names] = pm - mtx_tab;
                pm->found = 1;
                if (!print_all)
                    ncharmetrics++;
            }
            else
                warn("glyph `%s' not found", *pe);
        }
        fprintf(outfile, "\nStartCharMetrics %u\n", ncharmetrics);
        for (idx = mtx_index; idx - mtx_index <= MAX_CHAR_CODE; idx++) {
            if (dont_print(mtx_tab[*idx].name))
                continue;
            if (*idx != 0)
                if (mtx_tab[*idx].found == 1) {
                    fprintf(outfile, "C %d ; WX %i ; N ",
                           idx - mtx_index, 
                           (int)get_ttf_funit(mtx_tab[*idx].wx));
                    print_glyph_name(outfile, *idx);
                    fprintf(outfile, " ; B %i %i %i %i ;\n", 
                           (int)get_ttf_funit(mtx_tab[*idx].bbox[0]),
                           (int)get_ttf_funit(mtx_tab[*idx].bbox[1]),
                           (int)get_ttf_funit(mtx_tab[*idx].bbox[2]),
                           (int)get_ttf_funit(mtx_tab[*idx].bbox[3]));
                }
        }
        if (!print_all)
            goto end_metrics;
        for (pm = mtx_tab; pm - mtx_tab < nglyphs; pm++) {
            if (dont_print(pm->name))
                continue;
            if (pm->found == 0) {
                fprintf(outfile, "C -1 ; WX %i ; N ",
                       (int)get_ttf_funit(pm->wx));
                print_glyph_name(outfile, pm - mtx_tab);
                fprintf(outfile, " ; B %i %i %i %i ;\n", 
                       (int)get_ttf_funit(pm->bbox[0]),
                       (int)get_ttf_funit(pm->bbox[1]),
                       (int)get_ttf_funit(pm->bbox[2]),
                       (int)get_ttf_funit(pm->bbox[3]));
            }
        }
    }
end_metrics:
    fputs("EndCharMetrics\n", outfile);
    if (nkernpairs == 0)
        goto end_kerns;
    new_nkernpairs = 0;
    for (pk = kern_tab; pk - kern_tab < nglyphs; pk++)
        if (!dont_print(mtx_tab[pk - kern_tab].name) &&
            glyph_found(pk - kern_tab))
            for (qk = pk; qk != NULL; qk = qk->next)
                if (qk->value != 0 &&
                    !dont_print(mtx_tab[qk->adjacent].name) &&
                    glyph_found(qk->adjacent))
                    new_nkernpairs++;
    if (new_nkernpairs == 0)
        goto end_kerns;
    fprintf(outfile, "\nStartKernData\nStartKernPairs %i\n", new_nkernpairs);
    for (pk = kern_tab; pk - kern_tab < nglyphs; pk++)
        if (!dont_print(mtx_tab[pk - kern_tab].name) &&
            glyph_found(pk - kern_tab))
            for (qk = pk; qk != NULL; qk = qk->next)
                if (qk->value != 0 && 
                    !dont_print(mtx_tab[qk->adjacent].name) &&
                    glyph_found(qk->adjacent)) {
                    fputs("KPX ", outfile);
                    print_glyph_name(outfile, pk - kern_tab);
                    fputs(" ", outfile);
                    print_glyph_name(outfile, qk->adjacent);
                    fprintf(outfile, " %i\n", get_ttf_funit(qk->value));
                }
    fputs("EndKernPairs\nEndKernData\n", outfile);
end_kerns:
    fputs("EndFontMetrics\n", outfile);
}

int print_sep(FILE *file)
{
    static int names_counter = 0;
    if (++names_counter == 1) {
        fprintf(file, "\n");
        names_counter = 0;
        return 1;
    }
    fprintf(file, " ");
    return 0;
}

void print_encoding(char *fontname)
{
    long int netabs, i, k, first_code, length, last_sep;
    cmap_entry *cmap_tab, *e;
    seg_entry *seg_tab, *s;
    long cmap_offset;
    FILE *file;
    TTF_USHORT *glyphId, format, segCount;
    char *enc_name, *end_enc_name;
    const char *n;
    seek_tab("cmap", TTF_USHORT_SIZE); /* skip the table vesrion number (0) */
    netabs = get_ushort();
    cmap_offset = xftell(fontfile, cur_file_name) - 2*TTF_USHORT_SIZE;
    cmap_tab = ttf_alloc(netabs, cmap_entry);
    for (e = cmap_tab; e - cmap_tab < netabs; e++) {
        e->platform_id = get_ushort();
        e->encoding_id = get_ushort();
        e->offset = get_ulong();
    }
    enc_name = ttf_alloc(strlen(b_name) + 20, char);
    strcpy(enc_name, b_name);
    end_enc_name = strend(enc_name);
    for (e = cmap_tab; e - cmap_tab < netabs; e++) {
        seek_off("cmap", cmap_offset + e->offset);
        format = get_ushort();
        if (format != 0 && format != 6 && format != 4) {
            warn("format %i of encoding subtable unsupported", (int)format);
            continue;
        }
        sprintf(end_enc_name, ".e%i%i", 
                (int)e->platform_id, (int)e->encoding_id);
        if ((file = xfopen(enc_name, FOPEN_W_MODE)) == NULL)
            ttf_fail("cannot open file for writing (%s)\n", enc_name);
        fprintf(file, "%% Encoding table from font file %s\n", fontname);
        fprintf(file, "%% Platform ID %i", (int)e->platform_id);
        switch (e->platform_id) {
        case 0:  fprintf(file, " (Apple Unicode)"); break;
        case 1:  fprintf(file, " (Macintosh)"); break;
        case 2:  fprintf(file, " (ISO)"); break;
        case 3:  fprintf(file, " (Microsoft)"); break;
        }
        fprintf(file, "\n");
        fprintf(file, "%% Encoding ID %i", (int)e->encoding_id);
        if (e->platform_id == 1 &&  e->encoding_id == 0)
            fprintf(file, " (Roman)");
        if (e->platform_id == 3)
            switch (e->encoding_id) {
            case 0:  fprintf(file, " (Symbol)"); break;
            case 1:  fprintf(file, " (Unicode)"); break;
            }
        fprintf(file, "\n");
        fprintf(file, "%% Format %i", (int)(format));
        switch (format) {
        case 0:  fprintf(file, " (byte encoding table)"); break;
        case 6:  fprintf(file, " (trimmed table mapping)"); break;
        case 4:  fprintf(file, " (segment mapping to delta values)"); break;
        }
        fprintf(file, "\n");
        fprintf(file, "/Encoding%i [\n", (int)(e - cmap_tab + 1));
        switch (format) {
        case 0:
            get_ushort(); /* skip length */
            get_ushort(); /* skip version number */
            for (i = 0; i <= MAX_CHAR_CODE; i++) {
                k = get_byte();
                n = mtx_tab[k].name;
                fputs("/", file);
                print_glyph_name(file, k);
                last_sep = print_sep(file);
            }
            break;
        case 6:
            get_ushort(); /* skip table length */
            get_ushort(); /* skip version number */
            first_code = get_ushort(); /* first character code */
            length = get_ushort(); /* number of character codes */
            for (i = 0; i < length; i++) {
                k = get_ushort() + first_code;
                n = mtx_tab[k].name;
                fputs("/", file);
                print_glyph_name(file, k);
                last_sep = print_sep(file);
            }
            break;
        case 4:
            length = get_ushort(); /* length of subtable */
            get_ushort(); /* skip the version number */
            segCount = get_ushort();
            segCount /= 2;
            get_ushort(); /* skip searchRange */
            get_ushort(); /* skip entrySelector */
            get_ushort(); /* skip rangeShift */
            seg_tab = ttf_alloc(segCount, seg_entry);
            for (s = seg_tab; s - seg_tab < segCount; s++)
                s->endCode = get_ushort();
            get_ushort(); /* skip reversedPad */
            for (s = seg_tab; s - seg_tab < segCount; s++)
                s->startCode = get_ushort();
            for (s = seg_tab; s - seg_tab < segCount; s++)
                s->idDelta = get_ushort();
            for (s = seg_tab; s - seg_tab < segCount; s++)
                s->idRangeOffset = get_ushort();
            length -= 8*TTF_USHORT_SIZE + 4*segCount*TTF_USHORT_SIZE;
            glyphId = ttf_alloc(length, TTF_USHORT);
            for (i = 0; i < length; i++)
                glyphId[i] = get_ushort();
            for (i = 0; i <= MAX_CHAR_CODE; i++) {
                for (s = seg_tab; s - seg_tab < segCount; s++)
                    if (s->endCode >= i)
                        break;
                if (s - seg_tab < segCount && s->startCode <= i) {
                    if (s->idRangeOffset != 0) {
                        k = glyphId[(i-s->startCode) + s->idRangeOffset/2 - (segCount-(s-seg_tab))];
                        if (k != 0)
                            k = (k + s->idDelta) % 0xFFFF;
                    }
                    else
                        k = (s->idDelta + i) % 0xFFFF;
                    n = mtx_tab[k].name;
                    fputs("/", file);
                    print_glyph_name(file, k);
                }
                else {
                    fputs("/", file);
                    print_glyph_name(file, 0);
                }
                last_sep = print_sep(file);
            }
            if (last_sep == 0)
                fprintf(file, "\n");
            fprintf(file, "%% Characters with code larger than %i have been ignored:\n", 
                           (int)MAX_CHAR_CODE);
            for (s = seg_tab; s - seg_tab < segCount; s++) {
                if (s->endCode <= MAX_CHAR_CODE)
                    continue;
                else if (s->endCode == 0xFFFF)
                    break;
                for (i = s->startCode; i <= s->endCode; i++) {
                    if (s->idRangeOffset != 0) {
                        k = glyphId[(i-s->startCode) + s->idRangeOffset/2 - (segCount-(s-seg_tab))];
                        if (k != 0)
                            k = (k + s->idDelta) % 0xFFFF;
                    }
                    else
                        k = (s->idDelta + i) % 0xFFFF;
                    if (print_index == 2)
                        fprintf(file, "%% 0x%.4X -> 0x%.4X\n", (int)i, (int)k);
                    else
                        fprintf(file, "%% %i -> %i\n", (int)i, (int)k);
                }
            }
        }
        fprintf(file, "] def\n");
    }
    xfree(enc_name);
}

void usage()
{
    cur_file_name = NULL;
    fprintf(stderr,
        "Usage: ttf2afm [-a] [-c name] [-i] [-h|-d] [-e enc] [-o output] fontfile\n"
        "    -a:        print all glyphs\n"
        "    -i:        read names in encoding vector in form `%s<num>'\n"
        "    -d:        print glyph names in form `%s<decimal_num>'\n"
        "    -h:        print glyph names in form `%s<hex_num>'\n"
        "    -c name:   print encoding tables to `name.eMN'\n"
        "    -e enc:    use encoding from file `enc'\n"
        "    -o output: output to file `output' instead of stdout\n"
        "    fontfile:  the TrueType font\n",
        INDEXED_GLYPH_PREFIX, INDEXED_GLYPH_PREFIX, INDEXED_GLYPH_PREFIX);
    _exit(-1);
}

int main(int argc, char **argv)
{
    char date[128];
    time_t t = time(&t);
    int c;
    kpse_set_progname(argv[0]);
    kpse_init_prog("ttf2afm", 0, 0, 0) ;
    while ((c = getopt(argc, argv, "ac:e:ihdo:")) != -1)
        switch(c) {
        case 'a':
            print_all = 1;
            break;
        case 'c':
            print_cmap = 1;
            b_name = xstrdup(optarg);
            break;
        case 'e':
            cur_file_name = optarg;
            read_encoding(cur_file_name);
            break;
        case 'i':
            read_index = 1;
            break;
        case 'd':
            print_index = 1;
            break;
        case 'h':
            print_index = 2;
            break;
        case 'o':
            if (outfile != NULL)
                usage();
            cur_file_name = optarg;
            outfile = xfopen(cur_file_name, FOPEN_W_MODE);
            if (outfile == NULL)
                ttf_fail("cannot open file for writing");
            break;
        default:
            usage();
        }
    if (argc - optind != 1)
        usage();
    sprintf(date, "%s\n", ctime(&t));
    *(char *)strchr(date, '\n') = 0;
    cur_file_name = argv[optind];
    if ((fontfile = kpse_open_file(cur_file_name, kpse_truetype_format)) == NULL)
        ttf_fail("can't open font file for reading");
    read_font();
    if (outfile == NULL)
        outfile = stdout;
    print_afm(date, cur_file_name);
    if (print_cmap)
        print_encoding(cur_file_name);
    xfree(Notice);
    xfree(FullName);
    xfree(FontName);
    xfree(b_name);
    xfclose(fontfile, cur_file_name);
    return 0;
}
