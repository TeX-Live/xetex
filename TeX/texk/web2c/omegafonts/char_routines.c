/* char_routines.c: Data structures for character information

This file is part of Omega,
which is based on the web2c distribution of TeX,

Copyright (c) 1994--2001 John Plaice and Yannis Haralambous

Omega is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Omega is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Omega; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

*/

#include "cpascal.h"
#include "manifests.h"
#include "list_routines.h"
#include "char_routines.h"
#include "print_routines.h"
#include "out_routines.h"
#include "error_routines.h"
#include "ligkern_routines.h"
#include "dimen_routines.h"
#include "header_routines.h"
#include "font_routines.h"
#include "out_ofm.h"
#include "omfonts.h"
#include "dvi.h"

#define PLANE		0x10000
#define HALFPLANE	0x08000

#define MAX_START_OFM	65535
#define MAX_START_TFM	255

/*
 * Characters can range from 0x0 to 0x7fffffff (31 bits unsigned),
 * which is a lot of characters.  We would expect characters to either
 * be bunched up in a given region, or else sparsely defined throughout
 * the range.  The data structure to hold them is an array of HALFPLANE
 * arrays, each of which holds a full PLANE (0x10000) of characters.
 *
 * At all times, init_character ensures that planes[0] to planes[plane_max]
 * are either NULL or allocated arrays of PLANE characters.
 *
 * For allocated array p, init_character also ensures that planes[p][0]
 * to planes[p][char_max[p]] are either NULL or pointers to allocated
 * char_entry values.
 *
 * init_character can be called when actually reading a CHARACTER definition
 * or when the character is referenced in a charlist cycle, an extensible
 * piece or in a ligature/kerning table.  In the latter case, ensure_existence
 * is called, and it sets the defined field of the character to be FALSE;
 * 
 * init_character also sets current_character to the character entry;
 * 
 */

unsigned bc=0x7fffffff;
unsigned ec=0x0;
unsigned ne=0;

char_entry **planes[HALFPLANE];
unsigned char_max[HALFPLANE];
unsigned plane_max = 0;

char_entry *current_character = NULL;
char_entry *current_secondary_character = NULL;

queue exten_queue;
four_pieces **exten_table = NULL;

unsigned no_labels = 0;
label_entry *label_table;
int label_ptr, sort_ptr;
int lk_offset;
boolean extra_loc_needed;

extern unsigned bchar;

void
init_planes(void)
{
    plane_max = 0;
    planes[plane_max] = NULL;
    char_max[plane_max] = 0;
}

void
init_character(unsigned c, char_entry *ready_made)
{
    register unsigned i, index, plane;

    if ((c<CHAR_MINIMUM) || (c>CHAR_MAXIMUM)) {
        warning_1("CHARACTER index (H %X) not 31-bit unsigned integer; "
                  "ignored", c);
        current_character = NULL;
        return;
    }
    plane = c / PLANE;
    index = c % PLANE;
    for (i=plane_max+1; i<=plane; i++) {
	    planes[plane] = NULL;
    }
    if (planes[plane]==NULL) {
        planes[plane] = (char_entry **)xmalloc(PLANE * sizeof(char_entry *));
        char_max[plane] = 0;
        planes[plane][0] = NULL;
    }
    for (i=char_max[plane]+1; i<=index; i++) {
        planes[plane][i] = NULL;
    }
    if (plane>plane_max)       plane_max = plane;
    if (index>char_max[plane]) char_max[plane] = index;

    if (planes[plane][index] != NULL) {
        if (planes[plane][index]->defined == FALSE) {
            current_character = planes[plane][index];
            current_character->defined = TRUE;
            return;
        }
        warning_1("CHARACTER index (H %X) previously defined; "
                  "old definition ignored", c);
        free(current_character);
        current_character = NULL;
    }
    if (ready_made != NULL) {
        current_character = ready_made;
        planes[plane][index] = current_character;
    } else {
        current_character = (char_entry *)xmalloc(sizeof(char_entry));
        planes[plane][index] = current_character;
        for (i=C_MIN; i<=C_MAX; i++) {
            current_character->indices[i] = NULL;
            current_character->index_indices[i] = 0;
        }
        current_character->c = c;
        current_character->copies = 0;
        current_character->remainder = 0;
        current_character->tag = TAG_NONE;
        current_character->defined = TRUE;
        current_character->accent = ACC_NONE;
        current_character->ovf_packet_length = 0;
        current_character->ovf_packet = NULL;
        cur_packet = NULL;
    }
}

