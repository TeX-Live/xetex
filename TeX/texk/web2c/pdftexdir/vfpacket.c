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

$Id: //depot/Build/source/TeX/texk/web2c/pdftexdir/vfpacket.c#13 $
*/

#include "ptexlib.h"

typedef struct {
    internalfontnumber font;
    char *dataptr;
    int  len;
} packet_entry;

/* define packet_ptr, packet_array & packet_limit */
define_array(packet);   

typedef struct {
    char **data;
    int *len;
    internalfontnumber font;
}  vf_entry;

/* define vf_ptr, vf_array & vf_limit */
define_array(vf);   

static char *packet_data_ptr;

integer newvfpacket(internalfontnumber f)
{
    int i,
        n = fontec[f] - fontbc[f] + 1;
    alloc_array(vf, 1, SMALL_ARRAY_SIZE);
    vf_ptr->len = xtalloc(n, int);
    vf_ptr->data = xtalloc(n, char *);
    for (i = 0; i < n; i++) {
        vf_ptr->data[i] = NULL;
        vf_ptr->len[i] = 0;
    }
    vf_ptr->font = f;
    return vf_ptr++ - vf_array;
}

void storepacket(integer f, integer c, integer s)
{
    int l = strstart[s + 1] - strstart[s];
    vf_array[vfpacketbase[f]].len[c - fontbc[f]] = l;
    vf_array[vfpacketbase[f]].data[c - fontbc[f]] = xtalloc(l, char);
    memcpy((void *)vf_array[vfpacketbase[f]].data[c - fontbc[f]], 
           (void *)(strpool + strstart[s]), (unsigned)l);
}

void pushpacketstate()
{
    alloc_array(packet, 1, SMALL_ARRAY_SIZE);
    packet_ptr->font = f;
    packet_ptr->dataptr = packet_data_ptr;
    packet_ptr->len = vfpacketlength;
    packet_ptr++;
}

void poppacketstate()
{
    if (packet_ptr == packet_array)
        pdftex_fail("packet stack empty, impossible to pop");
    packet_ptr--;
    f = packet_ptr->font;
    packet_data_ptr = packet_ptr->dataptr;
    vfpacketlength = packet_ptr->len;
}

void startpacket(internalfontnumber f, integer c)
{
    packet_data_ptr = vf_array[vfpacketbase[f]].data[c - fontbc[f]];
    vfpacketlength = vf_array[vfpacketbase[f]].len[c - fontbc[f]];
}

eightbits packetbyte()
{
    vfpacketlength--;
    return *packet_data_ptr++;
}

void vf_free(void)
{
    vf_entry *v;
    int n;
    char **p;
    if (vf_array != NULL) {
        for (v = vf_array; v < vf_ptr; v++) {
            xfree(v->len);
            n = fontec[v->font] - fontec[v->font] + 1;
            for (p = v->data; p - v->data < n ; p++)
                xfree(*p);
            xfree(v->data);
        }
        xfree(vf_array);
    }
    xfree(packet_array);
}
