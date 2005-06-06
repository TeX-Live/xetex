/* print_routines.h: higher-level routines for printing OPL and OVP files.

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

extern void print_check_sum(void);
extern void print_design_size(void);
extern void print_design_units(void);
extern void print_coding_scheme(void);
extern void print_family(void);
extern void print_face(void);
extern void print_seven_bit_safe_flag(void);
extern void print_header_word(unsigned, unsigned);
extern void print_font_dimension(void);
extern void print_ligature_table(void);
extern void print_boundary_character(unsigned);
extern void print_character(unsigned);
extern void print_parameter(unsigned, fix);
extern void print_label_command(unsigned);
extern void print_ligature_command(unsigned, unsigned, unsigned);
extern void print_kerning_command(unsigned, fix);
extern void print_stop_command(void);
extern void print_skip_command(unsigned);
extern void print_character_measure(unsigned, fix);
extern void print_next_larger(unsigned);
extern void print_var_character(void);
extern void print_extensible_piece(unsigned, unsigned);

extern void print_vtitle(string);
extern void print_map_font(unsigned);
extern void print_font_name(string);
extern void print_font_area(string);
extern void print_font_check_sum(unsigned);
extern void print_font_at(fix);
extern void print_font_design_size(fix);
extern void print_map(void);
extern void print_select_font(unsigned);
extern void print_set_char(unsigned);
extern void print_put_char(unsigned);
extern void print_set_rule(fix, fix);
extern void print_put_rule(fix, fix);
extern void print_push(void);
extern void print_pop(void);
extern void print_move(unsigned, fix);
extern void print_special(string);
extern void print_special_hex(string);

extern void print_character_repeat(unsigned, unsigned);
extern void print_font_ivalue(unsigned);
extern void print_font_mvalue(unsigned);
extern void print_font_fvalue(unsigned);
extern void print_font_penalty(unsigned);
extern void print_font_rule(unsigned);
extern void print_font_glue(unsigned);
extern void print_c_label_command(unsigned);
extern void print_c_kerning_command(unsigned, unsigned);
extern void print_c_glue_command(unsigned, unsigned);
extern void print_c_penalty_command(unsigned, unsigned);
extern void print_c_penglue_command(unsigned, unsigned, unsigned);
extern void print_character_ivalue(unsigned, unsigned);
extern void print_character_mvalue(unsigned, unsigned);
extern void print_character_fvalue(unsigned, unsigned);
extern void print_character_penalty(unsigned, unsigned);
extern void print_character_rule(unsigned, unsigned);
extern void print_character_glue(unsigned, unsigned);
extern void print_font_ivalue_entry(unsigned);
extern void print_font_ivalue_definition(unsigned);
extern void print_font_mvalue_entry(unsigned);
extern void print_font_mvalue_definition(fix);
extern void print_font_fvalue_entry(unsigned);
extern void print_font_fvalue_definition(fix);
extern void print_font_penalty_entry(unsigned);
extern void print_font_penalty_definition(unsigned);
extern void print_font_rule_entry(unsigned);
extern void print_font_rule_measure(unsigned, fix);
extern void print_font_rule_height(fix);
extern void print_font_rule_depth(fix);
extern void print_font_glue_entry(unsigned);
extern void print_font_glue_type(unsigned);
extern void print_font_glue_width(fix);
extern void print_font_glue_character(unsigned);
extern void print_font_glue_rule(unsigned,unsigned);
extern void print_font_glue_shrink_stretch(unsigned, fix, unsigned);
extern void print_ofm_level(unsigned);
extern void print_font_dir(unsigned);
extern void print_accent(unsigned);