void
copy_characters(unsigned c, unsigned copies)
{
    unsigned i=0;
    unsigned plane, index;
    char_entry *the_entry;

    if ((c<CHAR_MINIMUM) || (c>CHAR_MAXIMUM)) {
        warning_1("CHARACTER index (H %X) not 31-bit unsigned integer; "
                  "ignored", c);
        current_character = NULL;
        return;
    }
    plane = c / PLANE;
    index = c % PLANE;
    if (planes[plane]==NULL)
        internal_error_1("copy_characters (plane %d)", plane);
    the_entry = planes[plane][index];
    if (the_entry==NULL)
        internal_error_1("copy_characters (index %d)", index);
    the_entry->copies = copies;
    for (i=(c+1); i<=(c+copies); i++)
        init_character(i, the_entry);
}

void
ensure_existence(unsigned c)
{
    register unsigned index, plane;
    plane = c / PLANE;
    index = c % PLANE;

    if ((planes[plane]==NULL) || (planes[plane][index]==NULL)) {
        init_character(c, NULL);
        planes[plane][index]->defined = FALSE;
    }
    current_secondary_character = planes[plane][index];
}

#define FOR_ALL_CHARACTERS(FOR_ALL_CHARACTERS_ACTION) \
    for (plane = 0; plane <=plane_max; plane++) { \
        if (planes[plane] != NULL) { \
            for (index = 0; index <=char_max[plane]; index++) { \
                entry = planes[plane][index]; \
                c = plane*PLANE + index; \
                if (entry != NULL) {  \
                    FOR_ALL_CHARACTERS_ACTION \
                } \
            } \
        } \
    }

#define FOR_ALL_EXISTING_CHARACTERS(FOR_ALL_EXISTING_CHARACTERS_ACTION) \
    FOR_ALL_CHARACTERS( \
        if (entry->defined == TRUE) { \
            FOR_ALL_EXISTING_CHARACTERS_ACTION \
        } \
    )

void
output_ovf_chars(void)
{
    register unsigned index, plane, c, k;
    char_entry *entry;
    fix wd;

    FOR_ALL_EXISTING_CHARACTERS(
        wd = lval(entry->indices[C_WD]);
        if (design_units != UNITY)
            wd = zround(((double)wd) / ((double)design_units) * 1048576.0);
        if ((entry->ovf_packet_length>241) ||
            (wd < 0) || (wd >= 0x1000000) ||
            (c < 0) || (c >255)) {
            out_ovf(242); out_ovf_4(entry->ovf_packet_length);
            out_ovf_4(c); out_ovf_4(wd);
        } else {
            out_ovf(entry->ovf_packet_length); out_ovf(c);
            out_ovf((wd >>16) & 0xff); out_ovf((wd >>8) & 0xff);
            out_ovf(wd & 0xff);
        }
        for (k=0; k<entry->ovf_packet_length; k++)
            out_ovf(entry->ovf_packet[k]);
    )
}


void
check_existence_all_character_fields(void)
{
    register unsigned index, plane, c;
    char_entry *entry;
    unsigned *exten;
    unsigned j;
 
    FOR_ALL_EXISTING_CHARACTERS(
        switch(entry->tag) {
            case TAG_NONE: { break; }
            case TAG_LIG: {
                check_ligature_program(c, entry->remainder);
                break;
            }
            case TAG_LIST:{
                check_existence_and_safety(c, entry->remainder, NULL,
                    "%sCharacter (H %X) NEXTLARGER than (H %X) "
                    "has no CHARACTER spec");
                break;
            }
            case TAG_EXT:{
                exten = entry->extens;
                for (j=E_MIN; j<=E_MAX; j++) {
                    if (exten[j]!=0)
                        check_existence_and_safety(c, exten[j],
                            extensible_pieces[j],
                            "%s piece (H %X) of character (H %X) "
                            "has no CHARACTER spec");
                }
                break;
            }
        }
    )
}  

