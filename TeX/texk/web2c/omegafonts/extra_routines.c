/* extra_routines.c: Tables for glues, rules, penalties, etc.

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
#include "extra_routines.h"
#include "header_routines.h"
#include "error_routines.h"

#define MAX_EXTRA_TABLES 16
#define MAX_TABLE_ENTRIES 256

unsigned *ivalue_tables[MAX_EXTRA_TABLES];
unsigned *penalty_tables[MAX_EXTRA_TABLES];
fix      *fvalue_tables[MAX_EXTRA_TABLES];
fix      *mvalue_tables[MAX_EXTRA_TABLES];
rule     *rule_tables[MAX_EXTRA_TABLES];
glue     *glue_tables[MAX_EXTRA_TABLES];

unsigned max_ivalue_entry[MAX_EXTRA_TABLES];
unsigned max_penalty_entry[MAX_EXTRA_TABLES];
unsigned max_fvalue_entry[MAX_EXTRA_TABLES];
unsigned max_mvalue_entry[MAX_EXTRA_TABLES];
unsigned max_rule_entry[MAX_EXTRA_TABLES];
unsigned max_glue_entry[MAX_EXTRA_TABLES];

unsigned no_ivalue_tables = 0;
unsigned no_penalty_tables = 0;
unsigned no_fvalue_tables = 0;
unsigned no_mvalue_tables = 0;
unsigned no_rule_tables = 0;
unsigned no_glue_tables = 0;

unsigned *cur_ivalue_table;
unsigned *cur_penalty_table;
fix      *cur_mvalue_table;
fix      *cur_fvalue_table;
rule     *cur_rule_table;
glue     *cur_glue_table;

unsigned cur_ivalue_table_index;
unsigned cur_penalty_table_index;
unsigned cur_mvalue_table_index;
unsigned cur_fvalue_table_index;
unsigned cur_rule_table_index;
unsigned cur_glue_table_index;

unsigned *cur_ivalue_entry;
unsigned *cur_penalty_entry;
fix      *cur_mvalue_entry;
fix      *cur_fvalue_entry;
rule     *cur_rule_entry;
glue     *cur_glue_entry;

unsigned nki=0;
unsigned nwi=0;
unsigned nkp=0;
unsigned nwp=0;
unsigned nkm=0;
unsigned nwm=0;
unsigned nkf=0;
unsigned nwf=0;
unsigned nkr=0;
unsigned nwr=0;
unsigned nkg=0;
unsigned nwg=0;

void
compute_ofm_extra_stuff(void)
{
    unsigned i=0;

    if (ofm_level >= OFM_LEVEL1) {
        nki = no_ivalue_tables;
        nkp = no_penalty_tables;
        nkm = no_mvalue_tables;
        nkf = no_fvalue_tables;
        nkr = no_rule_tables;
        nkg = no_glue_tables;
        for (i=0; i<nki; i++) {
            nwi += max_ivalue_entry[i];
        }
        for (i=0; i<nkp; i++) {
            nwp += max_penalty_entry[i];
        }
        for (i=0; i<nkm; i++) {
            nwm += max_mvalue_entry[i];
        }
        for (i=0; i<nkf; i++) {
            nwf += max_fvalue_entry[i];
        }
        for (i=0; i<nkr; i++) {
            nwr += 3 * max_rule_entry[i];
        }
        for (i=0; i<nkg; i++) {
            nwg += 4 * max_glue_entry[i];
        }
    }
}

void
output_ofm_extra_stuff(void)
{
    if (ofm_level >= OFM_LEVEL1) {
        fatal_error_0("OFM level 1 not currently supported");
    }
}

void
init_all_tables(void)
{
     unsigned i=0;

     for (i=0; i<MAX_EXTRA_TABLES; i++) {
         ivalue_tables[i] = NULL;
         penalty_tables[i] = NULL;
         fvalue_tables[i] = NULL;
         mvalue_tables[i] = NULL;
         rule_tables[i] = NULL;
         glue_tables[i] = NULL;
     }
}

void
init_font_ivalue(unsigned tab)
{
     unsigned j;

     if (tab>=MAX_EXTRA_TABLES)
         internal_error_1("init_font_ivalue (tab=%d)", tab);
     if (ivalue_tables[tab] != NULL) {
         warning_1("IVALUE table (D %d) previously defined; "
                   "old value ignored", tab);
         cur_ivalue_table = ivalue_tables[tab];
         for (j=0; j<MAX_TABLE_ENTRIES; j++) {
              cur_ivalue_table[j] = 0;
         }
     } else {
         no_ivalue_tables++;
         ivalue_tables[tab] =
             (unsigned *) xmalloc(MAX_TABLE_ENTRIES*sizeof(unsigned));
         cur_ivalue_table = ivalue_tables[tab];
     }
     max_ivalue_entry[tab] = 0;
}

void
init_font_penalty(unsigned tab)
{
     unsigned j;

     if (tab>=MAX_EXTRA_TABLES)
         internal_error_1("init_font_penalty (tab=%d)", tab);
     if (penalty_tables[tab] != NULL) {
         warning_1("IVALUE table (D %d) previously defined; "
                   "old value ignored", tab);
         cur_penalty_table = ivalue_tables[tab];
         for (j=0; j<MAX_TABLE_ENTRIES; j++) {
              cur_penalty_table[j] = 0;
         }
     } else {
         no_penalty_tables++;
         penalty_tables[tab] =
             (unsigned *) xmalloc(MAX_TABLE_ENTRIES*sizeof(unsigned));
         cur_penalty_table = ivalue_tables[tab];
     }
     max_penalty_entry[tab] = 0;
}

void
init_font_mvalue(unsigned tab)
{
     unsigned j;

     if (tab>=MAX_EXTRA_TABLES)
         internal_error_1("init_font_mvalue (tab=%d)", tab);
     if (mvalue_tables[tab] != NULL) {
         warning_1("MVALUE table (D %d) previously defined; "
                   "old value ignored", tab);
         cur_mvalue_table = mvalue_tables[tab];
         for (j=0; j<MAX_TABLE_ENTRIES; j++) {
              cur_mvalue_table[j] = 0;
         }
     } else {
         no_mvalue_tables++;
         mvalue_tables[tab] =
             (fix *) xmalloc(MAX_TABLE_ENTRIES*sizeof(unsigned));
         cur_mvalue_table = mvalue_tables[tab];
     }
     max_mvalue_entry[tab] = 0;
}

void
init_font_fvalue(unsigned tab)
{
     unsigned j;

     if (tab>=MAX_EXTRA_TABLES)
         internal_error_1("init_font_fvalue (tab=%d)", tab);
     if (fvalue_tables[tab] != NULL) {
         warning_1("FVALUE table (D %d) previously defined; "
                   "old value ignored", tab);
         cur_fvalue_table = fvalue_tables[tab];
         for (j=0; j<MAX_TABLE_ENTRIES; j++) {
              cur_fvalue_table[j] = 0;
         }
     } else {
         no_fvalue_tables++;
         fvalue_tables[tab] =
             (fix *) xmalloc(MAX_TABLE_ENTRIES*sizeof(unsigned));
         cur_fvalue_table = fvalue_tables[tab];
     }
     max_fvalue_entry[tab] = 0;
}

void
init_font_rule(unsigned tab)
{
     unsigned j;

     if (tab>=MAX_EXTRA_TABLES)
         internal_error_1("init_font_rule (tab=%d)", tab);
     if (rule_tables[tab] != NULL) {
         warning_1("RULE table (D %d) previously defined; "
                   "old value ignored", tab);
         cur_rule_table = rule_tables[tab];
         for (j=0; j<MAX_TABLE_ENTRIES; j++) {
              cur_rule_table[j].rule_wd = 0;
              cur_rule_table[j].rule_dp = 0;
              cur_rule_table[j].rule_ht = 0;
         }
     } else {
         no_rule_tables++;
         rule_tables[tab] =
             (rule *) xmalloc(MAX_TABLE_ENTRIES*sizeof(rule));
         cur_rule_table = rule_tables[tab];
     }
     max_rule_entry[tab] = 0;
}

void
init_font_glue(unsigned tab)
{
     unsigned j;

     if (tab>=MAX_EXTRA_TABLES)
         internal_error_1("init_font_glue (tab=%d)", tab);
     if (glue_tables[tab] != NULL) {
         warning_1("GLUE table (D %d) previously defined; "
                   "old value ignored", tab);
         cur_glue_table = glue_tables[tab];
         for (j=0; j<MAX_TABLE_ENTRIES; j++) {
              cur_glue_table[j].glue_width = 0;
              cur_glue_table[j].glue_stretch = 0;
              cur_glue_table[j].glue_shrink = 0;
              cur_glue_table[j].glue_stretch_order = 0;
              cur_glue_table[j].glue_shrink_order = 0;
              cur_glue_table[j].glue_type = 0;
              cur_glue_table[j].glue_arg_type = GLUEARG_NONE;
              cur_glue_table[j].glue_arg1 = 0;
              cur_glue_table[j].glue_arg2 = 0;
         }
     } else {
         no_glue_tables++;
         glue_tables[tab] =
             (glue *) xmalloc(MAX_TABLE_ENTRIES*sizeof(glue));
         cur_glue_table = glue_tables[tab];
     }
     max_glue_entry[tab] = 0;
}


void
init_font_ivalue_entry(unsigned index)
{
     if (index>=MAX_TABLE_ENTRIES)
         internal_error_1("init_font_ivalue_entry (index=%d)", index);
     cur_ivalue_entry = &cur_ivalue_table[index];
     if (max_ivalue_entry[cur_ivalue_table_index]<index)
         max_ivalue_entry[cur_ivalue_table_index] = index;
}

void
init_font_penalty_entry(unsigned index)
{
     if (index>=MAX_TABLE_ENTRIES)
         internal_error_1("init_font_penalty_entry (index=%d)", index);
     cur_penalty_entry = &cur_penalty_table[index];
     if (max_penalty_entry[cur_penalty_table_index]<index)
         max_penalty_entry[cur_penalty_table_index] = index;
}

void
init_font_mvalue_entry(unsigned index)
{
     if (index>=MAX_TABLE_ENTRIES)
         internal_error_1("init_font_mvalue_entry (index=%d)", index);
     cur_mvalue_entry = &cur_mvalue_table[index];
     if (max_mvalue_entry[cur_mvalue_table_index]<index)
         max_mvalue_entry[cur_mvalue_table_index] = index;
}

void
init_font_fvalue_entry(unsigned index)
{
     if (index>=MAX_TABLE_ENTRIES)
         internal_error_1("init_font_fvalue_entry (index=%d)", index);
     cur_fvalue_entry = &cur_fvalue_table[index];
     if (max_fvalue_entry[cur_fvalue_table_index]<index)
         max_fvalue_entry[cur_fvalue_table_index] = index;
}

void
init_font_rule_entry(unsigned index)
{
     if (index>=MAX_TABLE_ENTRIES)
         internal_error_1("init_font_rule_entry (index=%d)", index);
     cur_rule_entry = &cur_rule_table[index];
     if (max_rule_entry[cur_rule_table_index]<index)
         max_rule_entry[cur_rule_table_index] = index;
}

void
init_font_glue_entry(unsigned index)
{
     if (index>=MAX_TABLE_ENTRIES)
         internal_error_1("init_font_glue_entry (index=%d)", index);
     cur_glue_entry = &cur_glue_table[index];
     if (max_glue_entry[cur_glue_table_index]<index)
         max_glue_entry[cur_glue_table_index] = index;
}


void
set_font_ivalue_definition(unsigned val)
{
     *cur_ivalue_entry = val;
}

void
set_font_penalty_definition(unsigned val)
{
     *cur_penalty_entry = val;
}

void
set_font_mvalue_definition(fix fval)
{
     *cur_mvalue_entry = fval;
}

void
set_font_fvalue_definition(fix fval)
{
     *cur_fvalue_entry = fval;
}

void
set_font_rule_measure(unsigned measure, fix fval)
{
     switch (measure) {
         case RULE_WD: {
             cur_rule_entry->rule_wd = fval; break;
         }
         case RULE_HT: {
             cur_rule_entry->rule_ht = fval; break;
         }
         case RULE_DP: {
             cur_rule_entry->rule_dp = fval; break;
         }
         default: {
             internal_error_1("set_font_rule_measure (measure=%d)", measure);
         }
     }
}


void
set_font_glue_width(fix width)
{
     cur_glue_entry->glue_width = width;
}

void
set_font_glue_shrink_stretch(unsigned shrink_stretch,
                             fix width, unsigned order)
{
     switch (shrink_stretch) {
         case GLUE_SHRINK: {
             cur_glue_entry->glue_shrink = width; break;
             cur_glue_entry->glue_shrink_order = order; break;
         }
         case GLUE_STRETCH: {
             cur_glue_entry->glue_stretch_order = order; break;
         }
         default: {
             internal_error_1("set_font_rule_measure (shrink_stretch=%d)",
                              shrink_stretch);
         }
     }
}

void
set_font_glue_type(unsigned type)
{
     cur_glue_entry->glue_type = type;
}

void
set_font_glue_character(unsigned c)
{
    cur_glue_entry->glue_arg_type = GLUEARG_CHAR;
    cur_glue_entry->glue_arg1 = c;
}

void
set_font_glue_rule(unsigned rule_table, unsigned rule_index)
{
    cur_glue_entry->glue_arg_type = GLUEARG_RULE;
    cur_glue_entry->glue_arg1 = rule_table;
    cur_glue_entry->glue_arg2 = rule_index;
}

void
set_character_ivalue(unsigned table, unsigned index)
{
    fatal_error_0("CHARIVALUE not currently supported");
}

void
set_character_penalty(unsigned table, unsigned index)
{
    fatal_error_0("CHARPENALTY not currently supported");
}

void
set_character_mvalue(unsigned table, unsigned index)
{
    fatal_error_0("CHARMVALUE not currently supported");
}

void
set_character_fvalue(unsigned table, unsigned index)
{
    fatal_error_0("CHARFVALUE not currently supported");
}

void
set_character_rule(unsigned table, unsigned index)
{
    fatal_error_0("CHARRULE not currently supported");
}

void
set_character_glue(unsigned table, unsigned index)
{
    fatal_error_0("CHARGLUE not currently supported");
}