void
clear_ligature_entries(void)
{
    register unsigned index, plane, c;
    char_entry *entry;

    FOR_ALL_EXISTING_CHARACTERS(
        if (entry->tag == TAG_LIG) {
            entry->tag = TAG_NONE;
            entry->remainder = 0;
        }
    )
}

void
check_existence_and_safety(unsigned c, unsigned g, string extra, string fmt)
{
    char_entry *gentry = planes[g/PLANE][g%PLANE];

    if ((g<CHAR_MINIMUM) || (g>CHAR_MAXIMUM))
        internal_error_1("check_existence_and_safety (g=%d)", g);
    gentry = planes[g/PLANE][g%PLANE];
    if ((g>=128) && (c<128))
        seven_bit_calculated = 0;
    if ((gentry==NULL) || (gentry->defined == FALSE)) {
        warning_s_2(fmt, extra, g, c);
        current_character = gentry;
        set_character_measure(C_WD, 0);
    }
}

void
doublecheck_existence(unsigned g, string extra, char*fmt)
{
    char_entry *gentry = planes[g/PLANE][g%PLANE];

    if ((g<CHAR_MINIMUM) || (g>CHAR_MAXIMUM))
        internal_error_1("doublecheck_existence (g=%d)", g);
    gentry = planes[g/PLANE][g%PLANE];
    if ((gentry==NULL) || (gentry->defined == FALSE)) {
        warning_s_1(fmt, extra, g);
        current_character = gentry;
/*
        set_character_measure(C_WD, 0);
*/
    }
}

extern string character_measures[];

void
print_characters(boolean read_ovf)
{
    register unsigned index, plane, c;
    char_entry *entry;
    four_pieces *exten;
    four_entries *lentry;
    unsigned j,k;

    FOR_ALL_CHARACTERS(
        if (entry->index_indices[C_WD] != 0) {
        print_character(c);
        for (k=C_MIN; k<C_MAX; k++) {
            if (entry->index_indices[k] != 0) {
                print_character_measure(k,
                  dimen_tables[k][entry->index_indices[k]]);
            }
        }
        fflush(file_output);
        switch (entry->tag) {
            case TAG_NONE: { break; }
            case TAG_LIG:  {
                left();
                out("COMMENT"); out_ln();
                lentry = lig_kern_table + entry->remainder;
                if (lentry->entries[0] > STOP_FLAG) {
                    lentry = lig_kern_table +
                             (256*lentry->entries[2]+lentry->entries[3]); 
                }
		do {
                    print_one_lig_kern_entry(lentry, FALSE);
		    if (lentry->entries[0] >= STOP_FLAG) {
                        lentry = lig_kern_table + nl;
                    } else {
                        lentry = lentry + 1 + lentry->entries[0];
                    }
                } while (lentry < (lig_kern_table + nl));
                right();
                break;
            }
            case TAG_LIST: {
                print_next_larger(entry->remainder);
                break;
            }
            case TAG_EXT: {
	        print_var_character();
                exten = exten_table[entry->remainder];
                for (j=E_MIN; j<=E_MAX; j++) {
                    if (exten->pieces[j]!=0)
                        print_extensible_piece(j,exten->pieces[j]);
                }
                right();
                break;
            }
        }
        if ((read_ovf==TRUE) && (entry->ovf_packet_length>0)) {
            print_map();
            print_packet(entry->ovf_packet, entry->ovf_packet_length);
            right();
        }
        right();
        fflush(file_output);
        }
    )
}

unsigned stack_top = 0;
int wstack[1000];
int xstack[1000];
int ystack[1000];
int zstack[1000];

void
print_packet(unsigned char *packet_start, unsigned packet_length)
{
   unsigned cmd, arg;
   fix fix_arg, fix_arg1;
   unsigned char *packet = packet_start;
   unsigned char *max_packet = packet+packet_length;

   stack_top = 0;
   wstack[stack_top] = xstack[stack_top] =
   ystack[stack_top] = zstack[stack_top] = 0;
   while (packet <max_packet) {
   if (*packet <= DVI_SET_CHAR_127) {
         arg = *packet; packet++;
         print_set_char(arg);
   } else if ((*packet >= DVI_FNT_NUM_0) && (*packet <= DVI_FNT_NUM_63)) {
         arg = *packet - DVI_FNT_NUM_0; packet++;
         print_select_font(arg);
   } else switch(*packet) {
      case DVI_SET_1: case DVI_SET_2: case DVI_SET_3: case DVI_SET_4:
         cmd = *packet; packet++;
         arg = ovf_get_arg(&packet, cmd - DVI_SET_1 + 1, FALSE);
         print_set_char(arg);
         break;
      case DVI_NOP: 
         packet++;
         break;
      case DVI_PUSH:
         cmd = DVI_PUSH; packet++; stack_top++;
         wstack[stack_top] = wstack[stack_top-1];
         xstack[stack_top] = xstack[stack_top-1];
         ystack[stack_top] = ystack[stack_top-1];
         zstack[stack_top] = zstack[stack_top-1];
         print_push();
         break;
      case DVI_POP:
         cmd = DVI_PUSH; packet++; stack_top--;
         print_pop();
         break;
      case DVI_SET_RULE:
         cmd = DVI_SET_RULE; packet++; fix_arg = ovf_get_arg(&packet, 4, TRUE);
         fix_arg1 = ovf_get_arg(&packet, 4, TRUE);
         print_set_rule(fix_arg, fix_arg1);
         break;
      case DVI_PUT_RULE:
         cmd = DVI_PUT_RULE; packet++; fix_arg = ovf_get_arg(&packet, 4, TRUE);
         fix_arg1 = ovf_get_arg(&packet, 4, TRUE);
         print_put_rule(fix_arg, fix_arg1);
         break;
      case DVI_RIGHT_1: case DVI_RIGHT_2: case DVI_RIGHT_3: case DVI_RIGHT_4:
         cmd = *packet; packet++;
         fix_arg = ovf_get_arg(&packet, cmd - DVI_RIGHT_1 + 1, TRUE);
         print_move(M_RIGHT, fix_arg);
         break;
      case DVI_DOWN_1:  case DVI_DOWN_2:  case DVI_DOWN_3:  case DVI_DOWN_4:
         cmd = *packet; packet++;
         fix_arg = ovf_get_arg(&packet, cmd - DVI_DOWN_1 + 1, TRUE);
         print_move(M_DOWN, fix_arg);
         break;
      case DVI_W_0:
         cmd = DVI_W_0; packet++;
         print_move(M_RIGHT, wstack[stack_top]);
         break;
      case DVI_W_1: case DVI_W_2: case DVI_W_3: case DVI_W_4:
         cmd = *packet; packet++;
         wstack[stack_top] = ovf_get_arg(&packet, cmd - DVI_W_1 + 1, TRUE);
         print_move(M_RIGHT, wstack[stack_top]);
         break;
      case DVI_X_0:
         cmd = DVI_X_0; packet++;
         print_move(M_RIGHT, xstack[stack_top]);
         break;
      case DVI_X_1: case DVI_X_2: case DVI_X_3: case DVI_X_4:
         cmd = *packet; packet++;
         xstack[stack_top] = ovf_get_arg(&packet, cmd - DVI_X_1 + 1, TRUE);
         print_move(M_RIGHT, xstack[stack_top]);
         break;
      case DVI_Y_0:
         cmd = DVI_Y_0; packet++;
         print_move(M_DOWN, ystack[stack_top]);
         break;
      case DVI_Y_1: case DVI_Y_2: case DVI_Y_3: case DVI_Y_4:
         cmd = *packet; packet++;
         ystack[stack_top] = ovf_get_arg(&packet, cmd - DVI_Y_1 + 1, TRUE);
         print_move(M_DOWN, ystack[stack_top]);
         break;
      case DVI_Z_0:
         cmd = DVI_Z_0; packet++;
         print_move(M_DOWN, zstack[stack_top]);
         break;
      case DVI_Z_1: case DVI_Z_2: case DVI_Z_3: case DVI_Z_4:
         cmd = *packet; packet++;
         zstack[stack_top] = ovf_get_arg(&packet, cmd - DVI_Z_1 + 1, TRUE);
         print_move(M_DOWN, zstack[stack_top]);
         break;
      case DVI_PUT_1: case DVI_PUT_2: case DVI_PUT_3: case DVI_PUT_4:
         cmd = *packet; packet++;
         fix_arg = ovf_get_arg(&packet, cmd - DVI_PUT_1 + 1, FALSE);
         print_put_char(fix_arg);
         break;
      case DVI_XXX_1: case DVI_XXX_2: case DVI_XXX_3: case DVI_XXX_4:
         cmd = *packet; packet++;
         fix_arg = ovf_get_arg(&packet, cmd - DVI_XXX_1 + 1, FALSE);
         break;
      case DVI_FNT_1: case DVI_FNT_2: case DVI_FNT_3: case DVI_FNT_4:
         cmd = *packet; packet++;
         fix_arg = ovf_get_arg(&packet, cmd - DVI_FNT_1 + 1, FALSE);
         print_select_font(fix_arg);
         break;
      default:
         internal_error_1("Unrecognized DVI packet (%d)\n", *packet);
   }
   }
   fflush(file_output);
   fflush(stderr);
}

void
check_char_tag(unsigned c)
{
    ensure_existence(c);
}

void
set_char_tag(unsigned c, unsigned tag)
{
    ensure_existence(c);
    planes[c/PLANE][c%PLANE]->tag = tag;
}

void
set_char_remainder(unsigned c, unsigned remainder)
{
    ensure_existence(c);
    planes[c/PLANE][c%PLANE]->remainder = remainder;
}

int
get_char_remainder(unsigned c)
{
    ensure_existence(c);
    return planes[c/PLANE][c%PLANE]->remainder;
}

void
set_next_larger(unsigned larger)
{
    check_char_tag(current_character->c);
    set_char_tag(current_character->c, TAG_LIST);
    set_char_remainder(current_character->c, larger);
}

void
init_var_character(void)
{
    four_pieces *entry = (four_pieces *) xmalloc(sizeof(four_pieces));
    unsigned j;

    check_char_tag(current_character->c);
    set_char_tag(current_character->c, TAG_EXT);
    append_to_queue(&exten_queue, entry);
    for (j=E_MIN; j<=E_MAX; j++) {
        entry->pieces[j] = 0;
    }
    set_char_remainder(current_character->c, ne);
    current_character->extens = (unsigned int *)entry->pieces;
    ne++;
}

void
set_extensible_piece(unsigned piece, unsigned val)
{
    unsigned *exten = current_character->extens;

    if ((piece < E_MIN) || (piece > E_MAX))
        internal_error_1("set_extensible_piece (piece=%d)", piece);
    if (exten[piece]!=0)
        warning_0("value already defined");
    exten[piece] = val;
}

void
adjust_labels(boolean play_with_starts)
{
    unsigned plane, index;
    unsigned c;
    char_entry *entry;
    int max_start = (ofm_level==OFM_TFM) ? MAX_START_TFM : MAX_START_OFM;

    label_table = (label_entry *)xmalloc((no_labels+2)*sizeof(label_entry));
    label_ptr = 0;
    label_table[0].rr = -1; /* sentinel */
    FOR_ALL_CHARACTERS(
        if ((c>=bc) && (c<=ec) && (entry->tag == TAG_LIG)) {
            sort_ptr = label_ptr; /* hole at position sort_ptr+1 */
            while (label_table[sort_ptr].rr > (int)(entry->remainder)) {
                label_table[sort_ptr+1] = label_table[sort_ptr];
                sort_ptr--; /* move the hole */
            }
            label_table[sort_ptr+1].cc = c;
            label_table[sort_ptr+1].rr = entry->remainder;
            label_ptr++;
        }
    )
    if (play_with_starts) {
      if (bchar != CHAR_BOUNDARY) {
        extra_loc_needed = TRUE; lk_offset = 1;
      } else {
        extra_loc_needed = FALSE; lk_offset = 0;
      }
      sort_ptr = label_ptr; /* the largest unallocated label */
      if ((label_table[sort_ptr].rr + lk_offset) > max_start) {
        lk_offset=0; extra_loc_needed=FALSE;
        /* location 0 can do double duty */
        do {
            set_char_remainder(label_table[sort_ptr].cc, lk_offset);
            while (label_table[sort_ptr-1].rr == label_table[sort_ptr].rr) {
                sort_ptr--;
                set_char_remainder(label_table[sort_ptr].cc, lk_offset);
            }
            lk_offset++; sort_ptr--;
        } while ((lk_offset+label_table[sort_ptr].rr) > max_start);
        /* N.B. lk_offset=MAX_START+1 satisfies this when sort_ptr=0 */
      }
      if (lk_offset>0) {
        while (sort_ptr>0) {
            set_char_remainder(label_table[sort_ptr].cc,
                               get_char_remainder(label_table[sort_ptr].cc)
                               +lk_offset);
            sort_ptr--;
        }
      }
    }
}

void
print_labels(void)
{
    unsigned i;

    if (label_ptr>0) {
        left(); out("COMMENT"); out_ln();
        for (i=1; i<=label_ptr; i++) {
            left(); out("LABEL_ENTRY"); out(" ");
            out_int(i,10); out(" ");
            out_char(label_table[i].cc); out(" ");
            out_int(label_table[i].rr, 10); right();
        }
        right();
    }
}

void
check_and_correct(void)
{
    build_kern_table();
    build_dimen_tables();
    build_exten_table();
    check_ligature_ends_properly();
    compute_ofm_character_info();
    adjust_labels(TRUE);
    check_existence_all_character_fields();
    calculate_seven_bit_safe_flag();
    check_ligature_infinite_loops();
    check_charlist_infinite_loops();
    doublecheck_ligatures();
    doublecheck_extens();
}

void
check_charlist_infinite_loops(void)
{
    unsigned plane, index;
    unsigned c;
    char_entry *entry;
    unsigned g;

    FOR_ALL_CHARACTERS(
        if (entry->tag == TAG_LIST) {
            g = entry->remainder;
            while ((g < c) && (planes[g/PLANE][g%PLANE]->tag == TAG_LIST)) {
                g = planes[g/PLANE][g%PLANE]->remainder;
            }
            if (g == c) {
                entry->tag = TAG_NONE;
                entry->remainder = 0;
                warning_1("Cycle of NEXTLARGER characters "
                          "has been broken at ",c);
            }
        }
    )
}

void
build_exten_table(void)
{
    list L1 = exten_queue.front, L2;
    unsigned i = 0;

    exten_table = (four_pieces **) xmalloc(ne*sizeof(four_pieces *));
    while (L1 != NULL) {
        exten_table[i] = (four_pieces *)L1->contents;
        L2 = L1->ptr;
        free(L1); L1 = L2;
        i++;
    }
}

void
retrieve_exten_table(unsigned char *table)
{
    unsigned i = 0, j;
    four_pieces *entry;

    exten_table = (four_pieces **) xmalloc(ne*sizeof(four_pieces *));
    for (i=0; i<ne; i++) {
        exten_table[i] = entry = (four_pieces *) xmalloc(sizeof(four_pieces));
        for (j=E_MIN; j<=E_MAX; j++) {
            if (ofm_level==OFM_TFM) {
                entry->pieces[j] = table[4*i+j] & 0xff;
            } else {
                entry->pieces[j] =
                  ((table[8*i+j*2] & 0xff) << 8) |
                  (table[8*i+j*2+1] & 0xff);
            }
        }
    }
}

void
print_extens(void)
{
    unsigned i,j;

    if (ne>0) {
        left(); out("COMMENT"); out_ln();
        for (i=0; i<ne; i++) {
            left(); out("EXTEN_ENTRY");
            out(" "); out_int(i,10); out_ln();
            for (j=E_MIN; j<=E_MAX; j++) {
                if (exten_table[i]->pieces[j] != 0)
                    print_extensible_piece(j,exten_table[i]->pieces[j]);
            }
            right();
        }
        right();
    }
}

void
doublecheck_extens(void)
{
    unsigned i,j;

    if (ne>0) {
        for (i=0; i<ne; i++) {
            for (j=E_MIN; j<=E_MAX; j++) {
                if (exten_table[i]->pieces[j] != 0)
                    doublecheck_existence(
                        exten_table[i]->pieces[j], extensible_pieces[j],
                        "Unused %s piece (H %X) refers to "
                        "nonexistent character");
            }
        }
    }
}

void
compute_ligkern_offset(void)
{
}

void
compute_character_info_size(void)
{
}

void
output_ofm_extensible(void)
{
    unsigned i,j;

    for (i=0; i<ne; i++) {
        for (j=E_MIN; j<=E_MAX; j++) {
            if (exten_table[i]->pieces[j] != 0)
                out_ofm_char(exten_table[i]->pieces[j]);
            else out_ofm_char(0);
        }
    }
}

void
compute_ofm_character_info(void)
{
    unsigned plane, index;
    unsigned c;
    char_entry *entry;

    bc = 0x7fffffff; ec=0;
    switch (ofm_level) {
        case OFM_TFM: {
            FOR_ALL_EXISTING_CHARACTERS(
                if (c < bc) bc = c;
                if (c > ec) ec = c;
            )
            break;
        }
        case OFM_LEVEL0: {
            FOR_ALL_EXISTING_CHARACTERS(
                if (c < bc) bc = c;
                if (c > ec) ec = c;
            )
            break;
        }
/* Level 1 only uses plane 0.  Take advantage of this fact. */
        case OFM_LEVEL1: {
            FOR_ALL_CHARACTERS(
                if (c < bc) bc = c;
                if (c > ec) ec = c;
            )
            break;
        }
        default: { internal_error_0("compute_ofm_character_info"); }
    }
}

void
output_ofm_character_info(void)
{
    unsigned plane, index;
    unsigned c;
    char_entry *entry;
    unsigned wd, ht, dp, ic;

    switch (ofm_level) {
        case OFM_TFM: {
            plane=0;
            for (index = bc; index <=ec; index++) {
                entry = planes[plane][index]; 
                c = plane*PLANE + index;
                if (entry == NULL) { 
                    out_ofm_4(0);
                } else {
                    if (entry->indices[C_WD] != NULL)
                        wd = entry->indices[C_WD]->index;
                    else wd = 0;
                    if (entry->indices[C_HT] != NULL)
                        ht = entry->indices[C_HT]->index;
                    else ht = 0;
                    if (entry->indices[C_DP] != NULL)
                        dp = entry->indices[C_DP]->index;
                    else dp = 0;
                    if (entry->indices[C_IC] != NULL)
                        ic = entry->indices[C_IC]->index;
                    else ic = 0;
                    out_ofm(wd);
                    out_ofm(ht*16 + dp);
                    out_ofm(ic*4 + entry->tag);
                    out_ofm(entry->remainder);
                }
            }
            break;
        }
        case OFM_LEVEL0: {
            plane=0;
            for (index = bc; index <=ec; index++) {
                entry = planes[plane][index]; 
                c = plane*PLANE + index;
                if (entry == NULL) { 
                    out_ofm_4(0); out_ofm_4(0);
                } else {
                    if (entry->indices[C_WD] != NULL)
                        wd = entry->indices[C_WD]->index;
                    else wd = 0;
                    if (entry->indices[C_HT] != NULL)
                        ht = entry->indices[C_HT]->index;
                    else ht = 0;
                    if (entry->indices[C_DP] != NULL)
                        dp = entry->indices[C_DP]->index;
                    else dp = 0;
                    if (entry->indices[C_IC] != NULL)
                        ic = entry->indices[C_IC]->index;
                    else ic = 0;
                    out_ofm_2(wd);
                    out_ofm(ht);
                    out_ofm(dp);
                    out_ofm(ic);
                    out_ofm(entry->tag);
                    out_ofm_2(entry->remainder);
                }
            }
            break;
        }
        case OFM_LEVEL1: {
            internal_error_0("OFM level 1 not currently supported");
            break;
        }
        default: { internal_error_0("compute_ofm_character_info"); }
    }
}
